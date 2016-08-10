
#include <util/pool.h>

#include <string.h>

#include <util/bsd-queue.h>
#include <util/debug.h>
#include <util/memory.h>
#include <util/simple-math.h>

struct pool_element;
struct pool_page;

SLIST_HEAD(, pool) pools_global_list =
	SLIST_HEAD_INITIALIZER(pools_global_list);

struct pool {
    SLIST_ENTRY(pool) next;
    const char *name;
    size_t elem_size;
    size_t page_size;
    size_t memory_size;
    size_t nallocated_elems;
    size_t nelems;
    size_t nallocations;
    SLIST_HEAD(, pool_page) pages;
    SLIST_HEAD(, pool_element) recycled;
};

struct pool_page {
    SLIST_ENTRY(pool_page) next;
    void *pool_page_start;
    void *current_elem;
    size_t nfree_elems;
};

struct pool_element {
    SLIST_ENTRY(pool_element) next;
};

static struct pool_page *pool_page_new(struct pool *);
static void pool_page_delete(struct pool *, struct pool_page *);

struct pool *
pool_new(const char *name, size_t elem_size, size_t page_size) {
    struct pool *pool;

    pool = memory_alloc(sizeof *pool);
    pool->name = name;
    pool->elem_size = sm_max(sizeof (struct pool_element), elem_size);
    pool->page_size = page_size;
    pool->memory_size = 0;
    pool->nelems = 0;
    pool->nallocated_elems = 0;
    pool->nallocations = 0;
    SLIST_INIT(&pool->pages);
    SLIST_INIT(&pool->recycled);

    /* Insert the new pool into the global pools list */

    {
	struct pool **p = &SLIST_FIRST(&pools_global_list);

	for (; *p != NULL; p = &SLIST_NEXT(*p, next)) {
	    if ((*p)->name == NULL)
		break;

	    if (pool->name != NULL && strcmp(pool->name, (*p)->name) < 0)
		break;
	}

	SLIST_NEXT(pool, next) = *p;
	*p = pool;
    }

    return pool;
}

void
pool_delete(struct pool *pool) {
    struct pool **p = &SLIST_FIRST(&pools_global_list);
    struct pool_page *page;

    for (; *p != pool; p = &SLIST_NEXT(*p, next))
	;

    *p = SLIST_NEXT(*p, next);

    for (page = SLIST_FIRST(&pool->pages); page != NULL; ) {
	struct pool_page *tmp = SLIST_NEXT(page, next);
	pool_page_delete(pool, page);
	page = tmp;
    }

    memory_free(pool);
}

void *
pool_get(struct pool *pool) {
    struct pool_element *elt = SLIST_FIRST(&pool->recycled);
    struct pool_page *page;

    pool->nallocations++;

    /* See if there is a recycled element */
    if (elt != NULL) {
	SLIST_FIRST(&pool->recycled) = SLIST_NEXT(elt, next);
	pool->nelems++;
	return elt;
    }

    /* See if there is a page that is not full */
    SLIST_FOREACH(page, &pool->pages, next)
	if (page->nfree_elems != 0)
	    break;

    if (page == NULL) {
	page = pool_page_new(pool);
	SLIST_INSERT_HEAD(&pool->pages, page, next);
    }

    elt = page->current_elem;

    page->nfree_elems--;
    page->current_elem = (char *) page->current_elem + pool->elem_size;
    pool->nelems++;

    return elt;
}

void
pool_recycle(struct pool *pool, void *element) {
    struct pool_element *elem = element;

#if 0
    memset(element, -1, pool->elem_size);
#endif
    SLIST_INSERT_HEAD(&pool->recycled, elem, next);
    pool->nelems--;
}

static struct pool_page *
pool_page_new(struct pool *pool) {
    struct pool_page *page = memory_alloc(sizeof *page);

    page->current_elem = page->pool_page_start = memory_alloc(pool->page_size);
    pool->memory_size += pool->page_size;
    page->nfree_elems = pool->page_size / pool->elem_size;
    pool->nallocated_elems += page->nfree_elems;

    return page;
}

static void
pool_page_delete(struct pool *pool, struct pool_page *page) {
    memory_free(page->pool_page_start);
    pool->memory_size -= pool->page_size;
    pool->nallocated_elems -= pool->page_size / pool->elem_size;
    memory_free(page);
}

void
pool_stats(struct pool *pool) {
#ifdef DEBUG
    DPRINTF((DEBUG_POOL, "%10ld/%10ld elements (%7ld KiB)",
	(unsigned long) pool->nelems,
	(unsigned long) pool->nallocated_elems,
	(unsigned long) pool->memory_size / 1024));

    if (pool->name != NULL)
	DPRINTF((DEBUG_POOL, " | %-14s", pool->name));

    DPRINTF((DEBUG_POOL, " %10ld allocs", (unsigned long) pool->nallocations));
#else /* DEBUG */
    (void) pool;
#endif /* DEBUG */
}

void
pools_stats(void) {
#ifdef DEBUG
    struct pool *pool;

    SLIST_FOREACH(pool, &pools_global_list, next)
	if (pool->name != NULL) {
	    pool_stats(pool);
	    DPRINTF((DEBUG_POOL, "\n"));
	}
#endif /* DEBUG */
}
