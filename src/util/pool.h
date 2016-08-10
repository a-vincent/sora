#ifndef UTIL_POOL_H_
#define UTIL_POOL_H_

#include <stddef.h>

struct pool;

struct pool *pool_new(const char *, size_t, size_t);
void pool_delete(struct pool *);
void *pool_get(struct pool *);
void pool_recycle(struct pool *, void *);

void pool_stats(struct pool *);
void pools_stats(void);

#endif /* UTIL_POOL_H_ */
