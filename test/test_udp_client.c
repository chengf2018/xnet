#include "../src/xnet.h"

static int g_s = -1;

static void
error_func(struct xnet_context_t *ctx, int sock_id, short what) {
    xnet_socket_t *s = xnet_get_socket(ctx, sock_id);
	xnet_error(ctx, "-----socket [%d] error, what:[%u]", s->id, what);
}

static int
recv_func(struct xnet_context_t *ctx, int sock_id, char *buffer, int size, xnet_addr_t *addr_info) {
    xnet_socket_t *s = xnet_get_socket(ctx, sock_id);
    char str[64] = {0};
    xnet_addrtoa(addr_info, str);
	xnet_error(ctx, "-----socket [%d] recv buffer[%s], size[%d], from [%s]", s->id, buffer, size, str);
    return 0;
}

static void
timeout_func(struct xnet_context_t *ctx, int id) {
    char str[] = "hello!";
printf("timeout_func send buffer\n");
    xnet_udp_send_buffer(ctx, g_s, str, sizeof(str), false);
    xnet_add_timer(ctx, 1, 2000);
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
    if (ctx == NULL) {
        printf("context create error\n");
        return 1;
    }
    xnet_register_connecter(ctx, NULL, error_func, recv_func);

    g_s = xnet_udp_create(ctx, SOCKET_PROTOCOL_UDP);
    if (g_s == -1) goto _END;
    ret = xnet_udp_set_addr(ctx, g_s, "127.0.0.1", 4396);
    if (ret != 0) goto _END;

    xnet_register_timeout(ctx, timeout_func);
    xnet_add_timer(ctx, 1, 2000);

	xnet_dispatch_loop(ctx);
_END:
    xnet_destroy_context(ctx);
    xnet_deinit();
    return 0;
}