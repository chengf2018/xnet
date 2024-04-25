#include "xnet.h"

#define CMD_TYPE_EXIT 0
#define CMD_TYPE_LISTEN 1
#define CMD_TYPE_CLOSE 2
#define CMD_TYPE_CONNECT 3
#define CMD_TYPE_SNED_TCP_PKG 4

static void
cmd_listen(xnet_context_t *ctx, xnet_cmdreq_listen_t *req) {
    xnet_listen_tcp_socket(&ctx->poll, req->port, NULL);
}

static void
cmd_close(xnet_context_t *ctx, xnet_cmdreq_close_t *req) {
    xnet_socket_t *s = &ctx->poll.slots[req->id];
    if (s->type == SOCKET_TYPE_INVALID)
        return;

    xnet_poll_closefd(&ctx->poll, s);
}

static void
cmd_connect() {

}

static void
cmd_send_tcp_pkg() {

}

static int
has_cmd(xnet_context_t *ctx) {
    struct timeval tv = {0,0};
    int retval;
    SOCKET_TYPE recv_fd = ctx->poll.recv_fd;

    FD_SET(recv_fd, &ctx->poll.rfds);
    retval = select(recv_fd, &ctx->poll.rfds, NULL, NULL, &tv);
    if (retval == 1) {
        return 1;
    }
    return 0;
}

static int
ctrl_cmd(xnet_context_t *ctx) {
    SOCKET_TYPE fd = ctx->poll.recv_fd;
    uint8_t *buffer[256];
    uint8_t header[2];
    int type, len;

    block_recv(fd, buffer, sizeof(header));
    type = header[0]; len = header[1];
    block_recv(fd, buffer, len);

    switch (type) {
        case CMD_TYPE_EXIT:
            return 0;
        case CMD_TYPE_LISTEN:
            cmd_listen(ctx, (xnet_cmdreq_listen_t *)buffer);
        break;
        case CMD_TYPE_CLOSE:
        case CMD_TYPE_CONNECT:
        case CMD_TYPE_SNED_TCP_PKG:
        break;
    }
    return -1;
}

static void
send_cmd(xnet_context_t *ctx, xnet_cmdreq_t *req, int type, int len) {
    SOCKET_TYPE fd = ctx->poll.send_fd;
    req->header[6] = (uint8_t)type;
    req->header[7] = (uint8_t)len;
    block_send(fd, &req->header[6], len+2);
}

int
xnet_init() {
    xnet_socket_init();
    return 0;
}


int
xnet_deinit() {
    xnet_socket_deinit();
    return 0;
}


xnet_context_t *
xnet_create_context() {
    xnet_context_t *ctx;

    ctx = (xnet_context_t *)malloc(sizeof(xnet_context_t));
    if (!ctx) return NULL;

    //xnet_mq_init(&ctx->mq);
    xnet_poll_init(&ctx->poll);

    ctx->listen_func = NULL;
    ctx->recv_func = NULL;
    ctx->error_func = NULL;
    ctx->connect_func = NULL;
    return ctx;
}

void
xnet_release_context(xnet_context_t *ctx) {
    free(ctx);
}

int
xnet_register_listener(xnet_context_t *ctx, int port, xnet_listen_func_t listen_func, \
    xnet_error_func_t error_func, xnet_recv_func_t recv_func) {
    int ret;

    if (port != 0) {
        ret = xnet_listen_tcp_socket(&ctx->poll, port, NULL);
        if (ret == -1) {
            return -1;
        }
    }

    ctx->listen_func = listen_func;
    ctx->error_func = error_func;
    ctx->recv_func = recv_func;
    return 0;
}

int
xnet_dispatch_loop(xnet_context_t *ctx) {
    int ret, i;
    xnet_poll_event_t *poll_event = &ctx->poll.poll_event;
    xnet_poll_t *poll = &ctx->poll;
    xnet_socket_t *s, *ns;
    char *buffer;

    while (1) {
        if (has_cmd(ctx)) {
            ret = ctrl_cmd(ctx);
            if (ret == 0) {
                printf("exit loop\n");
                break;
            }
        }

        ret = xnet_poll_wait(poll);
        if (ret < 0) {
            //have error?
            printf("xnet_poll_wait error:%d\n", ret);
            continue;
        }
        //处理定时事件
        //... do deal with timieout event

        //对触发的事件进行处理
        for (i=0; i<poll_event->n; i++) {
            s = poll_event->s[i];

            if (poll_event->read[i]) {
                //输入缓存区有可读数据；处于监听状态，有连接到达；出错
                if (s->type == SOCKET_TYPE_LISTENING) {
                    ns = NULL;
                    if (xnet_accept_tcp_socket(poll, s, &ns) == 0) {
                        ctx->listen_func(ctx, s, ns);
                    } else {
                        printf("accept tcp socket have error:%d\n", s->id);
                    }
                } else {
                    ret = xnet_recv_data(poll, s, &buffer);
                    if (ret > 0) {
                        ctx->recv_func(ctx, s, buffer, ret);
                        free(buffer); buffer = NULL;
                    } else if (ret < 0) {
                        ctx->error_func(ctx, s, 0);
                        xnet_poll_closefd(poll, s);
                    }
                }
            }

            if (poll_event->write[i]) {
                //输出缓冲区可写；连接成功
                if (s->type == SOCKET_TYPE_CONNECTING) {
                    s->type = SOCKET_TYPE_CONNECTED;
                    xnet_enable_write(poll, s, false);
                    ctx->connect_func(ctx, s);
                } else {
                    //发送缓冲列表内的数据
                    xnet_send_data(&ctx->poll, s);
                    //ctx->send_func(ctx, s);发送成功通知？暂时不需要
                }
            }

            if (poll_event->error[i]) {
                //异常；带外数据
                printf("poll event error:%d\n", s->id);
                ctx->error_func(ctx, s, 1);
                xnet_poll_closefd(&ctx->poll, s);
            }
        }
    }

    return 0;
}

//只能在主线程使用，后续需要支持多线程
void
xnet_send_buffer(xnet_context_t *ctx, xnet_socket_t *s, char *buffer, int sz) {
    char *new_buffer = (char*)malloc(sz);
    memcpy(new_buffer, buffer, sz);
    append_send_buff(&ctx->poll, s, new_buffer, sz);
}
