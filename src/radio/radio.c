
#include <radio/radio.h>

#include <stddef.h>

#include <util/exception.h>

static int radio_dummy_set_frequency(struct radio *, t_frequency);
static int radio_dummy_get_frequency(struct radio *, t_frequency *);
static int radio_dummy_set_sample_rate(struct radio *, unsigned long);
static int radio_dummy_get_sample_rate(struct radio *, unsigned long *);
static ssize_t radio_dummy_read(struct radio *, struct sample *, size_t);
static int radio_dummy_read_spectrum(struct radio *, struct spectrum *);
static off_t radio_dummy_get_file_position(struct radio *);
static void radio_dummy_close(struct radio *);

static void radio_methods_fill_empty_slots(struct radio_methods *);

void
radio_init(struct radio *r, struct radio_methods *m) {
    radio_methods_fill_empty_slots(m);
    r->m = m;
}

static void
radio_methods_fill_empty_slots(struct radio_methods *m) {
    if (sizeof *m != 7 * sizeof (void *))
	EXCEPTION_RAISE(runtime_error,
	    "Missing slot initialisation in radio/radio.c");

    if (m->set_frequency == NULL)
	m->set_frequency = radio_dummy_set_frequency;
    if (m->get_frequency == NULL)
	m->get_frequency = radio_dummy_get_frequency;
    if (m->set_sample_rate == NULL)
	m->set_sample_rate = radio_dummy_set_sample_rate;
    if (m->get_sample_rate == NULL)
	m->get_sample_rate = radio_dummy_get_sample_rate;
    if (m->read == NULL)
	m->read = radio_dummy_read;
    if (m->read_spectrum == NULL)
	m->read_spectrum = radio_dummy_read_spectrum;
    if (m->get_file_position == NULL)
	m->get_file_position = radio_dummy_get_file_position;
    if (m->close == NULL)
	m->close = radio_dummy_close;
}

static int
radio_dummy_set_frequency(struct radio *r, t_frequency f) {
    (void) r; (void) f;
    return -1;
}

static int
radio_dummy_get_frequency(struct radio *r, t_frequency *fp) {
    (void) r; (void) fp;
    return -1;
}

static int
radio_dummy_set_sample_rate(struct radio *r, unsigned long s) {
    (void) r; (void) s;
    return -1;
}

static int
radio_dummy_get_sample_rate(struct radio *r, unsigned long *sp) {
    (void) r; (void) sp;
    return -1;
}

static ssize_t
radio_dummy_read(struct radio *r, struct sample *buf, size_t len) {
    (void) r; (void) buf; (void) len;
    return -1;
}

static int
radio_dummy_read_spectrum(struct radio *r, struct spectrum *s) {
    (void) r; (void) s;
    return -1;
}

static off_t
radio_dummy_get_file_position(struct radio *r) {
    (void) r;

    return 0;
}

static void
radio_dummy_close(struct radio *r) {
    (void) r;
}
