
#include <util/memory.h>

#ifdef USE_BOEHM_GC
#define GC_DEBUG
#include <gc/gc.h>

#define MEMORY_DO_MALLOC(s) GC_MALLOC((s))
#define MEMORY_DO_REALLOC(p, s) GC_REALLOC((p), (s))
#define MEMORY_DO_FREE(s) GC_FREE((s))
#else
#define MEMORY_DO_MALLOC(s) malloc((s))
#define MEMORY_DO_REALLOC(p, s) realloc((p), (s))
#define MEMORY_DO_FREE(s) free((s))
#endif /* USE_BOEHM_GC */

#include <stdlib.h>
#include <string.h>

EXCEPTION_DEFINE(memory_outage, runtime_error);

void *
memory_alloc(size_t size) {
    void *buf = MEMORY_DO_MALLOC(size);

    if (buf == NULL)
	EXCEPTION_RAISE(memory_outage, "memory_alloc");

    return buf;
}

void *
memory_realloc(void *mem, size_t size) {
    void *buf = MEMORY_DO_REALLOC(mem, size);

    if (buf == NULL)
	EXCEPTION_RAISE(memory_outage, "memory_realloc");

    return buf;
}

void
memory_free(void *mem) {
    MEMORY_DO_FREE(mem);
}

char *
memory_strdup(const char *string) {
    size_t len = strlen(string);
    char *buf = memory_alloc(len + 1);

    strcpy(buf, string);

    return buf;
}

int
memory_elt_delete_noop(void *unused) {
    (void) unused;
    return 1;
}

int
memory_pair_delete_noop(void *unused1, void *unused2) {
    (void) unused1;
    (void) unused2;
    return 1;
}
