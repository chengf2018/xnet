#include "xnet.h"
#include <stdio.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

static void
call_lua_start(lua_State *L) {
	lua_getglobal(L, "Start");
	if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
		return;
	}
}

static void
call_lua_init(lua_State *L) {
	lua_getglobal(L, "Init");
	if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
		return;
	}
}

static void
call_lua_stop(lua_State *L) {
	lua_getglobal(L, "Stop");
	if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
		return;
	}
}

int
main(int argc, char **argv) {
	lua_State *L = NULL;
	int ret;

	ret = xnet_init(NULL);
	if (ret != 0) {
		printf("xnet init error!\n");
		goto error;
	}
	L = luaL_newstate();
	if (!L) {
		printf("init lua state error\n");
		goto error;
	}
	luaL_openlibs(L);
	//lua state must have "Start", "Init" and "Stop" function
	ret = luaL_dofile(L, "main.lua");
	if (ret != LUA_OK) {
		printf("load main.lua error\n");
		goto error;
	}
	printf("start lua function\n");
	call_lua_start(L);
	call_lua_init(L);
	call_lua_stop(L);

	lua_close(L);
	xnet_deinit();
	return 0;
error:
	if (L) lua_close(L);
	xnet_deinit();
	return 1;
}