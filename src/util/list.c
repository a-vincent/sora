
#include <util/list.h>

#include <util/iterator.h>
#include <util/memory.h>
#include <util/pool.h>

static struct pool *list_cons_pool;

struct list_element {
    void *car;
    struct list_element *cdr;
};

t_list
list_cons(void *car, t_list cdr) {
    struct list_element *cons = pool_get(list_cons_pool);

    cons->car = car;
    cons->cdr = cdr;

    return cons;
}

void *
list_car(t_list cons) {
    return cons->car;
}

t_list
list_cdr(t_list cons) {
    return cons->cdr;
}

unsigned int
list_length(t_list cons) {
    unsigned int i;

    for (i = 0; cons != NULL; i++)
	cons = list_cdr(cons);

    return i;
}

void
list_delete(t_list cons) {
    pool_recycle(list_cons_pool, cons);
}

t_list
list_delete_elt(t_list cons, void *elt) {
    t_list *p = &cons;

    while (*p != NULL) {
	t_list tmp = *p;

	if (tmp->car == elt) {
	    *p = tmp->cdr;
	    list_delete(tmp);
	    break;
	}
	p = &tmp->cdr;
    }

    return cons;
}

void
list_delete_list(t_list cons, int elt_delete(void *)) {
    while (cons != NULL) {
	struct list_element *tofree = cons;

	elt_delete(cons->car);
	cons = cons->cdr;
	list_delete(tofree);
    }
}

struct list_iterator {
    struct iterator iter;
    t_list tail;
};

static int
list_iterator_is_at_end(struct iterator *i) {
    struct list_iterator *iter = (struct list_iterator *) i;

    return iter->tail == NULL;
}

static void *
list_iterator_next(struct iterator *i) {
    struct list_iterator *iter = (struct list_iterator *) i;
    void *car = iter->tail->car;

    iter->tail = iter->tail->cdr;

    return car;
}

static void
list_iterator_delete(struct iterator *i) {
    memory_free(i);
}

struct iterator *
list_iterator(t_list list) {
    struct list_iterator *i = memory_alloc(sizeof *i);

    i->iter.is_at_end = list_iterator_is_at_end;
    i->iter.next = list_iterator_next;
    i->iter.next_pair = NULL;
    i->iter.free = list_iterator_delete;

    i->tail = list;

    return (struct iterator *) i;
}

t_list
list_reverse(t_list list){

    struct list_element *cons1,*cons2,*cons3;

    cons1 = list;

    cons2 = cons1->cdr;
    cons1->cdr = NULL;

    while (cons2 != NULL){
	cons3 = cons2->cdr;
	cons2->cdr = cons1;
	cons1 = cons2;
	cons2 = cons3;
    }

    return cons1;
}

void
list_init(void) {
    list_cons_pool = pool_new("cons", sizeof (struct list_element), 16 * 1024);
}
