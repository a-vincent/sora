#ifndef RADIO_RADIO_FILE_H_
#define RADIO_RADIO_FILE_H_

#include <radio/radio.h>

#define RADIO_FILE_ENCODING_UC8		1
#define RADIO_FILE_ENCODING_SC16	2

#define RADIO_FILE_ENCODING_SC8		3

#define RADIO_FILE_ENCODING_U8		32
#define RADIO_FILE_ENCODING_S16		33

struct radio *radio_file_open(const char *, int);

#endif /* RADIO_RADIO_FILE_H_ */
