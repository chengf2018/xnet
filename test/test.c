#include "../src/xnet.h"

int
main(int argc, char** argv) {
	printf("start testing..\n");
	printf("test mq..\n");
	xnet_mq_t mq;
	xnet_mq_init(&mq);

	xnet_mq_push(&mq, xnet_newmessage("11111", 1, 1));
	xnet_mq_push(&mq, xnet_newmessage("22222", 2, 1));
	xnet_mq_push(&mq, xnet_newmessage("33333", 3, 1));
	xnet_mq_push(&mq, xnet_newmessage("44444", 4, 1));

	xnet_message_t *message = xnet_mq_pop(&mq);
	while (message) {
		printf("--------%s,%d,%d\n", message->data, message->size, message->type);

		//xnet_freemessage(message);
		message = xnet_mq_pop(&mq);
	}

	printf("test mq finished\n");
	return 0;
}