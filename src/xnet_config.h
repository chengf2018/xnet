#ifndef _XNET_CONFIG_H_
#define _XNET_CONFIG_H_

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#define VALUE_TYPE_NIL    0
#define VALUE_TYPE_INT    1
#define VALUE_TYPE_STRING 2
#define VALUE_TYPE_BOOL   3

typedef union {
	int i;
	char *s;
	bool b;
} map_elem_value_t;

typedef struct map_elem {
	char *key;
	uint8_t value_type;
	map_elem_value_t value;
	struct map_elem *next;
} map_elem_t;

typedef struct {
	map_elem_t *slots;
	int n;
	int size;
} config_map_t;

typedef struct {
	config_map_t fields;
} xnet_config_t;

void xnet_config_init(xnet_config_t *config);
int xnet_parse_config(xnet_config_t *config, const char *filename);
void xnet_release_config(xnet_config_t *config);
bool xnet_get_field2i(xnet_config_t *config, const char *field_name, int *out);
bool xnet_get_field2s(xnet_config_t *config, const char *field_name, char **out);
bool xnet_get_field2b(xnet_config_t *config, const char *field_name, bool *out);

#endif //_XNET_CONFIG_H_