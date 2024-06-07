#include "../src/xnet_packer.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

const char *g_sizebuff_pack_text = "hello world!";

static void
sizebuffer_callback(xnet_unpacker_t *up, void *arg) {
	xnet_sizebuffer_t *sb = (xnet_sizebuffer_t *)arg;
	printf("%s\n", sb->recv_buffer);
	printf("size:[%d]\n", sb->buffer_size);
	assert(sb->buffer_size == strlen(g_sizebuff_pack_text)+1 && strcmp(sb->recv_buffer, g_sizebuff_pack_text) == 0);
}

void
test_sizebuffer_unpacker() {
	xnet_unpacker_t *up;
	uint32_t len = strlen(g_sizebuff_pack_text) + 1;
	char buffer[512];
	int ret;
printf("--start test sizebuffer unpacker--\n");
	up = xnet_unpacker_new(sizeof(xnet_sizebuffer_t), sizebuffer_callback, xnet_unpack_sizebuffer, xnet_clear_sizebuffer, 1024);
	assert(up);

	memcpy(buffer, &len, sizeof(len));
	memcpy(buffer+sizeof(len), g_sizebuff_pack_text, len);
	ret = xnet_unpacker_recv(up, buffer, len);
	assert(ret == 0);
printf("step1,xnet_unpacker_recv, ret[%d]\n", ret);
	ret = xnet_unpacker_recv(up, buffer+len, sizeof(len));
printf("step2,xnet_unpacker_recv, ret[%d]\n", ret);
	assert(ret == 0);

	xnet_unpacker_free(up);
printf("--finshed sizebuffer unpacker test--\n");
}

static char *g_http_request_test_case[] = {
"GET / HTTP/1.1\r\n"
"User-Agent: test\r\n"
"Host: 127.0.0.1\r\n"
"Accept: */*\r\n"
"\r\n",

"GET / HTTP/1.2\r\n"
"User-Agent: test\r\n"
"Host: 127.0.0.1\r\n"
"Accept: */*\r\n"
"\r\n",

"POST / HTTP/1.1\r\n"
"User-Agent:test\r\n"
"Host: 127.0.0.1\r\n"
"Accept: */*\r\n"
"\r\n",

"GET / HTTP/1.1\r\n"
"User-Agent: test\r\n"
"Host: 127.0.0.1\r\n"
"Accept: */*\r\n"
"\n",

"GET / HTTP/1.1\r\n"
"User-\rAgent: test\r\n"
"Host: 127.0.0.1\r\n"
"Accept: */*\r\n"
"\r\n",

"POST / HTTP/1.1\r\n"
"User-Agent: test\r\n"
"Host: 127.0.0.1\r\n"
"Accept: */*\r\n"
"Content-Length: 26\r\n"
"\r\n"
"abcdefghijklmnopqrstuvwxyz",

"POST / HTTP/1.1\r\n"
"User-Agent: test\r\n"
"Host: 127.0.0.1\r\n"
"Accept: */*\r\n"
"Content-Length: 26\r\n"
"\r\n"
"abcdefghijklmnopqrstuvwxyz???????????",

"POST / HTTP/1.1\r\n"
"User-Agent: test\r\n"
"Host: 127.0.0.1\r\n"
"Accept: */*\r\n"
"\r\n"
"abcdefghijklmnopqrstuvwxyz???????????"
};

int g_http_request_expect[] = {
0,-1,-1,-1,-1,0,0,-1
};

static void
http_callback(xnet_unpacker_t *up, void *arg) {
	int i;
	xnet_httprequest_t *req = (xnet_httprequest_t *)arg;
printf("http callback:\n");
printf("method:%s\nurl:%s\nversion:%s\n",
	xnet_string_get_c_str(&req->method),
	xnet_string_get_c_str(&req->url),
	xnet_string_get_c_str(&req->version)
);
printf("header token:\n");
	for (i=0; i<req->header_count; i++) {
printf("%s : %s\n", xnet_string_get_c_str(&req->header[i].key),
	xnet_string_get_c_str(&req->header[i].value)
);
printf("header content length:[%d]\n", req->content_length);
	if (req->body) {
printf("body:\n%s\n", xnet_string_get_c_str(req->body));
	}
	}
}

void
test_http_unpacker() {
	xnet_unpacker_t *up;
	uint32_t len;

	int ret, i;
printf("--start test http unpacker--\n");
	up = xnet_unpacker_new(sizeof(xnet_httprequest_t), http_callback, xnet_unpack_http, xnet_clear_http, 1024);
	assert(up);
	for (i=0; i<sizeof(g_http_request_test_case)/sizeof(char*); i++) {
		len = strlen(g_http_request_test_case[i]);
		ret = xnet_unpacker_recv(up, g_http_request_test_case[i], len);
		printf("test case[%d], ret[%d], expect[%d]\n", i, ret, g_http_request_expect[i]);
		assert(g_http_request_expect[i] == ret);
	}

	xnet_unpacker_free(up);
printf("--finshed http unpacker test--\n");
}

int
main(int argc, char **argv) {
	test_sizebuffer_unpacker();

	test_http_unpacker();
	return 0;
}