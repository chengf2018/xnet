#ifndef _XNET_H_
#define _XNET_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xnet_struct.h"
#include "xnet_util.h"

int xnet_init();
int xnet_deinit();
xnet_context_t *xnet_create_context();
void xnet_release_context(xnet_context_t *ctx);

/*register类接口只能在主线程使用，在其他线程使用需要使用异步接口*/

//listen_func:服务端监听回调
void xnet_register_listener(xnet_context_t *ctx, xnet_listen_func_t listen_func, \
	xnet_error_func_t error_func, xnet_recv_func_t recv_func);

//connect_func:客户端连接成功回调
void xnet_register_connecter(xnet_context_t *ctx, xnet_connect_func_t connect_func, \
	xnet_error_func_t error_func, xnet_recv_func_t recv_func);

void xnet_register_timeout(xnet_context_t *ctx, xnet_timeout_func_t timeout_func);
void xnet_register_command(xnet_context_t *ctx, xnet_command_func_t command_func);

//int xnet_register_packer(xnet_context_t *ctx);
//int xnet_register_unpacker(xnet_context_t *ctx);

int xnet_dispatch_loop(xnet_context_t *ctx);
//以下接口只能在主线程使用
int xnet_add_timer(xnet_context_t *ctx, int id, int timeout);
void xnet_send_buffer(xnet_context_t *ctx, xnet_socket_t *s, char *buffer, int sz);
int xnet_connect(xnet_context_t *ctx, char *host, int port, xnet_socket_t **socket_out);//异步，通过connected事件回调
int xnet_listen(xnet_context_t *ctx, int port, xnet_socket_t **socket_out);
void xnet_close_socket(xnet_context_t *ctx, xnet_socket_t *s);
void xnet_exit(xnet_context_t *ctx);


//管道接口，支持多线程
int xnet_send_commond(xnet_context_t *ctx, xnet_context_t *to_ctx, int commond, void *data, int sz);
int xnet_asyn_listen(xnet_context_t *ctx, int port, int back_command);
int xnet_asyn_connect(xnet_context_t *ctx, char *host, int port, int back_command);
int xnet_asyn_send_buffer();
int xnet_asyn_close_socket();
int xnet_asyn_exit();

#endif //_XNET_H_