#include "../src/xnet.h"
#include "../src/xnet_packer.h"

static int g_s = -1;

static void
error_func(struct xnet_context_t *ctx, int sock_id, short what) {
	xnet_socket_t *s = xnet_get_socket(ctx, sock_id);
	xnet_error(ctx, "-----socket [%d] error, what:[%u]", sock_id, what);
	if (s->unpacker) {
		xnet_unpacker_free(s->unpacker);
		s->unpacker = NULL;
	}
}

static int
recv_func(struct xnet_context_t *ctx, int sock_id, char *buffer, int size, xnet_addr_t *addr_info) {
	xnet_socket_t *s = xnet_get_socket(ctx, sock_id);
	if (s->unpacker) {
		xnet_error(ctx, "---recv http socket data---\n");
		if (xnet_unpacker_recv(s->unpacker, buffer, size) != 0) {
			printf("unpacker recv error!\n");
		}
	}
    return 0;
}

static void
response(xnet_context_t *ctx, int sock_id, xnet_httpresponse_t *rsp) {
	xnet_string_t out_str;
	xnet_string_init(&out_str);
printf("pack http response\n");
	xnet_pack_http(rsp, &out_str);
printf("send http response, sz:[%d]\n", xnet_string_get_size(&out_str));
printf("[%s]\n", xnet_string_get_c_str(&out_str));
	xnet_tcp_send_buffer(ctx, sock_id, xnet_string_get_str(&out_str), xnet_string_get_size(&out_str), true);
	//xnet_string_clear(&out_str);
	xnet_close_socket(ctx, sock_id);
	xnet_clear_http_rsp(rsp);
}

static void
http_request(xnet_unpacker_t *up, void *arg) {
	xnet_httprequest_t *req = (xnet_httprequest_t *)arg;
	xnet_context_t *ctx = (xnet_context_t *)up->user_ptr;
	int sock_id = (long)up->user_arg;
	xnet_httpresponse_t rsp = {};
	int i;

	printf("method:[%s], url:[%s], version:[%s]\n", xnet_string_get_c_str(&req->method),
		xnet_string_get_c_str(&req->url), xnet_string_get_c_str(&req->version));
	for (i=0; i<req->header_count; i++) {
		printf("-header-[%s]: [%s]\n", xnet_string_get_c_str(&req->header[i].key), 
			xnet_string_get_c_str(&req->header[i].value));
	}

	xnet_set_http_rsp_code(&rsp, req->code);
	xnet_add_http_rsp_header(&rsp, "Content-Type", "text/html");
	//xnet_add_http_rsp_header(&rsp, "Transfer-Encoding", "chunked");
	xnet_set_http_rsp_body(&rsp, "<p>hello world!</p>");

	printf("respone http, state:[%d]:\n", req->code);
	response(ctx, sock_id, &rsp);
}

static void
listen_func(xnet_context_t *ctx, int sock_id, int acc_sock_id) {
	xnet_socket_t *ns = xnet_get_socket(ctx, acc_sock_id);
    char str[64] = {0};
    xnet_unpacker_t *up;
    xnet_addrtoa(&ns->addr_info, str);
	xnet_error(ctx, "-----socket [%d] accept new, new socket:[%d], [%s]", sock_id, acc_sock_id, str);

	up = xnet_unpacker_new(sizeof(xnet_httprequest_t), http_request, xnet_unpack_http, xnet_clear_http, 1024);
	if (up == NULL) {
		printf("new unpacker error\n");
		return;
	}
	up->user_ptr = ctx;
	up->user_arg = (void*)(long)acc_sock_id;
	ns->unpacker = up;
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
    g_s = xnet_tcp_listen(ctx, "0.0.0.0", 8080, 200);
    if (g_s == -1) goto _END;
	
	xnet_error(ctx, "------start loop------");
	xnet_dispatch_loop(ctx);
	xnet_error(ctx, "------end loop------");
_END:
    xnet_destroy_context(ctx);
    xnet_deinit();
    return 0;
}
