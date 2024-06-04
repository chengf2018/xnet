#include "../src/xnet_packer.h"
#include <stdio.h>
#include <string.h>

static void
sizebuffer_callback(xnet_unpacker_t *up, void *arg) {
	xnet_sizebuffer_t *sb = (xnet_sizebuffer_t *)arg;
	printf("%s\n", sb->recv_buffer);
	printf("size:[%d]\n", sb->buffer_size);
}

int
main(int argc, char **argv) {
	xnet_unpacker_t *up;
	char *text = "hello world!";
	uint32_t len = strlen(text) + 1;
	char buffer[512];
	int ret;
printf("--------------11\n");
	up = xnet_unpacker_new(sizeof(xnet_sizebuffer_t), sizebuffer_callback, xnet_unpack_sizebuffer, xnet_clear_sizebuffer, 1024);
	if (!up) {
		printf("new unpacker fail\n");
		return 1;
	}
printf("--------------22\n");
	memcpy(buffer, &len, sizeof(len));
	memcpy(buffer+sizeof(len), text, len);
	ret = xnet_unpacker_recv(up, buffer, len);
printf("xnet_unpacker_recv, ret[%d]\n", ret);
	ret = xnet_unpacker_recv(up, buffer+len, sizeof(len));
printf("xnet_unpacker_recv, ret[%d]\n", ret);
printf("--------------33\n");
	xnet_unpacker_free(up);
printf("--------------44\n");
	return 0;
}