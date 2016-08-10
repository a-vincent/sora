#ifndef UTIL_DEBUG_H_
#define UTIL_DEBUG_H_

extern int debug_mask;

#ifdef DEBUG

#define DEBUG_TUNER		0x0001
#define DEBUG_POOL		0x0100
#define DEBUG_HASH		0x0101

void debug_printf(int, const char *, ...);

#define DPRINTF(args) debug_printf args

#else /* DEBUG */

#define DPRINTF(args)

#endif /* DEBUG */

#endif /* UTIL_DEBUG_H_ */
