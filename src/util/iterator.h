#ifndef UTIL_ITERATOR_H_
#define UTIL_ITERATOR_H_

struct iterator {
    int (*is_at_end)(struct iterator *);
    void *(*next)(struct iterator *);
    void (*next_pair)(struct iterator *, const void **, void **);
    void (*free)(struct iterator *);
};

#define iterator_is_at_end(it) ((it)->is_at_end((it)))
#define iterator_next(it) ((it)->next((it)))
#define iterator_next_pair(it, a, b) ((it)->next_pair((it), (a), (b)))
#define iterator_delete(it) ((it)->free((it)))

#endif /* UTIL_ITERATOR_H_ */
