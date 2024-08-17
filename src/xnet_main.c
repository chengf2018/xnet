#include "xnet.h"
#include <stdio.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include "lua_binding.h"

static void
call_lua_start(lua_State *L, xnet_context_t *ctx) {
	lua_getglobal(L, "Start");
	if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
		xnet_error(ctx, "Start error:%s", lua_tostring(L, -1));
		return;
	}
}

static void
call_lua_init(lua_State *L, xnet_context_t *ctx) {
	lua_getglobal(L, "Init");
	if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
		xnet_error(ctx, "Init error:%s", lua_tostring(L, -1));
		return;
	}
}

static void
call_lua_stop(lua_State *L, xnet_context_t *ctx) {
	lua_getglobal(L, "Stop");
	if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
		xnet_error(ctx, "Stop error:%s", lua_tostring(L, -1));
		return;
	}
}


static void
listen_func(xnet_context_t *ctx, xnet_socket_t *s, xnet_socket_t *ns, xnet_addr_t *addr_info) {
    char addr_str[64] = {0};
    lua_State *L = ctx->user_ptr;

    xnet_addrtoa(addr_info, addr_str);
	xnet_error(ctx, "-----socket [%d] accept new, new socket:[%d], [%s]", s->id, ns->id, addr_str);
	
	int ftype = lua_getfield(L, LUA_REGISTRYINDEX, "reg_funcs");
	if (ftype != LUA_TTABLE) {
		xnet_error(ctx, "reg_funcs is not a table %d", ftype);
		return;
	}

	if (lua_getfield(L, -1, "listen") == LUA_TFUNCTION) {
		lua_pushlightuserdata(L, s);
		lua_pushlightuserdata(L, ns);
		lua_pushstring(L, addr_str);
		if (lua_pcall(L, 3, 0, 0) != LUA_OK) {
			xnet_error(ctx, "call listen fail:%s", lua_tostring(L, -1));
			return;
		}
	}
}

static void
error_func(xnet_context_t *ctx, xnet_socket_t *s, short what) {
	lua_State *L = ctx->user_ptr;
	xnet_error(ctx, "-----socket [%d] error, what:[%u]", s->id, what);

	int ftype = lua_getfield(L, LUA_REGISTRYINDEX, "reg_funcs");
	if (ftype != LUA_TTABLE) {
		xnet_error(ctx, "reg_funcs is not a table %d", ftype);
		return;
	}

	if (lua_getfield(L, -1, "error") == LUA_TFUNCTION) {
		lua_pushlightuserdata(L, s);
		lua_pushinteger(L, what);
		if (lua_pcall(L, 2, 0, 0) != LUA_OK) {
			xnet_error(ctx, "call error fail:%s", lua_tostring(L, -1));
			return;
		}		
	}
}

static int
recv_func(xnet_context_t *ctx, xnet_socket_t *s, char *buffer, int size, xnet_addr_t *addr_info) {
	lua_State *L = ctx->user_ptr;
	int ftype = lua_getfield(L, LUA_REGISTRYINDEX, "reg_funcs");
	if (ftype != LUA_TTABLE) {
		xnet_error(ctx, "reg_funcs is not a table %d", ftype);
		return 0;
	} 
	if (lua_getfield(L, -1, "recv") != LUA_OK) {
		lua_pushlightuserdata(L, s);
		lua_pushlstring(L, buffer, size);
		lua_pushinteger(L, size);
		char addr_str[64] = {0};
		xnet_addrtoa(addr_info, addr_str);
		lua_pushstring(L, addr_str);
		if (lua_pcall(L, 4, 0, 0) != LUA_OK) {
			xnet_error(ctx, "call recv error:%s", lua_tostring(L, -1));
			return 0;
		}
	}
    return 0;//返回0,由xnet释放
}

static void
timeout_func(xnet_context_t *ctx, int id) {
	lua_State *L = ctx->user_ptr;
    xnet_error(ctx, "timeout event[%d], nowtime:%llu", id, ctx->nowtime);
    int ftype = lua_getfield(L, LUA_REGISTRYINDEX, "reg_funcs");
    if (ftype != LUA_TTABLE) {
    	xnet_error(ctx, "reg_funcs is not a table %d", ftype);
    	return;
    }
    if (lua_getfield(L, -1, "timeout") == LUA_TFUNCTION) {
    	lua_pushinteger(L, id);
    	if (lua_pcall(L, 1, 0, 0) != LUA_OK) {
    		xnet_error(ctx, "call timeout error:%s", lua_tostring(L, -1));
    		return;
    	}
    }
}


static int
command_func(xnet_context_t *ctx, xnet_context_t *source, int command, void *data, int sz) {

    return 0;//返回0，由xnet释放data
}

static void
connect_func(struct xnet_context_t *ctx, xnet_socket_t *s, int error) {
	xnet_error(ctx, "-----socket [%d] connected. error:[%d]", s->id, error);
    
}

static void
bind_event(xnet_context_t *ctx) {
	xnet_register_event(ctx, listen_func, error_func, recv_func,            \
    					connect_func, timeout_func, command_func);
}



int
main(int argc, char **argv) {
	lua_State *L = NULL;
	xnet_context_t *ctx = NULL;
	int ret;

	ret = xnet_init(NULL);
	if (ret != 0) {
		printf("xnet init error!\n");
		goto error;
	}
	ctx = xnet_create_context();
	if (!ctx) {
		printf("create context error\n");
		goto error;
	}
	bind_event(ctx);
	L = luaL_newstate();
	if (!L) {
		printf("init lua state error\n");
		goto error;
	}
	luaL_openlibs(L);
	//lua state must have "Start", "Init" and "Stop" function
	ret = luaL_dofile(L, "main.lua");
	if (ret != LUA_OK) {
		xnet_error(ctx, "lua error:%s", lua_tostring(L, -1));
		goto error;
	}
	xnet_bind_lua(L, ctx);
	printf("start lua function\n");
	call_lua_start(L, ctx);
	call_lua_init(L, ctx);
	xnet_dispatch_loop(ctx);
	call_lua_stop(L, ctx);

	lua_close(L);
	xnet_destroy_context(ctx);
	xnet_deinit();
	return 0;
error:
	if (L) lua_close(L);
	if (ctx) xnet_destroy_context(ctx);
	xnet_deinit();
	return 1;
}