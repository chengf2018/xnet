#include "xnet.h"
#include <pthread.h>
static xnet_context_t *g_worker_ctx = NULL;

static void
listen_func(xnet_context_t *ctx, xnet_socket_t *s, xnet_socket_t *ns) {
	xnet_error(ctx, "-----socket [%d] accept new, new socket:[%d]", s->id, ns->id);
}

static void
error_func(xnet_context_t *ctx, xnet_socket_t *s, short what) {
	xnet_error(ctx, "-----socket [%d] error, what:[%u]", s->id, what);
}

static int
recv_func(xnet_context_t *ctx, xnet_socket_t *s, char *buffer, int size) {
    char *str = malloc(size+1);
    memcpy(str, buffer, size);
    str[size] = '\0';

	xnet_error(ctx, "-----socket [%d] recv buffer [%s], size[%d]", s->id, str, size);
	xnet_send_buffer(ctx, s, buffer, size);
    free(str);
    if (g_worker_ctx) {
        xnet_send_command(g_worker_ctx, ctx, 1, buffer, size);
        return 1;//返回非0，自行释放buffer
    }

    return 0;//返回0,由xnet释放
}

static void
timeout_func(xnet_context_t *ctx, int id) {
    static uint64_t last_time = 0;

    xnet_error(ctx, "timeout event[%d], nowtime:%llu, nowtime2:%llu,nowtime3:%llu", id, ctx->nowtime, get_time(), last_time);
    //xnet_add_timer(ctx, id, 2000);

    //last_time = get_time();
}


static int
command_func(xnet_context_t *ctx, xnet_context_t *source, int command, void *data, int sz) {
#define COMMAND_TYPE_SOCKET 1
    if (command == COMMAND_TYPE_SOCKET) {
        char *str = malloc(sz+1);
        memcpy(str, (char*)data, sz);
        str[sz] = '\0';
        xnet_error(ctx, "socket [%s], command", str);
        free(str);
    }
    xnet_error(ctx, "deal command[%d], size:[%d]", command, sz);
    return 0;//返回0，由xnet释放data
}

static void *
thread_worker(void *p) {
    xnet_context_t *ctx = p;
    xnet_register_command(ctx, command_func);
xnet_error(ctx, "start worker thread loop");
    xnet_dispatch_loop(ctx);
xnet_error(ctx, "worker thread exit loop");
}

int
main(int argc, char** argv) {
    int ret;
    xnet_context_t *ctx;
    pthread_t pid;

    ret = xnet_init();
    if (ret != 0) {
        printf("xnet init error:%d\n", ret);
        return 1;
    }

    ctx = xnet_create_context();
xnet_error(ctx, "------start run------");
g_worker_ctx = xnet_create_context();
pthread_create(&pid, NULL, thread_worker, g_worker_ctx);
pthread_detach(pid);

    xnet_register_listener(ctx, listen_func, error_func, recv_func);
xnet_error(ctx, "------start listen------");
    ret = xnet_listen(ctx, 8888, NULL);
    if (ret != 0) goto _END;

    xnet_register_timeout(ctx, timeout_func);
    xnet_add_timer(ctx, 1, 2000);
xnet_error(ctx, "------start loop------");
	xnet_dispatch_loop(ctx);
xnet_error(ctx, "exit loop");
_END:
    xnet_release_context(ctx);

    if (g_worker_ctx) xnet_release_context(g_worker_ctx);
    xnet_deinit();
    return 0;
}