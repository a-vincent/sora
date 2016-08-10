#ifndef UTIL_ARRAY_H_
#define UTIL_ARRAY_H_

typedef struct array *t_array;

t_array array_new(unsigned int);
void *array_get_data(t_array);
unsigned int array_get_length(t_array);
void array_resize(t_array, unsigned int);
void array_delete(t_array);

#endif /* UTIL_ARRAY_H_ */
