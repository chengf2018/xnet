#include "xnet_config.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>

#define CONFIG_MAP_ELEM_MIN_SIZE 32
#define TO_UCHAR(c) (unsigned char)(c)
#define TO_UINT(i) (unsigned int)(i)

static bool config_map_insert(config_map_t *cm, char *key, uint8_t value_type, map_elem_value_t value, bool raw);

static unsigned int
string_hash(const char *key) {
	unsigned int h;
	size_t len;
	h = TO_UINT(len = strlen(key));
	while (len-- > 0) {
		h ^= ((h<<5) + (h>>2) + TO_UCHAR(key[len]));
	}
	return h;
}

static void
config_map_init(config_map_t *cm) {
	cm->slots = malloc(sizeof(map_elem_t) * CONFIG_MAP_ELEM_MIN_SIZE);
	memset(cm->slots, 0, sizeof(map_elem_t) * CONFIG_MAP_ELEM_MIN_SIZE);
	cm->size = CONFIG_MAP_ELEM_MIN_SIZE;
	cm->n = 0;
}

static void
config_map_deinit(config_map_t *cm) {
	int i;
	map_elem_t *p, *tmp;
	if (!cm) return;
	if (cm->slots) {
		for (i=0; i<cm->size; i++) {
			p = &cm->slots[i];
			while (p && p->value_type != VALUE_TYPE_NIL) {
				tmp = p; p = p->next;
				if (tmp->key) free(tmp->key);
				if (tmp->value_type == VALUE_TYPE_STRING) free(tmp->value.s);
				if (tmp != &cm->slots[i]) free(tmp);
			}
		}
		free(cm->slots);
	}
	cm->size = 0;
	cm->n = 0;
	cm->slots = NULL;
}

static bool
config_map_find(config_map_t *cm, const char *key, map_elem_t **out) {
	map_elem_t *p;
	unsigned int slot_index = string_hash(key) % cm->size;
	p = &cm->slots[slot_index];
	while (p && p->value_type != VALUE_TYPE_NIL) {
		if (strcmp(key, p->key) == 0) {
			if (out) *out = p;
			return true;
		}
		p = p->next;
	}
	return false;
}

static void
config_map_rehash(config_map_t *old_cm, config_map_t *new_cm) {
	int i;
	map_elem_t *p, *tmp;
	for (i=0; i<old_cm->size; i++) {
		p = &old_cm->slots[i];
		while (p && p->value_type != VALUE_TYPE_NIL) {
			assert(config_map_insert(new_cm, p->key, p->value_type, p->value, true)==true);
			tmp = p; p = p->next;
			if (tmp != &old_cm->slots[i])//first node don't need free
				free(tmp);
		}
	}
	free(old_cm->slots);
	old_cm->slots = new_cm->slots;
	old_cm->size = new_cm->size;
	assert(old_cm->n == new_cm->n);
}

static void
config_map_reserve(config_map_t *cm) {
	int newsize;
	config_map_t new_cm;

	if ((cm->n / cm->size) > 2.0f) {
		newsize = cm->size * 2;
		new_cm.size = newsize;
		new_cm.n = 0;
		new_cm.slots = malloc(newsize * sizeof(map_elem_t));
		memset(new_cm.slots, 0, newsize * sizeof(map_elem_t));
		config_map_rehash(cm, &new_cm);
	}
}

static bool
config_map_insert(config_map_t *cm, char *key, uint8_t value_type, map_elem_value_t value, bool raw) {
	unsigned int slot_index;
	map_elem_t *elem, *last, *p;

	if (config_map_find(cm, key, NULL)) return false;

	config_map_reserve(cm);
	slot_index = string_hash(key) % cm->size;
	p = &cm->slots[slot_index];
	if (p->value_type == VALUE_TYPE_NIL) {
		elem = p;
		assert(p->next == NULL);
	} else {
		elem = malloc(sizeof(map_elem_t));
		if (p->next) elem->next = p->next;
		else elem->next = NULL;
		p->next = elem;
	}

	elem->key = raw ? key : strdup(key);
	elem->value_type = value_type;
	if (value_type == VALUE_TYPE_STRING) elem->value.s = raw ? value.s : strdup(value.s);
	else elem->value = value;
	cm->n++;
	return true;
}

typedef struct {
	FILE *fp;
	char buffer[BUFSIZ];
	const char *p;
	int n;
} loadfile_t;
typedef struct {
	char *buffer;
	int n;
	int size;
} tempbuff_t;
#define tempbuff_init(tb) ((tb)->size = (tb)->n = 0, (tb)->buffer = NULL)
#define tempbuff_resize(tb, sz) ((tb)->buffer = realloc((tb)->buffer, (sz)), (tb)->size=(sz))
#define tempbuff_reset(tb) (tb)->n = 0
#define tempbuff_free(tb) (((tb)->buffer) ? free((tb)->buffer) : 0)

static void
tempbuff_save(tempbuff_t *tb, int c) {
	int newsize;
	if (tb->n + 1 > tb->size) {
		newsize = tb->size ? (tb->size*2) : 32;
		tempbuff_resize(tb, newsize);
	}
	tb->buffer[tb->n++] = (char)c;
}

static int
nextc(loadfile_t *lf) {
	if (lf->n == 0) {
		lf->n = fread(lf->buffer, 1, sizeof(lf->buffer), lf->fp);
		lf->p = lf->buffer;
		if (lf->n == 0) return -1;
	}
	lf->n--;
	return (int)*(lf->p++);
}

static int
peekc(loadfile_t *lf) {
	if (lf->n == 0) {
		lf->n = fread(lf->buffer, 1, sizeof(lf->buffer), lf->fp);
		lf->p = lf->buffer;
		if (lf->n == 0) return -1;
	}
	return (int)*(lf->p);
}

static int
skipblank(loadfile_t *lf) {
	int c = peekc(lf);
	while (c != -1 && ((char)c==' ')||(char)c=='\t') {
		nextc(lf); c = peekc(lf);
	};
	return c;
}

static int
skipblank_rn(loadfile_t *lf) {
	int c = peekc(lf);
	while (c != -1 && ((char)c==' '||(char)c=='\t'||(char)c=='\r'||(char)c=='\n')){
		nextc(lf); c = peekc(lf);
	}
	return c;
}

static int
skipnoteline(loadfile_t *lf) {
	int c = peekc(lf);
	if (c != -1 && (char)c == '#') {
		nextc(lf);
		c = nextc(lf);
		while (c != -1 && (char)c != '\n')
			c = nextc(lf);
	}
	return c;
}
#define IS_NUMBER_CHAR(c) ((char)(c)>='0' && (char)(c) <= '9')
#define IS_TOKEN_FIRST_CHAR(c) (((char)(c)>='a' && (char)(c)<='z') || ((char)(c)>='A' && (char)(c)<='Z') || (char)(c) == '_')
#define IS_TOKEN_OTHER_CHAR(c) IS_TOKEN_FIRST_CHAR(c) || IS_NUMBER_CHAR(c)
#define IS_TOKEN_TRUE(c, i) ("true"[(i)] == (c))
#define IS_TOKEN_FALSE(c, i) ("false"[(i)] == (c))

static int
parse_token(loadfile_t *lf, tempbuff_t *tb) {
	int c;
	c = nextc(lf);
	if (!IS_TOKEN_FIRST_CHAR(c)) return -2;
	tempbuff_save(tb, c);
	c = nextc(lf);
	while (c != -1 && IS_TOKEN_OTHER_CHAR(c)) {
		tempbuff_save(tb, c);
		c = nextc(lf);
	}
	return c;
}

static int
parse_value(loadfile_t *lf, tempbuff_t *tb, int *type, map_elem_value_t *value) {
	int c, i;
	c = nextc(lf);
	if (c == '\"') {
		c = nextc(lf);
		while (c != -1 && c != '\"') {
			tempbuff_save(tb, c);
			c = nextc(lf);
		}
		tempbuff_save(tb, '\0');
		if (c != '\"') return -2;
		*type = VALUE_TYPE_STRING;
		value->s = tb->buffer;
		c = nextc(lf);
	} else if (IS_NUMBER_CHAR(c) || c == '-') {	
		tempbuff_save(tb, c);
		c = nextc(lf);
		while (c != -1 && IS_NUMBER_CHAR(c)) {
			tempbuff_save(tb, c);
			c = nextc(lf);
		}
		tempbuff_save(tb, '\0');
		*type = VALUE_TYPE_INT;
		value->i = atoi(tb->buffer);
	} else if (c == 't') {
		for (i=1; i<4; i++) {
			c = nextc(lf);
			if (c == -1 || !IS_TOKEN_TRUE(c, i))
				break;
		}
		if (i != 4) return -2;
		*type = VALUE_TYPE_BOOL;
		value->b = true;
		c = nextc(lf);
	} else if (c == 'f') {
		for (i=1; i<5; i++) {
			c = nextc(lf);
			if (c == -1 || !IS_TOKEN_FALSE(c, i))
				break;
		}
		if (i != 5) return -2;
		*type = VALUE_TYPE_BOOL;
		value->b = false;
		c = nextc(lf);
	} else {
		return -2;
	}
	return c;
}

static int
parse_field(xnet_config_t *config, loadfile_t *lf, tempbuff_t *tb) {
	int c, value_type;
	char *key = NULL;
	map_elem_value_t value;

	c = peekc(lf);
	if (c == '#') {
		c = skipnoteline(lf);
		return c;
	}

	tempbuff_reset(tb);

	c = parse_token(lf, tb);
	if (c < 0) return -2;

	tempbuff_save(tb, '\0');
	key = strdup(tb->buffer);

	skipblank(lf);
	c = nextc(lf);
	if (c != '=') goto _PARSE_ERROR;
	skipblank(lf);
	value_type = VALUE_TYPE_NIL;
	tempbuff_reset(tb);
	c = parse_value(lf, tb, &value_type, &value);
	if (c == -2 || value_type == VALUE_TYPE_NIL) goto _PARSE_ERROR;

	if (!config_map_insert(&config->fields, key, value_type, value, false)) {
		printf("repated field:%s\n", key);
	}
	free(key);
	return c;
_PARSE_ERROR:
	if (key) free(key);
	return -2;
}

//return 0:success, -1:can't open file, -2:parse error
int
xnet_parse_config(xnet_config_t *config, const char *filename) {
	int c;
	loadfile_t lf;
	tempbuff_t tb;
	FILE *fp = fopen(filename, "r");
	if (!fp) return -1;
	memset(&lf, 0, sizeof(lf));
	lf.fp = fp;
	tempbuff_init(&tb);

	for (;;) {
		c = parse_field(config, &lf, &tb);
		if (c < 0) break;
		if (c == '\r' || c == '\n' || c == ' ' || c == '\n' || c == '#') {
			c = skipblank_rn(&lf);
			if (c < 0) break;
		} else {
			break;
		}
	}

	tempbuff_free(&tb);
	fclose(fp);
	return c == -1 ? 0 : -2;
}

void
xnet_config_init(xnet_config_t *config) {
	config_map_init(&config->fields);
}

void
xnet_release_config(xnet_config_t *config) {
	config_map_deinit(&config->fields);
}

bool
xnet_get_field2i(xnet_config_t *config, const char *field_name, int *out) {
	map_elem_t *elem = NULL;
	if (config_map_find(&config->fields, field_name, &elem)) {
		if (elem->value_type != VALUE_TYPE_INT) return false;
		if (out) *out = elem->value.i;
		return true;
	}
	return false;
}

bool
xnet_get_field2s(xnet_config_t *config, const char *field_name, char **out) {
	map_elem_t *elem = NULL;
	if (config_map_find(&config->fields, field_name, &elem)) {
		if (elem->value_type != VALUE_TYPE_STRING) return false;
		if (out) *out = elem->value.s;
		return true;
	}
	return false;
}

bool
xnet_get_field2b(xnet_config_t *config, const char *field_name, bool *out) {
	map_elem_t *elem = NULL;
	if (config_map_find(&config->fields, field_name, &elem)) {
		if (elem->value_type != VALUE_TYPE_BOOL) return false;
		if (out) *out = elem->value.b;
		return true;
	}
	return false;
}