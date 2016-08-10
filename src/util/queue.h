#ifndef UTIL_QUEUE_H_
#define UTIL_QUEUE_H_

typedef struct queue *t_queue;

t_queue queue_empty(void);
int queue_is_empty(t_queue);
t_queue queue_pop(t_queue);
t_queue queue_push(t_queue, void *);
void *queue_top(t_queue);
unsigned int queue_length(t_queue);
void queue_delete_queue(t_queue, int (*)(void *));

void queue_init(void);

#endif /* UTIL_QUEUE_H_ */
