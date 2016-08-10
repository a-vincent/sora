
#include <util/debug.h>

#include <util/message.h>

#include <stdarg.h>
#include <stdio.h>

int debug_mask;

void
debug_printf(int mask, const char *s, ...) {
    va_list ap;

    if (!(debug_mask & mask))
	return;

    va_start(ap, s);
    message_print_va(MESSAGE_DEBUG, s, ap);
    va_end(ap);

    fflush(stderr);
}
