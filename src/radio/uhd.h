#ifndef RADIO_UHD_H_
#define RADIO_UHD_H_

#include <radio/radio.h>

struct radio *uhd_radio_open(const char *, const char *, const char *);

#endif /* RADIO_UHD_H_ */
