#include "../src/xnet.h"
#include "../src/xnet_config.h"
#include "../src/xnet_util.h"

#include <pthread.h>
static xnet_context_t *g_worker_ctx = NULL;

static void
listen_func(xnet_context_t *ctx, xnet_socket_t *s, xnet_socket_t *ns, xnet_addr_t *addr_info) {
    char str[64] = {0};
    xnet_addrtoa(addr_info, str);
	xnet_error(ctx, "-----socket [%d] accept new, new socket:[%d], [%s]", s->id, ns->id, str);
}

static void
error_func(xnet_context_t *ctx, xnet_socket_t *s, short what) {
	xnet_error(ctx, "-----socket [%d] error, what:[%u]", s->id, what);
}

static int
recv_func(xnet_context_t *ctx, xnet_socket_t *s, char *buffer, int size, xnet_addr_t *addr_info) {
    char *str = xnet_send_buffer_malloc(size+1);
    memcpy(str, buffer, size);
    str[size] = '\0';

	//xnet_error(ctx, "-----socket [%d] recv buffer [%s], size[%d]", s->id, str, size);
	xnet_tcp_send_buffer_ref(ctx, s, str, size, true);
    xnet_tcp_send_buffer_ref(ctx, s, str, size, true);

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
        //xnet_error(ctx, "socket [%s], command", str);
        free(str);
    }
    return 0;//返回0，由xnet释放data
}

static void *
thread_worker(void *p) {
    xnet_context_t *ctx = p;
    xnet_register_command(ctx, command_func);
xnet_error(ctx, "start worker thread loop");
    xnet_dispatch_loop(ctx);
xnet_error(ctx, "worker thread exit loop");
    return NULL;
}

static void
start_init_config(xnet_init_config_t *init_config, const char *config_name) {
    xnet_config_t config;
    char *log_path;
    init_config->log_path = NULL;
    init_config->disable_thread = false;

    if (!config_name) return;

    xnet_config_init(&config);
    if (xnet_parse_config(&config, config_name) != 0) {
        xnet_release_config(&config);
        return;
    }

    if (xnet_get_field2s(&config, "log_path", &log_path)) {
        init_config->log_path = strdup(log_path);
    }

    xnet_get_field2b(&config, "disable_log_thread", &init_config->disable_thread);

    xnet_release_config(&config);
}

static void
release_init_config(xnet_init_config_t *init_config) {
    if (init_config->log_path) free(init_config->log_path);
}

int
main(int argc, char** argv) {
    int ret;
    xnet_context_t *ctx;
    pthread_t pid;
    xnet_init_config_t init_config;
    const char *config_name = NULL;
    if (argc == 2) config_name = argv[1];
printf("config name:[%s]\n", config_name);

    start_init_config(&init_config, config_name);

    ret = xnet_init(&init_config);
    if (ret != 0) {
        printf("xnet init error:%d\n", ret);
        release_init_config(&init_config);
        return 1;
    }

    ctx = xnet_create_context();
xnet_error(ctx, "------start run------");
g_worker_ctx = xnet_create_context();
pthread_create(&pid, NULL, thread_worker, g_worker_ctx);
pthread_detach(pid);

    xnet_register_listener(ctx, listen_func, error_func, recv_func);
xnet_error(ctx, "------start listen------");
    ret = xnet_tcp_listen(ctx, NULL, 8888, 5, NULL);
    if (ret != 0) goto _END;

    xnet_register_timeout(ctx, timeout_func);
    xnet_add_timer(ctx, 1, 2000);
xnet_error(ctx, "------start loop------");
	xnet_dispatch_loop(ctx);
xnet_error(ctx, "------exit loop------");
_END:
    xnet_destroy_context(ctx);

    if (g_worker_ctx) xnet_destroy_context(g_worker_ctx);
    xnet_deinit();
    release_init_config(&init_config);
    return 0;
}
