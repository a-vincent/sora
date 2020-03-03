#ifndef SIGNAL_SPECTRUM_H_
#define SIGNAL_SPECTRUM_H_

#include <stdio.h>

struct spectrum;

struct spectrum *spectrum_new(double min_freq, double max_freq, int nbins);
struct spectrum *spectrum_load(FILE *);
int spectrum_save(struct spectrum *, FILE *);

int spectrum_get_nbins(struct spectrum *);
double *spectrum_get_bins(struct spectrum *);
double spectrum_get_min_freq(struct spectrum *);
double spectrum_get_max_freq(struct spectrum *);

#endif /* SIGNAL_SPECTRUM_H_ */
