#ifndef ADS_B_PLANES_MAIN_LOOP_H_
#define ADS_B_PLANES_MAIN_LOOP_H_

#include <stdio.h>

void planes_main_loop(FILE *, int flags);
#define PLANES_INPUT_FROM_RAW		0x01
#define PLANES_OUTPUT_AS_DECODED	0x10
#define PLANES_OUTPUT_AS_BITSTRING	0x20

#endif /* ADS_B_PLANES_MAIN_LOOP_H_ */
