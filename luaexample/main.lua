package.path = "lualib/?.lua;"

local pack = require "pack"
local util = require "util"

local socket_list = { }

function Start()
	print("lua start!")
	local port = xnet.get_env("port") or 9090
	local rc, sock = xnet.tcp_listen("0.0.0.0", port, 5);
	print("lua tcp_listen", rc, sock, port)
	print(xnet.get_env("luabooter"))
	print(xnet.get_env("log_path"))

	xnet.add_timer(1, 1000);
	xnet.register({
		listen = function(sid, ns, addr)
			print("----lua: new connection", sid, ns, xnet.addrtoa(addr))
			socket_list[ns] = ns
			xnet.register_packer(ns, xnet.PACKER_TYPE_HTTP)
		end,
		error = function(sid, what)
			print("----lua: error", sid, what)
			socket_list[ns] = nil
		end,
		recv = function(sid, pkg_type, pkg, sz, addr)
			print("----lua: recv", sid, pkg_type, pkg, sz, xnet.addrtoa(addr))
			if type(pkg) == "table" then
				util.dump_table(pkg)
				local msg = pack.pack_http(200, nil, "hello world")
				xnet.tcp_send_buffer(sid, msg)
				print("tcp_send_buffer", sid, string.len(msg))
				xnet.close_socket(sid)
			end
		end,
		timeout = function(id)
			print("----lua:timeout", id)
			--xnet.add_timer(1, 1000);
		end,
		command = function(source, command, data, sz)
			print("----lua:command", source, command, sz)
		end,
		connected = function(sid, err)
			print("----lua:connected", sid, err)
		end,

	})
end

function Init()
	print "lua init!"
end

function Stop()
	print "lua stop!"
end
