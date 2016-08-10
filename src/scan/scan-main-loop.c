#include <scan/scan-main-loop.h>

#include <complex.h>
#include <math.h>
#include <fftw3.h>
#include <time.h>

#include <radio/radio.h>
#include <util/memory.h>

#define DEFAULT_FFT_SIZE 1024

#define DEFAULT_BACKLOG_SIZE 10
#define DEFAULT_DECISION_SIZE 10	// this many "rounds" below noise

struct bin_info {
    double complex values[DEFAULT_BACKLOG_SIZE];
    double noise_level;
    int flags;
#define BIN_INSIDE_SIGNAL	0x1
    long long samples_above_noise;
    long long samples_below_noise;
};

static t_frequency
scan_bin_to_frequency(t_frequency tune, t_frequency rate, int nbins, int bin) {
    if (bin >= nbins / 2)
	bin = bin - nbins;

    return tune + (double) bin * rate / nbins;
}

void
scan_main_loop(struct radio *r, double threshold_db) {
    fftw_plan fft_plan = NULL;
    struct sample *in_buf = NULL;
    double complex *out_buf = NULL;
    struct bin_info *bins = memory_alloc(sizeof *bins * DEFAULT_FFT_SIZE);
    double threshold = pow(10, threshold_db / 20);
    t_frequency tune = 0;
    unsigned long rate = 1;
    int current_index = 0;
    int learning = DEFAULT_BACKLOG_SIZE;
    int i;

    r->m->get_frequency(r, &tune);
    r->m->get_sample_rate(r, &rate);

    in_buf = fftw_malloc(sizeof *in_buf * DEFAULT_FFT_SIZE);
    if (in_buf == NULL)
	goto err;
    out_buf = fftw_malloc(sizeof *out_buf * DEFAULT_FFT_SIZE);
    if (out_buf == NULL)
	goto err;

    fft_plan = fftw_plan_dft_1d(DEFAULT_FFT_SIZE,
	(double complex *) in_buf, out_buf, FFTW_FORWARD, FFTW_ESTIMATE);
    if (fft_plan == NULL)
	goto err;

    for (i = 0; i < DEFAULT_FFT_SIZE; i++) {
	struct bin_info *b = bins + i;

	b->noise_level = 0;
	b->flags = 0;
	b->samples_below_noise = 0;
	b->samples_above_noise = 0;
	for (size_t j = 0; j < sizeof b->values / sizeof b->values[0]; j++)
	    b->values[j] = 0;
    }

    for (;;) {
	size_t to_read = DEFAULT_FFT_SIZE;
	off_t file_offset = r->m->get_file_position(r);
	int next_index = (current_index + 1) % DEFAULT_BACKLOG_SIZE;
	ssize_t ret;

	while (to_read != 0) {
	    ret = r->m->read(r,
		(struct sample *) in_buf + DEFAULT_FFT_SIZE - to_read, to_read);
	    if (ret == -1)
		goto err;
	    to_read -= ret;
	}

	fftw_execute(fft_plan);

	for (i = 0; i < DEFAULT_FFT_SIZE; i++) {
	    struct bin_info *b = bins + i;
	    double level = cabs(out_buf[i]);
	    double noise_level = b->noise_level / DEFAULT_BACKLOG_SIZE;
	    if (i == 0)
		continue;

	    if (!learning) {
		if (level >= threshold * noise_level) {
		    b->samples_above_noise++;
		    b->samples_below_noise = 0;
		} else {
		    b->samples_above_noise = 0;
		    b->samples_below_noise++;
		}

		if (b->samples_above_noise > 0 &&
			!(b->flags & BIN_INSIDE_SIGNAL)) {
		    char timestamp_string[100];
		    time_t timestamp = time(NULL);
		    t_frequency freq =
			scan_bin_to_frequency(tune, rate, DEFAULT_FFT_SIZE, i);
		    char *hfreq = frequency_human_print(freq);
		    if (freq < 10e9) {
			strftime(timestamp_string, sizeof timestamp_string,
				"%F %T", localtime(&timestamp));
			printf("%-9s %s %llu %4.1f\n", hfreq, timestamp_string,
				(unsigned long long) file_offset,
				20 * log10(level / noise_level));
		    }
		    memory_free(hfreq);

		    b->flags |= BIN_INSIDE_SIGNAL;
		} else if (b->samples_below_noise > DEFAULT_DECISION_SIZE &&
			(b->flags & BIN_INSIDE_SIGNAL)) {
		    b->flags &= ~BIN_INSIDE_SIGNAL;
		    //printf("bin %d stop\n", i);
		}
	    }

#if 0
	    if (i == 0) {
		printf("current level = %g, noise_level = %g\n", level, noise_level);
	    }
#endif
	    b->noise_level =
		b->noise_level + level - cabs(b->values[current_index]);
	    b->values[current_index] = out_buf[i];
	}

	current_index = next_index;
	if (learning)
	    learning--;
    }

    if (0) {
err:
	if (fft_plan != NULL)
	    fftw_destroy_plan(fft_plan);
	if (out_buf != NULL)
	    fftw_free(out_buf);
	if (in_buf != NULL)
	    fftw_free(in_buf);
    }
}
