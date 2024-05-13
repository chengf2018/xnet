#include "../src/xnet.h"

static xnet_socket_t *g_s = NULL;

static void
connected_func(struct xnet_context_t *ctx, xnet_socket_t *s, int error) {
	xnet_error(ctx, "-----socket [%d] connected. error:[%d]", s->id, error);
    if (error == 0) {g_s = s;}
    else {
        xnet_add_timer(ctx, 2, 5000);
    }
}

static void
error_func(struct xnet_context_t *ctx, xnet_socket_t *s, short what) {
	xnet_error(ctx, "-----socket [%d] error, what:[%u]", s->id, what);
    g_s = NULL;
    xnet_add_timer(ctx, 2, 5000);
}

static int
recv_func(struct xnet_context_t *ctx, xnet_socket_t *s, char *buffer, int size) {
	xnet_error(ctx, "-----socket [%d] recv buffer[%s], size[%d]", s->id, buffer, size);
    return 0;
}

static void
timeout_func(struct xnet_context_t *ctx, int id) {
    char buffer[] = "hello world!";

    if (id == 1) {
        xnet_add_timer(ctx, 1, 2000);
        if (g_s) {
            xnet_error(ctx, "send buffer");
            xnet_send_buffer(ctx, g_s, buffer, sizeof(buffer));
        }
    } else if (id == 2) {
        xnet_error(ctx, "try connect server");
        xnet_connect(ctx, "127.0.0.1", 8888, NULL);
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

    xnet_connect(ctx, "127.0.0.1", 8888, NULL);
    if (ret != 0) goto _END;

    xnet_register_timeout(ctx, timeout_func);
    xnet_add_timer(ctx, 1, 2000);

	xnet_dispatch_loop(ctx);
_END:
    xnet_release_context(ctx);
    xnet_deinit();
    return 0;
}