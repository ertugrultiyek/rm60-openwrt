#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/socket.h>

#include <sys/mman.h>
#include <sys/stat.h>

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>
#include <arpa/inet.h>

#include "../include/hlk_platform.h"

char *hlk_tool_numtostr(int num)
{
	char *str;
	static char buf[256];

	memset(buf, 0, 256);
	sprintf(buf, "%d", num);

	return buf;
}

void hlk_tool_print_hexdump(char *msg, int msg_len, int type)
{
	int rc;
	int i = 0;
	char *buf, *ptr;
	int offset = 0;

	if (msg_len <= 0) {
		return;
	}

	buf = malloc( (msg_len)*3 + msg_len/32+1);
	if (NULL == buf) {
		return;
	}
	memset(buf, 0, (msg_len)*3 + msg_len/32+1);

	ptr = buf;
	
	for (i = 0; i < msg_len; i++) {
		if (i != 0 && (i+1)%32 == 0) {
			if (0 == type ) {
				rc = sprintf(ptr, "%02X \n", (unsigned char)msg[i]);
			} else if (1 == type) {
				rc = sprintf(ptr, "%C \n", msg[i]);
			}
		} else {
			if (0 == type ) {
				rc = sprintf(ptr, "%02X ", (unsigned char)msg[i]);
			} else if (1 == type) {
				rc = sprintf(ptr, "%C", msg[i]);
			}
		}
		ptr += strlen(ptr);
	}

	//printf("%s", buf);
	
	free(buf);

	return;
}

void hlk_tool_do_system(char *fmt, ...)
{
	va_list ap;
	char cmd[256];

	va_start(ap, fmt);
	vsprintf(cmd, fmt, ap);
	va_end(ap);

	sprintf(cmd, "%s", cmd);
	system(cmd);

	return;
}

int hlk_tool_processIsRun(char *proc, char *opt, int cnt)
{
    FILE* fp = NULL; 
    int count = 0;
    int BUFSZ = 100;
    char buf[BUFSZ];
    char command[64];
 
    sprintf(command, "ps | grep %s|grep -v grep| wc -l", proc); 

    if( (fp = popen(command,"r")) == NULL) { 
        return -1;
    }
	
    if( (fgets(buf, BUFSZ, fp)) != NULL) { 
        count = atoi(buf); 
    }
    
    pclose(fp); 

	if (!strcmp(opt, "==") ) {
		if (count == cnt) {
			return 0;
		} else {
			return -1;
		}
	} else if (!strcmp(opt, ">") ) {
		if (count > cnt) {
			return 0;
		} else {
			return -1;
		}
	} else if (!strcmp(opt, ">=") ) {
		if (count >= cnt) {
			return 0;
		} else {
			return -1;
		}
	} 

	return 0;
}

int hlk_nettool_getmacinfo(char *ifname, char *mac, int colon)
{
	char *ptr;
	int sock, i;
	struct ifreq ifreq;
	struct sockaddr_in sin;
	
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		return -1;
	}

	strcpy(ifreq.ifr_name, ifname);
	
	if(ioctl(sock, SIOCGIFHWADDR, &ifreq) < 0) {
		close(sock);
		return -1;
	}

	ptr = mac;
	for (i = 0; i < 6; i++) {
		sprintf(ptr, "%02x", (unsigned char)ifreq.ifr_hwaddr.sa_data[i]);
		if (i != 5) {
			ptr += strlen(ptr);
			if (colon) {
				sprintf(ptr, ":");
				ptr += strlen(ptr);
			}
		}
	}

	close(sock);
	
	return 0;
}

int hlk_nettool_getipinfo(char *ip, char *bip, char *ifname)
{
	struct ifreq ifreq;
	int sock, i;
	char *ptr;
	struct sockaddr_in sin;
	
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		return -1;
	}
	
	strcpy(ifreq.ifr_name, ifname);
	if (ioctl(sock, SIOCGIFADDR, &ifreq) < 0) {
		close(sock);
		return -1;
	}
	memcpy(&sin, &ifreq.ifr_addr, sizeof(sin));
	snprintf(ip, 17, "%s", inet_ntoa(sin.sin_addr));
	
	if (ioctl(sock, SIOCGIFBRDADDR, &ifreq) < 0) {
		close(sock);
		return -1;
	}
	
	memcpy(&sin, &ifreq.ifr_addr, sizeof(sin));
	if (bip != NULL)
		snprintf(bip, 17, "%s", inet_ntoa(sin.sin_addr));
 
	
	close(sock);
	
	return 0;
}

void hlk_tool_sleep(int ms)
{
	struct timeval tv;

	tv.tv_sec = 0;
	tv.tv_usec = ms*1000;

	select(0, NULL, NULL, NULL, &tv);

    return;
}

unsigned long hlk_tool_getrandom(void)
{
	int fd = 0;
	char *buff = NULL;
	unsigned long ulTest = 0;

	fd = open("/dev/urandom", O_RDONLY);
    if (fd < 0) {
        ulTest = 0x12345678;
    } else {
        read(fd, &ulTest, sizeof(ulTest));
        close(fd);
    }

	return ulTest;
}

#define hlk_timercmp(a, b, CMP)                                                  \
  (((a)->tv_sec == (b)->tv_sec) ?                                             \
   ((a)->tv_usec CMP (b)->tv_usec) :                                          \
   ((a)->tv_sec CMP (b)->tv_sec))

#define hlk_timersub(a, b, result)                                               \
do {                                                                        \
	(result)->tv_sec = (a)->tv_sec - (b)->tv_sec;                             \
	(result)->tv_usec = (a)->tv_usec - (b)->tv_usec;                          \
	if ((result)->tv_usec < 0) {                                              \
		--(result)->tv_sec;                                                     \
		(result)->tv_usec += 1000000;                                           \
	}                                                                         \
} while (0)

//大于等于base时间 返回0 否则返回 -1		基准时间是ms 超时返回0 为超时返回 -1
int hlk_tool_timerout(struct timeval *last, struct timeval *current, struct timeval *timeout)
{
	struct timeval tm;

	hlk_timersub(current, last, &tm);
	
	if (hlk_timercmp(&tm, timeout, >=)) {
		return 0;
	}
	
	return -1;
}

/*
    函数名称：hlk_tool_get_cmd_ret
    函数功能：获取命令执行后的返回字符串
    函数参数：
              cmd ：执行的命令
              rstr: 命令执行返回的指针
              len: 
    函数返回值： > 0 执行并返回成功
               -1 执行失败 或读取返回数据失败
*/
int hlk_tool_get_cmd_ret(char *cmd, char *rstr, int len)
{
    int rc = -1;
	FILE *pp;
    int offset = 0;

    if (NULL == rstr ) {
        return -1;
    }
	
	if (!(pp = popen(cmd, "r"))) {
		return -1;
	}

    do {
        rc = fread(rstr+offset, 1, len-offset, pp);
        if (rc < 0) {
            offset = -1;
            break;
        }
        offset += rc;

    } while (rc > 0);

	pclose(pp);

	return offset;
}

