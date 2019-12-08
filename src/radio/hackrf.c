
#include <radio/hackrf.h>

#include <util/async-buffer.h>
#include <util/memory.h>

#include <libhackrf/hackrf.h>

#include <stdio.h>

#include <pthread.h>

#define DEFAULT_BUFFER_SIZE (256 * 1024)

static int hackrf_radio_set_frequency(struct radio *, t_frequency);
static int hackrf_radio_get_frequency(struct radio *, t_frequency *);
static int hackrf_radio_set_sample_rate(struct radio *, unsigned long);
static int hackrf_radio_get_sample_rate(struct radio *, unsigned long *);
static ssize_t hackrf_radio_read(struct radio *, struct sample *, size_t);
static void hackrf_radio_close(struct radio *);
static void hackrf_radio_flush(struct radio *);

struct hackrf_radio {
    struct radio radio;
    hackrf_device *dev;
    int reading;
    struct async_buffer *buffer;
    t_frequency frequency;
    unsigned long sample_rate;
    int flags;
#define HACKRF_RADIO_FREQUENCY_IS_SET	0x01
#define HACKRF_RADIO_SAMPLE_RATE_IS_SET	0x02
};

static struct radio_methods hackrf_radio_methods = {
    .set_frequency = hackrf_radio_set_frequency,
    .get_frequency = hackrf_radio_get_frequency,
    .set_sample_rate = hackrf_radio_set_sample_rate,
    .get_sample_rate = hackrf_radio_get_sample_rate,
    .read = hackrf_radio_read,
    .close = hackrf_radio_close,
};

struct radio *
hackrf_radio_open(void) {
    static int hackrf_inited = 0;
    struct hackrf_radio *hrf = memory_alloc(sizeof *hrf);

    if (!hackrf_inited) {
	hackrf_init();
	hackrf_inited = 1;
    }

    if (hackrf_open(&hrf->dev) != HACKRF_SUCCESS) {
	fprintf(stderr, "can't open hackrf\n");
	memory_free(hrf);
	return NULL;
    }

    hackrf_set_amp_enable(hrf->dev, 1);
    hackrf_set_lna_gain(hrf->dev, 32);
    hackrf_set_vga_gain(hrf->dev, 10);

    radio_init(&hrf->radio, &hackrf_radio_methods);

    hrf->reading = 0;
    hrf->buffer =
	async_buffer_new(DEFAULT_BUFFER_SIZE, ASYNC_BUFFER_READER_CAN_WAIT);
    hrf->flags = 0;

    return &hrf->radio;
}

static int
hackrf_radio_set_frequency(struct radio *r, t_frequency f) {
    struct hackrf_radio *hrf = (struct hackrf_radio *) r;

    if (hackrf_set_freq(hrf->dev, f) != HACKRF_SUCCESS)
	return -1;

    hrf->frequency = f;
    hrf->flags |= HACKRF_RADIO_FREQUENCY_IS_SET;

    hackrf_radio_flush(r);

    return 0;
}

static int
hackrf_radio_get_frequency(struct radio *r, t_frequency *fp) {
    struct hackrf_radio *hrf = (struct hackrf_radio *) r;

    if (hrf->flags & HACKRF_RADIO_FREQUENCY_IS_SET) {
	*fp = hrf->frequency;
	return 0;
    }

    return -1;
}

static int
hackrf_radio_set_sample_rate(struct radio *r, unsigned long s) {
    struct hackrf_radio *hrf = (struct hackrf_radio *) r;
    unsigned long filter_bw;

    if (hackrf_set_sample_rate(hrf->dev, s) != HACKRF_SUCCESS)
	return -1;

    filter_bw = hackrf_compute_baseband_filter_bw(s / 2);
    if (hackrf_set_baseband_filter_bandwidth(hrf->dev, filter_bw) !=
		HACKRF_SUCCESS) {
	fprintf(stderr,
	    "hackrf_radio_set_sample_rate(): couldn't set filter bw to %lu\n",
	    filter_bw);
    }
    hrf->sample_rate = s;
    hrf->flags |= HACKRF_RADIO_SAMPLE_RATE_IS_SET;

    hackrf_radio_flush(r);

    return 0;
}

static int
hackrf_radio_get_sample_rate(struct radio *r, unsigned long *sp) {
    struct hackrf_radio *hrf = (struct hackrf_radio *) r;

    if (hrf->flags & HACKRF_RADIO_SAMPLE_RATE_IS_SET) {
	*sp = hrf->sample_rate;
	return 0;
    }

    return -1;
}

#define DEFAULT_CONVERSION_CHUNK_SIZE 4096

static int
hackrf_radio_read_callback(hackrf_transfer *transfer) {
    struct hackrf_radio *hrf = transfer->rx_ctx;
    int8_t *orig_buffer = transfer->buffer;
    size_t valid_length = transfer->valid_length;
    struct sample buffer[DEFAULT_CONVERSION_CHUNK_SIZE];
    size_t idx = 0;

    if (valid_length % 2)
	fprintf(stderr, "odd number of bytes in buffer!\n");

    while (valid_length >= 1) {
	size_t i;

	for (i = 0; i < valid_length / 2 && i < DEFAULT_CONVERSION_CHUNK_SIZE;
		i++, idx++) {
	    buffer[i].v =
		(orig_buffer[2*idx] + I * orig_buffer[2*idx+1]) / 128.0;
	}

	if (async_buffer_write(hrf->buffer, buffer, i * sizeof buffer[0]) == -1) {
	    //fprintf(stderr, "O");
	}

	valid_length -= 2 * i;
    }

    return HACKRF_SUCCESS;
}

static ssize_t
hackrf_radio_read(struct radio *r, struct sample *buf, size_t len) {
    struct hackrf_radio *hrf = (struct hackrf_radio *) r;

    if (!hrf->reading) {
	int err = hackrf_start_rx(hrf->dev, hackrf_radio_read_callback, hrf);

	if (err != HACKRF_SUCCESS) {
	    fprintf(stderr, "start_rx failed because '%s'\n", hackrf_error_name(err));
	    return -1;
	}

	hrf->reading = 1;
    }

    return async_buffer_read(hrf->buffer, buf, len * sizeof buf[0]) /
	sizeof buf[0];
}

static void
hackrf_radio_close(struct radio *r) {
    struct hackrf_radio *hrf = (struct hackrf_radio *) r;

    if (hrf->reading)
	hackrf_stop_rx(hrf->dev);

    hackrf_close(hrf->dev);
    async_buffer_delete(hrf->buffer);
    memory_free(hrf);
}

static void
hackrf_radio_flush(struct radio *r) {
    struct hackrf_radio *hrf = (struct hackrf_radio *) r;

    if (hrf->reading)
	async_buffer_empty(hrf->buffer);
}
