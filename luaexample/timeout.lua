package.path = "lualib/?.lua;"

local timer = require "timer"

function Start()
	print("lua start!")

	timer.register_timeout(1, 1000, function(id)
		print("hello", xnet.now())
		xnet.add_timer(id, 1000)
	end)
	xnet.register({
		timeout = timer.timeout_dispatch,
	})

	--timer.unregister_timeout(1)
end

function Init()
	print "lua init!"
end

function Stop()
	print "lua stop!"
end
