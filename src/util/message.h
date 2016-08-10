#ifndef UTIL_MESSAGE_H_
#define UTIL_MESSAGE_H_

#include <stdarg.h>

#define MESSAGE_RAW_OUTPUT		0x0001
#define MESSAGE_WARNING			0x0002
#define MESSAGE_ERROR			0x0004
#define MESSAGE_SYSTEM_ERROR		0x0008

#define MESSAGE_USER_INPUT		0x0100
#define MESSAGE_PROMPT			0x0200

#define MESSAGE_DEBUG			0x1000

#define MESSAGE_ALL_USER_OUTPUT \
	(MESSAGE_RAW_OUTPUT | MESSAGE_WARNING | MESSAGE_ERROR | \
	MESSAGE_SYSTEM_ERROR | MESSAGE_DEBUG)

#define MESSAGE_ALL			0x1fff

#define MESSAGE_LISTEN_EXCLUSIVE	0x8000

typedef void (*t_message_listener)(int, const char *, void *);

void message_register_listener(int, t_message_listener, void *);
void message_remove_listener(t_message_listener, void *);

void message_listener_to_string(int, const char *, void *);

void message_printf(const char *, ...);
void message_print(int, const char *, ...);
void message_print_va(int, const char *, va_list);

#endif /* UTIL_MESSAGE_H_ */
