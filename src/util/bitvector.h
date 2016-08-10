#ifndef UTIL_BITVECTOR_H_
#define UTIL_BITVECTOR_H_

#include <util/array.h>
#include <util/exception.h>

EXCEPTION_DECLARE(bitvec_empty);

typedef t_array t_bitvector;

void bitvec_init(void);
t_bitvector bitvec_new(void);
void bitvec_delete(t_bitvector);
int bitvec_get(t_bitvector, unsigned int);
int bitvec_set(t_bitvector, unsigned int, int);
t_bitvector bitvec_copy(t_bitvector);

/* Write or read a given bitfield (Least Significant First) from/to an int */
void bitvec_write_int_lsf(t_bitvector, unsigned int, unsigned int, int);
int bitvec_read_int_lsf(t_bitvector, unsigned int, unsigned int);

void bitvec_print_hex_lsf(int, t_bitvector, unsigned int, int);

unsigned int bitvec_number_of_ones(t_bitvector);
unsigned int bitvec_index_of_last_one(t_bitvector);
void bitvec_add_one(t_bitvector);
int bitvec_are_equal(t_bitvector, t_bitvector);

#endif /* UTIL_BITVECTOR_H_ */
