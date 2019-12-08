
#include <radio/rtlsdr.h>

#include <util/async-buffer.h>
#include <util/memory.h>

#include <rtl-sdr.h>

#include <stdio.h>

#include <pthread.h>

#define DEFAULT_BUFFER_SIZE (256 * 1024)

static int rtlsdr_radio_set_frequency(struct radio *, t_frequency);
static int rtlsdr_radio_get_frequency(struct radio *, t_frequency *);
static int rtlsdr_radio_set_sample_rate(struct radio *, unsigned long);
static int rtlsdr_radio_get_sample_rate(struct radio *, unsigned long *);
static ssize_t rtlsdr_radio_read(struct radio *, struct sample *, size_t);
static void rtlsdr_radio_close(struct radio *);

struct rtlsdr_radio {
    struct radio radio;
    rtlsdr_dev_t *dev;
};

static struct radio_methods rtlsdr_radio_methods = {
    .set_frequency = rtlsdr_radio_set_frequency,
    .get_frequency = rtlsdr_radio_get_frequency,
    .set_sample_rate = rtlsdr_radio_set_sample_rate,
    .get_sample_rate = rtlsdr_radio_get_sample_rate,
    .read = rtlsdr_radio_read,
    .close = rtlsdr_radio_close,
};

struct radio *
rtlsdr_radio_open(int index) {
    struct rtlsdr_radio *rs = memory_alloc(sizeof *rs);

    if (rtlsdr_open(&rs->dev, index) != 0) {
	fprintf(stderr, "can't open rtlsdr\n");
	memory_free(rs);
	return NULL;
    }

#if 0
    if (rtlsdr_set_tuner_gain_mode(rs->dev, 0) != 0)
	fprintf(stderr, "rtlsdr_set_tuner_gain_mode() failed\n");
    if (rtlsdr_set_agc_mode(rs->dev, 1) != 0)
	fprintf(stderr, "rtlsdr_set_agc_mode() failed\n");
#else
    if (rtlsdr_set_agc_mode(rs->dev, 0) != 0)
	fprintf(stderr, "rtlsdr_set_agc_mode() failed\n");
    if (rtlsdr_set_tuner_gain_mode(rs->dev, 1) != 0)
	fprintf(stderr, "rtlsdr_set_tuner_gain_mode() failed\n");
    if (rtlsdr_set_tuner_gain(rs->dev, 490) != 0)
	fprintf(stderr, "rtlsdr_set_tuner_gain() failed\n");
#endif

    if (rtlsdr_reset_buffer(rs->dev) != 0)
	fprintf(stderr, "rtlsdr_reset_buffer() failed\n");

    radio_init(&rs->radio, &rtlsdr_radio_methods);

    //printf("tuner gain = %g\n", rtlsdr_get_tuner_gain(rs->dev) / 10.0);

    return &rs->radio;
}

static int
rtlsdr_radio_set_frequency(struct radio *r, t_frequency f) {
    struct rtlsdr_radio *rs = (struct rtlsdr_radio *) r;
    int ret = rtlsdr_set_center_freq(rs->dev, f) == 0? 0 : -1;

    rtlsdr_reset_buffer(rs->dev);

    return ret;
}

static int
rtlsdr_radio_get_frequency(struct radio *r, t_frequency *fp) {
    struct rtlsdr_radio *rs = (struct rtlsdr_radio *) r;
    t_frequency freq = rtlsdr_get_center_freq(rs->dev);

    if (freq == 0)
	return -1;

    *fp = freq;

    return 0;
}

static int
rtlsdr_radio_set_sample_rate(struct radio *r, unsigned long s) {
    struct rtlsdr_radio *rs = (struct rtlsdr_radio *) r;

    if (rtlsdr_set_sample_rate(rs->dev, s) != 0)
	return -1;

    return 0;
}

static int
rtlsdr_radio_get_sample_rate(struct radio *r, unsigned long *sp) {
    struct rtlsdr_radio *rs = (struct rtlsdr_radio *) r;
    unsigned long sample_rate = rtlsdr_get_sample_rate(rs->dev);

    if (sample_rate == 0)
	return -1;

    *sp = sample_rate;
    return 0;
}

static ssize_t
rtlsdr_radio_read(struct radio *r, struct sample *buf, size_t len) {
    struct rtlsdr_radio *rs = (struct rtlsdr_radio *) r;
    int nread;
    uint8_t *byte_buf = (uint8_t *) buf;
    int ret = rtlsdr_read_sync(rs->dev, byte_buf, len * 2, &nread);
    int i;

    if (ret != 0)
	return -1;

    for (i = nread - 2; i >= 0; i -= 2)
	buf[i / 2].v =
	    (byte_buf[i] - 128.0 + I*(byte_buf[i+1] - 128.0)) / 128.0;

    return nread / 2;
}

static void
rtlsdr_radio_close(struct radio *r) {
    struct rtlsdr_radio *rs = (struct rtlsdr_radio *) r;

    rtlsdr_close(rs->dev);
    memory_free(rs);
}
