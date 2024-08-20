

function Start()
	print("lua start!")
	local rc, sock = xnet.tcp_listen("127.0.0.1", 9090, 5);
	print("lua tcp_listen", rc, sock)
	xnet.add_timer(1, 1000);
	xnet.register({
		listen = function(s, ns, addr)
			print("----lua: new connection", s, ns, addr, xnet.addrtoa(addr))
		end,
		error = function(s, what)
			print("----lua: error", s, what)
		end,
		recv = function(s, buffer, sz, addr)
			print("----lua: recv", s, sz, addr, buffer, xnet.addrtoa(addr))
		end,
		timeout = function(id)
			print("----lua:timeout", id)
			xnet.add_timer(1, 1000);
		end,
		command = function(source, command, data, sz)
			print("----lua:command", source, command, sz)
		end,
		connected = function(s, err)
			print("----lua:connected", s, err)
		end,
	})
end

function Init()
	print "lua init!"
end

function Stop()
	print "lua stop!"
end
