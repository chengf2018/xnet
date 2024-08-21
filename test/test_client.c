#include "../src/xnet.h"

static int g_s = -1;

static void
connected_func(struct xnet_context_t *ctx, int sock_id, int error) {
	xnet_error(ctx, "-----socket [%d] connected. error:[%d]", sock_id, error);
    if (error == 0) {g_s = sock_id;}
    else {
        xnet_add_timer(ctx, 2, 5000);
    }
}

static void
error_func(struct xnet_context_t *ctx, int sock_id, short what) {
    xnet_socket_t *s = xnet_get_socket(ctx, sock_id);
	xnet_error(ctx, "-----socket [%d] error, what:[%u]", s->id, what);
    g_s = -1;
    xnet_add_timer(ctx, 2, 5000);
}

static int
recv_func(struct xnet_context_t *ctx, int sock_id, char *buffer, int size, xnet_addr_t *addr_info) {
    xnet_socket_t *s = xnet_get_socket(ctx, sock_id);
	xnet_error(ctx, "-----socket [%d] recv buffer[%s], size[%d]", s->id, buffer, size);
    return 0;
}

static void
timeout_func(struct xnet_context_t *ctx, int id) {
    char buffer[] = "hello world!";

    if (id == 1) {
        xnet_add_timer(ctx, 1, 2);
        if (g_s != -1) {
            xnet_error(ctx, "send buffer");
            xnet_tcp_send_buffer(ctx, g_s, buffer, sizeof(buffer), false);
        }
    } else if (id == 2) {
        xnet_error(ctx, "try connect server");
        xnet_tcp_connect(ctx, "127.0.0.1", 8888);
    }
}

int
main(int argc, char** argv) {
    int ret;
    xnet_context_t *ctx;

    ret = xnet_init(NULL);
    if (ret != 0) {
        printf("xnet init error:%d\n", ret);
        return 1;
    }

    ctx = xnet_create_context();
    xnet_register_connecter(ctx, connected_func, error_func, recv_func);

    ret = xnet_tcp_connect(ctx, "127.0.0.1", 8888);
    if (ret == -1) goto _END;

    xnet_register_timeout(ctx, timeout_func);
    xnet_add_timer(ctx, 1, 20);

	xnet_dispatch_loop(ctx);
_END:
    xnet_destroy_context(ctx);
    xnet_deinit();
    return 0;
}