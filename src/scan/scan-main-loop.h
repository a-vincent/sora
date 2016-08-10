#ifndef SCAN_SCAN_MAIN_LOOP_H_
#define SCAN_SCAN_MAIN_LOOP_H_

struct radio;

void scan_main_loop(struct radio *, double /* threshold in dB */);

#endif /* SCAN_SCAN_MAIN_LOOP_H_ */
