#define GET_XNET_CTX xnet_context_t *ctx;            \
lua_getfield((L), LUA_REGISTRYINDEX, "xnet_ctx");    \
ctx = lua_touserdata((L), -1);                       \
if (ctx == NULL) {                                   \
	return luaL_error(L, "init xnet context first"); \
}                                                    \
lua_pop(L, 1);                                       \

#define XNET_PACKER_TYPE_NORMAL       0
#define XNET_PACKER_TYPE_HTTP         1
#define XNET_PACKER_TYPE_SIZEBUFFER   2
#define XNET_PACKER_TYPE_LINE         3

static int
_xnet_tcp_connect(lua_State *L) {
	GET_XNET_CTX

	const char *addr = luaL_checkstring(L, 1);
	int port = (int)luaL_checkinteger(L, 2);

	int rc = xnet_tcp_connect(ctx, addr, port);
	if (rc == -1) {
		lua_pushinteger(L, rc);
		return 1;
	}

	lua_pushinteger(L, rc);
	return 1;
}

static int
_xnet_tcp_listen(lua_State *L) {
	GET_XNET_CTX
	const char *host = luaL_checkstring(L, 1);
	int port = luaL_checkinteger(L, 2);
	int backlog = luaL_optinteger(L, 3, 5);

	int sock_id = xnet_tcp_listen(ctx, host, port, backlog);
	if (sock_id == -1) {
		lua_pushboolean(L, 0);
		return 1;
	}
	lua_pushboolean(L, 1);
	lua_pushinteger(L, sock_id);
	return 2;
}

static int
_xnet_tcp_send_buffer(lua_State *L) {
	GET_XNET_CTX
	int sock_id = luaL_checkinteger(L, 1);
	int type = lua_type(L, 2);
	if (type == LUA_TSTRING) {
		const char *buffer = NULL;
		size_t sz = 0;
		buffer = lua_tolstring(L, 2, &sz);
		xnet_tcp_send_buffer(ctx, sock_id, buffer, (int)sz, false);
	} else if (type == LUA_TLIGHTUSERDATA) {
		//light user data is don't need free.
		char *buffer = lua_touserdata(L, 2);
		int sz = luaL_checkinteger(L, 3);
		xnet_tcp_send_buffer(ctx, sock_id, buffer, sz, true);
	} else {
		luaL_error(L, "error buffer type");
	}
	return 0;
}

static int
_xnet_close_socket(lua_State *L) {
	GET_XNET_CTX
	int sock_id = luaL_checkinteger(L, 1);
	xnet_close_socket(ctx, sock_id);
	return 0;
}

static int
_xnet_add_timer(lua_State *L) {
	GET_XNET_CTX
	int id = luaL_checkinteger(L, 1);
	int timeout = luaL_checkinteger(L, 2);

	int rc = xnet_add_timer(ctx, id, timeout);
	lua_pushinteger(L, rc);
	return 1;
}

static int
_xnet_exit(lua_State *L) {
	GET_XNET_CTX
	xnet_exit(ctx);
	return 0;
}

static int
_xnet_udp_listen(lua_State *L) {
	GET_XNET_CTX
	const char *host = luaL_checkstring(L, 1);
	int port = luaL_checkinteger(L, 2);

	int rc = xnet_udp_listen(ctx, host, port);
	if (rc == -1) {
		lua_pushboolean(L, 0);
		return 1;
	}
	lua_pushboolean(L, 1);
	lua_pushinteger(L, rc);
	return 2;
}

static int
_xnet_udp_sendto(lua_State *L) {
	GET_XNET_CTX

	size_t sz = 0;
	int sock_id = luaL_checkinteger(L, 1);
	const char *addr_string = luaL_checklstring(L, 2, &sz);
	if (sz != sizeof(xnet_addr_t)) {
		luaL_error(L, "error addr");
	}
	xnet_addr_t addr;
	memcpy(&addr, addr_string, sizeof(addr));

	int type = lua_type(L, 3);
	if (type == LUA_TSTRING) {
		size_t size = 0;
		const char *buffer = lua_tolstring(L, 3, &size);
		xnet_udp_sendto(ctx, sock_id, &addr, buffer, (int)size, false);
	} else if (type == LUA_TLIGHTUSERDATA) {
		char *buffer = lua_touserdata(L, 3);
		int size = luaL_checkinteger(L, 4);
		xnet_udp_sendto(ctx, sock_id, &addr, buffer, size, true);
	} else {
		luaL_error(L, "error buffer type");
	}
	return 0;
}

static int
_xnet_udp_create(lua_State *L) {
	GET_XNET_CTX
	int protocol = luaL_checkinteger(L, 1);
	if (protocol != SOCKET_PROTOCOL_UDP && protocol != SOCKET_PROTOCOL_UDP_IPV6) {
		luaL_error(L, "unknow protocol %d", protocol);
	}

	int rc = xnet_udp_create(ctx, protocol);
	if (rc == -1) {
		lua_pushboolean(L, 0);
		return 1;
	}
	lua_pushboolean(L, 1);
	lua_pushinteger(L, rc);
	return 2;
}

static int
_xnet_udp_set_addr(lua_State *L) {
	GET_XNET_CTX

	int sock_id = luaL_checkinteger(L, 1);
	const char *host = luaL_checkstring(L, 2);
	int port = luaL_checkinteger(L, 3);
	int rc = xnet_udp_set_addr(ctx, sock_id, host, port);
	if (rc == -1) {
		lua_pushboolean(L, 0);
		return 1;
	}
	lua_pushboolean(L, 1);
	return 1;
}

static int
_xnet_udp_send_buffer(lua_State *L) {
	GET_XNET_CTX
	int sock_id = luaL_checkinteger(L, 1);
	int type = lua_type(L, 2);
	if (type == LUA_TSTRING) {
		size_t size = 0;
		const char *buffer = lua_tolstring(L, 2, &size);
		xnet_udp_send_buffer(ctx, sock_id, buffer, (int)size, false);
	} else if (type == LUA_TLIGHTUSERDATA) {
		char *buffer = lua_touserdata(L, 2);
		int size = luaL_checkinteger(L, 3);
		xnet_udp_send_buffer(ctx, sock_id, buffer, size, true);
	} else {
		luaL_error(L, "error buffer type");
	}
	return 0;
}

static int
_register(lua_State *L) {
	luaL_checktype(L, 1, LUA_TTABLE);
	lua_setfield(L, LUA_REGISTRYINDEX, "reg_funcs");
	return 0;
}

static int
_xnet_addrtoa(lua_State *L) {
	size_t sz = 0;
	const char *addr_string = luaL_checklstring(L, 1, &sz);
	if (sz != sizeof(xnet_addr_t)) {
		luaL_error(L, "error addr");
	}

	xnet_addr_t addr;
	memcpy(&addr, addr_string, sizeof(addr));
	char str[64] = {0};
	xnet_addrtoa(&addr, str);
	lua_pushstring(L, str);
	return 1;
}

static void
http_callback(xnet_unpacker_t *up, void *arg) {
	xnet_httprequest_t *req = (xnet_httprequest_t *) arg;
	xnet_context_t *ctx = (xnet_context_t *)up->user_ptr;
	int sock_id = (int)up->user_arg;
	xnet_socket_t *s = xnet_get_socket(ctx, sock_id);
	lua_State *L = ctx->user_ptr;

	int ftype = lua_getfield(L, LUA_REGISTRYINDEX, "reg_funcs");
	if (ftype != LUA_TTABLE) {
		xnet_error(ctx, "reg_funcs is not a table %d", ftype);
		return;
	}

	if (lua_getfield(L, -1, "recv") != LUA_TFUNCTION) {
		xnet_error(ctx, "recv is not a function");
		return;
	}

	int i;
	lua_pushlightuserdata(L, s);
	lua_pushinteger(L, XNET_PACKER_TYPE_HTTP);

	lua_newtable(L);
	lua_pushinteger(L, req->code);
	lua_setfield(L, -2, "code");
	lua_pushstring(L, xnet_string_get_c_str(&req->method));
	lua_setfield(L, -2, "method");
	lua_pushstring(L, xnet_string_get_c_str(&req->url));
	lua_setfield(L, -2, "url");
	lua_pushstring(L, xnet_string_get_c_str(&req->version));
			
	lua_newtable(L);
	for (i=0; i<req->header_count; i++) {
		lua_pushstring(L, xnet_string_get_c_str(&req->header[i].value));
		lua_setfield(L, -2, xnet_string_get_c_str(&req->header[i].key));
	}
	lua_setfield(L, -2, "header");

	if (req->body) {
		lua_pushstring(L, xnet_string_get_c_str(req->body));
		lua_setfield(L, -2, "body");
	}
	
	lua_pushnil(L);
	lua_pushlstring(L, (char*)&s->addr_info, sizeof(xnet_addr_t));
	if (lua_pcall(L, 5, 0, 0) != LUA_OK) {
		xnet_error(ctx, "http call recv error:%s", lua_tostring(L, -1));
	}
}

static void
sizebuffer_callback(xnet_unpacker_t *up, void *arg) {
	xnet_sizebuffer_t *sb = (xnet_sizebuffer_t *)arg;
	xnet_context_t *ctx = (xnet_context_t *)up->user_ptr;
	int sock_id = (int)up->user_arg;
	xnet_socket_t *s = xnet_get_socket(ctx, sock_id);
	lua_State *L = ctx->user_ptr;

	int ftype = lua_getfield(L, LUA_REGISTRYINDEX, "reg_funcs");
	if (ftype != LUA_TTABLE) {
		xnet_error(ctx, "reg_funcs is not a table %d", ftype);
		return;
	}

	if (lua_getfield(L, -1, "recv") != LUA_TFUNCTION) {
		xnet_error(ctx, "recv is not a function");
		return;
	}
	lua_pushlightuserdata(L, s);
	lua_pushinteger(L, XNET_PACKER_TYPE_SIZEBUFFER);
	lua_pushlstring(L, sb->recv_buffer, sb->buffer_size);
	lua_pushinteger(L, sb->buffer_size);
	lua_pushlstring(L, (char*)&s->addr_info, sizeof(xnet_addr_t));
	if (lua_pcall(L, 5, 0, 0) != LUA_OK) {
		xnet_error(ctx, "sizebuffer call recv error:%s", lua_tostring(L, -1));
	}
}

static void
linebuffer_callback(xnet_unpacker_t *up, void *arg) {
	xnet_linebuffer_t *lb = (xnet_linebuffer_t *)arg;
	xnet_context_t *ctx = (xnet_context_t *)up->user_ptr;
	int sock_id = (int)up->user_arg;
	lua_State *L = ctx->user_ptr;
	xnet_socket_t *s = xnet_get_socket(ctx, sock_id);

	int ftype = lua_getfield(L, LUA_REGISTRYINDEX, "reg_funcs");
	if (ftype != LUA_TTABLE) {
		xnet_error(ctx, "reg_funcs is not a table %d", ftype);
		return;
	}

	if (lua_getfield(L, -1, "recv") != LUA_TFUNCTION) {
		xnet_error(ctx, "recv is not a function");
		return;
	}
	lua_pushinteger(L, sock_id);
	lua_pushinteger(L, XNET_PACKER_TYPE_LINE);
	lua_pushstring(L, xnet_string_get_c_str(&lb->line_str));
	lua_pushnil(L);
	lua_pushlstring(L, (char*)&s->addr_info, sizeof(xnet_addr_t));
	if (lua_pcall(L, 5, 0, 0) != LUA_OK) {
		xnet_error(ctx, "linebuffer call recv error:%s", lua_tostring(L, -1));
	}
}


static int
_register_packer(lua_State *L) {
	GET_XNET_CTX
	int sock_id = luaL_checkinteger(L, 1);
	int pack_type = luaL_checkinteger(L, 2);

	xnet_socket_t *s = xnet_get_socket(ctx, sock_id);
	if (s == NULL) {
		luaL_error(L, "error sock id: %s", sock_id);
	}

	if (s->unpacker) {
		luaL_error(L, "already register pack type");
	}

	xnet_unpacker_t *up = NULL;
	switch (pack_type) {
		case XNET_PACKER_TYPE_HTTP:
			up = xnet_unpacker_new(sizeof(xnet_httprequest_t), http_callback, xnet_unpack_http, xnet_clear_http, 1024);
		break;
		case XNET_PACKER_TYPE_SIZEBUFFER:
			up = xnet_unpacker_new(sizeof(xnet_sizebuffer_t), sizebuffer_callback, xnet_unpack_sizebuffer, xnet_clear_sizebuffer, 65535);
		break;
		case XNET_PACKER_TYPE_LINE:
			up = xnet_unpacker_new(sizeof(xnet_linebuffer_t), linebuffer_callback, xnet_unpack_line, xnet_clear_line, 1024);
		break;
		default:
			luaL_error(L, "unknow pack type %d", pack_type);
	}
	if (up == NULL) {
		luaL_error(L, "register pack type error");
	}
	up->user_ptr = ctx;
	up->user_arg = (void*)sock_id;
	s->unpacker = up;	

	return 0;
}

static void
xnet_bind_lua(lua_State *L, xnet_context_t *ctx) {
	ctx->user_ptr = L;
	lua_pushlightuserdata(L, ctx);
	lua_setfield(L, LUA_REGISTRYINDEX, "xnet_ctx");
	lua_newtable(L);

	//xnet_tcp_connect
	lua_pushcfunction(L, _xnet_tcp_connect);
	lua_setfield(L, -2, "tcp_connect");

	//xnet_tcp_listen
	lua_pushcfunction(L, _xnet_tcp_listen);
	lua_setfield(L, -2, "tcp_listen");

	//xnet_tcp_send_buffer
	lua_pushcfunction(L, _xnet_tcp_send_buffer);
	lua_setfield(L, -2, "tcp_send_buffer");

	//xnet_udp_listen
	lua_pushcfunction(L, _xnet_udp_listen);
	lua_setfield(L, -2, "udp_listen");

	//xnet_udp_sendto
	lua_pushcfunction(L, _xnet_udp_sendto);
	lua_setfield(L, -2, "udp_sendto");

	//xnet_udp_create
	lua_pushcfunction(L, _xnet_udp_create);
	lua_setfield(L, -2, "udp_create");

	//xnet_udp_set_addr
	lua_pushcfunction(L, _xnet_udp_set_addr);
	lua_setfield(L, -2, "udp_set_addr");

	//xnet_udp_send_buffer
	lua_pushcfunction(L, _xnet_udp_send_buffer);
	lua_setfield(L, -2, "udp_send_buffer");

	//xnet_close_socket
	lua_pushcfunction(L, _xnet_close_socket);
	lua_setfield(L, -2, "close_socket");
	//xnet_add_timer
	lua_pushcfunction(L, _xnet_add_timer);
	lua_setfield(L, -2, "add_timer");
	//xnet_exit
	lua_pushcfunction(L, _xnet_exit);
	lua_setfield(L, -2, "exit");

	//register
	lua_pushcfunction(L, _register);
	lua_setfield(L, -2, "register");

	//addrtoa
	lua_pushcfunction(L, _xnet_addrtoa);
	lua_setfield(L, -2, "addrtoa");

	//register_packer
	lua_pushcfunction(L, _register_packer);
	lua_setfield(L, -2, "register_packer");

	//packer type
	lua_pushinteger(L, XNET_PACKER_TYPE_HTTP);
	lua_setfield(L, -2, "PACKER_TYPE_HTTP");
	lua_pushinteger(L, XNET_PACKER_TYPE_SIZEBUFFER);
	lua_setfield(L, -2, "PACKER_TYPE_SIZEBUFFER");
	lua_pushinteger(L, XNET_PACKER_TYPE_LINE);
	lua_setfield(L, -2, "PACKER_TYPE_LINE");

	//protocol type
	lua_pushinteger(L, SOCKET_PROTOCOL_TCP);
	lua_setfield(L, -2, "PROTOCOL_TCP");
	lua_pushinteger(L, SOCKET_PROTOCOL_UDP);
	lua_setfield(L, -2, "PROTOCOL_UDP");
	lua_pushinteger(L, SOCKET_PROTOCOL_UDP_IPV6);
	lua_setfield(L, -2, "PROTOCOL_UDP_IPV6");

	lua_setglobal(L, "xnet");
}
