#ifndef _XNET_UTIL_H_
#define _XNET_UTIL_H_
#include <time.h>
#include <stdint.h>
#include <stdarg.h>

#ifdef _WIN32
	#include <Windows.h>

	int util_gettimeofday(struct timeval *tv, struct timezone *tz);
#else
	#include <sys/time.h>
	#define util_gettimeofday gettimeofday
#endif

uint64_t get_time();
void timestring(uint64_t time, char *out, int size);

int xnet_vsnprintf(char *buf, size_t buflen, const char *format, va_list ap);
#endif //_XNET_UTIL_H_