local _M = {}

local timeout_event = { }

function _M.register_timeout(id, timeout, event)
	xnet.add_timer(id, timeout)
	timeout_event[id] = event
end

function _M.unregister_timeout(id)
	timeout_event[id] = nil
end

function _M.timeout_dispatch(id)
	local func = timeout_event[id]
	if func then
		func(id)
	end
end

return _M