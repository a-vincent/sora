
#include <common/frequency.h>

#include <util/string.h>

#include <ctype.h>

int
frequency_parse(const char s[], t_frequency *freq) {
    t_frequency val = 0;
    int order = 0;
    int seen_dot = 0;
    int i;

    for (i = 0; isdigit((unsigned) s[i]) || s[i] == '.' || s[i] == ','; i++) {
	if (isdigit((unsigned) s[i])) {
	    val = val * 10 + (s[i] - '0');
	    order -= seen_dot;
	} else {
	    if (seen_dot)
		return -1;
	    seen_dot = 1;
	}
    }

    switch (s[i]) {
    case 'k': case 'K':
	order += 3;
	i++;
	break;
    case 'm': case 'M':
	order += 6;
	i++;
	break;
    case 'g': case 'G':
	order += 9;
	i++;
	break;
    }

    if (order < 0)
	return 0;

    if ((tolower((unsigned) s[i]) == 'h' && tolower((unsigned) s[i+1]) == 'z' &&
	    s[i+2] == '\0') || s[i] == '\0') {
	for (i = 0; i < order; i++)
	    val *= 10;
	*freq = val;
	return 1;
    }

    return 0;
}

/*
 * Print the given frequency in a human readable way, not losing any precision.
 * Uses three fractional digits unless more are needed.
 */
char *
frequency_human_print(t_frequency freq) {
    char *result;
    const char *suffix;
    int integral_part;
    unsigned long mod_part;
    int frac;

    if (freq < 1000) {
	suffix = "";
	integral_part = freq;
	mod_part = 0;
	frac = 0;
    } else if (freq < 1000000) {
	suffix = "K";
	integral_part = freq / 1000;
	mod_part = freq % 1000;
	frac = 3;
    } else if (freq < 1000000000) {
	suffix = "M";
	integral_part = freq / 1000000;
	mod_part = freq % 1000000;
	frac = 6;
    } else {
	suffix = "G";
	integral_part = freq / 1000000000;
	mod_part = freq % 1000000000;
	frac = 9;
    }

    for (int i = 0; i < frac - 3; i++) {
	mod_part /= 10;
    }

    result = (frac == 0)?
	string_printf_to_string("%dHz", integral_part) :
	string_printf_to_string("%d.%03d%sHz",
		integral_part, mod_part, suffix);
    return result;
}
