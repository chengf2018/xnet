#include "xnet_util.h"
#include <time.h>

#ifdef _WIN32

int
util_gettimeofday(struct timeval *tv, struct timezone *tz) {
	/* Conversion logic taken from Tor, which in turn took it
	 * from Perl.  GetSystemTimeAsFileTime returns its value as
	 * an unaligned (!) 64-bit value containing the number of
	 * 100-nanosecond intervals since 1 January 1601 UTC. */
#define EPOCH_BIAS 116444736000000000llu
#define UNITS_PER_SEC 10000000llu
#define USEC_PER_SEC 1000000llu
#define UNITS_PER_USEC 10llu
	union {
		FILETIME ft_ft;
		uint64_t ft_64;
	} ft;

	if (tv == NULL)
		return -1;

	GetSystemTimeAsFileTime(&ft.ft_ft);

	ft.ft_64 -= EPOCH_BIAS;
	tv->tv_sec = (long) (ft.ft_64 / UNITS_PER_SEC);
	tv->tv_usec = (long) ((ft.ft_64 / UNITS_PER_USEC) % USEC_PER_SEC);
    return 0;
}

#endif


uint64_t
get_time() {
// #ifdef _WIN32
    struct timeval tv;
    util_gettimeofday(&tv, NULL);
    return (uint64_t)(tv.tv_sec) * 1000 + (uint64_t)(tv.tv_usec) / 1000;
// #else
//     uint64_t t;
// 	struct timespec ti;
// 	clock_gettime(CLOCK_MONOTONIC, &ti);
// 	t = (uint64_t)ti.tv_sec * 1000;
// 	t += ti.tv_nsec / 1000000;
// 	return t;
// #endif
}

#ifdef _WIN32
	#define localtime_r(a,b) localtime_s((b), (a))
#endif
void
timestring(uint64_t time, char *out, int size) {
	struct tm info;
	time_t ti = time;
	localtime_r(&ti, &info);
	strftime(out, size, "%d/%m/%y %H:%M:%S", &info);
}