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
#include <syslog.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

int getipinfo(char *ip, char *bip, char *netmask, char *ifname)
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
	if (bip != NULL) {
		snprintf(bip, 17, "%s", inet_ntoa(sin.sin_addr));
    }
 
	if (ioctl(sock, SIOCGIFNETMASK, &ifreq) < 0) {
		close(sock);
		return -1;
	}

	memcpy(&sin, &ifreq.ifr_addr, sizeof(sin));
    snprintf(netmask, 17, "%s", inet_ntoa(sin.sin_addr));
    
	close(sock);
	
	return 0;
}

int GetGatewayForInterface(const char* interface, char *gateway) 
{
	int ret = -1;

    char cmd [128] = {0x0};
    char line[256] = {0x0};

    sprintf(cmd, "route -n | grep %s  | grep 'UG[ \t]' | awk '{print $2}'", interface);

    FILE* fp = popen(cmd, "r");

    if (fgets(line, sizeof(line), fp) != NULL) {
        strcpy(gateway, line);
        syslog(LOG_DEBUG, "gateway : %s ",line);
		ret = 0;
	}

    pclose(fp);

	return ret;
}

int GetDNSForInterface(const char* interface, char *dns)
{
    int ret = -1;

    char cmd [128] = {0x0};
    char line[256] = {0x0};
    int dns_interface_wan = 0;
    char *res = NULL;
    int len = 0;

    sprintf(cmd, "cat /tmp/resolv.conf.auto |awk '{if(NR==1)print $0}'");

    FILE* fp = popen(cmd, "r");
    printf("%s %d\n", __FUNCTION__, __LINE__);

    if (fgets(line, sizeof(line), fp) != NULL) {
        len = strlen(line);
        printf("%s %d line:%s len-1:%x len-2:%x\n", __FUNCTION__, __LINE__, line, line[len-1], line[len-2]);
        if (!strcmp(line, "# Interface wan\n")) {
            printf("%s %d\n", __FUNCTION__, __LINE__);
            dns_interface_wan = 1;
        }
    }

    pclose(fp);
    printf("%s %d\n", __FUNCTION__, __LINE__);

    if (dns_interface_wan) {
        printf("%s %d\n", __FUNCTION__, __LINE__);
        memset(cmd, 0, sizeof(cmd));
        sprintf(cmd, "cat /tmp/resolv.conf.auto |awk '{if(NR==2)print $2}'");
        printf("cmd:%s \n", cmd);

        FILE* fp = popen(cmd, "r");

        if (fgets(line, sizeof(line), fp) != NULL) {
            strcpy(dns, line);
            printf("dns : %s \n",line);
            ret = 0;
        }

        pclose(fp);
    }


	return ret;
}