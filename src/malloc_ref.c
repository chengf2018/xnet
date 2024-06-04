#include "malloc_ref.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#define REF_SIZE sizeof(REF_INT)

void *
mf_malloc(size_t size) {
	void *ptr = malloc(size+REF_SIZE);
	REF_INT *ref = ptr;
	*ref = 0;
	return ptr + REF_SIZE;
}

void
mf_add_ref(void *ptr) {
	REF_INT *ref = (REF_INT *)(ptr-REF_SIZE);
	(*ref)++;
}

void
mf_set_ref(void *ptr, REF_INT n) {
	REF_INT *ref = (REF_INT *)(ptr-REF_SIZE);
	(*ref) = n;
}

void
mf_free(void *ptr) {
	REF_INT *ref = (REF_INT *)(ptr-REF_SIZE);
	if (*ref == 0 || *ref-1 == 0) {
		free(ptr-REF_SIZE);
		return;
	}
	(*ref)--;
}