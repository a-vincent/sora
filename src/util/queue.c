
#include <util/queue.h>

#include <util/bsd-queue.h>
#include <util/pool.h>

static struct pool *queue_elmt_pool;
static struct pool *queue_pool;

struct queue_element;

struct queue {
    SIMPLEQ_HEAD(, queue_element) head;
};

struct queue_element {
    SIMPLEQ_ENTRY(queue_element) next;
    void *value;
};

t_queue
queue_empty(void) {
    struct queue *q = pool_get(queue_pool);
    SIMPLEQ_INIT(&q->head);
    return q;
}

int
queue_is_empty(t_queue q) {
    return SIMPLEQ_FIRST(&q->head) == NULL;
}

t_queue
queue_pop(t_queue q) {
    struct queue_element *elt = SIMPLEQ_FIRST(&q->head);

    SIMPLEQ_REMOVE_HEAD(&q->head, next);
    pool_recycle(queue_elmt_pool, elt);

    return q;
}

t_queue
queue_push(t_queue q, void *value) {
    struct queue_element *elt = pool_get(queue_elmt_pool);

    elt->value = value;
    SIMPLEQ_INSERT_TAIL(&q->head, elt, next);

    return q;
}

void *
queue_top(t_queue q) {

    return SIMPLEQ_FIRST(&q->head)->value;
}

unsigned int
queue_length(t_queue q) {
    struct queue_element *elt;
    unsigned int i = 0;

    SIMPLEQ_FOREACH(elt, &q->head, next)
	i++;

    return i;
}

void
queue_delete_queue(t_queue q, int elt_delete(void *)) {
    struct queue_element *elt = SIMPLEQ_FIRST(&q->head);

    while (elt != NULL) {
	struct queue_element *next_elt = SIMPLEQ_NEXT(elt, next);

	if (elt_delete != NULL)
	    elt_delete(elt->value);

	pool_recycle(queue_elmt_pool, elt);
	elt = next_elt;
    }

    pool_recycle(queue_pool, q);
}

void
queue_init(void) {
    queue_pool = pool_new("queue", sizeof (struct queue), 1024);
    queue_elmt_pool =
	pool_new("queue-elmt", sizeof (struct queue_element), 16 * 1024);
}
