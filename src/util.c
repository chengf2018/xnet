#include "xnet_util.h"
#include <stdio.h>

int
xnet_vsnprintf(char *buf, size_t buflen, const char *format, va_list ap) {
	int r;
	if (!buflen) return 0;
#ifdef _WIN32
	r = _vsnprintf(buf, buflen, format, ap);
	if (r < 0)
		r = _vscprintf(format, ap);
#else
	r = vsnprintf(buf, buflen, format, ap);
#endif
	buf[buflen-1] = '\0';
	return r;
}
