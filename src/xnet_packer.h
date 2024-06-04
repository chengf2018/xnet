#ifndef _UNPACKER_H_
#define _UNPACKER_H_
#include <stdint.h>
#include <stdbool.h>
#include "xnet_string.h"

typedef struct xnet_unpacker xnet_unpacker_t;


typedef void (*unpack_callback_t)(xnet_unpacker_t *up, void *arg);
typedef uint32_t (*unpack_method_t)(xnet_unpacker_t *up, char *buffer, uint32_t sz);
typedef void (*clear_method_t)(void *arg);

struct xnet_unpacker {
	unpack_callback_t cb;
	unpack_method_t um;
	clear_method_t cm;
	uint32_t limit;//包大小限制,为0表示没有限制
	bool full;//收完一个整包，需设置此标记为true，在进行回调后，会调用cm方法清理用户缓存
	char arg[0];
};

xnet_unpacker_t *xnet_unpacker_new(uint32_t arg_sz, unpack_callback_t cb, unpack_method_t um, clear_method_t cm, uint32_t limit);
void xnet_unpacker_free(xnet_unpacker_t * up);
int xnet_unpacker_recv(xnet_unpacker_t *up, char *buffer, uint32_t sz);



typedef struct {
	xnet_string_t header_key;
	xnet_string_t header_value;
} xnet_httpheader_t;

typedef struct {
	uint16_t state;//method->url->version
	uint8_t method;
	uint8_t version;
	xnet_string_t url;
	xnet_httpheader_t *header;
	uint16_t header_count;
	xnet_string_t *body;
} xnet_httprequest_t;

/**
 * unpack method
 * return values:
 * >0:recv len
 * 0:error
 */
uint32_t xnet_unpack_http(xnet_unpacker_t *up, char *buffer, uint32_t sz);
void xnet_clear_http(void *arg);

#define BUFFER_HEADER_SIZE sizeof(uint32_t)
typedef struct {
	uint32_t buffer_size;
	uint32_t recv_len;
	char *recv_buffer;
	char header[BUFFER_HEADER_SIZE];
} xnet_sizebuffer_t;

uint32_t xnet_unpack_sizebuffer(xnet_unpacker_t *up, char *buffer, uint32_t sz);
void xnet_clear_sizebuffer(void *arg);



uint32_t xnet_unpack_line(xnet_unpacker_t *up, char *buffer, uint32_t sz);
void xnet_clear_line(void *arg);

#endif //_UNPACKER_H_