#ifndef _XNET_STRUCT_H_
#define _XNET_STRUCT_H_

#include <stdint.h>
#include <stdbool.h>
#include "xnet_mq.h"
#include "xnet_socket.h"

struct xnet_context_t;

typedef void (*xnet_connect_func_t)(struct xnet_context_t *ctx, xnet_socket_t *s);
typedef void (*xnet_listen_func_t)(struct xnet_context_t *ctx, xnet_socket_t *s, xnet_socket_t *ns);
typedef void (*xnet_recv_func_t)(struct xnet_context_t *ctx, xnet_socket_t *s, char *buffer, int size);
//typedef void (*xnet_send_func_t)(struct xnet_context_t *, xnet_socket_t *);
typedef void (*xnet_error_func_t)(struct xnet_context_t *ctx, xnet_socket_t *s, short what);


typedef struct xnet_context_t{
	//xnet_mq_t mq;
	xnet_poll_t poll;

	xnet_listen_func_t listen_func;
	xnet_recv_func_t recv_func;
	//xnet_send_func_t send_func;
	xnet_error_func_t error_func;
	xnet_connect_func_t connect_func;
} xnet_context_t;

typedef struct {
	int id;
} xnet_cmdreq_close_t;

typedef struct {
	int port;
} xnet_cmdreq_listen_t;

typedef struct {
	uint8_t header[8];//实际只用了第7和第8byte，8字节只是为了内存对齐
	union {
		uint8_t buffer[256];
		xnet_cmdreq_listen_t listen_req;
		xnet_cmdreq_close_t close_req;
	} pkg;
} xnet_cmdreq_t;

#endif //_XNET_STRUCT_H_