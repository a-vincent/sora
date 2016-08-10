#ifndef UTIL_TIMER_H_
#define UTIL_TIMER_H_

struct timer_value {
	unsigned long tv_sec;
	unsigned long tv_usec;
};

struct timer;
struct iterator;

struct timer *timer_new(const char *);
void timer_remove(struct timer *);
struct timer_value timer_get_value(struct timer *);
int timer_is_active(struct timer *);
void timer_set_active(struct timer *, int);
void timer_reset(struct timer *);
void timer_start(struct timer *);
void timer_stop(struct timer *);
char *timer_value_to_string(struct timer *);
const char *timer_get_description(struct timer *);
struct iterator *timer_iterator(void);

#endif /* UTIL_TIMER_H_ */
