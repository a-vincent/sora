
#include <util/timer.h>

#include <util/list.h>
#include <util/memory.h>
#include <util/string.h>

#if defined(HAVE_CLOCK)
#include <sys/time.h>
#include <time.h>
#elif defined(HAVE_GETRUSAGE)
#include <sys/resource.h>
#endif

t_list timer_all_timers = NULL;

struct timer {
    char *description;
#if defined(HAVE_CLOCK)
    clock_t clock_value;
    clock_t last_start;
#elif defined(HAVE_GETRUSAGE)
    struct timer_value value;
    struct timeval last_start;
#endif
    int flags;
#define TIMER_ACTIVE	0x1
#define TIMER_STARTED	0x2
};

#ifdef HAVE_GETRUSAGE
static struct timeval
timer_timeval_normalize(struct timeval tv) {
    struct timeval r = tv;

    r.tv_sec += r.tv_usec / 1000000;

    if (r.tv_usec < 0) {
	r.tv_usec = 1000000 - (-r.tv_usec % 1000000);
	r.tv_sec--;
    } else
	r.tv_usec = r.tv_usec % 1000000;

    return r;
}

static struct timeval
timer_timeval_diff(struct timeval tv1, struct timeval tv2) {

    tv1.tv_sec -= tv2.tv_sec;
    tv1.tv_usec -= tv2.tv_usec;

    return timer_timeval_normalize(tv1);
}
#endif /* HAVE_GETRUSAGE */

struct timer *
timer_new(const char *description) {
    struct timer *timer = memory_alloc(sizeof *timer);

    timer->description = description == NULL? NULL : memory_strdup(description);
    timer_reset(timer);
    timer->flags = 0;

    timer_all_timers = list_cons(timer, timer_all_timers);

    return timer;
}

void
timer_remove(struct timer *timer) {
    timer_all_timers = list_delete_elt(timer_all_timers, timer);

    if (timer->description != NULL)
	memory_free(timer->description);
    memory_free(timer);
}

struct timer_value
timer_get_value(struct timer *timer) {
    struct timer_value ret = {0 , 0};

#if defined(HAVE_CLOCK)
    ret.tv_sec = timer->clock_value / CLOCKS_PER_SEC;
    if (CLOCKS_PER_SEC >= 1000000) {
	ret.tv_usec =
	    (timer->clock_value / (CLOCKS_PER_SEC / 1000000)) % 1000000;
    } else {
	ret.tv_usec =
	    (timer->clock_value % CLOCKS_PER_SEC) * (1000000 / CLOCKS_PER_SEC);
    }
#elif defined(HAVE_GETRUSAGE)
    ret = timer->value;
#endif

    return ret;
}

int
timer_is_active(struct timer *timer) {
    return timer->flags & TIMER_ACTIVE;
}

void
timer_set_active(struct timer *timer, int isactive) {
    timer->flags &= ~TIMER_ACTIVE;
    timer->flags |= isactive? TIMER_ACTIVE : 0;
}

void
timer_reset(struct timer *timer) {
#if defined(HAVE_CLOCK)
    timer->clock_value = 0;
#elif defined(HAVE_GETRUSAGE)
    timer->value.tv_sec = 0;
    timer->value.tv_usec = 0;
#endif
}

void
timer_start(struct timer *timer) {
    if (!timer_is_active(timer) || timer->flags & TIMER_STARTED)
	return;

#if defined(HAVE_CLOCK)
    timer->last_start = clock();    
#elif defined(HAVE_GETRUSAGE)
    {
	struct rusage rusage;

	if (getrusage(RUSAGE_SELF, &rusage) == -1)
	    return;

	timer->last_start = rusage.ru_utime;
    }
#endif

    timer->flags |= TIMER_STARTED;
}

void
timer_stop(struct timer *timer) {
    if (!timer_is_active(timer) || !(timer->flags & TIMER_STARTED))
	return;

#if defined(HAVE_CLOCK)
    timer->clock_value += clock() - timer->last_start;
#elif defined(HAVE_GETRUSAGE)
    {
	struct rusage rusage;
	struct timeval tv;

	if(getrusage(RUSAGE_SELF, &rusage) == -1)
	    return;

	tv = timer_timeval_diff(rusage.ru_utime, timer->last_start);
	tv.tv_sec += timer->value.tv_sec;
	tv.tv_usec += timer->value.tv_usec;
	tv = timer_timeval_normalize(tv);
	timer->value.tv_sec = tv.tv_sec;
	timer->value.tv_usec = tv.tv_usec;
    }
#endif
    timer->flags &= ~TIMER_STARTED;
}

char *
timer_value_to_string(struct timer *timer) {
    struct timer_value val;
    int hour;
    int min;
    int sec;
    int usec;

    if (!timer_is_active(timer))
	return memory_strdup("inactive");

    val = timer_get_value(timer);

    hour = val.tv_sec / 3600;
    min = (val.tv_sec / 60) % 60;
    sec = val.tv_sec % 60;

    usec = val.tv_usec;

    return string_printf_to_string("%dh%d'%d\"%06dusec", hour, min, sec, usec);
}

const char *
timer_get_description(struct timer *timer) {
    return timer->description;
}

struct iterator *
timer_iterator(void) {
    return list_iterator(timer_all_timers);
}
