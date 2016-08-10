#ifndef UTIL_HASH_H
#define UTIL_HASH_H

#include <util/exception.h>

EXCEPTION_DECLARE(hash_key_not_found);

struct iterator;

typedef struct hash_map *t_hash;

t_hash hash_new(unsigned int (*)(const void *),
		int (*)(const void *, const void *));
void hash_delete(t_hash, int (*)(void *, void *));
void *hash_put(t_hash, const void *, void *);
void * hash_get_non_null(t_hash, const void *);
void *hash_get(t_hash, const void *);
void *hash_remove(t_hash, const void *);
int hash_get_size(t_hash);
void hash_remove_elements(t_hash, int (*)(void *), void (*)(void *));
struct iterator *hash_iterator(t_hash);

t_hash hash_framework_new(unsigned int (*)(const void *),
		int (*)(const void *, const void *), int);
void hash_framework_put(t_hash, const void *);

unsigned int hash_hashfun_pointer(const void *);
int hash_eqfun_pointer(const void *, const void *);

unsigned int hash_hashfun_string(const void *);
int hash_eqfun_string(const void *, const void *);

void hash_init(void);

int hash_delete_memory_free_value(void *, void *);

void hash_stats(t_hash);

#endif /* UTIL_HASH_H */
