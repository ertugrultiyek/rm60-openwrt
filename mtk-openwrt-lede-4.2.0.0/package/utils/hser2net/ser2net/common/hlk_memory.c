#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <termios.h>
#include <fcntl.h>
#include <pthread.h>
#include <syslog.h>
#include <signal.h>
#include <netdb.h>
#include <stdio.h>
#include <sys/ioctl.h>

#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <ctype.h>
#include <limits.h>
#include <stdarg.h>
#include <assert.h>
#include <stdint.h>
#include <errno.h>
#include "flash.h"
#include <net/if.h>
#include <linux/if_ether.h>

#include "pthread.h"

#include <sys/socket.h>
#include <sys/un.h>

#include <unistd.h>

#include <sys/msg.h>

#include <signal.h>

#include <net/route.h>

#include "ser2net.h"

uchar strnull[] = {0};

uchar *split(uchar *src, uchar *sp, uchar index)
{
	uchar *sret = strnull;
	uchar *p = src;
	uchar i;

	for (i = 0; i < index; i++, p += strlen(sp))
	{
		p = strstr(p, sp);
		if (p == 0)
			return strnull;
	}
	return p;
}

uchar strcmpT(uchar *a, uchar *b)
{
	while (1)
	{
		if (*a != *b)
		{
			if ((*a == 0) && (*b == ','))
			{
				return 0;
			}
			if ((*b == 0) && (*a == ','))
			{
				return 0;
			}
			return 1;
		}
		else if (*a == 0)
		{
			return 0;
		}
		else if (*a == ',')
		{
			return 0;
		}
		a++;
		b++;
	}
}

void strcpyT(uchar *a, uchar *b, uint n)
{
	*a = 0;
	uint i = 0;
	while (*b)
	{
		if (*b == ',')
		{
			*a = 0;
			return;
		}
		if (*b == '\r')
		{
			*a = 0;
			return;
		}
		*a = *b;
		a++;
		b++;
		if (i++ >= n)
		{
			*a = 0;
			return;
		}
	}
	*a = 0;
}
void strcpyT2(uchar *a, uchar *b, uint n, char cc)
{
	*a = 0;
	uint i = 0;
	while (*b)
	{
		if (*b == cc)
		{
			*a = 0;
			return;
		}
		if (*b == '\r')
		{
			*a = 0;
			return;
		}
		*a = *b;
		a++;
		b++;
		if (i++ >= n)
		{
			*a = 0;
			return;
		}
	}
	*a = 0;
}

int hlk_strtol(char *str, int base, long *val)
{
	char *endptr = NULL;
	errno = 0;

	*val = strtol(str, &endptr, base);

	/* Check for various possible errors */
	if ( (errno == ERANGE && (*val == LONG_MAX || *val == LONG_MIN)) || (errno != 0 && *val == 0) ) {
		syslog(LOG_DEBUG, "%s %d errno:%d *val :%d\n", __FUNCTION__, __LINE__, errno, *val);
		return -1;
	}

	if (endptr == str) {
		syslog(LOG_DEBUG, "No digits were found\n");
		return -1;
	}

	/* If we got here, strtol() successfully parsed a number */
	syslog(LOG_DEBUG, "strtol() returned %ld\n", *val);

	if (*endptr != '\0') {       /* Not necessarily an error... */
		syslog(LOG_DEBUG, "Further characters after number: %s\n", endptr);
	}

	return 0;
}

int get_cmd_ret_str(char *rstr, char *cmd)
{
	FILE *pp;
	char command[512] = {0};
	
	//获取下载文件长度
	sprintf(command, "%s", cmd);
	if(!(pp = popen(command, "r"))) {
		return -1;
	}

	if (fgets(command, sizeof(command), pp)) {
		strcpy(rstr, command);
	} else {
	}

	fclose(pp);

	return 0;
}

void do_system(char *fmt, ...)
{
	va_list ap;
	char cmd[256] = {0};

	va_start(ap, fmt);
	vsprintf(cmd, fmt, ap);
	va_end(ap);

	//sprintf(cmd, "%s", cmd);

	system(cmd);

	return;
}
