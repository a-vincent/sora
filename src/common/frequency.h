#ifndef COMMON_FREQUENCY_H_
#define COMMON_FREQUENCY_H_

typedef unsigned long long t_frequency;

#define PRIuFREQUENCY "llu"

int frequency_parse(const char *, t_frequency *);
char *frequency_human_print(t_frequency);

#endif /* COMMON_FREQUENCY_H_ */
