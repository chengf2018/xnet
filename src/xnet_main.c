#include "xnet.h"

static void
listen_func(struct xnet_context_t *ctx, xnet_socket_t *s, xnet_socket_t *ns) {
	printf("-----socket [%d] accept new, new socket:[%d]\n", s->id, ns->id);
}

static void
error_func(struct xnet_context_t *ctx, xnet_socket_t *s, short what) {
	printf("-----socket [%d] error, what:[%u]\n", s->id, what);
}

static void
recv_func(struct xnet_context_t *ctx, xnet_socket_t *s, char *buffer, int size) {
	printf("-----socket [%d] recv buffer, size[%d]\n", s->id, size);
	xnet_send_buffer(ctx, s, buffer, size);
}

int
main(int argc, char** argv) {
    int ret;
    xnet_context_t *ctx;

    ret = xnet_init();
    if (ret != 0) {
        printf("xnet init error:%d\n", ret);
        return 1;
    }
printf("start run\n");
    ctx = xnet_create_context();

printf("start run1111\n");
    ret = xnet_register_listener(ctx, 8888, listen_func, error_func, recv_func);
    if (ret != 0) goto _END;
printf("start run2222\n");
	xnet_dispatch_loop(ctx);
printf("start run3333\n");
_END:
    xnet_release_context(ctx);
    xnet_deinit();
    return 0;
}