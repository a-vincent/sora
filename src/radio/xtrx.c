
#include <radio/xtrx.h>

#include <util/async-buffer.h>
#include <util/memory.h>

#include <xtrx_api.h>

#include <pthread.h>
#include <stdio.h>

static int xtrx_radio_set_frequency(struct radio *, t_frequency);
static int xtrx_radio_get_frequency(struct radio *, t_frequency *);
static int xtrx_radio_set_sample_rate(struct radio *, unsigned long);
static int xtrx_radio_get_sample_rate(struct radio *, unsigned long *);
static ssize_t xtrx_radio_read(struct radio *, struct sample *, size_t);
static void xtrx_radio_close(struct radio *);

struct xtrx_radio {
    struct radio radio;
    struct xtrx_dev *dev;
#define RADIO_XTRX_FLAGS_STARTED	1
#define RADIO_XTRX_FLAGS_FREQUENCY_SET	2
#define RADIO_XTRX_FLAGS_SAMPLERATE_SET	4
    int flags;
    double last_set_frequency;
    double last_set_samplerate;
    pthread_t pthread;
    struct async_buffer *buf;
};

static struct radio_methods xtrx_radio_methods = {
    .set_frequency = xtrx_radio_set_frequency,
    .get_frequency = xtrx_radio_get_frequency,
    .set_sample_rate = xtrx_radio_set_sample_rate,
    .get_sample_rate = xtrx_radio_get_sample_rate,
    .read = xtrx_radio_read,
    .close = xtrx_radio_close,
};

static void 
xtrx_radio_log_function(int severity, const struct tm *stm,
			int nsec, const char subsys[4],
			const char *function, const char *file,
			int line_no, const char *fmt, va_list list) {
    vfprintf(stderr, fmt, list);
}

#define MAX_XTRX_DEVICES 16

struct radio *
xtrx_radio_open(void) {
    struct xtrx_radio *rs = memory_alloc(sizeof *rs);
    struct xtrx_device_info devs[MAX_XTRX_DEVICES];
    double gain;

    if (xtrx_discovery(devs, MAX_XTRX_DEVICES) <= 0) {
	fprintf(stderr, "can't discover xtrx devices\n");
	memory_free(rs);
	return NULL;
    }

    xtrx_log_setfunc(xtrx_radio_log_function);
    xtrx_log_setlevel(2, NULL);


    if (xtrx_open(devs[0].uniqname, 2 | XTRX_O_RESET, &rs->dev) != 0) {
	fprintf(stderr, "can't open xtrx\n");
	memory_free(rs);
	return NULL;
    }

    xtrx_set_antenna(rs->dev, XTRX_RX_AUTO);

    xtrx_set_gain(rs->dev, XTRX_CH_ALL, XTRX_RX_LNA_GAIN, 15, &gain);
    xtrx_set_gain(rs->dev, XTRX_CH_ALL, XTRX_RX_PGA_GAIN, 0, &gain);
    xtrx_set_gain(rs->dev, XTRX_CH_ALL, XTRX_RX_TIA_GAIN, 9, &gain);

    rs->last_set_frequency = 0.0;
    rs->last_set_samplerate = 0.0;
    rs->flags = 0;

    rs->buf = async_buffer_new(1024 * 1024, ASYNC_BUFFER_READER_CAN_WAIT);

    radio_init(&rs->radio, &xtrx_radio_methods);

    return &rs->radio;
}

static int
xtrx_radio_set_frequency(struct radio *r, t_frequency f) {
    struct xtrx_radio *rs = (struct xtrx_radio *) r;
    int ret = xtrx_tune(rs->dev, XTRX_TUNE_RX_FDD, f, &rs->last_set_frequency);
    double gain;
    double actual_bw;

    xtrx_set_antenna(rs->dev, XTRX_RX_AUTO);

    if (xtrx_tune_rx_bandwidth(rs->dev, XTRX_CH_ALL, rs->last_set_samplerate * 0.9, &actual_bw) != 0)
	return -1;

    xtrx_set_gain(rs->dev, XTRX_CH_ALL, XTRX_RX_LNA_GAIN, 15, &gain);
    xtrx_set_gain(rs->dev, XTRX_CH_ALL, XTRX_RX_PGA_GAIN, 0, &gain);
    xtrx_set_gain(rs->dev, XTRX_CH_ALL, XTRX_RX_TIA_GAIN, 9, &gain);

    return ret == 0? 0 : -1;
}

static int
xtrx_radio_get_frequency(struct radio *r, t_frequency *fp) {
    struct xtrx_radio *rs = (struct xtrx_radio *) r;
    t_frequency freq = rs->last_set_frequency;

    if (freq == 0)
	return -1;

    *fp = freq;

    return 0;
}

static int
xtrx_radio_set_sample_rate(struct radio *r, unsigned long s) {
    struct xtrx_radio *rs = (struct xtrx_radio *) r;
    double actual_cgen;
    double actual_tx;

    if (xtrx_set_samplerate(rs->dev, 0., s, 0., 0,
		&actual_cgen, &rs->last_set_samplerate, &actual_tx) != 0)
	return -1;

    return 0;
}

static int
xtrx_radio_get_sample_rate(struct radio *r, unsigned long *sp) {
    struct xtrx_radio *rs = (struct xtrx_radio *) r;
    unsigned long sample_rate = rs->last_set_samplerate;

    if (sample_rate == 0)
	return -1;

    *sp = sample_rate;
    return 0;
}

#define CHUNK_NSAMPLES 4096

static void *
xtrx_radio_read_thread(void *r) {
    struct xtrx_radio *rs = r;
    int16_t tmpbuf[CHUNK_NSAMPLES * 2];
    void *buffers_array[1] = { tmpbuf };
    struct xtrx_recv_ex_info ri;

    for (;;) {
	ri.samples = CHUNK_NSAMPLES;
	ri.buffers = buffers_array;
	ri.buffer_count = 1;
	ri.flags = RCVEX_DONT_INSER_ZEROS | RCVEX_DROP_OLD_ON_OVERFLOW;

	if (xtrx_recv_sync_ex(rs->dev, &ri) != 0)
	    pthread_exit(NULL);

	if (ri.out_events & RCVEX_EVENT_OVERFLOW) {
	    fprintf(stderr, "%lu %lu %lu\n", ri.out_first_sample,
			    ri.out_overrun_at, ri.out_resumed_at);
	}

	async_buffer_write(rs->buf, tmpbuf,
		ri.out_samples * sizeof tmpbuf[0] * 2);
    }

    return NULL;
}

static ssize_t
xtrx_radio_read(struct radio *r, struct sample *buf, size_t len) {
    struct xtrx_radio *rs = (struct xtrx_radio *) r;
    static struct xtrx_run_params params;
    int16_t tmpbuf[CHUNK_NSAMPLES * 2];
    ssize_t nread;
    int i;

    if (len > sizeof tmpbuf / 2)
	len = sizeof tmpbuf / 2;

    if ((rs->flags & RADIO_XTRX_FLAGS_STARTED) == 0) {
	xtrx_run_params_init(&params);
	params.dir = XTRX_RX;
	params.nflags = 0;
	params.rx.wfmt = XTRX_WF_16;
	params.rx.hfmt = XTRX_IQ_INT16;
	params.rx.chs = XTRX_CH_ALL;
	params.rx.paketsize = 0;
	params.rx.flags = XTRX_RSP_SISO_MODE;
#if 0
	params.rx.flags |= XTRX_RSP_SCALE;
	params.rx.scale = 1.0;
#endif

	params.rx_stream_start = 0;

	if (xtrx_run_ex(rs->dev, &params) != 0) {
	    fprintf(stderr, "can't start radio rx\n");
	    return -1;
	}

	pthread_create(&rs->pthread, NULL, xtrx_radio_read_thread, rs);

	rs->flags |= RADIO_XTRX_FLAGS_STARTED;
    }

    nread = async_buffer_read(rs->buf, tmpbuf, len * 2 * sizeof tmpbuf[0]);

    for (i = 0; i < nread / (2 * sizeof tmpbuf[0]); i++) {
	buf[i].v = tmpbuf[2 * i] / 32768. + tmpbuf[2 * i + 1] / 32768. * I;
    }

    return nread / (2 * sizeof tmpbuf[0]);
}

static void
xtrx_radio_close(struct radio *r) {
    struct xtrx_radio *rs = (struct xtrx_radio *) r;

    if ((rs->flags & RADIO_XTRX_FLAGS_STARTED) != 0) {
	xtrx_stop(rs->dev, XTRX_RX);
	xtrx_stop(rs->dev, XTRX_RX);
    }

    xtrx_close(rs->dev);
    memory_free(rs);
}
