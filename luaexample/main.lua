package.path = "lualib/?.lua;"

local pack = require "pack"

local socket_list = { }

function get_type_first_print(t)
	local str = type(t)
	return string.upper(string.sub(str, 1, 1)) .. ":"
end

function dump_table(t, indent_input, print)
	local indent = indent_input
	if indent_input == nil then
		indent = 1
	end

	if print == nil then
		print = _G["print"]
	end

	local formatting = string.rep("    ", indent)
	if t == nil then
		print(formatting .. "nil")
	end

	if type(t) ~= "table" then
		print(formatting..get_type_first_print(t)..tostring(t))
		return
	end

	local output_count = 0
	for k,v in pairs(t) do
		local str_k = get_type_first_print(k)
		if type(v) == "table" then
			print(formatting .. str_k .. k .. " -> ")
			dump_table(v, indent+1, print)
		else
			print(formatting .. str_k .. k .. " -> " .. get_type_first_print(v) .. tostring(v))
		end
		output_count = output_count + 1
	end

	if output_count == 0 then
		print(formatting .. "{}")
	end
end


function Start()
	print("lua start!")
	local rc, sock = xnet.tcp_listen("0.0.0.0", 9090, 5);
	print("lua tcp_listen", rc, sock)
	xnet.add_timer(1, 1000);
	xnet.register({
		listen = function(s, ns, addr)
			print("----lua: new connection", s, ns, xnet.addrtoa(addr))
			socket_list[ns] = ns
			xnet.register_packer(ns, xnet.PACKER_TYPE_HTTP)
		end,
		error = function(s, what)
			print("----lua: error", s, what)
			socket_list[ns] = nil
		end,
		recv = function(s, pkg_type, pkg, sz, addr)
			print("----lua: recv", s, pkg_type, pkg, sz, xnet.addrtoa(addr))
			if type(pkg) == "table" then
				dump_table(pkg)
				local msg = pack.pack_http(200, nil, "hello world")
				xnet.tcp_send_buffer(s, msg)
				print("tcp_send_buffer", s, string.len(msg))
				xnet.close_socket(s)
			end
		end,
		timeout = function(id)
			print("----lua:timeout", id)
			--xnet.add_timer(1, 1000);
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
