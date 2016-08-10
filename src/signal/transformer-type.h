#ifndef SIGNAL_TRANSFORMER_TYPE_H_
#define SIGNAL_TRANSFORMER_TYPE_H_

#include <stddef.h>

/* sizes are in bytes */

struct transformer_type {
    const char *name;
    int input_elem_size;
    int output_elem_size;

    int apply(struct transformer_type *,
	void *dest, const void *src, size_t src_size, void *aux);
};

#endif /* SIGNAL_TRANSFORMER_TYPE_H_ */
