#ifndef _XNET_TIMEHEAP_H_
#define _XNET_TIMEHEAP_H_

#include <stdint.h>
#define HEAP_MIN_SIZE 32

typedef struct {
	int id;
	uint64_t expire;
} xnet_timeinfo_t;

typedef struct {
	xnet_timeinfo_t *heap;//heap[0] is not use
	int n;
	int size;
} xnet_timeheap_t;

void xnet_timeheap_init(xnet_timeheap_t *th);
void xnet_timeheap_release(xnet_timeheap_t *th);
void xnet_timeheap_push(xnet_timeheap_t *th, xnet_timeinfo_t *in);
int xnet_timeheap_pop(xnet_timeheap_t *th, xnet_timeinfo_t *out);
int xnet_timeheap_top(xnet_timeheap_t *th, xnet_timeinfo_t *out);

#endif //_XNET_TIMEHEAP_H_