#ifndef RADIO_RADIO_H_
#define RADIO_RADIO_H_

#include <common/frequency.h>
#include <signal/sample.h>

#include <sys/types.h>

struct radio_methods;

struct radio {
    struct radio_methods *m;
};

void radio_init(struct radio *, struct radio_methods *);

struct radio_methods {
    int (*set_frequency)(struct radio *, t_frequency);
    int (*get_frequency)(struct radio *, t_frequency *);
    int (*set_sample_rate)(struct radio *, unsigned long);
    int (*get_sample_rate)(struct radio *, unsigned long *);
    ssize_t (*read)(struct radio *, struct sample *, size_t);
    off_t (*get_file_position)(struct radio *);
    void (*close)(struct radio *);
};

#endif /* RADIO_RADIO_H_ */
