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
}

void
xnet_unpacker_free(xnet_unpacker_t *up) {
	up->cm(up->arg);
	free(up);
}

//return 0:success
//return -1:fail
int
xnet_unpacker_recv(xnet_unpacker_t *up, char *buffer, uint32_t sz) {
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
	}
	return (ret == 0) ? -1 : 0;
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
parse_request(xnet_httprequest_t *req, char *buffer, uint32_t sz, uint32_t body_limit) {
	uint16_t state = req->state;
	uint16_t subState = req->subState;
	char *q = buffer;
	char *eq = q + sz;
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
				length_header = xnet_get_http_header_value(req, "Content-Length");
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
xnet_unpack_http(xnet_unpacker_t *up, char *buffer, uint32_t sz) {
	xnet_httprequest_t *req = (xnet_httprequest_t *)up->arg;
	int ret, err_code;

	ret = parse_request(req, buffer, sz, up->limit);
	if (ret == -1) {
		//bad request
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
xnet_get_http_header_value(xnet_httprequest_t *req, const char *key) {
	xnet_httpheader_t *header = req->header;
	int i;
	if (!header || req->header_count == 0) return NULL;
	for (i=0; i<req->header_count; i++) {
		if (xnet_string_compare_cs(&header[i].key, key) == 0) {
			return &header[i];
		}
	}
	return NULL;
}

int
xnet_pack_http(xnet_httpresponse_t *rsp, char **buffer) {
	char pack_str[64];
	sprintf("HTTP/1.1 %d OK\r\n", rsp->code);
}



static uint32_t
read_size(char header[BUFFER_HEADER_SIZE]) {
	uint32_t sz = 0;
	memcpy(&sz, header, BUFFER_HEADER_SIZE);//小端序
	return sz;
}

uint32_t
xnet_unpack_sizebuffer(xnet_unpacker_t *up, char *buffer, uint32_t sz) {
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

uint32_t
xnet_unpack_line(xnet_unpacker_t *up, char *buffer, uint32_t sz) {

}

void
xnet_clear_line(void *arg) {

}