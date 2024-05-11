#include "../src/xnet.h"
#include "../src/xnet_timeheap.h"
#include "../src/xnet_config.h"
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

	printf("--------start test timeheap..--------\n");
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


	printf("--------start test config parse--------\n");
	xnet_config_t config;
	int ret, cfg_thread, cfg_port;
	char *cfg_log_path;
	bool succ, cfg_enable_gm;

	xnet_config_init(&config);
	ret = xnet_parse_config(&config, "./test.config");
	printf("parse config return:[%d]\n", ret);

	succ = xnet_get_field2i(&config, "thread", &cfg_thread);
	printf("parse thread succ:[%d]\n", succ);
	succ = xnet_get_field2i(&config, "port", &cfg_port);
	printf("parse port succ:[%d]\n", succ);
	succ = xnet_get_field2s(&config, "log_path", &cfg_log_path);
	printf("parse log_path succ:[%d]\n", succ);
	succ = xnet_get_field2b(&config, "enable_gm", &cfg_enable_gm);
	printf("parse enable_gm succ:[%d]\n", succ);
	xnet_release_config(&config);
	printf("thread:[%d], port:[%d], log_path:[%s], enable_gm[%d]\n", cfg_thread,
		cfg_port, cfg_log_path, cfg_enable_gm);
	//test case, see "test.config"
	assert(cfg_thread == 4);
	assert(cfg_port == 8888);
	assert(strcmp(cfg_log_path, "./run.log") == 0);
	assert(cfg_enable_gm == true);

	printf("test config parse finished\n");


	printf("all test finished!\n");
	return 0;
}