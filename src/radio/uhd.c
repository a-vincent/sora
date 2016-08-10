
#include <radio/uhd.h>

#include <radio/uhd-wrapper.h>
#include <util/async-buffer.h>
#include <util/memory.h>

#include <stdio.h>

#define DEFAULT_BUFFER_SIZE (256 * 1024)

static int uhd_radio_set_frequency(struct radio *, t_frequency);
static int uhd_radio_get_frequency(struct radio *, t_frequency *);
static int uhd_radio_set_sample_rate(struct radio *, unsigned long);
static int uhd_radio_get_sample_rate(struct radio *, unsigned long *);
static ssize_t uhd_radio_read(struct radio *, struct sample *, size_t);
static void uhd_radio_close(struct radio *);

struct uhd_radio {
    struct radio radio;
    struct uhd_wrapper *dev;
};

static struct radio_methods uhd_radio_methods = {
    .set_frequency = uhd_radio_set_frequency,
    .get_frequency = uhd_radio_get_frequency,
    .set_sample_rate = uhd_radio_set_sample_rate,
    .get_sample_rate = uhd_radio_get_sample_rate,
    .read = uhd_radio_read,
    .close = uhd_radio_close,
};

struct radio *
uhd_radio_open(const char *addr, const char *spec, const char *ant) {
    struct uhd_radio *r = memory_alloc(sizeof *r);

    r->dev = uhd_wrapper_open(addr, spec, ant);
    if (r->dev == NULL) {
	fprintf(stderr, "can't open uhd\n");
	memory_free(r);
	return NULL;
    }

    radio_init(&r->radio, &uhd_radio_methods);

    return &r->radio;
}

static int
uhd_radio_set_frequency(struct radio *r, t_frequency f) {
    struct uhd_radio *ur = (struct uhd_radio *) r;

    return uhd_wrapper_set_frequency(ur->dev, f);
}

static int
uhd_radio_get_frequency(struct radio *r, t_frequency *fp) {
    struct uhd_radio *ur = (struct uhd_radio *) r;

    *fp = (t_frequency) uhd_wrapper_get_frequency(ur->dev);

    return 0;
}

static int
uhd_radio_set_sample_rate(struct radio *r, unsigned long s) {
    struct uhd_radio *ur = (struct uhd_radio *) r;
    unsigned long filter_bw;

    if (uhd_wrapper_set_sample_rate(ur->dev, s) != 0)
	return -1;

    return 0;
}

static int
uhd_radio_get_sample_rate(struct radio *r, unsigned long *sp) {
    struct uhd_radio *ur = (struct uhd_radio *) r;

    *sp = (unsigned long) uhd_wrapper_get_sample_rate(ur->dev);

    return 0;
}

static ssize_t
uhd_radio_read(struct radio *r, struct sample *buf, size_t len) {
    struct uhd_radio *ur = (struct uhd_radio *) r;

    return uhd_wrapper_read(ur->dev, (void *) buf, len);
}

static void
uhd_radio_close(struct radio *r) {
    struct uhd_radio *ur = (struct uhd_radio *) r;

    uhd_wrapper_close(ur->dev);
    memory_free(ur);
}
