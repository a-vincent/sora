#ifndef ADSB_PLANE_IQ_H_
#define ADSB_PLANE_IQ_H_

#include <sys/time.h>

#include <stdio.h>

struct plane_iq_state;

struct plane_iq_state *plane_iq_state_new(FILE *);
void plane_iq_state_delete(struct plane_iq_state *);
const char *plane_iq_get_next(struct plane_iq_state *, struct timeval *,
	double *);

#endif /* ADSB_PLANE_IQ_H_ */
