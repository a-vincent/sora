#ifndef UTIL_LIST_H_
#define UTIL_LIST_H_

struct iterator;

typedef struct list_element *t_list;

t_list list_cons(void *, t_list);
void *list_car(t_list);
t_list list_cdr(t_list);
void list_delete(t_list);
t_list list_delete_elt(t_list, void *);
void list_delete_list(t_list, int (*)(void *));
unsigned int list_length(t_list);
struct iterator *list_iterator(t_list);
t_list list_reverse(t_list);

void list_init(void);

#endif /* UTIL_LIST_H_ */
