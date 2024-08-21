local _M = {}


local http_status_msg = {
	[100] = "Continue",
	[101] = "Switching Protocols",
	[200] = "OK",
	[201] = "Created",
	[202] = "Accepted",
	[203] = "Non-Authoritative Information",
	[204] = "No Content",
	[205] = "Reset Content",
	[206] = "Partial Content",
	[300] = "Multiple Choices",
	[301] = "Moved Permanently",
	[302] = "Found",
	[303] = "See Other",
	[304] = "Not Modified",
	[305] = "Use Proxy",
	[307] = "Temporary Redirect",
	[400] = "Bad Request",
	[401] = "Unauthorized",
	[402] = "Payment Required",
	[403] = "Forbidden",
	[404] = "Not Found",
	[405] = "Method Not Allowed",
	[406] = "Not Acceptable",
	[407] = "Proxy Authentication Required",
	[408] = "Request Time-out",
	[409] = "Conflict",
	[410] = "Gone",
	[411] = "Length Required",
	[412] = "Precondition Failed",
	[413] = "Request Entity Too Large",
	[414] = "Request-URI Too Large",
	[415] = "Unsupported Media Type",
	[416] = "Requested range not satisfiable",
	[417] = "Expectation Failed",
	[500] = "Internal Server Error",
	[501] = "Not Implemented",
	[502] = "Bad Gateway",
	[503] = "Service Unavailable",
	[504] = "Gateway Time-out",
	[505] = "HTTP Version not supported",
}

function _M.pack_http(code, header, body)
	local result = string.format("HTTP/1.1 %d %s\r\n", code, http_status_msg[code] or "")
	if header then
		for k,v in pairs(header) do
			result = result .. string.format("%s: %s\r\n", k, v)
		end
	end

	if body then
		result = result .. string.format("content-length: %d\r\n\r\n", string.len(body))
		result = result .. body
	else
		result = result .. "\r\n"
	end
	return result
end

function _M.pack_sizebuffer(buffer, sz)
	sz = sz or string.len(buffer)
	return string.pack("BBBB", sz & 0xFF, (sz >> 8) & 0xFF, (sz >> 16) & 0xFF, (sz >> 24) & 0xFF) .. buffer
end

function _M.pack_line(str)
	return str .. "\r\n"
end

return _M
