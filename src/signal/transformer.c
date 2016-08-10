
#include <signal/transformer.h>
#include <util/exception.h>
#include <util/memory.h>

struct transformer {
    struct transformer_type *type;

    t_list producers;			/* struct transformer */
    t_list consumers;			/* struct transformer */

    void *private_data;

    size_t input_chunk_max_size;
    size_t output_chunk_max_size;
};

struct transformer *
transformer_new(struct transformer_type *type, void *private_data) {
    struct transformer *trans = memory_alloc(sizeof *trans);

    trans->type = type;
    trans->producers = NULL;
    trans->consumers = NULL;
    trans->private_data = private_data;

    trans->input_chunk_max_size = type->input_chunk_max_size;
    trans->output_chunk_max_size = type->output_chunk_max_size;

    return trans;
}

void
transformer_delete(struct transformer *trans) {

	/* XXX Unlink from other transformers */

    memory_free(trans);
}

struct transformer_type *
transformer_get_type(struct transformer *trans) {
    return trans->type;
}

t_list
transformer_get_producers(struct transformer *trans) {
    return trans->producers;
}

t_list
transformer_get_consumers(struct transformer *trans) {
    return trans->consumers;
}

void *
transformer_get_private(struct transformer *trans) {
    return trans->private_data;
}

size_t
transformer_get_input_chunk_max_size(struct transformer *trans) {
    return trans->input_chunk_max_size;
}

size_t
transformer_get_output_chunk_max_size(struct transformer *trans) {
    return trans->input_chunk_max_size;
}

void
transformer_set_input_chunk_max_size(struct transformer *trans, size_t s) {
    if (s <= trans->type->input_chunk_max_size) {
	trans->input_chunk_max_size = s;
    } else {
	EXCEPTION_RAISE(runtime_error,
	    "Trying to exceed transformer type's maximum input chunk size");
    }
}

void
transformer_set_output_chunk_max_size(struct transformer *trans, size_t s) {
    if (s <= trans->type->output_chunk_max_size) {
	trans->output_chunk_max_size = s;
    } else {
	EXCEPTION_RAISE(runtime_error,
	    "Trying to exceed transformer type's maximum output chunk size");
    }
}
