#include "xnet_packer.h"
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

xnet_unpacker_t *
xnet_unpacker_new(uint32_t arg_sz, unpack_callback_t cb, unpack_method_t um,
	clear_method_t cm, uint32_t limit) {
	xnet_unpacker_t *up;
	up = malloc(sizeof(xnet_unpacker_t) + arg_sz);
	assert(up);
	memset(up, 0, sizeof(xnet_unpacker_t) + arg_sz);
	up->limit = limit;
	up->cb = cb;
	up->um = um;
	up->cm = cm;
	return up;
}

void
xnet_unpacker_free(xnet_unpacker_t *up) {
	up->cm(up->arg);
	free(up);
}

//return 0:success
//return -1:fail
int
xnet_unpacker_recv(xnet_unpacker_t *up, const char *buffer, uint32_t sz) {
	uint32_t ret;

	ret = up->um(up, buffer, sz);
	while (ret != 0) {
		if (up->full) {
			up->cb(up, up->arg);
			up->cm(up->arg);
			up->full = false;
			if (up->close) {
				up->close = false;
				break;
			}
		}

		buffer += ret;
		sz -= ret;
		if (sz == 0) break;
		ret = up->um(up, buffer, sz);
	}
	if (up->full) {
		up->cb(up, up->arg);
		up->cm(up->arg);
		up->full = false;
	} else if (ret == 0) {
		up->cm(up->arg);
		return -1;
	}
	return 0;
}

#define HTTP_VERSION "HTTP/1.1"
#define HEADER_TOKEN_FILTER "()<>@,;:\\\"/[]?={} \t\n\r"

static inline bool
check_in(const char *s, char c) {
	while(*s) {
		if (*s == c) return true;
		s++;
	}
	return false;
}

static int
push_key_char(xnet_httprequest_t *req, char c) {
	int new_capacity;
	xnet_httpheader_t *header;

	if (req->header_count >= req->header_capacity) {
		new_capacity = req->header_capacity ? req->header_capacity*2 : 32;
		req->header = realloc(req->header, new_capacity*sizeof(*req->header));
		assert(req->header);
		memset(req->header + req->header_capacity, 0,
			sizeof(*req->header)*(new_capacity - req->header_capacity));
		req->header_capacity = new_capacity;
	}
	header = &req->header[req->header_count];
	if (xnet_string_get_size(&header->key) >= 1024) return -1;
	xnet_string_add(&header->key, c);
	return 0;
}

static int
push_value_char(xnet_httprequest_t *req, char c) {
	xnet_httpheader_t *header = &req->header[req->header_count];
	if (xnet_string_get_size(&header->value) >= 4096) return -1;
	xnet_string_add(&header->value, c);
	return 0;
}

static int
parse_request(xnet_httprequest_t *req, const char *buffer, uint32_t sz, uint32_t body_limit) {
	uint16_t state = req->state;
	uint16_t subState = req->subState;
	const char *q = buffer;
	const char *eq = q + sz;
	xnet_httpheader_t *length_header;
	uint32_t version_sz;

	if (q == eq) return 0;

	while (q < eq) {
		switch (state) {
			case HTTP_STATE_METHOD:/*[a-zA-Z]+ ' '*/
				if (xnet_string_get_size(&req->method) >= 32) {
					req->code = 413;
					return -1;
				}

				if ((*q >= 'a' && *q <= 'z') || (*q >= 'A' && *q <= 'Z')) {
					xnet_string_add(&req->method, *q);
					q++;
				} else if (*q == ' ') {
					if (xnet_string_get_size(&req->method) > 0) {
						q++;
						state = HTTP_STATE_URL;
						subState = 0;
					} else {
						req->code = 400;
						return -1;
					}
				}
			break;
			case HTTP_STATE_URL:/*[^ ]+ ' '*/
				if (xnet_string_get_size(&req->url) >= 1024) {
					req->code = 414;
					return -1;
				}

				if (*q != ' ') {
					xnet_string_add(&req->url, *q);
					q++;
				} else {
					if (xnet_string_get_size(&req->url) > 0) {
						q++;
						state = HTTP_STATE_VERSION;
						subState = 0;
					} else {
						req->code = 400;
						return -1;
					}
				}
			break;
			case HTTP_STATE_VERSION:/*'HTTP/1.1' \r\n*/
				version_sz = xnet_string_get_size(&req->version);
				if (version_sz >= 16) {
					req->code = 505;
					return -1;
				}
				if (subState == 0) {
					if (version_sz < sizeof(HTTP_VERSION) && *q == HTTP_VERSION[version_sz]) {
						xnet_string_add(&req->version, *q);
						q++;
					} else {
						if (version_sz != sizeof(HTTP_VERSION)-1 || *q != '\r') {
							req->code = 505;
							return -1;
						}
						q++;
						subState = 1;
					}
				} else if (subState == 1) {
					if (*q == '\n') {
						q++;
						state = HTTP_STATE_HEADER_KEY;
						subState = 0;
					} else {
						req->code = 400;
						return -1;
					}
				} else {
					req->code = 400;
					return -1;
				}
			break;
			case HTTP_STATE_HEADER_KEY:/*^[()<>@,;:\\"/\[\]?={} \t]:[ \t] */
				if (req->header_count >= 128) {
					req->code = 413;
					return -1;
				}
				if (subState == 0) {
					if (*q == '\r') {
						q++;
						subState = 3;
					} else {
						subState = 1;
					}
				} else if (subState == 1) {
					if (!check_in(HEADER_TOKEN_FILTER, *q)) {
						if (push_key_char(req, *q) != 0) {
							req->code = 413;
							return -1;
						}
						q++;
					} else if (*q == ':') {
						subState = 2;
						q++;
					} else {
						req->code = 400;
						return -1;
					}
				} else if (subState == 2) {
					if (*q == ' ' || *q == '\t') {
						state = HTTP_STATE_HEADER_VALUE;
						subState = 0;
						q++;
					} else {
						req->code = 400;
						return -1;
					}
				} else if (subState == 3) {
					state = HTTP_STATE_HEADER_DONE;
					subState = 0;
				} else {
					req->code = 400;
					return -1;
				}
			break;
			case HTTP_STATE_HEADER_VALUE:/* ^[\r] \r\n*/
				if (subState == 0) {
					if (*q == '\r') {
						q++;
						subState = 1;
					} else {
						subState = 2;
					}
				} else if (subState == 1) {
					if (*q == '\n') {
						q++;
						state = HTTP_STATE_HEADER_KEY;
						subState = 0;
						req->header_count++;
					} else {
						req->code = 400;
						return -1;
					}
				} else if (subState == 2) {
					if (*q == '\r') {
						subState = 1;
						q++;
					} else {
						if (push_value_char(req, *q) != 0){
							req->code = 413;
							return -1;
						}
						q++;
					}
				} else {
					req->code = 400;
					return -1;
				}
			break;
			case HTTP_STATE_HEADER_DONE:
				q++;// '\n'
				//recv header done.
				length_header = xnet_get_http_header_value(req, "content-length");
				if (length_header) {
					req->content_length = (uint32_t)xnet_string_toint(&length_header->value);
					if (body_limit != 0 && req->content_length > 0 &&
						(req->content_length + req->recv_len + (uint32_t)(eq - q)) > body_limit) {
						req->code = 413;
						return -1;
					}
				}
				if (xnet_string_compare_cs(&req->method, "POST") == 0) {
					state = HTTP_STATE_BODY;
					subState = 0;
				} else {
					state = HTTP_STATE_DONE;
					subState = 0;
					goto _out;
				}
			break;
			case HTTP_STATE_BODY:
				if (req->content_length == 0) {
					req->code = 400;
					return -1;
				}
				if (!req->body) req->body = xnet_string_create();
				xnet_string_add(req->body, *q);
				q++;
				if (xnet_string_get_size(req->body) > body_limit) {
					req->code = 413;
					return -1;
				}
				if (xnet_string_get_size(req->body) == req->content_length) {
					state = HTTP_STATE_DONE;
					subState = 0;
					goto _out;
				}
			break;
			default:
				req->code = 400;
				return -1;
		};
	}
_out:
	req->state = state;
	req->subState = subState;

	return (int)(q - buffer);
}

uint32_t
xnet_unpack_http(xnet_unpacker_t *up, const char *buffer, uint32_t sz) {
	xnet_httprequest_t *req = (xnet_httprequest_t *)up->arg;
	int ret;

	ret = parse_request(req, buffer, sz, up->limit);
	if (ret == -1) {
		//bad request
		if (req->code == 0) req->code = 400;
		up->full = true;
		return 0;
	}
	if (req->state == HTTP_STATE_DONE) {
		up->full = true;
		up->close = true;
		req->code = 200;
	}
	req->recv_len += ret;
	return (uint32_t)ret;
}

void
xnet_clear_http(void *arg) {
	int i;
	xnet_httprequest_t *req = (xnet_httprequest_t *)arg;
	if (req->body) {
		xnet_string_destroy(req->body);
	}
	xnet_string_clear(&req->method);
	xnet_string_clear(&req->url);
	xnet_string_clear(&req->version);
	if (req->header) {
		for (i=0; i<req->header_count; i++) {
			xnet_string_clear(&req->header[i].key);
			xnet_string_clear(&req->header[i].value);
		}
		free(req->header);
	}
	memset(req, 0, sizeof(*req));
}

xnet_httpheader_t *
xnet_get_http_header_value(void *req_or_rsp, const char *key) {
	xnet_httprequest_t *req = (xnet_httprequest_t *)req_or_rsp;
	xnet_httpheader_t *header = req->header;
	int i;
	if (!header || req->header_count == 0) return NULL;
	for (i=0; i<req->header_count; i++) {
		if (xnet_string_casecompare_cs(&header[i].key, key) == 0) {
			return &header[i];
		}
	}
	return NULL;
}

static const char *
get_http_status_msg(int state) {
	switch (state) {
		case 100:
			return "Continue";
		case 101:
			return "Switching Protocols";
		case 200:
			return "OK";
		case 201:
			return "Created";
		case 202:
			return "Accepted";
		case 203:
			return "Non-Authoritative Information";
		case 204:
			return "No Content";
		case 205:
			return "Reset Content";
		case 206:
			return "Partial Content";
		case 300:
			return "Multiple Choices";
		case 301:
			return "Moved Permanently";
		case 302:
			return "Found";
		case 303:
			return "See Other";
		case 304:
			return "Not Modified";
		case 305:
			return "Use Proxy";
		case 307:
			return "Temporary Redirect";
		case 400:
			return "Bad Request";
		case 401:
			return "Unauthorized";
		case 402:
			return "Payment Required";
		case 403:
			return "Forbidden";
		case 404:
			return "Not Found";
		case 405:
			return "Method Not Allowed";
		case 406:
			return "Not Acceptable";
		case 407:
			return "Proxy Authentication Required";
		case 408:
			return "Request Time-out";
		case 409:
			return "Conflict";
		case 410:
			return "Gone";
		case 411:
			return "Length Required";
		case 412:
			return "Precondition Failed";
		case 413:
			return "Request Entity Too Large";
		case 414:
			return "Request-URI Too Large";
		case 415:
			return "Unsupported Media Type";
		case 416:
			return "Requested range not satisfiable";
		case 417:
			return "Expectation Failed";
		case 500:
			return "Internal Server Error";
		case 501:
			return "Not Implemented";
		case 502:
			return "Bad Gateway";
		case 503:
			return "Service Unavailable";
		case 504:
			return "Gateway Time-out";
		case 505:
			return "HTTP Version not supported";
	};
	return "";
};

int
xnet_pack_http(xnet_httpresponse_t *rsp, xnet_string_t *out) {
	int i;
	char pack_str[128] = {};
	sprintf(pack_str, "HTTP/1.1 %d %s\r\n", rsp->code, get_http_status_msg(rsp->code));
	xnet_string_append_cs(out, pack_str);

	if (rsp->header) {
		for (i=0; i<rsp->header_count; i++) {
			xnet_string_append(out, &rsp->header[i].key);
			xnet_string_append_cs(out, ": ");
			xnet_string_append(out, &rsp->header[i].value);
			xnet_string_append_cs(out, "\r\n");
		}
	}
	if (rsp->body && xnet_string_get_size(rsp->body) > 0) {
		//if 'transfer-encoding' is set, don't need append 'content-length'
		if (xnet_get_http_header_value(rsp, "transfer-encoding")) {
			xnet_string_append_cs(out, "\r\n");
		} else {
			sprintf(pack_str, "content-length: %d\r\n\r\n", xnet_string_get_size(rsp->body));
			xnet_string_append_cs(out, pack_str);
		}
		xnet_string_append(out, rsp->body);
	} else {
		xnet_string_append_cs(out, "\r\n");
	}
	return 0;
}

inline void
xnet_set_http_rsp_code(xnet_httpresponse_t *rsp, int code) {
	rsp->code = code;
}

void
xnet_add_http_rsp_header(xnet_httpresponse_t *rsp, const char *key, const char *value) {
	uint32_t new_capacity;
	if (rsp->header_capacity >= rsp->header_count) {
		new_capacity = rsp->header_capacity ? rsp->header_capacity*2 : 32;
		rsp->header = realloc(rsp->header, sizeof(*rsp->header)*new_capacity);
		assert(rsp->header);
		memset(rsp->header + rsp->header_capacity, 0,
			sizeof(*rsp->header)*(new_capacity - rsp->header_capacity));
		rsp->header_capacity = new_capacity;
	}
	xnet_string_set_cs(&rsp->header[rsp->header_count].key, key);
	xnet_string_set_cs(&rsp->header[rsp->header_count].value, value);
	rsp->header_count ++;
}

void
xnet_set_http_rsp_body(xnet_httpresponse_t *rsp, const char *body) {
	if (!rsp->body) {
		rsp->body = xnet_string_create();
	}
	xnet_string_set_cs(rsp->body, body);
}

void
xnet_raw_set_http_rsp_body(xnet_httpresponse_t *rsp, char *body) {
	if (!rsp->body) {
		rsp->body = xnet_string_create();
	}
	xnet_string_raw_set_cs(rsp->body, body);
}

void
xnet_set_http_rsp_byte_body(xnet_httpresponse_t *rsp, const char *body, uint32_t sz) {
	if (!rsp->body) {
		rsp->body = xnet_string_create();
	}
	xnet_string_set(rsp->body, body, sz);
}

void
xnet_raw_set_http_rsp_byte_body(xnet_httpresponse_t *rsp, char *body, uint32_t sz) {
	if (!rsp->body) {
		rsp->body = xnet_string_create();
	}
	xnet_string_raw_set(rsp->body, body, sz);
}

void
xnet_clear_http_rsp(xnet_httpresponse_t *rsp) {
	int i;
	if (rsp->header) {
		for (i=0; i<rsp->header_count; i++) {
			xnet_string_clear(&rsp->header[i].key);
			xnet_string_clear(&rsp->header[i].value);
		}
		free(rsp->header);
	}
	if (rsp->body) {
		xnet_string_destroy(rsp->body);
	}
	memset(rsp, 0, sizeof(*rsp));
}


static uint32_t
read_size(char header[BUFFER_HEADER_SIZE]) {
	uint32_t sz = 0;
	memcpy(&sz, header, BUFFER_HEADER_SIZE);//小端序
	return sz;
}

static void
write_size(uint32_t sz, char *buffer) {
	memcpy(buffer, &sz, BUFFER_HEADER_SIZE);
}

uint32_t
xnet_unpack_sizebuffer(xnet_unpacker_t *up, const char *buffer, uint32_t sz) {
	uint32_t fill_size = 0;
	uint32_t fill_body;
	uint32_t body_size;
	xnet_sizebuffer_t *sb = (xnet_sizebuffer_t *)up->arg;

	if (sb->recv_len < BUFFER_HEADER_SIZE) {
		if (sb->recv_len + sz < BUFFER_HEADER_SIZE) {
			memcpy(sb->header+sb->recv_len, buffer, sz);
			sb->recv_len += sz;
			return sz;
		} else {
			fill_size = BUFFER_HEADER_SIZE - sb->recv_len;
			memcpy(sb->header+sb->recv_len, buffer, fill_size);
			sb->recv_len += fill_size;
			sb->buffer_size = read_size(sb->header);
			sz -= fill_size;
			if (sz == 0) return fill_size;
			buffer += fill_size;
		}
	}
	if (sb->buffer_size == 0 || (up->limit != 0 && sb->buffer_size > up->limit))
		return 0;

	if (sb->recv_buffer == NULL) {
		sb->recv_buffer = malloc(sb->buffer_size);
		assert(sb->recv_buffer);
	}

	body_size = sb->recv_len - BUFFER_HEADER_SIZE;
	if ((sz + body_size) < sb->buffer_size) {
		memcpy(sb->recv_buffer + body_size, buffer, sz);
		fill_size += sz;
		sb->recv_len += sz;
		return fill_size;
	}

	fill_body = sb->buffer_size - body_size;
	memcpy(sb->recv_buffer + body_size, buffer, fill_body);
	sb->recv_len += fill_body;
	fill_size += fill_body;
	up->full = true;
	return fill_size;
}

void
xnet_clear_sizebuffer(void *arg) {
	xnet_sizebuffer_t *sb = (xnet_sizebuffer_t *)arg;

	if (sb->recv_buffer) {
		free(sb->recv_buffer);
		sb->recv_buffer = NULL;
	}

	memset(sb, 0, sizeof(*sb));
}

int
xnet_pack_sizebuff(const char *buffer, uint32_t sz, xnet_string_t *out) {
	char *pack_buffer = malloc(sz + BUFFER_HEADER_SIZE);
	assert(pack_buffer);
	write_size(sz, pack_buffer);
	memcpy(pack_buffer + BUFFER_HEADER_SIZE, buffer, sz);
	xnet_string_raw_set(out, pack_buffer, sz + BUFFER_HEADER_SIZE);
	return 0;
}

uint32_t
xnet_unpack_line(xnet_unpacker_t *up, const char *buffer, uint32_t sz) {
	xnet_linebuffer_t *lb = (xnet_linebuffer_t *)up->arg;
	const char *q = buffer;
	const char *eq = q + sz;
	char *str;

	while (q < eq && *q != '\n') {
		if (up->limit != 0 && xnet_string_get_size(&lb->line_str) > up->limit) {
			return 0;
		}
		xnet_string_add(&lb->line_str, *q);
		q++;
	}
	if (*q == '\n') {
		q++;
		up->full = true;
		lb->sep = false;
		if (xnet_string_get_size(&lb->line_str) > 0) {
			str = xnet_string_get_str(&lb->line_str);
			if (str[xnet_string_get_size(&lb->line_str)-1] == '\r') {
				lb->line_str.size --;
				lb->sep = true;
			}
		}
	}
	return (uint32_t)(q - buffer);
}

void
xnet_clear_line(void *arg) {
	xnet_linebuffer_t *lb = (xnet_linebuffer_t *)arg;
	xnet_string_clear(&lb->line_str);
}