#ifndef _SOCKET_WIN_H_
#define _SOCKET_WIN_H_

#define XNET_EINTR WSAEINTR
#define XNET_HAVR_WOULDBLOCK(err) (err == WSAEWOULDBLOCK)

//windows下没有pipe函数，模拟实现一个
static int
pipe(int pipefd[2]) {
    SOCKET_TYPE listener = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr = {0};
    socklen_t addrlen = sizeof(addr);
    int reuse = 1;

    if (listener == INVALID_SOCKET) {
        return -1;
    }

    // 创建监听 socket
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = 0;  // 让系统自动分配一个端口

    if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse)) == -1 ||
        bind(listener, (struct sockaddr*)&addr, sizeof(addr)) == -1 ||
        listen(listener, 1) == -1 ||
        getsockname(listener, (struct sockaddr*)&addr, &addrlen) == -1) {
        closesocket(listener);
        return -1;
    }

    // 创建用于写入的 socket
    pipefd[1] = socket(AF_INET, SOCK_STREAM, 0);
    if (pipefd[1] == -1) {
        closesocket(listener);
        return -1;
    }

    // 连接到监听 socket
    if (connect(pipefd[1], (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        closesocket(listener);
        return -1;
    }

    // 接受连接，创建用于读取的 socket
    pipefd[0] = accept(listener, NULL, NULL);
    if (pipefd[0] == -1) {
        closesocket(pipefd[1]);
        pipefd[1] = -1;
        closesocket(listener);
        return -1;
    }

    closesocket(listener);
    return 0;
}

static void
add_to_socketlist(xnet_poll_t *poll, SOCKET_TYPE fd, int id) {
    dlink_node_t *new_node = (dlink_node_t *)malloc(sizeof(dlink_node_t));
    if (!new_node) return;

    new_node->fd = fd;
    new_node->sock_id = id;
    new_node->last = NULL;
    //插到头部
    if (poll->socket_list) {
        new_node->next = poll->socket_list;
        poll->socket_list->last = new_node;
        poll->socket_list = new_node;
    } else {
        poll->socket_list = new_node;
        new_node->next = NULL;
    }
}

static void
del_socketlist(xnet_poll_t *poll, dlink_node_t *node) {
    if (node->last) {
        node->last->next = node->next;
    }
    if (node->next) {
        node->next->last = node->last;
    }
    if (node == poll->socket_list) {
        poll->socket_list = node->next;
    }
    free(node);
}

static void
find_del_socketlist(xnet_poll_t *poll, SOCKET_TYPE fd) {
    dlink_node_t *p = poll->socket_list;
    while (p) {
        if (p->fd == fd) {
            del_socketlist(poll, p);
            break;
        }
        p = p->next;
    }
}

static int
poll_enable_write(xnet_poll_t *poll, xnet_socket_t *s, bool enable) {
    if (s->writing != enable) {
        s->writing = enable;
        if (enable) {
            FD_SET(s->fd, &poll->writefds);
        } else {
            FD_CLR(s->fd, &poll->writefds);
        }
    }
    return 0;
}

static int
poll_enable_read(xnet_poll_t *poll, xnet_socket_t *s, bool enable) {
    if (s->reading != enable) {
        s->reading = enable;
        if (enable) {
            FD_SET(s->fd, &poll->readfds);
        } else {
            FD_CLR(s->fd, &poll->readfds);
        }
    }
    return 0;
}

static void
poll_set_nonblocking(SOCKET_TYPE fd) {
    u_long mode = 1;
    int result = ioctlsocket(fd, FIONBIO, &mode);
    if (result != NO_ERROR) {
        printf("set_nonblocking error:%d\n", fd);
    }
}

static int
poll_wait(xnet_poll_t *poll, int timeout) {
    //select
    dlink_node_t *p;
    xnet_poll_event_t *poll_event = &poll->poll_event;
    int n = 0, activity;
    int have_read, have_write, have_error;
    xnet_socket_t *s;
    fd_set readfds, writefds, errorfds;
    struct timeval tv;

    if (timeout > 0) {
        tv.tv_sec = timeout / 1000;
        tv.tv_usec = (timeout % 1000)*1000;
    }

    readfds = poll->readfds;
    writefds = poll->writefds;
    errorfds = poll->errorfds;
    activity = select(0, &readfds,&writefds, &errorfds,
        (timeout > 0) ? &tv : NULL);
    if (activity == -1)
        return -1;

    memset(poll_event, 0, sizeof(xnet_poll_event_t));

    //select 模式需要遍历socket_list检查是否有触发
    p = poll->socket_list;
    while (p) {
        s = &poll->slots[p->sock_id];
        have_read = false;
        have_write = false;
        have_error = false;
        if (FD_ISSET(p->fd, &readfds))
            have_read = true;
        if (FD_ISSET(p->fd, &writefds))
            have_write = true;
        if (FD_ISSET(p->fd, &errorfds))
            have_error = true;

        if (have_read || have_write || have_error) {
            poll_event->s[n] = s;
            poll_event->read[n] = have_read;
            poll_event->write[n] = have_write;
            poll_event->error[n] = have_error;
            n++;
            if (n >= POLL_EVENT_MAX)
                break;
        }
        
        p = p->next;
    }
    poll_event->n = n;
    return n;
}

const char*
inet_ntop(int af, const void* src, char* dst, int size) {
    struct sockaddr_storage src_addr;

    ZeroMemory(&src_addr, sizeof(struct sockaddr_storage));
    src_addr.ss_family = af;

    if (af == AF_INET6)
        ((struct sockaddr_in6*)&src_addr)->sin6_addr = *(struct in6_addr*)src;
    else if (af == AF_INET)
        ((struct sockaddr_in*)&src_addr)->sin_addr = *(struct in_addr*)src;

    if (WSAAddressToString((struct sockaddr*)&src_addr, sizeof(src_addr), 0, dst, (LPDWORD)&size) != 0)
        return NULL; // call failed

    return dst;
}

#endif //_SOCKET_WIN_H_