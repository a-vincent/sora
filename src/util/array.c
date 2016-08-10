
#include <util/array.h>

#include <util/exception.h>
#include <util/memory.h>

struct array {
    unsigned int length;
    unsigned int alloc_size;
    void *data;
};

t_array
array_new(unsigned int length) {
    t_array array = NULL;

    EXCEPTION_CATCH(runtime_error, {
	array = memory_alloc(sizeof *array);
	array->data = NULL;
	array->data = memory_alloc(length);
    });
    if (EXCEPTION_IS_RAISED(runtime_error)) {
	if (array != NULL) {
	    if (array->data != NULL)
		memory_free(array->data);
	    memory_free(array);
	}
	EXCEPTION_RAISE_SAME();
    }

    array->length = length;
    array->alloc_size = length;
    return array;
}

void *
array_get_data(t_array array) {
    return array->data;
}

unsigned int
array_get_length(t_array array) {
    return array->length;
}

void
array_resize(t_array array, unsigned int length) {
    unsigned int new_size;
    void *new_data;

    if (length > array->alloc_size)
	new_size = 2 * length;
    else
	new_size = length;

    new_data = memory_realloc(array->data, new_size);

    array->alloc_size = new_size;
    array->length = length;
    array->data = new_data;
}

void
array_delete(t_array array) {
    memory_free(array->data);
    memory_free(array);
}
