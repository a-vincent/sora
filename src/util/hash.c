
#include <util/hash.h>

#include <string.h>

#include <util/bsd-queue.h>
#include <util/debug.h>
#include <util/exception.h>
#include <util/iterator.h>
#include <util/memory.h>
#include <util/pool.h>
#ifdef DEBUG
#include <util/simple-math.h>
#endif

#define FIELD_PTR_BY_OFFSET(ptr, off) ((void *) ((char *)ptr + off))

SLIST_HEAD(hash_map_head, hash_element);

EXCEPTION_DEFINE(hash_key_not_found, runtime_error);

static struct pool *hash_elements_pool;

struct hash_map {
    int length;		/* number of buckets actually containing data */
    int number_of_elements;
    unsigned int (*hashfun)(const void *);
    int (*eqfun)(const void *, const void *);
    struct hash_map_head *table;
    int is_framework;
    int next_elt_offset;
#ifdef DEBUG
    int has_active_iter;
#endif
};

struct hash_element {
    const void *key;
    void *data;
    unsigned int hval;
    SLIST_ENTRY(hash_element) next;
};

#define HASH_PRIME_NUMBER 7

t_hash
hash_new(unsigned int hashfun(const void *),
			      int eqfun(const void *, const void *)) {
    t_hash hash = memory_alloc(sizeof *hash);

    hash->number_of_elements = 0;
    hash->length = 0;
    hash->table = NULL;
    hash->hashfun = hashfun;
    hash->eqfun = eqfun;
    hash->is_framework = 0;
    hash->next_elt_offset = 0;
#ifdef DEBUG
    hash->has_active_iter = 0;
#endif

    return hash;
}

t_hash
hash_framework_new(unsigned int hashfun(const void *),
		int eqfun(const void *, const void *), int offset) {
    t_hash hash = hash_new(hashfun, eqfun);

    hash->is_framework = 1;
    hash->next_elt_offset = offset;

    return hash;
}

static void
hash_new_size(t_hash hash, int size) {
    struct hash_map_head *table = memory_alloc(sizeof *table * size);
    int i;

    if (hash->is_framework) {
	void **t = (void *) table;

	for (i = 0; i < size; i++)
	    t[i] = NULL;
    } else {
	for (i = 0; i < size; i++)
	    SLIST_INIT(&table[i]);
    }

    hash->length = size;
    hash->table = table;
}

void
hash_rehash(t_hash hash) {
    struct hash_map_head *old_table = hash->table;
    int old_size = hash->length;
    int new_size = old_size == 0? HASH_PRIME_NUMBER : 2 * old_size + 1;
    int i;

    hash_new_size(hash, new_size);

    if (hash->is_framework) {
	void **old_t = (void *) old_table;
	void **new_t = (void *) hash->table;

	for (i = 0; i < old_size; i++) {
	    void *next_hep;
	    void *hep;

	    for (hep = old_t[i]; hep != NULL; hep = next_hep) {
		void **next_hepp =
			FIELD_PTR_BY_OFFSET(hep, hash->next_elt_offset);
		void **new_tp = &new_t[hash->hashfun(hep) % new_size];

		next_hep = *next_hepp;
		*next_hepp = *new_tp;
		*new_tp = hep;
	    }
	}
    } else {
	for (i = 0; i < old_size; i++) {
	    struct hash_element *next_hep;
	    struct hash_element *hep;

	    for (hep = SLIST_FIRST(&old_table[i]); hep != NULL;
							hep = next_hep) {
		struct hash_map_head *new_head =
			&hash->table[hep->hval % new_size];

		next_hep = SLIST_NEXT(hep, next);
		SLIST_INSERT_HEAD(new_head, hep, next);
	    }
	}
    }

    if (old_table != NULL)
	memory_free(old_table);
}

void
hash_delete(t_hash hash, int delete_elt(void *, void *)) {
    int i;

    if (hash->is_framework)
	EXCEPTION_RAISE(logic_error, "hash_delete() called on framework hash");

    for (i = 0; i < hash->length; i++) {
	struct hash_element *next_hep;
	struct hash_element *hep = SLIST_FIRST(hash->table + i);

	for (; hep != NULL; hep = next_hep) {
	    next_hep = SLIST_NEXT(hep, next);
	    delete_elt((void *) hep->key, (void *) hep->data);
	    pool_recycle(hash_elements_pool, hep);
	}
    }

    memory_free(hash->table);
    memory_free(hash);
}

void
hash_remove_elements(t_hash hash, int delete_elt_p(void *),
						void delete_elt(void *)) {
    int i;
    int length = hash->length;

    if (length == 0)
	return;

    if (hash->is_framework) {
	void **table = (void *) hash->table;
	int offset = hash->next_elt_offset;

	for (i = 0; i < length; i++) {
	    void **hepp;

	    for (hepp = &table[i]; *hepp != NULL;) {
		void *hep = *hepp;
		void **ptr = FIELD_PTR_BY_OFFSET(hep, offset);

		if (delete_elt_p(hep)) {
		    *hepp = *ptr;
		    hash->number_of_elements--;
		    delete_elt(hep);
		} else
		    hepp = ptr;
	    }
	}
    } else {
	for (i = 0; i < length; i++) {
	    struct hash_element **hepp;

	    for (hepp = &SLIST_FIRST(hash->table + i); *hepp != NULL;) {
		struct hash_element *hep = *hepp;
		void *key = (void *) hep->key;

		if (delete_elt_p(key)) {
		    *hepp = SLIST_NEXT(hep, next);
		    pool_recycle(hash_elements_pool, hep);
		    hash->number_of_elements--;
		    delete_elt(key);
		} else
		    hepp = &SLIST_NEXT(*hepp, next);
	    }
	}
    }
}

void *
hash_put(t_hash hash, const void *key, void *data) {
    unsigned int hval = hash->hashfun(key);
    struct hash_element *hep;
    struct hash_map_head *head;

#ifdef DEBUG
    if (hash->has_active_iter)
	EXCEPTION_RAISE(logic_error,
		"can't use hash_put() with active iterator");
    if (hash->is_framework)
	EXCEPTION_RAISE(logic_error,
		"hash_put() used on framework hash table");
#endif

    if (hash->length == 0 || hash->number_of_elements > hash->length)
	hash_rehash(hash);

    head = &hash->table[hval % hash->length];

    hep = SLIST_FIRST(head);
    for (; hep != NULL; hep = SLIST_NEXT(hep, next))
	if (hep->hval == hval && hash->eqfun(hep->key, key)) {
	    void *former = hep->data;

	    hep->data = data;
	    return former;
	}

    hep = pool_get(hash_elements_pool);
    hep->key = key;
    hep->data = data;
    hep->hval = hval;
    SLIST_INSERT_HEAD(head, hep, next);

    hash->number_of_elements++;
    return NULL;
}

void
hash_framework_put(t_hash hash, const void *key) {
    unsigned int hval = hash->hashfun(key);
    void **head;

#ifdef DEBUG
    if (hash->has_active_iter)
	EXCEPTION_RAISE(logic_error,
		"can't use hash_put() with active iterator");
#endif

    if (hash->length == 0 || hash->number_of_elements > hash->length)
	hash_rehash(hash);

    head = &((void **)hash->table)[hval % hash->length];

    *(void **) FIELD_PTR_BY_OFFSET(key, hash->next_elt_offset) = *head;
    *head = (void *) key;

    hash->number_of_elements++;
}

void *
hash_get_non_null(t_hash hash, const void *key) {
    unsigned int hval = hash->hashfun(key);

    if (hash->length == 0)
	return NULL;

    if (hash->is_framework) {
	void *hep = ((void **)hash->table)[hval % hash->length];
	int offset = hash->next_elt_offset;

	for (; hep != NULL; hep = *(void **) FIELD_PTR_BY_OFFSET(hep, offset))
	    if (hash->hashfun(hep) == hval && hash->eqfun(hep, key))
		return hep;
    } else {
	struct hash_element *hep =
		SLIST_FIRST(&hash->table[hval % hash->length]);
	for (; hep != NULL; hep = SLIST_NEXT(hep, next))
	    if (hep->hval == hval && hash->eqfun(hep->key, key))
		return hep->data;
    }

    return NULL;
}

void *
hash_get(t_hash hash, const void *key) {
    unsigned int hval = hash->hashfun(key);

    if (hash->length == 0)
	EXCEPTION_RAISE(hash_key_not_found, key);

    if (hash->is_framework) {
	void *hep = ((void **)hash->table)[hval % hash->length];
	int offset = hash->next_elt_offset;

	for (; hep != NULL; hep = *(void **) FIELD_PTR_BY_OFFSET(hep, offset))
	    if (hash->hashfun(hep) == hval && hash->eqfun(hep, key))
		return hep;
    } else {
	struct hash_element *hep =
		SLIST_FIRST(&hash->table[hval % hash->length]);
	for (; hep != NULL; hep = SLIST_NEXT(hep, next))
	    if (hep->hval == hval && hash->eqfun(hep->key, key))
		return hep->data;
    }

    EXCEPTION_RAISE(hash_key_not_found, key);
}

void *
hash_remove(t_hash hash, const void *key) {
    unsigned int hval = hash->hashfun(key);
    struct hash_element **hepp;

    if (hash->length == 0)
	EXCEPTION_RAISE(hash_key_not_found, key);

    if (hash->is_framework)
	EXCEPTION_RAISE(logic_error,
	    "hash_remove() not yet implemented for framework hash tables");

    hepp = &SLIST_FIRST(&hash->table[hval % hash->length]);
    for (; *hepp != NULL; hepp = &SLIST_NEXT(*hepp, next)) {
	struct hash_element *hep = *hepp;

	if (hep->hval == hval && hash->eqfun(hep->key, key)) {
	    void *data = hep->data;

	    *hepp = SLIST_NEXT(hep, next);
	    pool_recycle(hash_elements_pool, hep);
	    hash->number_of_elements--;
	    return data;
	}
    }

    EXCEPTION_RAISE(hash_key_not_found, key);
}

int
hash_get_size(t_hash hash) {
    return hash->number_of_elements;
}

struct hash_iterator {
    struct iterator iter;
    t_hash table;
    int bucket_no;
    void *next;
};

static int
hash_iterator_is_at_end(struct iterator *i) {
    struct hash_iterator *iter = (struct hash_iterator *) i;

    return iter->next == NULL;
}

static void
hash_iterator_next_pair(struct iterator *i, const void **key, void **data) {
    struct hash_iterator *iter = (struct hash_iterator *) i;
    struct hash_element *ret = iter->next;

    if (iter->next == NULL)
	iter->bucket_no = -1;
    else
	iter->next = SLIST_NEXT((struct hash_element *)iter->next, next);
    if (iter->next == NULL)
	while (++iter->bucket_no < iter->table->length && (iter->next =
	       SLIST_FIRST(&iter->table->table[iter->bucket_no])) == NULL)
	    ;

    if (ret == NULL)
	*key = *data = NULL;
    else {
	*key = ret->key;
	*data = ret->data;
    }
}

static void *
hash_iterator_framework_next(struct iterator *i) {
    struct hash_iterator *iter = (struct hash_iterator *) i;
    void *ret = iter->next;
    int offset = iter->table->next_elt_offset;

    if (iter->next == NULL)
	iter->bucket_no = -1;
    else
	iter->next = *(void **) FIELD_PTR_BY_OFFSET(iter->next, offset);

    if (iter->next == NULL)
	while (++iter->bucket_no < iter->table->length && (iter->next =
		((void **)iter->table->table)[iter->bucket_no]) == NULL)
	    ;

    return ret;
}

static void *
hash_iterator_next(struct iterator *i) {
    const void *key;
    void *data;

    hash_iterator_next_pair(i, &key, &data);

    return data;
}

static void
hash_iterator_delete(struct iterator *i) {
    struct hash_iterator *iter = (struct hash_iterator *) i;

#ifdef DEBUG
    iter->table->has_active_iter--;
#endif

    memory_free(iter);
}

struct iterator *
hash_iterator(t_hash hash) {
    struct hash_iterator *iter = memory_alloc(sizeof *iter);

    iter->iter.is_at_end = hash_iterator_is_at_end;
    if (hash->is_framework) {
	iter->iter.next = hash_iterator_framework_next;
	iter->iter.next_pair = NULL;
    } else {
	iter->iter.next = hash_iterator_next;
	iter->iter.next_pair = hash_iterator_next_pair;
    }
    iter->iter.free = hash_iterator_delete;

    iter->table = hash;
    iter->next = NULL;

#ifdef DEBUG
    hash->has_active_iter++;
#endif

    hash_iterator_next((struct iterator *) iter);

    return (struct iterator *) iter;
}

unsigned int
hash_hashfun_pointer(const void *ivoid) {
    unsigned int i;
    unsigned int res = 0;
    unsigned char a[sizeof ivoid];

    *(const void **) a = ivoid;
    for (i = 0; i < sizeof ivoid; i++)
	res = res * 33 + a[i];
    return res + (res >> 5);
}

int
hash_eqfun_pointer(const void *ivoid1, const void *ivoid2) {
    return ivoid1 == ivoid2;
}


unsigned int
hash_hashfun_string(const void *key) {
    const char *ckey = (const char *) key;
    unsigned int hval = 0;

    while (*ckey)
	hval = hval * 33 + *ckey++;

    return hval + (hval >> 5);
}

int
hash_eqfun_string(const void *key1, const void *key2) {
    return !strcmp((const char *) key1, (const char *) key2);
}

void
hash_init(void) {
    hash_elements_pool = pool_new("hash element", sizeof (struct hash_element),
	1024 * 1024);
}

int
hash_delete_memory_free_value(void *key, void *val) {
    (void) key;
    memory_free(val);
    return 1;
}

void
hash_stats(t_hash hash) {
#ifdef DEBUG
    unsigned int i;
    unsigned int n = hash->length;
    int lengths[64];

    DPRINTF((DEBUG_HASH, "hash map length: %d, number of elements: %d\n", n,
	hash->number_of_elements));

    for (i = 0; i < sizeof lengths / sizeof lengths[0]; i++)
	lengths[i] = 0;

    for (i = 0; i < n; i++) {
	int chain_length = 0;

	if (hash->is_framework) {
	    void *hep = ((void **) hash->table)[i];
	    int offset = hash->next_elt_offset;

	    while (hep != NULL) {
		chain_length++;
		hep = *(void **) FIELD_PTR_BY_OFFSET(hep, offset);
	    }
	} else {
	    struct hash_element *hep;

	    for (hep = SLIST_FIRST(&hash->table[i]); hep != NULL;
						hep = SLIST_NEXT(hep, next))
		chain_length++;
	}

	lengths[chain_length == 0? 0 : sm_log_base_2(chain_length)]++;
    }

    for (i = 0; i < sizeof lengths / sizeof lengths[0]; i++) {
	if (lengths[i] != 0)
	    DPRINTF((DEBUG_HASH,
		     "Chains of length <= %d: %d\n", 1 << i, lengths[i]));
    }
#else /* DEBUG */
    (void) hash;
#endif /* DEBUG*/
}
