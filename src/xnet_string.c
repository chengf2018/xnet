#include "xnet_string.h"
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>

#define MIN_STRING_SIZE 256

static uint32_t
fixsize(uint32_t sz) {
	uint32_t nsz = MIN_STRING_SIZE;
	assert(sz < 0x7FFFFFFF);
	while (nsz < sz) nsz *= 2;
	return nsz;
}

static void
reserve(xnet_string_t *s, uint32_t add) {
	uint32_t new_capacity;
	if (s->size + add > s->capacity) {
		new_capacity = fixsize(s->size + add);
		s->str = realloc(s->str, new_capacity);
		s->capacity = new_capacity;
	}
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
xnet_string_set(xnet_string_t *s, const char *cs, uint32_t cs_size) {
	if (s->capacity < cs_size) {
		s->str = realloc(s->str, cs_size);
		s->capacity = cs_size;
	}
	s->size = cs_size;
	if (cs_size != 0)
		memcpy(s->str, cs, cs_size);
}

void
xnet_string_set_cs(xnet_string_t *s, const char *cs) {
	uint32_t cs_size = strlen(cs);
	xnet_string_set(s, cs, cs_size);
}

inline char *
xnet_string_get_str(xnet_string_t *s) {
	if (s->size == 0) return "";
	return s->str;
}

//自动填充\0结尾
inline char *
xnet_string_get_c_str(xnet_string_t *s) {
	uint32_t new_capacity;

	if (s->size == 0) return "";
	if (s->str[s->size-1] == '\0') {
		return s->str;
	}
	reserve(s, 1);
	s->str[s->size] = '\0';
	return s->str;
}

inline uint32_t
xnet_string_get_size(xnet_string_t *s) {
	return s->size;
}

void
xnet_string_append(xnet_string_t *as, xnet_string_t *bs) {
	if (bs->size == 0) return;
	reserve(as, bs->size);
	memcpy(as->str + as->size, bs->str, bs->size);
	as->size += bs->size;

}

void
xnet_string_append_cs(xnet_string_t *s, char *cs) {
	uint32_t len = strlen(cs);
	reserve(s, len);
	memcpy(s->str + s->size, cs, len);
	s->size += len;
}

void
xnet_string_add(xnet_string_t *s, char c) {
	reserve(s, 1);
	s->str[s->size++] = c;
}

static int
__strcasecmp(const char *s1, const char *s2) {
    for (; *s1 && *s2; ++s1, ++s2) {
        if (tolower((unsigned char)*s1) != tolower((unsigned char)*s2)) {
            return tolower((unsigned char)*s1) - tolower((unsigned char)*s2);
        }
    }
    return (*s1 == '\0') ? (*s2 == '\0' ? 0 : -1) : 1;
}

int
xnet_string_compare_cs(xnet_string_t *s, const char *cs) {
	char *str = xnet_string_get_c_str(s);
	return strcmp(str, cs);
}

int
xnet_string_casecompare_cs(xnet_string_t *s, const char *cs) {
	char *str = xnet_string_get_c_str(s);
	return __strcasecmp(str, cs);
}

int
xnet_string_toint(xnet_string_t *s) {
	char temp_str[16];
	if (s->size > 11 || !s->str) return 0;
	strncpy(temp_str, s->str, s->size);
	temp_str[s->size] = '\0';
	return atoi(temp_str);
}