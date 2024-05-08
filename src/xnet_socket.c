#include "xnet_socket.h"
#include <errno.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


#ifdef _WIN32
    #include "socket_win.h"
#else
    #include "socket_linux.h"
#endif

static int
get_last_error() {
#ifdef _WIN32
    return WSAGetLastError();
#else
    return errno;
#endif
}

static int
alloc_socket_id(xnet_poll_t *poll) {
    int i, id;
    xnet_socket_t *slots = poll->slots;
    for (i=0; i<MAX_CLIENT_NUM; i++) {
        id = (poll->slot_index + i) % MAX_CLIENT_NUM;
        if (slots[id].type == SOCKET_TYPE_INVALID) {
            poll->slot_index = id + 1;
            return id;
        }
    }
    return -1;
}

static xnet_socket_t *
new_fd(xnet_poll_t *poll, SOCKET_TYPE fd, int id, uint8_t protocol, bool reading) {
    xnet_socket_t *s = &poll->slots[id];
    s->fd = fd;
    s->id = id;
    s->protocol = protocol;
    s->reading = false;
    s->writing = false;
    s->closing = false;
    s->read_size = MIN_READ_SIZE;
    assert(s->wb_list.head == NULL && s->wb_list.tail == NULL);
    s->wb_size = 0;

    xnet_poll_addfd(poll, fd, id);
    xnet_enable_read(poll, s, reading);
    return s;
}

static void
free_wb(xnet_write_buff_t *wb) {
    free(wb->buffer);
    free(wb);
}

static void
insert_wb_list(xnet_wb_list_t *wb_list, xnet_write_buff_t *wb) {
    if (wb_list->head == NULL) {
        wb_list->head = wb_list->tail = wb;
    } else {
        assert(wb_list->tail != NULL);
        assert(wb_list->tail->next == NULL);
        wb_list->tail->next = wb;
        wb_list->tail = wb;
    }
}

static void
clear_wb_list(xnet_wb_list_t *wb_list) {
    xnet_write_buff_t *wb;
    if (wb_list->head) {
        assert(wb_list->tail != NULL);
        assert(wb_list->tail->next == NULL);
        while (wb_list->head) {
            wb = wb_list->head;
            wb_list->head = wb->next;
            free_wb(wb);
        }
        wb_list->tail = NULL;
    }
}

int
xnet_socket_init() {
#ifdef _WIN32
    WSADATA wsaData;
    // 初始化 Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("Failed to initialize Winsock\n");
        return 1;
    }
#else
    return 0;
#endif
}

int
xnet_socket_deinit() {
#ifdef _WIN32
    WSACleanup();
#else
#endif
    return 0;
}

int
xnet_poll_init(xnet_poll_t *poll) {
    int i;
    xnet_socket_t *s;

    poll->slot_index = 0;
    for (i=0; i<MAX_CLIENT_NUM; i++) {
        s = &poll->slots[i];
        s->id = s->fd = 0;
        s->type = SOCKET_TYPE_INVALID;
        s->wb_list.head = s->wb_list.tail = NULL;
        s->wb_size = 0;
    }

    //pipe
    SOCKET_TYPE fd[2];
    if (pipe(fd))
        return -1;
    poll->recv_fd = fd[0];
    poll->send_fd = fd[1];
    FD_ZERO(&poll->rfds);

#ifdef _WIN32
    //select
    FD_ZERO(&poll->readfds);
    FD_ZERO(&poll->writefds);
    FD_ZERO(&poll->errorfds);
    FD_SET(fd[0], &poll->readfds);
    poll->socket_list = NULL;
#else
    //epoll
    poll->epoll_fd = epoll_create(1024);
    if (poll->epoll_fd == -1) {
        closesocket(fd[0]);
        closesocket(fd[1]);
        return -1;
    }

    poll_add(poll->epoll_fd, fd[0], NULL);
#endif

    return 0;
}

int
xnet_poll_addfd(xnet_poll_t *poll, SOCKET_TYPE fd, int id) {
#ifdef _WIN32
    //select
    FD_SET(fd, &poll->errorfds);
    add_to_socketlist(poll, fd, id);
#else
    //epoll
    xnet_socket_t *s = &poll->slots[id];
    poll_add(poll->epoll_fd, fd, s);
#endif
}

int
xnet_poll_closefd(xnet_poll_t *poll, xnet_socket_t *s) {
#ifdef _WIN32
    //select
    FD_CLR(s->fd, &poll->errorfds);
    if (s->writing) FD_CLR(s->fd, &poll->writefds);
    if (s->reading) FD_CLR(s->fd, &poll->readfds);
    find_del_socketlist(poll, s->fd);

#else
    //epoll
    poll_del(poll->epoll_fd, s->fd);
#endif
    closesocket(s->fd);

    s->id = s->fd = 0;
    s->writing = false;
    s->reading = false;
    s->type = SOCKET_TYPE_INVALID;
    clear_wb_list(&s->wb_list);
}

int
xnet_enable_read(xnet_poll_t *poll, xnet_socket_t *s, bool enable) {
    return poll_enable_read(poll, s, enable);
}

int
xnet_enable_write(xnet_poll_t *poll, xnet_socket_t *s, bool enable) {
    return poll_enable_write(poll, s, enable);
}

inline bool
wb_list_empty(xnet_socket_t *s) {
    return s->wb_list.head == NULL;
}

void
set_nonblocking(SOCKET_TYPE fd) {
    poll_set_nonblocking(fd);
}

void
set_keepalive(SOCKET_TYPE fd) {
    int keepalive = 1;
    setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (void *)&keepalive , sizeof(keepalive));  
}

/*
xnet_poll_wait会将触发事件的socket列表加入到poll_event数组中，
每次最多触发POLL_EVENT_MAX个socket
timeout:超时时间，单位毫秒，-1为不设定超时
*/
int
xnet_poll_wait(xnet_poll_t *poll, int timeout) {
    return poll_wait(poll, timeout);
}

int
xnet_listen_tcp_socket(xnet_poll_t *poll, int port, xnet_socket_t **socket_out) {
    struct sockaddr_in serverAddr;
    SOCKET_TYPE fd = 0;
    xnet_socket_t *s;
    int id;
    int reuse = 1;

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) goto FAILED;

    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (void *)&reuse, sizeof(int)) == -1) {
        goto FAILED;
    }

    if (bind(fd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1)
        goto FAILED;
    if (listen(fd, 5) == -1) goto FAILED;
    id = alloc_socket_id(poll);
    if (id == -1) goto FAILED;
    s = new_fd(poll, fd, id, SOCKET_PROTOCOL_TCP, true);
    s->type = SOCKET_TYPE_LISTENING;

    if (socket_out) *socket_out = s;
    return 0;
FAILED:
    if (fd) closesocket(fd);
    return -1;
}

int
xnet_accept_tcp_socket(xnet_poll_t *poll, xnet_socket_t *listen_s, xnet_socket_t **socket_out) {
    struct sockaddr_in client_addr;
    xnet_socket_t *s;
    socklen_t client_addrlen = sizeof(client_addr);
    SOCKET_TYPE fd = 0;
    int id;
    
    if ((fd = accept(listen_s->fd, (struct sockaddr*)&client_addr, &client_addrlen)) == -1) {
        return -1;
    }

    id = alloc_socket_id(poll);
    if (id == -1) goto FAILED;
    s = new_fd(poll, fd, id, SOCKET_PROTOCOL_TCP, true);
    s->type = SOCKET_TYPE_ACCEPTED;

    set_nonblocking(fd);
    set_keepalive(fd);

    if (socket_out) *socket_out = s;
    return 0;
FAILED:
    if (fd) closesocket(fd);
    return -1;
}

//返回1，连接成功
//返回0，连接中
//返回-1，连接失败
int
xnet_connect_tcp_socket(xnet_poll_t *poll, char *host, int port, xnet_socket_t **socket_out) {
    int status, err, id;
    struct addrinfo ai_hints;
    struct addrinfo *ai_list = NULL;
    struct addrinfo *ai_ptr = NULL;
    char str_port[16];
    SOCKET_TYPE sock = -1;
    xnet_socket_t *s;

    sprintf(str_port, "%d", port);
    memset(&ai_hints, 0, sizeof(ai_hints));
    ai_hints.ai_family = AF_UNSPEC;
    ai_hints.ai_socktype = SOCK_STREAM;
    ai_hints.ai_protocol = IPPROTO_TCP;

    status = getaddrinfo(host, str_port, &ai_hints, &ai_list);
    if ( status != 0 ) {
        return -1;
    }

    for (ai_ptr = ai_list; ai_ptr != NULL; ai_ptr = ai_ptr->ai_next) {
        sock = socket(ai_ptr->ai_family, ai_ptr->ai_socktype, ai_ptr->ai_protocol);
        if (sock < 0)
            continue;

        set_keepalive(sock);
        set_nonblocking(sock);
        status = connect(sock, ai_ptr->ai_addr, ai_ptr->ai_addrlen);
        if (status != 0) {
            err = get_last_error();
#ifdef _WIN32
            if (err != WSAEINPROGRESS && err != WSAEINVAL && err != WSAEWOULDBLOCK) {
#else
            if (err != EINPROGRESS) {
#endif
                closesocket(sock);
                sock = -1;
                continue;
            }
        }

        break;
    }

    if (sock < 0) {
        goto FAILED;
    }

    id = alloc_socket_id(poll);
    s = new_fd(poll, sock, id, SOCKET_PROTOCOL_TCP, true);
    if (socket_out) *socket_out = s;

    if (status == 0) {
        //连接成功
        s->type = SOCKET_TYPE_CONNECTED;
        freeaddrinfo(ai_list);
        return 1;
    }

    //连接中
    s->type = SOCKET_TYPE_CONNECTING;
    xnet_enable_write(poll, s, true);
    freeaddrinfo(ai_list);
    return 0;
FAILED:
    freeaddrinfo(ai_list);
    return -1;
}

//返回-2，socket已关闭
//返回-1，socket产生错误
//返回0，表示可以继续等待
//返回>0，表示收到数据，返回的值就是收到数据的大小
int
xnet_recv_data(xnet_poll_t *poll, xnet_socket_t *s, char **out_data) {
    int err;
    int sz = s->read_size;
    char *buffer = malloc(sz);
    int n = recv(s->fd, buffer, sz, 0);
    if (n < 0) {
        free(buffer);
        err = get_last_error();

        if (!XNET_HAVR_WOULDBLOCK(err) && err != XNET_EINTR) {
            printf("xnet_recv_data error: %d\n", err);
            return -1;
        }
        return 0;
    }

    if (n == 0) {
        free(buffer);
        printf("xnet_recv_data close\n");
        return -2;
    }

    if (n == sz) {
        s->read_size *= 2;
    } else if(sz > MIN_READ_SIZE && n*2 < sz) {
        s->read_size /= 2;
    }

    if (out_data)
        *out_data = buffer;
    else
        free(buffer);

    return n;
}

int
xnet_send_data(xnet_poll_t *poll, xnet_socket_t *s) {
    xnet_wb_list_t *wb_list = &s->wb_list;
    xnet_write_buff_t *wb;
    int n, err;

    while (wb_list->head) {
        wb = wb_list->head;
        for (;;) {
            n = send(s->fd, wb->ptr, wb->sz, 0);
            if (n < 0) {
                err = get_last_error();
                if (err == XNET_EINTR) continue;
                if (XNET_HAVR_WOULDBLOCK(err)) return -1;
                xnet_enable_write(poll, s, false);
                return -1;
            }
            s->wb_size -= n;
            if (n != wb->sz) {
                wb->ptr += n;
                wb->sz -= n;
                return -1;
            }
            break;
        }

        wb_list->head = wb->next;
        free_wb(wb);
    }

    if (s->closing) {
        xnet_poll_closefd(poll, s);
    } else {
        //发送完了,禁用写事件
        xnet_enable_write(poll, s, false);
    }

    return -1;
}

void
append_send_buff(xnet_poll_t *poll, xnet_socket_t *s, char *buffer, int sz) {
    xnet_write_buff_t *wb = (xnet_write_buff_t *)malloc(sizeof(xnet_write_buff_t));
    wb->buffer = wb->ptr = buffer;
    wb->sz = sz;
    wb->next = NULL;
    insert_wb_list(&s->wb_list, wb);
    s->wb_size += sz;

    xnet_enable_write(poll, s, true);
}

void
block_recv(SOCKET_TYPE fd, void *buffer, int sz) {
    int err, n;
    for (;;) {
        n = recv(fd, buffer, sz, 0);
        if (n < 0) {
            err = get_last_error();
            if (err == XNET_EINTR)
                continue;
            printf("block_recv error:%d\n", err);
            return;
        }
        assert(n == sz);
        return;
    }
}

void
block_send(SOCKET_TYPE fd, void *buffer, int sz) {
    int err, n;
    for (;;) {
        n = send(fd, buffer, sz, 0);
        if (n < 0) {
            err = get_last_error();
            if (err != XNET_EINTR) {
                printf("block_send error:%d\n", err);
            }
            continue;
        }
        assert(n == sz);
        return;
    }
}

inline int
get_sockopt(SOCKET_TYPE fd, int level, int optname, int *optval, int *optlen) {
#ifdef _WIN32
    return getsockopt(fd, level, optname, (char*)optval, optlen);
#else
    return getsockopt(fd, level, optname, optval, optlen);
#endif
}