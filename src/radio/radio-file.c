
#include <radio/radio-file.h>

#include <inttypes.h>
#include <stdio.h>

#include <util/memory.h>

static int radio_file_set_frequency(struct radio *, t_frequency);
static int radio_file_get_frequency(struct radio *, t_frequency *);
static int radio_file_set_sample_rate(struct radio *, unsigned long);
static int radio_file_get_sample_rate(struct radio *, unsigned long *);
static ssize_t radio_file_read(struct radio *, struct sample *, size_t);
static off_t radio_file_get_file_position(struct radio *);
static void radio_file_close(struct radio *);

struct radio_file {
    struct radio radio;
    FILE *file;
    t_frequency freq;
    unsigned long rate;
    int encoding;
};

static struct radio_methods radio_file_methods = {
    .set_frequency = radio_file_set_frequency,
    .get_frequency = radio_file_get_frequency,
    .set_sample_rate = radio_file_set_sample_rate,
    .get_sample_rate = radio_file_get_sample_rate,
    .read = radio_file_read,
    .get_file_position = radio_file_get_file_position,
    .close = radio_file_close,
};

struct radio *
radio_file_open(const char *name, int encoding) {
    struct radio_file *fr = memory_alloc(sizeof *fr);

    if (strcmp(name, "-") == 0)
	fr->file = stdin;
    else {
	fr->file = fopen(name, "rb");
	if (fr->file == NULL) {
	    perror(name);
	    memory_free(fr);
	    return NULL;
	}
    }

    fr->freq = 0;
    fr->rate = 1;
    fr->encoding = encoding;

    radio_init(&fr->radio, &radio_file_methods);

    return &fr->radio;
}

static int
radio_file_set_frequency(struct radio *r, t_frequency f) {
    struct radio_file *fr = (struct radio_file *) r;

    fr->freq = f;

    return 0;
}

static int
radio_file_get_frequency(struct radio *r, t_frequency *fp) {
    struct radio_file *fr = (struct radio_file *) r;

    *fp = fr->freq;

    return 0;
}

static int
radio_file_set_sample_rate(struct radio *r, unsigned long s) {
    struct radio_file *fr = (struct radio_file *) r;

    fr->rate = s;

    return 0;
}

static int
radio_file_get_sample_rate(struct radio *r, unsigned long *sp) {
    struct radio_file *fr = (struct radio_file *) r;

    *sp = fr->rate;

    return 0;
}

static ssize_t
radio_file_read(struct radio *r, struct sample *buf, size_t len) {
    struct radio_file *fr = (struct radio_file *) r;
    int bytes_per_sample;
    int nread;
    int i;

    switch (fr->encoding) {
    case RADIO_FILE_ENCODING_UC8:
    case RADIO_FILE_ENCODING_SC8:
	bytes_per_sample = 2;
	break;
    case RADIO_FILE_ENCODING_SC16:
	bytes_per_sample = 4;
	break;
    case RADIO_FILE_ENCODING_U8:
	bytes_per_sample = 1;
	break;
    case RADIO_FILE_ENCODING_S16:
	bytes_per_sample = 2;
	break;
    default:
	fprintf(stderr, "unsupported encoding\n");
	return 0;
    }

    nread = fread(buf, bytes_per_sample, len, fr->file);
    if (nread < bytes_per_sample)
	return 0;

    for (i = nread - 1; i >= 0; i--) {
	uint8_t *buf_u8 = (void *) buf;
	int8_t *buf_s8 = (void *) buf;
	int16_t *buf_s16 = (void *) buf;

	switch (fr->encoding) {
	case RADIO_FILE_ENCODING_UC8:
	    buf[i].v =
		(buf_u8[2*i] - 128.0 + I*(buf_u8[2*i+1] - 128.0)) / 128.0;
	    break;
	case RADIO_FILE_ENCODING_SC8:
	    buf[i].v = (buf_s8[2*i] + I*buf_s8[2*i+1]) / 128.0;
	    break;
	case RADIO_FILE_ENCODING_SC16:
	    buf[i].v = (buf_s16[2*i] + I*buf_s16[2*i+1]) / 32768.0;
	    break;
	case RADIO_FILE_ENCODING_U8:
	    buf[i].v = (buf_u8[i] - 128.0) / 128.0;
	    break;
	case RADIO_FILE_ENCODING_S16:
	    buf[i].v = buf_s16[2] / 32768.0;
	    break;
	}
    }

    return nread;
}

static off_t
radio_file_get_file_position(struct radio *r) {
    struct radio_file *fr = (struct radio_file *) r;

    return ftello(fr->file);
}

static void
radio_file_close(struct radio *r) {
    struct radio_file *fr = (struct radio_file *) r;

    fclose(fr->file);
    memory_free(fr);
}
