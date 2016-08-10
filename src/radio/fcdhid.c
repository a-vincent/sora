
#include <radio/fcdhid.h>

#include <common/options.h>

#include <stdio.h>
#include <stdlib.h>

#include <fcntl.h>
#include <unistd.h>

#if 0
#include <alsa/asoundlib.h>
#endif

#define FCDHID_BUFFER_LEN 64		/* Hardware dictates this value */

struct fcdhid {
    struct radio radio;
    int fd;
    int nb_writes;
};

static int fcdhid_set_frequency(struct radio *, t_frequency);
static int fcdhid_get_frequency(struct radio *, t_frequency *);
static void fcdhid_close(struct radio *);

static struct radio_methods fcdhid_methods = {
    .set_frequency = fcdhid_set_frequency,
    .get_frequency = fcdhid_get_frequency,
    .close = fcdhid_close,
};

static long fcdhid_read_long_le(void *vp, int len);
static void fcdhid_write_long_le(void *vp, long v, int len);

struct radio *
fcdhid_open(const char *device_path, const char *audio_path) {
    struct fcdhid *fcdhid = malloc(sizeof *fcdhid);
    if (fcdhid == NULL)
	return NULL;
    radio_init(&fcdhid->radio, &fcdhid_methods);

    fcdhid->fd = open(device_path, O_RDWR);
    if (fcdhid->fd == -1) {
	free(fcdhid);
	return NULL;
    }

    fcdhid->nb_writes = 0;

    return &fcdhid->radio;
}

static void
fcdhid_close(struct radio *r) {
    struct fcdhid *fcdhid = (struct fcdhid *) r;
    if (fcdhid->nb_writes % 2) {		/* Workaround bug in NetBSD */
	t_frequency dummy;
	(void) fcdhid_get_frequency(r, &dummy);
    }
    close(fcdhid->fd);
    free(fcdhid);
}

static int
fcdhid_do_command(struct fcdhid *fcdhid, char *buffer) {
    ssize_t len;

    len = write(fcdhid->fd, buffer, FCDHID_BUFFER_LEN);
    if (len == -1) {
	if (option_verbose)
	    perror("write()");
	return -1;
    }

    fcdhid->nb_writes++;

    len = read(fcdhid->fd, buffer, FCDHID_BUFFER_LEN);
    if (len == -1) {
	if (option_verbose)
	    perror("read()");
	return -1;
    }
    if (buffer[1] != 1) {
	if (option_verbose)
	    fprintf(stderr, "Command %d failed\n", (int) buffer[0]);
	return -1;
    }

    return 0;
}

static int
fcdhid_set_frequency(struct radio *r, t_frequency freq) {
    struct fcdhid *fcdhid = (struct fcdhid *) r;
    char buffer[FCDHID_BUFFER_LEN];

    buffer[0] = 101;
    fcdhid_write_long_le(buffer + 1, freq, 4);

    if (fcdhid_do_command(fcdhid, buffer) == -1)
	return -1;

    return 0;
}

static int
fcdhid_get_frequency(struct radio *r, t_frequency *freq) {
    struct fcdhid *fcdhid = (struct fcdhid *) r;
    char buffer[FCDHID_BUFFER_LEN];

    buffer[0] = 102;

    if (fcdhid_do_command(fcdhid, buffer) == -1)
	return -1;

    *freq = (t_frequency) fcdhid_read_long_le(buffer + 2, 4);

    return 0;
}

static long
fcdhid_read_long_le(void *vp, int len) {
    unsigned char *p = vp;
    long v = 0;

    for (int i = len - 1; i >= 0; i--)
	v = (v << 8) | p[i];

    return v;
}

static void
fcdhid_write_long_le(void *vp, long v, int len) {
    char *p = vp;
    for (int i = 0; i < len; i++) {
	p[i] = v & 0xff;
	v >>= 8;
    }
}
