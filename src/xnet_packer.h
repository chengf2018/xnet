#ifndef _UNPACKER_H_
#define _UNPACKER_H_
#include <stdint.h>
#include <stdbool.h>
#include "xnet_string.h"

typedef struct xnet_unpacker xnet_unpacker_t;


typedef void (*unpack_callback_t)(xnet_unpacker_t *up, void *arg);
typedef uint32_t (*unpack_method_t)(xnet_unpacker_t *up, const char *buffer, uint32_t sz);
typedef void (*clear_method_t)(void *arg);

struct xnet_unpacker {
	unpack_callback_t cb;
	unpack_method_t um;
	clear_method_t cm;
	uint32_t limit;//包大小限制,为0表示没有限制,实际限制范围取决于解包方法
	bool full;//收完一个整包，需设置此标记为true，在进行回调后，会调用cm方法清理用户缓存
	bool close;//用于在成功进行一次回调后终止剩余数据的解包
	//保留给用户
	void *user_ptr;
	void *user_arg;
	char arg[0];
};

xnet_unpacker_t *xnet_unpacker_new(uint32_t arg_sz, unpack_callback_t cb, unpack_method_t um, clear_method_t cm, uint32_t limit);
void xnet_unpacker_free(xnet_unpacker_t * up);
int xnet_unpacker_recv(xnet_unpacker_t *up, const char *buffer, uint32_t sz);

/*http 封包解包*/
enum http_state_e {
	HTTP_STATE_METHOD = 0,
	HTTP_STATE_URL,
	HTTP_STATE_VERSION,
	HTTP_STATE_HEADER_KEY,
	HTTP_STATE_HEADER_VALUE,
	HTTP_STATE_HEADER_DONE,
	HTTP_STATE_BODY,
	HTTP_STATE_DONE
};
typedef struct {
	xnet_string_t key;
	xnet_string_t value;
} xnet_httpheader_t;

#define HTTP_CMMOND_HEAD \
xnet_httpheader_t *header; \
uint16_t header_count; \
uint16_t header_capacity;

typedef struct {
	HTTP_CMMOND_HEAD
	uint16_t state;
	uint16_t subState;
	xnet_string_t method;
	xnet_string_t url;
	xnet_string_t version;
	xnet_string_t *body;
	uint32_t content_length;
	uint32_t recv_len;
	int code;
} xnet_httprequest_t;

typedef struct {
	HTTP_CMMOND_HEAD
	int code;
	xnet_string_t *body;
} xnet_httpresponse_t;

/**
 * unpack method
 * return values:
 * >0:recv len
 * 0:error
 */
uint32_t xnet_unpack_http(xnet_unpacker_t *up, const char *buffer, uint32_t sz);
void xnet_clear_http(void *arg);
xnet_httpheader_t *xnet_get_http_header_value(void *req_or_rsp, const char *key);
int xnet_pack_http(xnet_httpresponse_t *rsp, xnet_string_t *out);
void xnet_set_http_rsp_code(xnet_httpresponse_t *rsp, int code);
void xnet_add_http_rsp_header(xnet_httpresponse_t *rsp, const char *key, const char *value);
void xnet_set_http_rsp_body(xnet_httpresponse_t *rsp, const char *body);
void xnet_raw_set_http_rsp_body(xnet_httpresponse_t *rsp, char *body);
void xnet_set_http_rsp_byte_body(xnet_httpresponse_t *rsp, const char *body, uint32_t sz);
void xnet_raw_set_http_rsp_byte_body(xnet_httpresponse_t *rsp, char *body, uint32_t sz);
void xnet_clear_http_rsp(xnet_httpresponse_t *rsp);

/*4字节长度+实际数据*/
#define BUFFER_HEADER_SIZE sizeof(uint32_t)
typedef struct {
	uint32_t buffer_size;
	uint32_t recv_len;
	char *recv_buffer;
	char header[BUFFER_HEADER_SIZE];
} xnet_sizebuffer_t;

uint32_t xnet_unpack_sizebuffer(xnet_unpacker_t *up, const char *buffer, uint32_t sz);
void xnet_clear_sizebuffer(void *arg);
int xnet_pack_sizebuff(const char *buffer, uint32_t sz, xnet_string_t *out);

/*以'\n'或者'\r\n'结尾的行*/
typedef struct {
	xnet_string_t line_str;
	bool sep;//true:'\r\n' false:'\n'
} xnet_linebuffer_t;

uint32_t xnet_unpack_line(xnet_unpacker_t *up, const char *buffer, uint32_t sz);
void xnet_clear_line(void *arg);

#endif //_UNPACKER_H_