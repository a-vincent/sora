
#include <ads-b/plane.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <util/hash.h>
#include <util/memory.h>

#ifndef M_PI
#define M_PI 3.14159265358979
#endif /* M_PI */

struct plane {
    uint32_t address;			// 24-bit ICAO address

    int flags;
#define PLANE_SQUAWK_ID_AVAILABLE	0x01
#define PLANE_CATEGORY_AVAILABLE	0x02
#define PLANE_FLIGHT_ID_AVAILABLE	0x04
#define PLANE_GROUND_VELOCITY_AVAILABLE	0x08
#define PLANE_ALTITUDE_AVAILABLE	0x10
#define PLANE_GNSS_VS_BARO_AVAILABLE	0x20
#define PLANE_VERT_RATE_AVAILABLE	0x40
#define PLANE_COORDINATES_AVAILABLE	0x80

    int category_set;
    int category;

    int velocity_we;			// kt
    int velocity_sn;			// kt
    long altitude;			// ft
    int gnss_vs_baro;			// ft

    int vert_rate;			// ft/min

    unsigned int enc_latitude[2];
    unsigned int enc_longitude[2];
    struct timeval enc_lat_long_ts[2];
    double latitude;
    double longitude;
    struct timeval coordinates_ts;

    struct timeval last_update;

    char flight_id[9];
    char squawk_id[5];

    double last_msg_amplitude;
};

static struct plane *plane_new(uint32_t address);

struct plane_set {
    t_hash set;
};

struct plane_set *
plane_set_new(void) {
    struct plane_set *pset = memory_alloc(sizeof *pset);

    pset->set = hash_new(hash_hashfun_pointer, hash_eqfun_pointer);

    return pset;
}

static int
plane_set_delete_helper(void *id, void *plane) {
    (void) id;
    plane_delete(plane);

    return 0;
}

void
plane_set_delete(struct plane_set *pset) {
    hash_delete(pset->set, plane_set_delete_helper);
    memory_free(pset);
}

void
plane_set_free(struct plane_set *pset) {
    hash_delete(pset->set, NULL);
    memory_free(pset);
}

struct plane *
plane_set_lookup(struct plane_set *pset, uint32_t address) {
    return hash_get_non_null(pset->set, (void *) (intptr_t) address);
}

struct plane *
plane_set_lookup_create(struct plane_set *pset, uint32_t address) {
    struct plane *plane = plane_set_lookup(pset, address);

    if (plane == NULL) {
	plane = plane_new(address);
	hash_put(pset->set, (void *) (intptr_t) address, plane);
    }

    return plane;
}

struct iterator *
plane_set_iterate(struct plane_set *pset) {
    return hash_iterator(pset->set);
}

static struct plane *
plane_new(uint32_t address) {
    struct plane *plane = memory_alloc(sizeof *plane);

    memset(plane, 0, sizeof *plane);

    plane->address = address;

    return plane;
}

struct plane *
plane_clone(struct plane *plane) {
    struct plane *copy = plane_new(plane->address);

    *copy = *plane;

    return copy;
}

void
plane_delete(struct plane *plane) {
    memory_free(plane);
}

const char *
plane_get_category(struct plane *plane) {
    const char *r = NULL;

    if (!(plane->flags & PLANE_CATEGORY_AVAILABLE))
	return r;

    switch (plane->category_set) {
    case 0:					// A
	switch (plane->category) {
	case 0:
	    r = "Cat A";
	    break;
	case 1:
	    r = "Light";
	    break;
	case 2:
	    r = "Medium 1";
	    break;
	case 3:
	    r = "Medium 2";
	    break;
	case 4:
	    r = "High vortex aircraft";
	    break;
	case 5:
	    r = "Heavy";
	    break;
	case 6:
	    r = "High performance";
	    break;
	case 7:
	    r = "Rotorcraft";
	    break;
	}
	break;
    case 1:					// B
	switch (plane->category) {
	case 0:
	    r = "Cat B";
	    break;
	case 1:
	    r = "Glider/sailplane";
	    break;
	case 2:
	    r = "Lighter-than-air";
	    break;
	case 3:
	    r = "Parachutist/skydiver";
	    break;
	case 4:
	    r = "Ultralight/hang-glider/paraglider";
	    break;
	case 5:
	    r = "Cat B5";
	    break;
	case 6:
	    r = "Unmanned Aerial Vehicle";
	    break;
	case 7:
	    r = "Space/transatmospheric vehicle";
	    break;
	}
	break;
    case 2:					// C
	switch (plane->category) {
	case 0:
	    r = "Cat C";
	    break;
	case 1:
	    r = "Surface vehicle -- Emergency vehicle";
	    break;
	case 2:
	    r = "Surface vehicle -- Service vehicle";
	    break;
	case 3:
	    r = "Fixed ground or tethered obstruction";
	    break;
	case 4:
	    r = "Cat C4";
	    break;
	case 5:
	    r = "Cat C5";
	    break;
	case 6:
	    r = "Cat C6";
	    break;
	case 7:
	    r = "Cat C7";
	    break;
	}
	break;
    case 3:					// D
	r = "Cat D";
	break;
    }

    return r;
}

const char *
plane_get_squawk_id(struct plane *plane) {
    if (plane->flags & PLANE_SQUAWK_ID_AVAILABLE)
	return plane->squawk_id;

    return NULL;
}

const char *
plane_get_flight_id(struct plane *plane) {
    if (plane->flags & PLANE_FLIGHT_ID_AVAILABLE)
	return plane->flight_id;

    return NULL;
}

int
plane_is_ground_velocity_available(struct plane *plane) {
    return !!(plane->flags & PLANE_GROUND_VELOCITY_AVAILABLE);
}

int
plane_is_heading_available(struct plane *plane) {
    return !!(plane->flags & PLANE_GROUND_VELOCITY_AVAILABLE);
}

double
plane_get_ground_velocity(struct plane *plane) {
    (void) plane;
    return 0;		// XXX
}

double
plane_get_heading(struct plane *plane) {
    (void) plane;
    return 0;		// XXX
}

int
plane_is_altitude_available(struct plane *plane) {
    return !!(plane->flags & PLANE_ALTITUDE_AVAILABLE);
}

long
plane_get_altitude(struct plane *plane) {
    return plane->altitude;
}

int
plane_is_gnss_vs_baro_available(struct plane *plane) {
    return !!(plane->flags & PLANE_GNSS_VS_BARO_AVAILABLE);
}

int
plane_get_gnss_vs_baro(struct plane *plane) {
    return plane->gnss_vs_baro;
}

int
plane_is_vert_rate_available(struct plane *plane) {
    return !!(plane->flags & PLANE_VERT_RATE_AVAILABLE);
}

int
plane_get_vert_rate(struct plane *plane) {
    return plane->vert_rate;
}

int
plane_is_lat_long_available(struct plane *plane) {
    return (plane->enc_lat_long_ts[0].tv_sec != 0 &&
	plane->enc_lat_long_ts[0].tv_usec != 0 &&
    	plane->enc_lat_long_ts[1].tv_sec != 0 &&
	plane->enc_lat_long_ts[1].tv_usec != 0);
}

double
plane_get_latitude(struct plane *plane) {
    (void) plane;
    return 0;		// XXX
}

double
plane_get_longitude(struct plane *plane) {
    (void) plane;
    return 0;		// XXX
}

/////////////////////////////////////////////////////////////////

static int
gray_to_binary(uint16_t g) {
    g ^= (g >> 8);
    g ^= (g >> 4);
    g ^= (g >> 2);
    g ^= (g >> 1);

    return g;
}

static char
plane_decode_char(uint8_t val) {
    if (val >= 1 && val <= 26)
	return 'A' + val - 1;

    if (val == 32)
	return ' ';

    if (val >= 48 && val <= 57)
	return '0' + val - 48;		// identity

    return '?';
}

static uint64_t
read_bit_string(const char *s, int off, int sz) {
    uint64_t val = 0;

    for (int i = 0; i < sz; i++) {
	val <<= 1;
	val |= s[off + i] == '1';
    }

    return val;
}

#define BITS(x, off, n) (((x) >> (off)) & ((1 << (n)) - 1))
#define BIT(x, off) BITS((x), (off), 1)

#define ID_DIGIT(x, n) ((BIT((x),(n)) << 2) | \
			(BIT((x),(n) + 2) << 1) | \
			(BIT((x),(n) + 4)))
#define AC_DIGIT(x, n) (BIT((x),(n)) | \
			(BIT((x),(n) + 2) << 1) | \
			(BIT((x),(n) + 4) << 2))

static uint32_t
plane_adsb_compute_crc(const char *msg, int size) {
    uint32_t val = 0;

    for (int i = 0; i < size; i++) {
	val ^= read_bit_string(msg, i, 1) << 23;
	val <<= 1;
	if (val & (1 << 24))
	    val ^= 0x1fff409;
    }

    return val;
}

void
plane_decode_cpr(struct plane *plane) {
    int odd;
    long long diff;
    int j0;
    int rlat;
    int arlat;
    int nl;

    plane->flags &= ~PLANE_COORDINATES_AVAILABLE;
    diff = plane->enc_lat_long_ts[1].tv_sec - plane->enc_lat_long_ts[0].tv_sec;
    if (llabs(diff) > 10)
	return;

    odd = (diff > 0);

    j0 = floor((59 * (double) plane->enc_latitude[0] -
		60 * (double) plane->enc_latitude[1]) / (1 << 17) + 0.5);
    if (j0 < 0) {
	j0 += 60 - odd;
    }

    if (!((j0 >= 0 && j0 <= 15) || (j0 >= 45 && j0 <= 60)))
	return;

    if (j0 >= 45)
	j0 -= 60;

    plane->latitude = (j0 + (double) plane->enc_latitude[odd] / (1 << 17)) *
		360 / (60 - odd);

    rlat = (plane->latitude < 0? ceil : floor)(plane->latitude / 6) * 6 + 3;
//printf("latj0 = %d, rlat = %d, ", j0, rlat);
    arlat = abs(rlat);
    if (arlat <= 3)
	nl = 59;
    else if (arlat == 87)
	nl = 2;
    else if (arlat > 87)
	nl = 1;
    else {
	double tmp = cos(M_PI * arlat / 180);
	nl = floor(2 * M_PI / acos(1 - (1 - cos(M_PI / 30)) / (tmp * tmp)));
    }
//printf(", nl = %d ", nl);

    j0 = floor(((nl - 1) * (double) plane->enc_longitude[0] -
		nl * (double) plane->enc_longitude[1]) / (1 << 17) + 0.5);
//printf(", longj0 = %d ", j0);
    if (j0 < 0) {
	j0 += nl - odd;
    }

//printf(", longj0 = %d ", j0);

    plane->longitude = (j0 + (double) plane->enc_longitude[odd] / (1 << 17)) *
	360.0 / (nl - odd);
    if (plane->longitude >= 180)
	plane->longitude -= 360;
    plane->coordinates_ts = plane->enc_lat_long_ts[odd];
    
    plane->flags |= PLANE_COORDINATES_AVAILABLE;
//printf("\n");
}

const int ac_c_digit_conversion_table[2][8] = {
    { -1, 0, 2, 1, 4, -1, 3, -1 },
    { -1, 4, 2, 3, 0, -1, 1, -1 }
};

struct plane *
plane_set_parse_message(struct plane_set *pset, const char *msg,
					struct timeval tv, double amplitude) {
    struct plane *plane = NULL;
    int fmt = read_bit_string(msg, 0, 5);
    int long_message = (fmt >= 16 && fmt <= 24);
    uint32_t address;
    uint32_t ap = read_bit_string(msg, long_message? 88 : 32, 24);
    uint16_t ac = read_bit_string(msg, 19, 13);
    uint16_t id = ac;
    uint32_t crc;

    /*
     * Get address and CRC to lookup the plane involved and validate the message
     */
    crc = plane_adsb_compute_crc(msg, long_message? 88 : 32);
    if (fmt == 11 || fmt == 17 || fmt == 18) {
	address = read_bit_string(msg, 8, 24);
	crc ^= ap;
	if (crc != 0 || (fmt == 11 && crc >= 80))
	    return plane;
	plane = plane_set_lookup_create(pset, address);
    } else {
	address = crc ^ ap;
	plane = plane_set_lookup(pset, address);
	if (plane == NULL)
	    return plane;
    }

    plane->last_update = tv;
    plane->last_msg_amplitude = amplitude;

    if (fmt == 11 || fmt == 17) {
	if (fmt == 17) {
	    uint8_t type = read_bit_string(msg, 32, 5);
	    ap = read_bit_string(msg, 88, 24);
	    if (type >= 1 && type <= 4) {
		int i;

		for (i = 0; i < 8; i++)
		    plane->flight_id[i] =
			plane_decode_char(read_bit_string(msg, 40 + i * 6, 6));

		plane->flight_id[i] = '\0';
		plane->flags |= PLANE_FLIGHT_ID_AVAILABLE;
	    }
	    if (type >= 9 && type <= 18) {
		int odd;
		uint32_t alt = read_bit_string(msg, 40, 12);
		if (BIT(alt, 4)) {
		    int32_t altitude = (BITS(alt, 5, 7) << 4) |
					BITS(alt, 0, 4);
		    plane->altitude = altitude * 25 - 1000;
		    plane->flags |= PLANE_ALTITUDE_AVAILABLE;
		} else {
//		    printf(", Q = 0");
		}

		odd = read_bit_string(msg, 53, 1) == 1;
		plane->enc_latitude[odd] = read_bit_string(msg, 54, 17);
		plane->enc_longitude[odd] = read_bit_string(msg, 71, 17);
		plane->enc_lat_long_ts[odd] = tv;
		plane_decode_cpr(plane);

//		printf(", surv = %d", read_bit_string(msg, 37, 2));
	    } else if (type == 19) {
		uint8_t subtype = read_bit_string(msg, 37, 3);

		if (subtype == 1 || subtype == 2) {
		    double w_e_velo;
		    double s_n_velo;

		    w_e_velo = read_bit_string(msg, 46, 10);
		    if (w_e_velo == 0)
			goto no_velo;
		    w_e_velo -= 1;
		    if (read_bit_string(msg, 45, 1) == 1)
			w_e_velo = -w_e_velo;

		    s_n_velo = read_bit_string(msg, 57, 10);
		    if (s_n_velo == 0)
			goto no_velo;
		    s_n_velo -= 1;
		    if (read_bit_string(msg, 56, 1) == 1)
			s_n_velo = -s_n_velo;

		    if (subtype == 2) {
			w_e_velo *= 4;
			s_n_velo *= 4;
		    }
		    plane->velocity_we = w_e_velo;
		    plane->velocity_sn = s_n_velo;
		    plane->flags |= PLANE_GROUND_VELOCITY_AVAILABLE;
no_velo: ;
		} else {
//		    printf(", type = 19, subtype = %02" PRIu8, subtype);
		}
	    } else {
//		printf(", type = %02" PRIu8, type);
	    }
	}
    } else if (fmt == 20 || fmt == 21) {
    } else if (fmt == 4) {
	int m = BIT(ac, 6);
	int q = BIT(ac, 4);

	if (m == 0) {
	    if (q == 1) {
		int32_t altitude = (BITS(ac, 7, 6) << 5) |
				    (BIT(ac, 5) << 4) |
				    BITS(ac, 0, 4);

		plane->altitude = altitude * 25 - 1000;
		plane->flags |= PLANE_ALTITUDE_AVAILABLE;
	    } else {
		int DAB = ((AC_DIGIT(ac, 0) & 0x3) << 6) |
			    (AC_DIGIT(ac, 7) << 3) |
			    AC_DIGIT(ac, 1);
		int alt_code = gray_to_binary(DAB);
		int adjust_C =
		    ac_c_digit_conversion_table[alt_code & 1][AC_DIGIT(ac, 8)];

		if (adjust_C != -1) {
		    plane->altitude = (alt_code * 5 - 12 + adjust_C) * 100;
		    plane->flags |= PLANE_ALTITUDE_AVAILABLE;
		}
		// printf("DAB = %x, C = %x\n", DAB, C);
	    }
	}
    } else if (fmt == 5) {
	plane->squawk_id[0] = ID_DIGIT(id, 7) + '0';
	plane->squawk_id[1] = ID_DIGIT(id, 1) + '0';
	plane->squawk_id[2] = ID_DIGIT(id, 8) + '0';
	plane->squawk_id[3] = ID_DIGIT(id, 0) + '0';
	plane->squawk_id[4] = '\0';
	plane->flags |= PLANE_SQUAWK_ID_AVAILABLE;
    }

    return plane;
}

void
plane_print(struct plane *p) {
    printf("%06" PRIX32 " ", p->address);
    if (p->flags & PLANE_FLIGHT_ID_AVAILABLE)
	printf("%-8s ", p->flight_id);
    else
	printf("         ");
    if (p->flags & PLANE_SQUAWK_ID_AVAILABLE)
	printf("%4s ", p->squawk_id);
    else
	printf("     ");
    if (p->flags & PLANE_ALTITUDE_AVAILABLE)
	printf("%5ldft ", p->altitude);
    else
	printf("        ");
    if (p->flags & PLANE_GROUND_VELOCITY_AVAILABLE) {
	double w_e_velo = p->velocity_we;
	double s_n_velo = p->velocity_sn;
	double heading = atan2(w_e_velo, s_n_velo) * 180 / M_PI;
	if (heading < 0)
	    heading += 360;
	printf("%6.1fkt %05.1fd ",
		sqrt(w_e_velo * w_e_velo + s_n_velo * s_n_velo), heading);
    } else
	printf("                ");
    if (p->flags & PLANE_COORDINATES_AVAILABLE) {
	printf("% 8.5f % 8.5f ", p->latitude, p->longitude);
    } else
	printf("                   ");

    struct tm *tm = localtime(&p->last_update.tv_sec);
    printf("%02d:%02d:%02d", tm->tm_hour, tm->tm_min, tm->tm_sec);

    printf(" %5.1f", p->last_msg_amplitude);
}
