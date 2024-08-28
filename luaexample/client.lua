package.path = "lualib/?.lua;"

local pack = require "pack"
local util = require "util"
local timer = require "timer"

local socket_list = { }

function Start()
	print("lua start!")
	xnet.tcp_connect("127.0.0.1", 8989)

	xnet.register({
		error = function(sid, what)
			print("----lua: error", sid, what)
			socket_list[sid] = nil
		end,
		recv = function(sid, pkg_type, pkg, sz, addr)
			print("----lua: recv", sid, pkg_type, pkg, sz, xnet.addrtoa(addr))
		end,
		timeout = timer.timeout_dispatch,
		connected = function(sid, err)
			print("----lua:connected", sid, err)
			if (err == 0) then --connect success
				xnet.register_packer(sid, xnet.PACKER_TYPE_LINE)
				xnet.tcp_send_buffer(sid, "hello world\r\n")
				xnet.close_socket(sid)

				timer.register_timeout(1, 0, function(id)
					xnet.exit()
				end)
			end
		end,

	})
end

function Init()
	print "lua init!"
end

function Stop()
	print "lua stop!"
end
