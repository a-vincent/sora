
#include <util/message.h>

#include <util/bsd-queue.h>
#include <util/exception.h>
#include <util/memory.h>

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

struct message_listener {
    SLIST_ENTRY(message_listener) next;
    int flags;
    t_message_listener listener;
    void *aux;
};


SLIST_HEAD(, message_listener) message_listeners =
    SLIST_HEAD_INITIALIZER(message_listener);

void
message_register_listener(int flags, t_message_listener listener, void *aux) {
    struct message_listener *lis = memory_alloc(sizeof *lis);

    lis->flags = flags;
    lis->listener = listener;
    lis->aux = aux;
    SLIST_INSERT_HEAD(&message_listeners, lis, next);
}

void
message_remove_listener(t_message_listener listener, void *aux) {
    struct message_listener **lisp = &SLIST_FIRST(&message_listeners);
    struct message_listener *lis = *lisp;

    while (lis != NULL) {
	if (lis->listener == listener && lis->aux == aux)
	    break;

	lisp = &SLIST_NEXT(lis, next);
	lis = *lisp;
    }

    if (lis == NULL)
	EXCEPTION_RAISE(logic_error, "listener not registered");

    *lisp = SLIST_NEXT(*lisp, next);

    memory_free(lis);
}

void
message_print_va(int type, const char *fmt, va_list args) {
    va_list args2;
    struct message_listener *lis;
    static char *message;
    static size_t message_size;
    int ret;

    if (message == NULL) {
	message_size = 100;
	message = memory_alloc(message_size);
    }

    for (;;) {
#ifdef HAVE_VA_COPY
	va_copy(args2, args);
#else
	__va_copy(args2, args);
#endif
	ret = vsnprintf(message, message_size, fmt, args2);
	va_end(args2);
	if ((size_t) ret < message_size)
	    break;

	message_size *= 2;
	message = memory_realloc(message, message_size);
    }

    SLIST_FOREACH(lis, &message_listeners, next) {
	if (lis->flags & type) {
		lis->listener(type, message, lis->aux);
	    if (lis->flags & MESSAGE_LISTEN_EXCLUSIVE)
		break;
	}
    }
}

void
message_print(int type, const char *fmt, ...) {
    va_list args;

    va_start(args, fmt);
    message_print_va(type, fmt, args);
    va_end(args);
}

void
message_printf(const char *fmt, ...) {
    va_list args;

    va_start(args, fmt);
    message_print_va(MESSAGE_RAW_OUTPUT, fmt, args);
    va_end(args);
}

void
message_listener_to_string(int t, const char *s, void *d) {
    char **stringp = d;
    size_t len = (*stringp != NULL)? strlen(*stringp) : 0;
    (void) t;

    *stringp = memory_realloc(*stringp, len + strlen(s) + 1);
    strcpy((*stringp) + len, s);
}
