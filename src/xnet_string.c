#include "xnet_string.h"
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#define MIN_STRING_SIZE 256

static void
fixsize(uint32_t sz) {
	uint32_t nsz = MIN_STRING_SIZE;
	assert(sz < 0x7FFFFFFF);
	while (nsz < sz) nsz *= 2;
	return nsz;
}

inline void
xnet_string_init(xnet_string_t *s) {
	memset(s, 0, sizeof(xnet_string_t));
}

inline void
xnet_string_clear(xnet_string_t *s) {
	if (s->str) free(s->str);
	memset(s, 0, sizeof(xnet_string_t));
}

xnet_string_t *
xnet_string_create() {
	xnet_string_t *s = malloc(sizeof(xnet_string_t));
	assert(s);
	memset(s, 0, sizeof(xnet_string_t));
	return s;
}

xnet_string_t *
xnet_string_create_link() {
	xnet_string_t *s = malloc(sizeof(xnet_string_t)+sizeof(xnet_string_t*));
	assert(s);
	memset(s, 0, sizeof(xnet_string_t)+sizeof(xnet_string_t*));
	return s;
}

void
xnet_string_destroy(xnet_string_t *s) {
	if (s->str) free(s->str);
	free(s);
}

void
xnet_string_set(xnet_string_t *s, char *cs, uint32_t cs_size) {
	if (cs_size == 0) return;
	if (s->capacity < cs_size) {
		s->str = realloc(s->str, cs_size);
		s->capacity = cs_size;
	}
	s->size = cs_size;
	memcpy(s->str, cs, cs_size);
}

inline char *
xnet_string_get(xnet_string_t *s) {
	return s->str;
}

void
xnet_string_append(xnet_string_t *s, char *cs, uint32_t cs_size) {
	uint32_t new_capacity;
	if (s->size + cs_size > s->capacity) {
		new_capacity = fixsize(s->size + cs_size);
		s->str = realloc(s->str, new_capacity);
		s->capacity = new_capacity;
	}
	memcpy(s->str + s->size, cs, cs_size);
	s->size += cs_size;
}