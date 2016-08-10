#include <ads-b/plane-iq.h>

#include <math.h>

#include <util/bsd-queue.h>
#include <util/memory.h>

#define ADSB_BUFFER_SIZE 4096
#define ADSB_MAX_MESSAGE_SIZE 112

struct in_flight_msg {
    LIST_ENTRY(in_flight_msg) link;
    unsigned long long position;
    struct timeval time_stamp;
    char msg[ADSB_MAX_MESSAGE_SIZE + 1];
    int msg_i;
    double amplitude_56;
    double amplitude;
    int parity;
};

struct plane_iq_state {
    FILE *f;
    unsigned long long position;
    float signal[ADSB_BUFFER_SIZE];
#if ADSB_BUFFER_SIZE < 10
#error "ADSB_BUFFER_SIZE must be at least 10 (length of preamble)"
#endif
    int signal_i;
    size_t signal_len;
    LIST_HEAD(, in_flight_msg) ifm_inuse;
    LIST_HEAD(, in_flight_msg) ifm_free;
};


static struct in_flight_msg *
plane_iq_ifm_new(struct plane_iq_state *state) {
    struct in_flight_msg *m = LIST_FIRST(&state->ifm_free);

    if (m != NULL) {
	LIST_REMOVE(m, link);
    } else {
	m = memory_alloc(sizeof *m);
    }

    (void) gettimeofday(&m->time_stamp, NULL);
    m->position = state->position;
    m->parity = 0;
    m->msg[ADSB_MAX_MESSAGE_SIZE] = '\0';
    m->msg_i = 0;
    m->amplitude = 0;
    m->amplitude_56 = 0;
    LIST_INSERT_HEAD(&state->ifm_inuse, m, link);

    return m;
}

static struct in_flight_msg *
plane_iq_ifms_feed_bit(struct plane_iq_state *state, float s0, float s1) {
    struct in_flight_msg *m;
    struct in_flight_msg *ret = NULL;
    double amplitude = (s0 + s1) / 2;

    LIST_FOREACH(m, &state->ifm_inuse, link) {
	int bit = s1 <= s0;

	m->parity ^= 1;
	if (!m->parity)
	    continue;

	m->amplitude += amplitude;
	if (m->msg_i < 56)
	    m->amplitude_56 += amplitude;

	m->msg[m->msg_i++] = '0' + bit;
	if (m->msg_i == ADSB_MAX_MESSAGE_SIZE)
	    ret = m;
    }

    if (ret != NULL) {
	LIST_REMOVE(ret, link);
	LIST_INSERT_HEAD(&state->ifm_free, ret, link);
    }

    return ret;
}

struct plane_iq_state *
plane_iq_state_new(FILE *f) {
    struct plane_iq_state *state = memory_alloc(sizeof *state);

    state->f = f;
    state->position = 0;
    state->signal_len = 0;
    state->signal_i = 0;

    LIST_INIT(&state->ifm_inuse);
    LIST_INIT(&state->ifm_free);

    return state;
}

static void
plane_iq_state_delete_ifms(struct in_flight_msg *head) {
    struct in_flight_msg *next;

    for (; head != NULL; head = next) {
	next = LIST_NEXT(head, link);
	LIST_REMOVE(head, link);
	memory_free(head);
    }
}

void
plane_iq_state_delete(struct plane_iq_state *state) {
    plane_iq_state_delete_ifms(LIST_FIRST(&state->ifm_inuse));
    plane_iq_state_delete_ifms(LIST_FIRST(&state->ifm_free));
    memory_free(state);
}

static int
plane_iq_read_chunk(float *dest, unsigned int nsamples, FILE *f) {
    unsigned char buf[ADSB_BUFFER_SIZE * 2];
    size_t nread;
    size_t i;

    if (2 * nsamples > sizeof buf)
	nsamples = sizeof buf / 2;

    nread = fread(buf, 1, nsamples * 2, f);
    for (i = 0; i < nread / 2; i++) {
	float s_i = (buf[2 * i] - 128) / 128.f;
	float s_q = (buf[2 * i + 1] - 128) / 128.f;

	dest[i] = s_i*s_i + s_q*s_q;
    }

    return nread / 2;
}

const char *
plane_iq_get_next(struct plane_iq_state *state, struct timeval *tvp,
							double *amplitude) {
    for (;;) {
	struct in_flight_msg *m;
	int i;

	if (state->signal_len < 18) {
	    size_t nread;
	    if (state->signal_i + state->signal_len > ADSB_BUFFER_SIZE) {
		int start = (state->signal_i + state->signal_len) %
			ADSB_BUFFER_SIZE;
		nread = plane_iq_read_chunk(state->signal + start,
			state->signal_i - start, state->f);
	    } else {
		int start = state->signal_i + state->signal_len;
		nread = plane_iq_read_chunk(state->signal + start,
			ADSB_BUFFER_SIZE - start, state->f);
		nread += plane_iq_read_chunk(state->signal, state->signal_i,
			state->f);
	    }
	    state->signal_len += nread;
	}

	if (state->signal_len < 18)
	    return NULL;

	float tmp[16];

	for (i = 0; i < (int) (sizeof tmp / sizeof tmp[0]); i++)
	    tmp[i] = state->signal[(state->signal_i + i) % ADSB_BUFFER_SIZE];

	if (tmp[1] < tmp[0] && tmp[2] > tmp[1] && tmp[3] < tmp[2] &&
		tmp[4] < tmp[0] && tmp[5] < tmp[0] && tmp[6] < tmp[0] &&
		tmp[8] < tmp[7] && tmp[9] > tmp[8]) {
	    plane_iq_ifm_new(state);
	}

	m = plane_iq_ifms_feed_bit(state,
		state->signal[(state->signal_i + 16) % ADSB_BUFFER_SIZE],
		state->signal[(state->signal_i + 17) % ADSB_BUFFER_SIZE]);

	state->position++;
	state->signal_i = (state->signal_i + 1) % ADSB_BUFFER_SIZE;
	state->signal_len--;

	if (m != NULL) {
	    *tvp = m->time_stamp;
	    *amplitude = m->amplitude_56 / 56;
	    //printf("message at %llu: %s\n", m->position, m->msg);
	    return m->msg;
	}
    }
}
