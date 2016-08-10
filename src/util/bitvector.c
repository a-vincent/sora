
#include <util/bitvector.h>

#include <util/message.h>

#include <stddef.h>

EXCEPTION_DEFINE(bitvec_empty, runtime_error);

#define BITS_PER_UNSIGNED_CHAR 8U

static int bitvec_bits_per_uchar[1 << BITS_PER_UNSIGNED_CHAR];

static void
bitvec_zero_array(t_array array, int start, int end) {
    char *data = array_get_data(array);
    int i;

    for (i = start; i < end; i++)
	data[i] = 0;
}

void
bitvec_init(void) {
    int i;

    bitvec_bits_per_uchar[0] = 0;

    for (i = 1; i < (1 << BITS_PER_UNSIGNED_CHAR); i++)
	bitvec_bits_per_uchar[i] = bitvec_bits_per_uchar[i / 2] + (i & 1);
}

t_bitvector
bitvec_new(void) {
    t_array array = array_new(4);

    bitvec_zero_array(array, 0, 4);

    return (t_bitvector) array;
}

void
bitvec_delete(t_bitvector bv) {
    array_delete((t_array) bv);
}

int
bitvec_get(t_bitvector bv, unsigned int index) {
    t_array v = (t_array) bv;
    unsigned char *data = array_get_data(v);

    if (index / BITS_PER_UNSIGNED_CHAR >= array_get_length(v))
	return 0;

    return !!(data[index / BITS_PER_UNSIGNED_CHAR] &
	(1 << index % BITS_PER_UNSIGNED_CHAR));
}

int
bitvec_set(t_bitvector bv, unsigned int index, int value) {
    t_array v = (t_array) bv;
    const unsigned char bit_mask = 1 << index % BITS_PER_UNSIGNED_CHAR;
    unsigned char *data;
    int old;

    old = array_get_length(v);
    if (index >= BITS_PER_UNSIGNED_CHAR * old) {
	array_resize(v, index / BITS_PER_UNSIGNED_CHAR + 1);
	bitvec_zero_array(v, old, index / BITS_PER_UNSIGNED_CHAR + 1);
    }

    data = array_get_data(v);

    old = !!(data[index / BITS_PER_UNSIGNED_CHAR] & bit_mask);

    if (value)
	data[index / BITS_PER_UNSIGNED_CHAR] |= bit_mask;
    else
	data[index / BITS_PER_UNSIGNED_CHAR] &= ~bit_mask;

    return old;
}

t_bitvector
bitvec_copy(t_bitvector bv) {

    unsigned int i = 0;

    unsigned int length = array_get_length((t_array) bv);
    t_array array = array_new(length);

    unsigned char *data = array_get_data((t_array)bv);
    unsigned char *ndata = array_get_data(array);

    while (i < length){
	ndata[i] = data[i];
	i++;
    }

    return (t_bitvector) array;
}

void
bitvec_write_int_lsf(t_bitvector bv, unsigned int start, unsigned int size,
	int value) {
    unsigned int i;

    for (i = 0; i < size; i++) {
	bitvec_set(bv, start + size - i - 1, value & 1);
	value >>= 1;
    }
}

int
bitvec_read_int_lsf(t_bitvector bv, unsigned int start, unsigned int size) {
    unsigned int i;
    int ret = 0;
    int neg = bitvec_get(bv, start + size - 1);

    for (i = 0; i < size; i++) {
	ret <<= 1;
	ret |= bitvec_get(bv, start + size - i - 1);
    }

    /* Sign extension */
    if (neg) {
	ret |= (-1) << size;
    }

    return ret;
}

void
bitvec_print_hex_lsf(int out, t_bitvector bv, unsigned int start, int size) {
    unsigned long buf;
    int i;
    int n = size % 32;

    if (n != 0) {
	buf = 0;
	for (i = 0; i < n; i++) {
	    buf <<= 1;
	    buf |= bitvec_get(bv, start + size - 1);
	    size--;
	}
	message_print(out, "%lx", buf);
    }

    for (i = 0; i < size; i += 32) {
	int j;
	buf = 0;
	for (j = 0; j < 32; j++) {
	    buf <<= 1;
	    buf |= bitvec_get(bv, start + size - 1);
	    size--;
	}
	message_print(out, "%08lx", buf);
    }
}

unsigned int
bitvec_number_of_ones(t_bitvector bv) {
    unsigned char *data = array_get_data((t_array) bv);
    int length;
    unsigned int noo = 0;

    for (length = array_get_length((t_array) bv); length > 0; length--)
	noo += bitvec_bits_per_uchar[*data++];

    return noo;
}

unsigned int
bitvec_index_of_last_one(t_bitvector bv) {
    unsigned int length = array_get_length((t_array) bv);
    unsigned char *data = array_get_data((t_array) bv);
    unsigned int i;

    while (length > 0) {
	length--;

	if (data[length] != 0) {
	    int mask = 0x80;
	    unsigned char dat = data[length];

	    for (i = (length + 1) * 8 - 1; !(dat & mask); i--)
		mask >>= 1;
	    return i;
	}
    }

    EXCEPTION_RAISE(bitvec_empty, NULL);
}

void
bitvec_add_one(t_bitvector bv) {
    int i;

    for (i = 0; bitvec_get(bv, i) == 1; i++)
	bitvec_set(bv, i, 0);

    bitvec_set(bv, i, 1);
}

int
bitvec_are_equal(t_bitvector bv1, t_bitvector bv2) {
    unsigned int l1 = array_get_length((t_array) bv1);
    unsigned char *d1 = array_get_data((t_array) bv1);
    unsigned int l2 = array_get_length((t_array) bv2);
    unsigned char *d2 = array_get_data((t_array) bv2);
    unsigned int i;

    if (l1 < l2) {
	unsigned int tl = l1;
	unsigned char *td = d1;
	l1 = l2;
	d1 = d2;
	l2 = tl;
	d2 = td;
    }

    for (i = 0; i < l2; i++)
	if (d1[i] != d2[i])
	    return 0;

    for ( ; i < l1; i++)
	if (d1[i] != 0)
	    return 0;

    return 1;
}
