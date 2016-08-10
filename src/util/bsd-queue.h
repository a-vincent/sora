
/* Public domain replacement for the classical BSD queue macros we need. */

#define SLIST_ENTRY(type) struct type *
#define SLIST_HEAD(headname, type) struct headname { struct type *sle_first; }
#define SLIST_HEAD_INITIALIZER(head) { NULL }
#define SLIST_INIT(head) (head)->sle_first = NULL
#define SLIST_FOREACH(var, head, entry) \
    for (var = (head)->sle_first; (var) != NULL; var = (var)->entry)
#define SLIST_NEXT(cell, entry) ((cell)->entry)
#define SLIST_FIRST(head) ((head)->sle_first)
#define SLIST_INSERT_HEAD(head, cell, entry) \
    do { (cell)->entry = (head)->sle_first; (head)->sle_first = (cell); } \
    while (/* CONSTCOND */ 0)
#define SLIST_INSERT_AFTER(before, after, entry) do {			\
	(after)->entry = (before)->entry;				\
	(before)->entry = (after);					\
    } while (/* CONSTCOND */ 0)


#define LIST_ENTRY(type) struct { \
	struct type *le_next;						\
	struct type **le_pprev;						\
}
#define LIST_HEAD(headname, type) struct headname { struct type *le_first; }
#define LIST_HEAD_INITIALIZER(head) { NULL }
#define LIST_INIT(head) (head)->le_first = NULL
#define LIST_FOREACH(var, head, entry) \
    for (var = (head)->le_first; (var) != NULL; var = (var)->entry.le_next)
#define LIST_NEXT(cell, entry) ((cell)->entry.le_next)
#define LIST_FIRST(head) ((head)->le_first)
#define LIST_INSERT_HEAD(head, cell, entry) do {			\
	if ((head)->le_first != NULL) {					\
	    (head)->le_first->entry.le_pprev = &(cell)->entry.le_next;	\
	}								\
	(cell)->entry.le_next = (head)->le_first;			\
	(cell)->entry.le_pprev = &(head)->le_first;			\
	(head)->le_first = (cell);					\
    } while (/* CONSTCOND */ 0)
#define LIST_INSERT_AFTER(before, after, entry) do {			\
	(after)->entry.le_next = (before)->entry.le_next;		\
	(after)->entry.le_pprev = &(before)->entry.le_next;		\
	(after)->entry.le_next->entry.le_prev = (after);		\
	(before)->entry.le_next = (after);				\
    } while (/* CONSTCOND */ 0)
#define LIST_REMOVE(elt, entry) do {					\
	*(elt)->entry.le_pprev= (elt)->entry.le_next;			\
	if ((elt)->entry.le_next != NULL)				\
	    (elt)->entry.le_next->entry.le_pprev = (elt)->entry.le_pprev;\
    } while (/* CONSTCOND */ 0)


#define SIMPLEQ_ENTRY(type) struct type *
#define SIMPLEQ_HEAD(headname, type) \
    struct headname { \
	struct type *sq_first; \
	struct type *sq_last; \
    }
#define SIMPLEQ_HEAD_INITIALIZER(head) { NULL, NULL }
#define SIMPLEQ_INIT(head) \
    do { (head)->sq_first = NULL; (head)->sq_last = NULL; } \
    while (/* CONSTCOND */ 0)
#define SIMPLEQ_FOREACH(var, head, entry) \
    for (var = (head)->sq_first; (var) != NULL; var = (var)->entry)
#define SIMPLEQ_NEXT(cell, entry) ((cell)->entry)
#define SIMPLEQ_FIRST(head) ((head)->sq_first)
#define SIMPLEQ_INSERT_HEAD(head, cell, entry) \
    do { \
	(cell)->entry = (head)->sq_first; \
	(head)->sq_first = (cell); \
	if ((head)->sq_last == NULL) \
		(head)->sq_last = (cell); \
    } while (/* CONSTCOND */ 0)
#define SIMPLEQ_INSERT_TAIL(head, cell, entry) \
    do { \
	(cell)->entry = NULL; \
	if ((head)->sq_last != NULL) \
	    (head)->sq_last->entry = (cell); \
	else \
	    (head)->sq_first = (cell); \
	(head)->sq_last = (cell); \
    } while (/* CONSTCOND */ 0)
#define SIMPLEQ_REMOVE_HEAD(head, entry) \
    do { \
	(head)->sq_first = (head)->sq_first->entry; \
	if ((head)->sq_first == NULL) \
	    (head)->sq_last = NULL; \
    } while (/* CONSTCOND */ 0)
