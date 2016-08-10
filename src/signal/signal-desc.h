#ifndef SIGNAL_DESC_H_
#define SIGNAL_DESC_H_

struct signal_desc {
    int flags;
#define SIGNAL_DESC_HAVE_SAMPLE_RATE		0x0001
#define SIGNAL_DESC_HAVE_TUNER_FREQUENCY	0x0002
    unsigned int sample_rate;
    unsigned long tuner_frequency;
    char *file_basename;
};
#endif /* SIGNAL_DESC_H_ */
