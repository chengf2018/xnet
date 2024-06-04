#ifndef _MALLOC_REF_C_
#define _MALLOC_REF_C_
#include <stdint.h>
#include <stddef.h>

#define REF_INT uint16_t

void *mf_malloc(size_t size);
void mf_add_ref(void *ptr);
void mf_set_ref(void *ptr, REF_INT n);
void mf_free(void *ptr);

#endif //_MALLOC_REF_C_