#include "xnet_timeheap.h"
#include <stdlib.h>
#include <string.h>

static void
reserve(xnet_timeheap_t *th) {
	if (th->n + 1 >= th->size) {
		th->heap = realloc(th->heap, sizeof(xnet_timeinfo_t) * th->size * 2);
		memset(&th->heap[th->size], 0, sizeof(xnet_timeinfo_t) * th->size);
		th->size *= 2;
	}
}

static void
shift_up(xnet_timeheap_t *th, xnet_timeinfo_t *ti) {
	xnet_timeinfo_t *heap = th->heap;
	int n = th->n;
	int parent = n / 2;

	while (parent > 0) {
		if (ti->expire < heap[parent].expire) {
			heap[n] = heap[parent];
			n = parent;
			parent /= 2;
		} else { break; }
	}
	heap[n] = *ti;
}

static void
shift_down(xnet_timeheap_t *th) {
	int l, r, s;
	int p = 1;
	xnet_timeinfo_t *heap = th->heap;
	xnet_timeinfo_t ti = heap[th->n--];

	while ((l = p * 2) <= th->n) {
		r = l + 1;
		s = (r <= th->n && heap[r].expire < heap[l].expire) ? r : l;
		if (heap[s].expire < ti.expire) {
			heap[p] = heap[s];
			p = s;
		} else { break; }
	}
	heap[p] = ti;
}

void
xnet_timeheap_init(xnet_timeheap_t *th) {
	th->size = HEAP_MIN_SIZE;
	th->n = 0;
	th->heap = malloc(sizeof(xnet_timeinfo_t) * th->size);
	memset(th->heap, 0, sizeof(xnet_timeinfo_t) * th->size);
}

void
xnet_timeheap_release(xnet_timeheap_t *th) {
	free(th->heap);
	th->heap = NULL;
	th->size = th->n = 0;
}

void
xnet_timeheap_push(xnet_timeheap_t *th, xnet_timeinfo_t *timeinfo) {
	reserve(th);
	++th->n;
	shift_up(th, timeinfo);
}

int
xnet_timeheap_pop(xnet_timeheap_t *th, xnet_timeinfo_t *out) {
	if (th->n < 0) return 0;
	if (out) *out = th->heap[1];
	if (th->n > 1) shift_down(th);
	else th->n --;
	return 1;
}

int
xnet_timeheap_top(xnet_timeheap_t *th, xnet_timeinfo_t *out) {
	if (th->n <= 0 || !out) return 0;
	*out = th->heap[1];
	return 1;
}