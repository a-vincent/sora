#ifndef ADS_B_PLANE_H_
#define ADS_B_PLANE_H_

#include <inttypes.h>
#include <sys/time.h>

#include <util/iterator.h>

struct plane;
struct plane_set;

struct plane_set *plane_set_new(void);
void plane_set_delete(struct plane_set *);
void plane_set_free(struct plane_set *);
struct plane *plane_set_lookup_create(struct plane_set *, uint32_t address);
struct plane *plane_set_parse_message(struct plane_set *, const char *,
		struct timeval, double);
struct iterator *plane_set_iterate(struct plane_set *);

struct plane *plane_clone(struct plane *);
void plane_delete(struct plane *);
void plane_print(struct plane *);
const char *plane_get_category(struct plane *);
const char *plane_get_squawk_id(struct plane *);
const char *plane_get_flight_id(struct plane *);
int plane_is_ground_velocity_available(struct plane *);
int plane_is_heading_available(struct plane *);
double plane_get_ground_velocity(struct plane *);
double plane_get_heading(struct plane *);
int plane_is_altitude_available(struct plane *);
long plane_get_altitude(struct plane *);
int plane_is_gnss_vs_baro_available(struct plane *);
int plane_get_gnss_vs_baro(struct plane *);
int plane_is_vert_rate_available(struct plane *);
int plane_get_vert_rate(struct plane *);
int plane_is_lat_long_available(struct plane *);
double plane_get_latitude(struct plane *);
double plane_get_longitude(struct plane *);

#endif /* ADS_B_PLANE_H_ */
