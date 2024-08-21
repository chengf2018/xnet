#include "../src/xnet.h"

static int g_s = -1;

static void
error_func(struct xnet_context_t *ctx, int sock_id, short what) {
	xnet_error(ctx, "-----socket [%d] error, what:[%u]", sock_id, what);
}

static int
recv_func(struct xnet_context_t *ctx, int sock_id, char *buffer, int size, xnet_addr_t *addr_info) {
    char str[64] = {0};
    xnet_addrtoa(addr_info, str);
	xnet_error(ctx, "-----socket [%d] recv buffer[%s], size[%d], from[%s]", sock_id, buffer, size, str);
    xnet_udp_sendto(ctx, sock_id, addr_info, buffer, size, false);
    return 0;
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
    xnet_register_listener(ctx, NULL, error_func, recv_func);
    g_s = xnet_udp_listen(ctx, "0.0.0.0", 4396);
    if (g_s == -1) goto _END;
xnet_error(ctx, "start server loop");
	xnet_dispatch_loop(ctx);
_END:
    xnet_destroy_context(ctx);
    xnet_deinit();
    return 0;
}