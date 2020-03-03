
/* For endian.h functions/macros */
#define _DEFAULT_SOURCE

#include <assert.h>
#include <endian.h>
#include <inttypes.h>
#include <string.h>

#include <signal/spectrum.h>
#include <util/memory.h>

struct spectrum {
    double min_freq;
    double max_freq;
    int nbins;
    double *bins;
};

struct spectrum *
spectrum_new(double min_freq, double max_freq, int nbins) {
    struct spectrum *spct = memory_alloc(sizeof *spct);

    spct->bins = memory_alloc(nbins * sizeof *spct->bins);
    spct->min_freq = min_freq;
    spct->max_freq = max_freq;
    spct->nbins = nbins;

    return spct;
}

struct spectrum *
spectrum_load(FILE *f) {
    struct spectrum temp;
    struct spectrum *spct;
    uint32_t version;
    union {
	uint64_t tmp_uint64;
	double tmp_double;
    } u;
    uint32_t tmp_uint32;
    int i;
    char buf[4];

    assert(sizeof u.tmp_uint64 == sizeof u.tmp_double);

    if (fread(buf, 4, 1, f) != 1)
	goto malformed;

    if (strncmp(buf, "SPCT", 4) != 0)
	goto malformed;

    if (fread(&version, sizeof version, 1, f) != 1)
	goto malformed;

    version = be32toh(version);
    if (version != 1)
	goto malformed;

    if (fread(&u.tmp_uint64, sizeof u.tmp_uint64, 1, f) != 1)
	goto malformed;

    u.tmp_uint64 = be64toh(u.tmp_uint64);
    temp.min_freq = u.tmp_double;

    if (fread(&u.tmp_uint64, sizeof u.tmp_uint64, 1, f) != 1)
	goto malformed;

    u.tmp_uint64 = be64toh(u.tmp_uint64);
    temp.max_freq = u.tmp_double;

    if (fread(&tmp_uint32, sizeof tmp_uint32, 1, f) != 1)
	goto malformed;

    tmp_uint32 = be32toh(tmp_uint32);
    temp.nbins = tmp_uint32;

    spct = spectrum_new(temp.min_freq, temp.max_freq, temp.nbins);

    for (i = 0; i < temp.nbins; i++) {
	if (fread(&u.tmp_uint64, sizeof u.tmp_uint64, 1, f) != 1)
	    goto malformed_free;

	u.tmp_uint64 = be64toh(u.tmp_uint64);
	spct->bins[i] = u.tmp_double;
    }

    return spct;

malformed_free:
    memory_free(spct->bins);
    memory_free(spct);
malformed:
    return NULL;
}

int
spectrum_save(struct spectrum *spct, FILE *f) {
    union {
	uint64_t tmp_uint64;
	double tmp_double;
    } u;
    uint32_t tmp_uint32;
    int i;
    char buf[4] = "SPCT";

    assert(sizeof u.tmp_uint64 == sizeof u.tmp_double);

    /* Offset 0: "SPCT" */
    if (fwrite(buf, 4, 1, f) != 1)
	goto write_error;

    /* Offset 4: version 1 as uint32 */
    tmp_uint32 = htobe32(1);
    if (fwrite(&tmp_uint32, sizeof tmp_uint32, 1, f) != 1)
	goto write_error;

    /* Offset 8: min freq as double */
    u.tmp_double = spct->min_freq;
    u.tmp_uint64 = htobe64(u.tmp_uint64);
    if (fwrite(&u.tmp_uint64, sizeof u.tmp_uint64, 1, f) != 1)
	goto write_error;

    /* Offset 16: max freq as double */
    u.tmp_double = spct->max_freq;
    u.tmp_uint64 = htobe64(u.tmp_uint64);
    if (fwrite(&u.tmp_uint64, sizeof u.tmp_uint64, 1, f) != 1)
	goto write_error;

    /* Offset 24: number of bins as uint32 */
    tmp_uint32 = spct->nbins;
    tmp_uint32 = htobe32(tmp_uint32);
    if (fwrite(&tmp_uint32, sizeof tmp_uint32, 1, f) != 1)
	goto write_error;

    /* Offset 28: bin values as doubles */
    for (i = 0; i < spct->nbins; i++) {
	u.tmp_double = spct->bins[i];
	u.tmp_uint64 = htobe64(u.tmp_uint64);
	if (fwrite(&u.tmp_uint64, sizeof u.tmp_uint64, 1, f) != 1)
	    goto write_error;
    }

    return 0;

write_error:
    return -1;
}

int
spectrum_get_nbins(struct spectrum *spct) {
    return spct->nbins;
}

double *
spectrum_get_bins(struct spectrum *spct) {
    return spct->bins;
}

double
spectrum_get_min_freq(struct spectrum *spct) {
    return spct->min_freq;
}

double
spectrum_get_max_freq(struct spectrum *spct) {
    return spct->max_freq;
}
