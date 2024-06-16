#include "xnet_socket.h"
#include "malloc_ref.h"
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

int
get_last_error() {
#ifdef _WIN32
    return WSAGetLastError();
#else
    return errno;
#endif
}

static void
xnet_gen_addr(int type, union sockaddr_all *sa, xnet_addr_t *addr_out) {
    if (type == SOCKET_ADDR_TYPE_IPV4) {
        addr_out->type = SOCKET_ADDR_TYPE_IPV4;
        addr_out->port = sa->v4.sin_port;
        memcpy(addr_out->addr, &sa->v4.sin_addr, sizeof(sa->v4.sin_addr));
    } else {
        addr_out->type = SOCKET_ADDR_TYPE_IPV6;
        addr_out->port = sa->v6.sin6_port;
        memcpy(addr_out->addr, &sa->v6.sin6_addr, sizeof(sa->v6.sin6_addr));
    }
}

static socklen_t
xnet_addr_to_sockaddr(xnet_addr_t *addr, union sockaddr_all *sa) {
    if (addr->type == SOCKET_ADDR_TYPE_IPV4) {
        memset(&sa->v4, 0, sizeof(sa->v4));
        sa->s.sa_family = AF_INET;
        sa->v4.sin_port = addr->port;
        memcpy(&sa->v4.sin_addr, &addr->addr, sizeof(sa->v4.sin_addr));
        return sizeof(sa->v4);
    } else {
        memset(&sa->v6, 0, sizeof(sa->v6));
        sa->s.sa_family = AF_INET6;
        sa->v6.sin6_port = addr->port;
        memcpy(&sa->v6.sin6_addr, &addr->addr, sizeof(sa->v6.sin6_addr));
        return sizeof(sa->v6);
    }
}

static inline void
init_socket_slot(xnet_socket_t *s) {
    s->id = s->fd = 0;
    s->type = SOCKET_TYPE_INVALID;
    s->wb_list.head = s->wb_list.tail = NULL;
    s->wb_size = 0;
    memset(&s->addr_info, 0, sizeof(s->addr_info));
}

static int
alloc_socket_id(xnet_poll_t *poll) {
    int i, id, new_size;
    xnet_socket_t *slots = poll->slots;

    for (i=0; i<poll->slot_size; i++) {
        id = (poll->slot_index + i) % poll->slot_size;
        if (slots[id].type == SOCKET_TYPE_INVALID) {
            poll->slot_index = id + 1;
            return id;
        }
    }

    if (poll->slot_size < MAX_CLIENT_NUM) {
        new_size = poll->slot_size ? (poll->slot_size * 2) : 32;
        slots = realloc(poll->slots, sizeof(*poll->slots)*new_size);
        if (!slots) return -1;

        for (i=poll->slot_size; i<new_size; i++)
            init_socket_slot(&slots[i]);

        poll->slot_index = poll->slot_size;
        poll->slots = slots;
        poll->slot_size = new_size;
        return alloc_socket_id(poll);
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
    memset(&s->addr_info, 0, sizeof(s->addr_info));

    xnet_poll_addfd(poll, fd, id);
    xnet_enable_read(poll, s, reading);
    return s;
}

static void
free_wb(xnet_write_buff_t *wb) {
    if (wb->raw)
        free(wb->buffer);
    else
        mf_free(wb->buffer);
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
    }
    wb_list->tail = NULL;
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
#endif
    return 0;
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

    poll->slot_index = 0;

    poll->slot_size = 32;
    poll->slots = malloc(sizeof(*poll->slots)*poll->slot_size);
    if (!poll->slots) {
        return -1;
    }

    for (i=0; i<poll->slot_size; i++)
        init_socket_slot(&poll->slots[i]);

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
xnet_poll_deinit(xnet_poll_t *poll) {
    int i;
    xnet_socket_t *s;

#ifdef _WIN32
    dlink_node_t *tmp;
    while (poll->socket_list) {
        tmp = poll->socket_list;
        poll->socket_list = poll->socket_list->next;
        free(tmp);
    }
#else
    closesocket(poll->epoll_fd);
#endif

    closesocket(poll->send_fd);
    closesocket(poll->recv_fd);

    for (i=0; i<poll->slot_size; i++) {
        s = &poll->slots[i];
        if (s->type == SOCKET_TYPE_INVALID && s->fd) {
            closesocket(s->fd);
        }
        clear_wb_list(&s->wb_list);
    }

    if (poll->slots) {
        free(poll->slots);
        poll->slots = NULL;
    }
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
    return 0;
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
    s->unpacker = NULL;
    s->user_ptr = NULL;
    clear_wb_list(&s->wb_list);
    return 0;
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

static SOCKET_TYPE
do_bind(const char *host, int port, int protocol, int *family) {
    SOCKET_TYPE fd = -1;
    int status;
    int reuse = 1;
    struct addrinfo ai_hints;
    struct addrinfo *ai_list = NULL;
    char str_port[16];

    if (host == NULL || host[0] == 0) host = "0.0.0.0";
    memset(&ai_hints, 0, sizeof(ai_hints));
    sprintf(str_port, "%d", port);

    ai_hints.ai_family = AF_UNSPEC;
    ai_hints.ai_protocol = protocol;
    ai_hints.ai_socktype = (protocol==IPPROTO_TCP)?SOCK_STREAM:SOCK_DGRAM;

    status = getaddrinfo(host, str_port, &ai_hints, &ai_list);
    if ( status != 0 ) return -1;

    fd = socket(ai_list->ai_family, ai_list->ai_socktype, 0);
    if (fd < 0) goto FAILED_FD;

    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (void *)&reuse, sizeof(int)) == -1)
        goto FAILED;

    if (bind(fd, (struct sockaddr*)ai_list->ai_addr, ai_list->ai_addrlen) == -1)
        goto FAILED;

    *family = ai_list->ai_family;
    freeaddrinfo(ai_list);
    return fd;
FAILED:
    closesocket(fd);
FAILED_FD:
    freeaddrinfo(ai_list);
    return -1;
}

int
xnet_listen_tcp_socket(xnet_poll_t *poll, const char *host, int port, int backlog, xnet_socket_t **socket_out) {
    xnet_socket_t *s;
    int id, family;
    SOCKET_TYPE fd = do_bind(host, port, IPPROTO_TCP, &family);
    if (fd < 0) return -1;

    if (listen(fd, backlog) == -1) goto FAILED;

    id = alloc_socket_id(poll);
    if (id == -1) goto FAILED;

    s = new_fd(poll, fd, id, SOCKET_PROTOCOL_TCP, true);
    s->type = SOCKET_TYPE_LISTENING;

    if (socket_out) *socket_out = s;
    return 0;
FAILED:
    closesocket(fd);
    return -1;
}

int
xnet_accept_tcp_socket(xnet_poll_t *poll, xnet_socket_t *listen_s, xnet_socket_t **socket_out) {
    union sockaddr_all client_addr;
    xnet_socket_t *s;
    socklen_t client_addrlen = sizeof(client_addr);
    SOCKET_TYPE fd = 0;
    int id;
    
    if ((fd = accept(listen_s->fd, &client_addr.s, &client_addrlen)) == -1) {
        return -1;
    }

    id = alloc_socket_id(poll);
    if (id == -1) goto FAILED;
    s = new_fd(poll, fd, id, SOCKET_PROTOCOL_TCP, true);
    s->type = SOCKET_TYPE_ACCEPTED;

    set_nonblocking(fd);
    set_keepalive(fd);

    if (client_addrlen == sizeof(client_addr.v4))
        xnet_gen_addr(SOCKET_ADDR_TYPE_IPV4, &client_addr, &s->addr_info);
    else
        xnet_gen_addr(SOCKET_ADDR_TYPE_IPV6, &client_addr, &s->addr_info);

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
xnet_connect_tcp_socket(xnet_poll_t *poll, const char *host, int port, xnet_socket_t **socket_out) {
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
        //connect success
        s->type = SOCKET_TYPE_CONNECTED;
        freeaddrinfo(ai_list);
        return 1;
    }

    //connecting
    s->type = SOCKET_TYPE_CONNECTING;
    xnet_enable_write(poll, s, true);
    freeaddrinfo(ai_list);
    return 0;
FAILED:
    freeaddrinfo(ai_list);
    return -1;
}

int
xnet_listen_udp_socket(xnet_poll_t *poll, const char *host, int port, xnet_socket_t **socket_out) {
    xnet_socket_t *s;
    SOCKET_TYPE fd;
    int id, family, protocol;
    fd = do_bind(host, port, IPPROTO_UDP, &family);
    if (fd < 0) return -1;

    id = alloc_socket_id(poll);
    if (id == -1) goto FAILED;
    
    protocol = (family == AF_INET6) ? SOCKET_PROTOCOL_UDP_IPV6 : SOCKET_PROTOCOL_UDP;
    s = new_fd(poll, fd, id, protocol, true);
    s->type = SOCKET_TYPE_CONNECTED;
    set_nonblocking(fd);
#ifdef _WIN32
    disable_udp_resterr(fd);
#endif
    if (socket_out) *socket_out = s;
    return 0;
FAILED:
    if (fd) closesocket(fd);
    return -1;
}

int
xnet_create_udp_socket(xnet_poll_t *poll, int protocol, xnet_socket_t **socket_out) {
    xnet_socket_t *s;
    SOCKET_TYPE fd;
    int id, family;

    if (protocol == SOCKET_PROTOCOL_UDP){
        family = AF_INET;
    } else if (protocol == SOCKET_PROTOCOL_UDP_IPV6) {
        family = AF_INET6;
    } else {
        return -1;
    }
    fd = socket(family, SOCK_DGRAM, 0);
    if (fd < 0) return -1;

    id = alloc_socket_id(poll);
    if (id == -1) goto FAILED;
    s = new_fd(poll, fd, id, protocol, true);
    s->type = SOCKET_TYPE_CONNECTED;
    set_nonblocking(fd);
#ifdef _WIN32
    disable_udp_resterr(fd);
#endif
    if (socket_out) *socket_out = s;
    return 0;
FAILED:
    if (fd) closesocket(fd);
    return -1;
}

int
xnet_set_udp_socket_addr(xnet_poll_t *poll, xnet_socket_t *s, const char *host, int port) {
    int status, protocol, addr_type;
    struct addrinfo ai_hints;
    struct addrinfo *ai_list = NULL;
    char str_port[16];

    sprintf(str_port, "%d", port);
    memset(&ai_hints, 0, sizeof(ai_hints));
    ai_hints.ai_family = AF_UNSPEC;
    ai_hints.ai_socktype = SOCK_DGRAM;
    ai_hints.ai_protocol = IPPROTO_UDP;

    status = getaddrinfo(host, str_port, &ai_hints, &ai_list);
    if ( status != 0 ) {
        return -1;
    }

    if (ai_list->ai_family == AF_INET) {
        protocol = SOCKET_PROTOCOL_UDP;
        addr_type = SOCKET_ADDR_TYPE_IPV4;
    } else if (ai_list->ai_family == AF_INET6) {
        protocol = SOCKET_PROTOCOL_UDP_IPV6;
        addr_type = SOCKET_ADDR_TYPE_IPV6;
    } else {
        freeaddrinfo(ai_list);
        return -1;
    }

    if (s->protocol != protocol) {
        freeaddrinfo(ai_list);
        return -1;
    }

    xnet_gen_addr(addr_type, (union sockaddr_all *)ai_list->ai_addr, &s->addr_info);

    freeaddrinfo(ai_list);
    return 0;
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
xnet_recv_udp_data(xnet_poll_t *poll, xnet_socket_t *s, xnet_addr_t *addr_out) {
    union sockaddr_all sa;
    socklen_t slen = sizeof(sa);
    int n, err;

    n = recvfrom(s->fd, poll->udp_buffer, MAX_UDP_PACKAGE, 0, &sa.s, &slen);
    if (n < 0) {
        err = get_last_error();
        printf("recvfrom error:[%d]\n", err);
        return -1;
    }

    if (slen == sizeof(sa.v4)) {
        if (s->protocol != SOCKET_PROTOCOL_UDP)
            return -1;
        xnet_gen_addr(SOCKET_ADDR_TYPE_IPV4, &sa, addr_out);
    } else {
        if (s->protocol != SOCKET_PROTOCOL_UDP_IPV6)
            return -1;
        xnet_gen_addr(SOCKET_ADDR_TYPE_IPV6, &sa, addr_out);
    }
    return n;
}

static int
send_tcp_data(xnet_poll_t *poll, xnet_socket_t *s) {
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

    wb_list->tail = NULL;
    if (s->closing) {
        xnet_poll_closefd(poll, s);
    } else {
        //sending is over,disable write event
        xnet_enable_write(poll, s, false);
    }

    return -1;
}

static int
send_udp_data(xnet_poll_t *poll, xnet_socket_t *s) {
    xnet_wb_list_t *wb_list = &s->wb_list;
    xnet_udp_wirte_buff_t *udp_wb;
    int n, err;
    union sockaddr_all sa;
    socklen_t sasz;

    while (wb_list->head) {
        udp_wb = (xnet_udp_wirte_buff_t*)wb_list->head;
        sasz = xnet_addr_to_sockaddr(&udp_wb->udp_addr, &sa);
        n = sendto(s->fd, udp_wb->wb.ptr, udp_wb->wb.sz, 0, &sa.s, sasz);

        if (n < 0) {
            err = get_last_error();
            if (err == XNET_EINTR || XNET_HAVR_WOULDBLOCK(err)) return -1;
            
            //drop it, save other package to try for next time.
            s->wb_size -= udp_wb->wb.sz;
            wb_list->head = udp_wb->wb.next;
            free_wb((xnet_write_buff_t*)udp_wb);
            return -1;
        }
        s->wb_size -= udp_wb->wb.sz;
        wb_list->head = udp_wb->wb.next;
        free_wb((xnet_write_buff_t*)udp_wb);
    }
    wb_list->tail = NULL;

    if (s->closing) {
        xnet_poll_closefd(poll, s);
    } else {
        //sending is over,disable write event
        xnet_enable_write(poll, s, false);
    }

    return -1;
}

int
xnet_send_data(xnet_poll_t *poll, xnet_socket_t *s) {
    if (s->protocol == SOCKET_PROTOCOL_TCP)
        return send_tcp_data(poll, s);
    else
        return send_udp_data(poll, s);
}

//buffer must be assigned by mf_malloc
void
append_send_buff(xnet_poll_t *poll, xnet_socket_t *s, const char *buffer, int sz, bool raw) {
    xnet_write_buff_t *wb = (xnet_write_buff_t *)malloc(sizeof(xnet_write_buff_t));
    wb->buffer = wb->ptr = (char*)buffer;
    wb->sz = sz;
    wb->next = NULL;
    wb->raw = raw;
    insert_wb_list(&s->wb_list, wb);
    s->wb_size += sz;

    xnet_enable_write(poll, s, true);
}

void
append_udp_send_buff(xnet_poll_t *poll, xnet_socket_t *s, xnet_addr_t *addr, const char *buffer, int sz, bool raw) {
    xnet_udp_wirte_buff_t *udp_wb = (xnet_udp_wirte_buff_t *)malloc(sizeof(xnet_udp_wirte_buff_t));
    udp_wb->wb.buffer = udp_wb->wb.ptr = (char*)buffer;
    udp_wb->wb.sz = sz;
    udp_wb->wb.next = NULL;
    udp_wb->wb.raw = raw;
    memcpy(&udp_wb->udp_addr, addr, sizeof(xnet_addr_t));
    insert_wb_list(&s->wb_list, (xnet_write_buff_t*)udp_wb);

    s->wb_size += sz;
    xnet_enable_write(poll, s, true);
}

void
block_recv(SOCKET_TYPE fd, void *buffer, int sz) {
    int err, n;
    for (;;) {
#ifdef _WIN32
        n = recv(fd, buffer, sz, 0);
#else
        n = read(fd, buffer, sz);
#endif
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
#ifdef _WIN32
        n = send(fd, buffer, sz, 0);
#else
        n = write(fd, buffer, sz);
#endif
        if (n < 0) {
            err = get_last_error();
            if (err != XNET_EINTR) {
                printf("block_send error:%d, %d, %d, %d\n", err, fd, n, sz);
            }
            continue;
        }
        assert(n == sz);
        return;
    }
}

inline int
get_sockopt(SOCKET_TYPE fd, int level, int optname, int *optval, socklen_t *optlen) {
#ifdef _WIN32
    return getsockopt(fd, level, optname, (char*)optval, optlen);
#else
    return getsockopt(fd, level, optname, optval, optlen);
#endif
}

void
xnet_addrtoa(xnet_addr_t *addr, char str[64]) {
//ipv6 exmple: [2001:0db8:85a3:0000:0000:8a2e:0370:7334]:65535
//ipv4 exmple: 255.255.255.255:65535
    char tmp[INET6_ADDRSTRLEN];
    if (addr->type == SOCKET_ADDR_TYPE_IPV4) {
        inet_ntop(AF_INET, addr->addr, tmp, sizeof(tmp));
        sprintf(str, "%s:%d", tmp, addr->port);
    } else {
        inet_ntop(AF_INET6, addr->addr, tmp, sizeof(tmp));
        sprintf(str, "%s:%d", tmp, addr->port);
    }
}