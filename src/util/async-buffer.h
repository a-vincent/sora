/*
 * Thread-safe circular buffer
 */
#ifndef UTIL_ASYNC_BUFFER_H_
#define UTIL_ASYNC_BUFFER_H_

#include <sys/types.h>

struct async_buffer;

#define ASYNC_BUFFER_READER_CAN_WAIT	1
#define ASYNC_BUFFER_WRITER_CAN_WAIT	2

struct async_buffer *async_buffer_new(size_t, int);
void async_buffer_delete(struct async_buffer *);

ssize_t async_buffer_read(struct async_buffer *, void *, size_t);
ssize_t async_buffer_write(struct async_buffer *, void *, size_t);
void async_buffer_empty(struct async_buffer *);

#endif /* UTIL_ASYNC_BUFFER_H_ */
