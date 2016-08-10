
#include <radio/radio-audio.h>

#include <alsa/asoundlib.h>

#include <inttypes.h>
#include <stdio.h>

#include <util/memory.h>

static int radio_audio_set_frequency(struct radio *, t_frequency);
static int radio_audio_get_frequency(struct radio *, t_frequency *);
static int radio_audio_set_sample_rate(struct radio *, unsigned long);
static int radio_audio_get_sample_rate(struct radio *, unsigned long *);
static ssize_t radio_audio_read(struct radio *, struct sample *, size_t);
static off_t radio_audio_get_file_position(struct radio *);
static void radio_audio_close(struct radio *);

struct radio_audio {
    struct radio radio;
    snd_pcm_t *pcm;
    t_frequency freq;
    unsigned long rate;
    off_t stream_position;
};

static struct radio_methods radio_audio_methods = {
    .set_frequency = radio_audio_set_frequency,
    .get_frequency = radio_audio_get_frequency,
    .set_sample_rate = radio_audio_set_sample_rate,
    .get_sample_rate = radio_audio_get_sample_rate,
    .read = radio_audio_read,
    .get_file_position = radio_audio_get_file_position,
    .close = radio_audio_close,
};

struct radio *
radio_audio_open(const char *name) {
    struct radio_audio *fr = memory_alloc(sizeof *fr);
    int status;

    status = snd_pcm_open(&fr->pcm, name, SND_PCM_STREAM_CAPTURE, 0);
    if (status != 0) {
	memory_free(fr);
	return NULL;
    }

    fr->freq = 0;
    fr->rate = 1;
    fr->stream_position = 0;

    radio_init(&fr->radio, &radio_audio_methods);

    return &fr->radio;
}

static int
radio_audio_set_frequency(struct radio *r, t_frequency f) {
    struct radio_audio *fr = (struct radio_audio *) r;

    fr->freq = f;

    return 0;
}

static int
radio_audio_get_frequency(struct radio *r, t_frequency *fp) {
    struct radio_audio *fr = (struct radio_audio *) r;

    *fp = fr->freq;

    return 0;
}

static int
radio_audio_set_sample_rate(struct radio *r, unsigned long s) {
    struct radio_audio *fr = (struct radio_audio *) r;
    int status;

    status = snd_pcm_set_params(fr->pcm, SND_PCM_FORMAT_S16,
	SND_PCM_ACCESS_RW_INTERLEAVED, 1, s, 1, 0);
    if (status != 0)
	return -1;

    fr->rate = s;

    return 0;
}

static int
radio_audio_get_sample_rate(struct radio *r, unsigned long *sp) {
    struct radio_audio *fr = (struct radio_audio *) r;

    *sp = fr->rate;

    return 0;
}

static ssize_t
radio_audio_read(struct radio *r, struct sample *buf, size_t len) {
    struct radio_audio *fr = (struct radio_audio *) r;
    int bytes_per_sample = 2;
    int nread;
    int i;

    nread = snd_pcm_readi(fr->pcm, buf, len);

    if (nread == -EPIPE)
	snd_pcm_prepare(fr->pcm);
    else if (nread == -EAGAIN)
	;
    else if (nread < 0)
	return -1;

    if (nread < 1)
	return 0;

    for (i = nread - 1; i >= 0; i--) {
	int16_t *buf_s16 = (void *) buf;

	buf[i].v = buf_s16[i] / 32768.0;
    }

    fr->stream_position += nread * bytes_per_sample;

    return nread;
}

static off_t
radio_audio_get_file_position(struct radio *r) {
    struct radio_audio *fr = (struct radio_audio *) r;

    return fr->stream_position;
}

static void
radio_audio_close(struct radio *r) {
    struct radio_audio *fr = (struct radio_audio *) r;

    snd_pcm_close(fr->pcm);

    memory_free(fr);
}
