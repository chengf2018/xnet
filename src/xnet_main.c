#include "xnet.h"

static xnet_context_t *g_worker_ctx = NULL;

static void
listen_func(xnet_context_t *ctx, xnet_socket_t *s, xnet_socket_t *ns) {
	printf("-----socket [%d] accept new, new socket:[%d]\n", s->id, ns->id);
}

static void
error_func(xnet_context_t *ctx, xnet_socket_t *s, short what) {
	printf("-----socket [%d] error, what:[%u]\n", s->id, what);
}

static int
recv_func(xnet_context_t *ctx, xnet_socket_t *s, char *buffer, int size) {
    char *str = malloc(size+1);
    memcpy(str, buffer, size);
    str[size] = '\0';

	printf("-----socket [%d] recv buffer [%s], size[%d]\n", s->id, str, size);
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

    printf("timeout event[%d], nowtime:%llu, nowtime2:%llu,nowtime3:%llu\n", id, ctx->nowtime, get_time(), last_time);
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
        printf("socket [%s], command\n", str);
        free(str);
    }
    printf("deal command[%d], size:[%d]\n", command, sz);
    return 0;//返回0，由xnet释放data
}

static void *
thread_worker(void *p) {
    xnet_context_t *ctx = p;
    xnet_register_command(ctx, command_func);
printf("start test thread loop\n");
    xnet_dispatch_loop(ctx);
printf("test thread exit loop\n");
    xnet_release_context(ctx);
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


printf("start run\n");
    ctx = xnet_create_context();

g_worker_ctx = xnet_create_context();
pthread_create(&pid, NULL, thread_worker, g_worker_ctx);
pthread_detach(pid);

printf("register listener function\n");
    xnet_register_listener(ctx, listen_func, error_func, recv_func);

    ret = xnet_listen(ctx, 8888, NULL);
    if (ret != 0) goto _END;
printf("register timeout function\n");
    xnet_register_timeout(ctx, timeout_func);
    xnet_add_timer(ctx, 1, 2000);
printf("start loop\n");
	xnet_dispatch_loop(ctx);
printf("exit loop\n");
_END:
    xnet_release_context(ctx);
    xnet_deinit();
    return 0;
}