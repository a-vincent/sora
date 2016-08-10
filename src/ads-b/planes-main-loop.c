
#include <ads-b/planes-main-loop.h>

#include <ads-b/plane.h>
#include <ads-b/plane-iq.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_MESSAGE_SIZE 112
#define MAX_LINE_SIZE (MAX_MESSAGE_SIZE + 100)

static int
planes_read_ascii_line(FILE *f, char *line_buf, struct timeval *tvp,
							double *amplitudep) {
    const char *at;
    char *p;
    size_t len;

    do {
	if (fgets(line_buf, MAX_LINE_SIZE, f) == NULL)
	    return 0;
    } while (line_buf[0] == '\0' || line_buf[0] == '\n' ||
	strstr(line_buf, "UHD_") != NULL);

    at = strchr(line_buf, '@');
    if (at == NULL)
	return 0;

    len = at - line_buf - 1;
    len = len < MAX_MESSAGE_SIZE? len : MAX_MESSAGE_SIZE;
    line_buf[len] = '\0';
    tvp->tv_sec = strtoll(at + 1, &p, 10);
    tvp->tv_usec = 0;
    if (*p == ',') {
	p = strchr(p + 1, ',');
	if (p != NULL) {
	    p = strchr(p + 1, ',');
	    if (p != NULL)
		*amplitudep = strtod(p + 1, NULL);
	}
    }

    return 1;
}

void
planes_main_loop(FILE *f, int flags) {
    char line_buf[MAX_LINE_SIZE];
    const char *line;
    struct plane_set *pset = plane_set_new();
    int last_day_displayed = -1;
    struct timeval tv;
    double amplitude = 0;
    struct plane_iq_state *iq_state = NULL;

    if (flags & PLANES_INPUT_FROM_RAW)
	iq_state = plane_iq_state_new(f);

    for (;;) {
	struct plane *plane;

	if (flags & PLANES_INPUT_FROM_RAW) {
	    line = plane_iq_get_next(iq_state, &tv, &amplitude);
	    if (line == NULL)
		break;
	} else {
	    if (!planes_read_ascii_line(f, line_buf, &tv, &amplitude))
		break;
	    line = line_buf;
	}

	plane = plane_set_parse_message(pset, line, tv, amplitude);
	if (plane == NULL)
	    continue;

	if (flags & PLANES_OUTPUT_AS_BITSTRING) {
	    printf("%s @ %lld,%lld,%llu,%g\n", line, (long long) tv.tv_sec,
		(long long) tv.tv_usec, 0ll, amplitude);
	}

	if (flags & PLANES_OUTPUT_AS_DECODED) {
	    struct tm tm;
	    localtime_r(&tv.tv_sec, &tm);
	    if (last_day_displayed != tm.tm_yday) {
		printf("--- %d-%02d-%02d ---\n",
		    tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
		last_day_displayed = tm.tm_yday;
	    }
	    plane_print(plane);
	    printf("\n");
	}
    }

    if (flags & PLANES_INPUT_FROM_RAW)
	plane_iq_state_delete(iq_state);
}
