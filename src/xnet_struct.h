#ifndef _XNET_STRUCT_H_
#define _XNET_STRUCT_H_

#include <stdint.h>
#include <stdbool.h>

#include "xnet_socket.h"
#include "xnet_timeheap.h"

struct xnet_context_t;

typedef void (*xnet_connect_func_t)(struct xnet_context_t *ctx, xnet_socket_t *s, int error);
typedef void (*xnet_listen_func_t)(struct xnet_context_t *ctx, xnet_socket_t *s, xnet_socket_t *ns, xnet_addr_t *addr_info);
//recv_func返回0表示自动释放，返回其他值表示接管buffer，自行释放
typedef int (*xnet_recv_func_t)(struct xnet_context_t *ctx, xnet_socket_t *s, char *buffer, int size, xnet_addr_t *addr_info);
typedef void (*xnet_error_func_t)(struct xnet_context_t *ctx, xnet_socket_t *s, short what);
typedef void (*xnet_timeout_func_t)(struct xnet_context_t *ctx, int id);
//返回0表示自动释放，返回其他值表示接管data，自行释放
typedef int (*xnet_command_func_t)(struct xnet_context_t *ctx, struct xnet_context_t *source, int command, void *data, int sz);

typedef struct xnet_context_t {
	int id;
	bool to_quit;
	xnet_poll_t poll;
	xnet_timeheap_t th;
	uint64_t nowtime;

	xnet_listen_func_t listen_func;
	xnet_recv_func_t recv_func;
	xnet_error_func_t error_func;//socket关闭和发生错误都统一回调这个方法
	xnet_connect_func_t connect_func;
	xnet_timeout_func_t timeout_func;
	xnet_command_func_t command_func;
} xnet_context_t;

typedef struct {
	int id;
} xnet_cmdreq_close_t;

typedef struct {
	xnet_context_t *source;
	int back_command;
	int port;
	int backlog;
	char host[0];
} xnet_cmdreq_listen_t;

typedef struct {
	xnet_context_t *source;
	int back_command;
	int port;
	char host[0];
} xnet_cmdreq_connect_t;

typedef struct {
	int id;
	char *data;
	int size;
} xnet_cmdreq_sendtcp_t;

typedef struct {
	int *ids;
	char *data;
	int size;
} xnet_cmdreq_broadtcp_t;

typedef struct {
	xnet_context_t *source;
	int command;
	void *data;
	int size;
} xnet_cmdreq_command_t;

typedef struct {
	xnet_context_t *source;	
} xnet_cmdreq_exit_t;

typedef struct {
	uint8_t header[8];//实际只用了第7和第8byte，8字节只是为了内存对齐
	union {
		uint8_t buffer[256];
		xnet_cmdreq_listen_t listen_req;
		xnet_cmdreq_close_t close_req;
		xnet_cmdreq_connect_t connect_req;
		xnet_cmdreq_sendtcp_t send_tcp_req;
		xnet_cmdreq_broadtcp_t broad_tcp_req;
		xnet_cmdreq_command_t command_req;
		xnet_cmdreq_exit_t exit_req;
	} pkg;
} xnet_cmdreq_t;

typedef struct {
	char *log_path;
} xnet_init_config_t;

#endif //_XNET_STRUCT_H_