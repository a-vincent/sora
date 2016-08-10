#ifndef UTIL_MEMORY_H_
#define UTIL_MEMORY_H_

#include <stddef.h>

#include <util/exception.h>

EXCEPTION_DECLARE(memory_outage);

void *memory_alloc(size_t);
void *memory_realloc(void *, size_t);
void memory_free(void *);
char *memory_strdup(const char *);

int memory_elt_delete_noop(void *);
int memory_pair_delete_noop(void *, void *);

#endif /* UTIL_MEMORY_H_ */
