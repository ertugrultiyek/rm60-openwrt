#include <stdio.h>
#include <stdarg.h>
#include <syslog.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
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
	syslog(LOG_DEBUG, "%s:%d: \n" fmt "\n", __func__, __LINE__, ##arg);\
} while (0)

int read_str_from_file (char *file_path, char *rstr, int max_len)
{
    int res;

    int fd = open(file_path, O_RDONLY);
    if (fd > 0) {
        res = read(fd, rstr, max_len);
        if (res > 0) {
            close(fd);
            return 0;
        }
    }

    return -1;
}

int write_str_to_file(char *file_path, char *wstr)
{
    int res;

    int imei_fd = open(file_path, O_RDWR|O_CREAT);
    if (imei_fd > 0) {
        res = write(imei_fd, wstr, strlen(wstr));
        if (res != strlen(wstr)) {
            debug_log(DEBUG_DEBUG, "write failed\n");
        }
        close(imei_fd);
        return 0;
    }

    return -1;
}

void do_system(char *fmt, ...)
{
	va_list ap;
	char cmd[256];

	va_start(ap, fmt);
	vsprintf(cmd, fmt, ap);
	va_end(ap);
	
	system(cmd);

	return;
}

