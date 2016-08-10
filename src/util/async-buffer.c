
#include <util/async-buffer.h>

#include <string.h>

#include <pthread.h>

#include <util/memory.h>

struct async_buffer {
    pthread_cond_t cond;
    pthread_mutex_t mtx;
    void *buffer;
    size_t buffer_read_index;
    size_t buffer_write_index;
    size_t buffer_size;
    size_t buffer_room_left;
    int flags;
};

struct async_buffer *
async_buffer_new(size_t s, int flags) {
    struct async_buffer *b = memory_alloc(sizeof *b);

    b->buffer = memory_alloc(s);
    b->buffer_read_index = 0;
    b->buffer_write_index = 0;
    b->buffer_size = s;
    b->buffer_room_left = s;

    b->flags = flags;

    pthread_cond_init(&b->cond, NULL);
    pthread_mutex_init(&b->mtx, NULL);

    return b;
}

void
async_buffer_delete(struct async_buffer *b) {
    pthread_cond_destroy(&b->cond);
    pthread_mutex_destroy(&b->mtx);
    memory_free(b->buffer);
    memory_free(b);
}

ssize_t
async_buffer_read(struct async_buffer *b, void *buf, size_t s) {
    size_t avail;
    size_t first_chunk_size;

    if (s > b->buffer_size)
	return -1;

    pthread_mutex_lock(&b->mtx);

    while (avail = b->buffer_size - b->buffer_room_left, avail < s) {
	if (b->flags & ASYNC_BUFFER_READER_CAN_WAIT)
	    pthread_cond_wait(&b->cond, &b->mtx);
	else {
	    pthread_mutex_unlock(&b->mtx);
	    return -1;
	}
    }

    first_chunk_size = b->buffer_size - b->buffer_read_index >= s?
	s : b->buffer_size - b->buffer_read_index;

    memcpy(buf, b->buffer + b->buffer_read_index, first_chunk_size);
    memcpy(buf + first_chunk_size, b->buffer, s - first_chunk_size);

    b->buffer_room_left += s;
    b->buffer_read_index = (b->buffer_read_index + s) % b->buffer_size;
    pthread_mutex_unlock(&b->mtx);

    if (s != 0)
	pthread_cond_signal(&b->cond);

    return s;
}

ssize_t
async_buffer_write(struct async_buffer *b, void *buf, size_t s) {
    size_t avail;
    size_t first_chunk_size;

    if (s > b->buffer_size)
	return -1;

    pthread_mutex_lock(&b->mtx);

    while (b->buffer_room_left < s) {
	if (b->flags & ASYNC_BUFFER_WRITER_CAN_WAIT)
	    pthread_cond_wait(&b->cond, &b->mtx);
	else {
	    pthread_mutex_unlock(&b->mtx);
	    return -1;
	}
    }

    first_chunk_size = b->buffer_size - b->buffer_write_index >= s?
	s : b->buffer_size - b->buffer_write_index;

    memcpy(b->buffer + b->buffer_write_index, buf, first_chunk_size);
    memcpy(b->buffer, buf + first_chunk_size, s - first_chunk_size);

    b->buffer_room_left -= s;
    b->buffer_write_index = (b->buffer_write_index + s) % b->buffer_size;
    pthread_mutex_unlock(&b->mtx);

    if (s != 0)
	pthread_cond_signal(&b->cond);

    return s;
}

void
async_buffer_empty(struct async_buffer *b) {
    pthread_mutex_lock(&b->mtx);
    b->buffer_read_index = 0;
    b->buffer_write_index = 0;
    b->buffer_room_left = b->buffer_size;
    pthread_mutex_unlock(&b->mtx);
}
