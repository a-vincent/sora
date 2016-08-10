#ifndef SIGNAL_TRANSFORMER_H_
#define SIGNAL_TRANSFORMER_H_

#include <util/list.h>

#define TRANSFORMER_CHUNK_SIZE_UNLIMITED ((size_t) -1)

struct transfomer;
struct transformer_type;

struct transformer *transformer_new(struct transformer_type *, void *);
void transformer_delete(struct transformer *);
struct transformer_type *transformer_get_type(struct transformer *);
t_list transformer_get_producers(struct transformer *);
t_list transformer_get_consumers(struct transformer *);

void *transformer_get_private(struct transformer *);

size_t transformer_get_input_chunk_max_size(struct transformer *);
size_t transformer_get_output_chunk_max_size(struct transformer *);
void transformer_set_input_chunk_max_size(struct transformer *, size_t);
void transformer_set_output_chunk_max_size(struct transformer *, size_t);

#endif /* SIGNAL_TRANSFORMER_H_ */
