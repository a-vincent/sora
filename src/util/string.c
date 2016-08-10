
#include <util/string.h>

#include <util/memory.h>

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h> 

char *
string_printf_to_string(char *fmt, ...) {
    va_list args, args2;
    size_t size = strlen(fmt + 1);
    char *buffer = memory_alloc(size);
    int ret;

    va_start(args, fmt);
    for (;;) {
	va_copy(args2, args);
	ret = vsnprintf(buffer, size, fmt, args2);
	va_end(args2);
	if ((size_t) ret + 1 < size) {
	    buffer = memory_realloc(buffer, ret + 1);
	    break;
	}

	size *= 2;
	buffer = memory_realloc(buffer, size);
    }
    va_end(args);

    return buffer;
}

void
string_to_upper_case(char *str) {

    int i = 0;

    while (str[i] != '\0') {
	str[i] = toupper((unsigned char) str[i]);
	i++;
    }
}
