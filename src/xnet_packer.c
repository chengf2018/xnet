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
		}

		buffer += ret;
		sz -= ret;
		if (sz == 0) break;
		ret = up->um(up, buffer, sz);
	}
	return (ret == 0) ? -1 : 0;
}



uint32_t
xnet_unpack_http(xnet_unpacker_t *up, char *buffer, uint32_t sz) {
	xnet_httprequest_t *req = (xnet_httprequest_t *)up->arg;
	
}

void
xnet_clear_http(void *arg) {

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