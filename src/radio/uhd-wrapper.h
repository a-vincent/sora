#ifndef RADIO_UHD_WRAPPER_H_
#define RADIO_UHD_WRAPPER_H_

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

struct uhd_wrapper;

struct uhd_wrapper *uhd_wrapper_open(const char *, const char *, const char *);
void uhd_wrapper_close(struct uhd_wrapper *);
int uhd_wrapper_set_frequency(struct uhd_wrapper *, double);
double uhd_wrapper_get_frequency(struct uhd_wrapper *);
int uhd_wrapper_set_sample_rate(struct uhd_wrapper *, double);
double uhd_wrapper_get_sample_rate(struct uhd_wrapper *);
/* The buffer is made of double complex */
size_t uhd_wrapper_read(struct uhd_wrapper *, void *, size_t);

#ifdef __cplusplus
}
#endif

#endif /* RADIO_UHD_WRAPPER_H_ */
