
#include <util/simple-math.h>

int
sm_max(int a, int b) {
    return a < b? b : a;
}

int
sm_abs(int a) {
    return a < 0? -a : a;
}

int
sm_log_base_2(int n) {
    unsigned int un = (unsigned int) n;
    int l;

    for (l = -1; un > 0; l++)
	un >>= 1;

    return l;
}

int
sm_number_of_bits_for_int(int x) {
    return x == 0? 2 : 2 + sm_log_base_2(x);
}
