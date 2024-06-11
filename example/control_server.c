#include "../src/xnet.h"
#include "../src/xnet_packer.h"

static xnet_socket_t *g_s;

static void
error_func(struct xnet_context_t *ctx, xnet_socket_t *s, short what) {
	xnet_error(ctx, "-----socket [%d] error, what:[%u]", s->id, what);
	if (s->unpacker) {
		xnet_unpacker_free(s->unpacker);
		s->unpacker = NULL;
	}
}

static int
recv_func(struct xnet_context_t *ctx, xnet_socket_t *s, char *buffer, int size, xnet_addr_t *addr_info) {
	if (s->unpacker) {
		xnet_error(ctx, "---recv http socket data---\n");
		if (xnet_unpacker_recv(s->unpacker, buffer, size) != 0) {
			printf("unpacker recv error!\n");
		}
	}
    return 0;
}

static void
response(xnet_context_t *ctx, xnet_socket_t *s, const char *command) {
	//to deal command...

	xnet_tcp_send_buffer(ctx, s, "OK\r\n", 4, false);
}

static void
deal_command(xnet_unpacker_t *up, void *arg) {
	xnet_linebuffer_t *req = (xnet_linebuffer_t *)arg;
	xnet_context_t *ctx = (xnet_context_t *)up->user_ptr;
	xnet_socket_t *s = (xnet_socket_t *)up->user_arg;

	printf("command:[%s]\n", xnet_string_get_c_str(&req->line_str));
	printf("sep:[%d]\n", req->sep);
	response(ctx, s, xnet_string_get_c_str(&req->line_str));
	xnet_tcp_send_buffer(ctx, s, ">", 1, false);
}

static void
listen_func(xnet_context_t *ctx, xnet_socket_t *s, xnet_socket_t *ns, xnet_addr_t *addr_info) {
    char str[64] = {0};
    const char welcome[] = "welcome to console:\r\n>";
    xnet_unpacker_t *up;
    xnet_addrtoa(addr_info, str);
	xnet_error(ctx, "-----socket [%d] accept new, new socket:[%d], [%s]", s->id, ns->id, str);

	up = xnet_unpacker_new(sizeof(xnet_linebuffer_t), deal_command, xnet_unpack_line, xnet_clear_line, 256);
	if (up == NULL) {
		printf("new unpacker error\n");
		return;
	}
	up->user_ptr = ctx;
	up->user_arg = ns;
	ns->unpacker = up;
	xnet_tcp_send_buffer(ctx, ns, welcome, sizeof(welcome)-1, false);
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
    if (!ctx) {
    	printf("xnet create context fail\n");
    	return 1;
    }

    xnet_register_listener(ctx, listen_func, error_func, recv_func);
    ret = xnet_tcp_listen(ctx, "0.0.0.0", 8082, 200, &g_s);
    if (ret != 0) goto _END;
	
	xnet_error(ctx, "------start loop------");
	xnet_dispatch_loop(ctx);
	xnet_error(ctx, "------end loop------");
_END:
    xnet_destroy_context(ctx);
    xnet_deinit();
    return 0;
}