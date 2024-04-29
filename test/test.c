#include "../src/xnet.h"
#include "../src/xnet_timeheap.h"
#include <time.h>
#include <assert.h>

static void
dump_timheap(xnet_timeheap_t *th) {
	int i;
	xnet_timeinfo_t *heap = th->heap;
	printf("dump heap:[");
	for (i=1; i<=th->n; i++) {
		printf("{%d,%d}", heap[i].id, heap[i].expire);
	}
	printf("] n:%d, size:%d\n", th->n, th->size);
}

int
main(int argc, char** argv) {
	srand(time(0));
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

	printf("start test timeheap..\n");
	xnet_timeheap_t th;
	xnet_timeinfo_t ti;
	xnet_timeheap_init(&th);
	int i, last_pop=-1, r;
	for (i=0; i<64; i++) {
		r = rand()%1000;
		xnet_timeheap_push(&th, &(xnet_timeinfo_t){i, r});
		printf("push:[%d,%d]\n", i, r);
		dump_timheap(&th);
	}
	for (i=0; i<64; i++) {
		xnet_timeheap_pop(&th, &ti);
		printf("pop:[%d,%d]\n", ti.id, ti.expire);
		dump_timheap(&th);
		if (last_pop != -1){
			assert(last_pop <= ti.expire);
		}
		last_pop = ti.expire;
	}

	xnet_timeheap_release(&th);
	printf("test timeheap finished\n");
	return 0;
}