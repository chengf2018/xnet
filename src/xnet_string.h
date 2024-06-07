#ifndef _XNET_STRING_H_
#define _XNET_STRING_H_

#include <stdint.h>

/*
 * 注意，这是非‘\0’结尾的字符串
 */

typedef struct {
	char *str;
	uint32_t size;
	uint32_t capacity;
	char next[0];
} xnet_string_t;

void xnet_string_init(xnet_string_t *s);
void xnet_string_clear(xnet_string_t *s);
xnet_string_t *xnet_string_create();
xnet_string_t *xnet_string_create_link();
void xnet_string_destroy(xnet_string_t *s);
void xnet_string_set(xnet_string_t *s, char *cs, uint32_t cs_size);
char *xnet_string_get_str(xnet_string_t *s);
char *xnet_string_get_c_str(xnet_string_t *s);
uint32_t xnet_string_get_size(xnet_string_t *s);
void xnet_string_append(xnet_string_t *s, char *cs, uint32_t cs_size);
void xnet_string_add(xnet_string_t *s, char c);
int xnet_string_compare_cs(xnet_string_t *s, const char *cs);
int xnet_string_toint(xnet_string_t *s);

#endif //_XNET_STRING_H_