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
    printf("-----socket [%d] recv finish\n", s->id);
}

static void
timeout_func(struct xnet_context_t *ctx, int id) {
    static uint64_t last_time = 0;

    printf("timeout event[%d], nowtime:%llu, nowtime2:%llu,nowtime3:%llu\n", id, ctx->nowtime, get_time(), last_time);
    xnet_add_timer(ctx, id, 2000);

    last_time = get_time();
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
    xnet_register_listener(ctx, listen_func, error_func, recv_func);

    ret = xnet_listen(ctx, 8888, NULL);
    if (ret != 0) goto _END;
printf("start run2222\n");
    xnet_register_timeout(ctx, timeout_func);
    xnet_add_timer(ctx, 1, 2000);

	xnet_dispatch_loop(ctx);
printf("start run3333\n");
_END:
    xnet_release_context(ctx);
    xnet_deinit();
    return 0;
}