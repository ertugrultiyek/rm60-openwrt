#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <syslog.h>

enum DEBUG_LEVEL {
	DEBUG_OFF,
	DEBUG_ERROR,
	DEBUG_DEBUG,
	DEBUG_TRACE,
	DEBUG_INFO,
	DEBUG_NOISE,
	DEBUG_MAX
};

#define debug_log(debug, fmt, arg...) \
do {\
	struct timeval tv;	\
	struct timezone tz;	\
	gettimeofday(&tv,&tz);	\
	time_t Nowt = 0;\
	time_t Ti = 0;\
	struct tm *Pt;\
	Ti = time(&Nowt);\
	Pt = localtime(&Nowt);\
	syslog(LOG_DEBUG, "%s:%d: \n" fmt "\n", __func__, __LINE__, ##arg);\
} while (0)
