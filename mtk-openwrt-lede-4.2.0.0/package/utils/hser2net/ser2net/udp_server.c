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
#include "ring.h"
#include "defines.h"
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

/*MAC*/
#include <net/if.h>
#include <linux/if_ether.h>

#include "pthread.h"

#include <sys/socket.h>
#include <sys/un.h>

#include <unistd.h>

#include <sys/msg.h>

#include <linux/tcp.h>
#include <linux/socket.h>

//#include <netinet/tcp.h>

#include "ser2net.h"

#define DIS_STR "HLK"
#define AT_START_STR "HJ^&%$#(7854462cdEcEOl"
int udp_run = 1;

extern uchar *g_AT_discovery_udp_name;

uchar AT_version(uchar *comm, uchar *out);

void *udp_discovery_server(void *arg)
{
	int sock;
	//sendto��ʹ�õĶԷ���ַ
	struct sockaddr_in toAddr;
	//��recvfrom��ʹ�õĶԷ�������ַ
	struct sockaddr_in fromAddr;
	int recvLen;
	unsigned int addrLen;
	char recvBuffer[128];

	int g_select_quit_fd = 0;

	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock < 0)
	{
		TIM_ERROR("ERROR:%s(%d)\r\n", __FUNCTION__, __LINE__);
		pthread_exit(0);
	}
	TIM_DEBUG("In:%s(%d)\r\n", __FUNCTION__, __LINE__);

	{
		int yes = 1;
		setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &yes, sizeof(yes));
	}

	{
		int on = 1;
		setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
	}

	memset(&fromAddr, 0, sizeof(fromAddr));
	fromAddr.sin_family = AF_INET;
	fromAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	fromAddr.sin_port = htons(988);
	if (bind(sock, (struct sockaddr *)&fromAddr, sizeof(fromAddr)) < 0)
	{
		TIM_ERROR("ERROR:%s(%d)\r\n", __FUNCTION__, __LINE__);
		close(sock);
		pthread_exit(1);
	}

	{
		int flags;
		flags = fcntl(sock, F_GETFL, 0);
		fcntl(sock, F_SETFL, flags | O_NONBLOCK);
	}

	if ((mkfifo(UDP_QUIT_PIPE_DEV, 0777) < 0) && (errno != EEXIST))
	{
		TIM_DEBUG("ERROR:%s(%d)\r\n", __FUNCTION__, __LINE__);
	}

	g_select_quit_fd = open(UDP_QUIT_PIPE_DEV, O_NONBLOCK | O_RDWR);
	TIM_DEBUG("in:%s(%d)\r\n", __FUNCTION__, __LINE__);

	while (udp_run)
	{
		int maxfd = 0;
		fd_set select_fds;
		int ret;

		FD_ZERO(&select_fds);

		FD_SET(sock, &select_fds);
		maxfd = MAX_(maxfd, sock);

		FD_SET(g_select_quit_fd, &select_fds);
		maxfd = MAX_(maxfd, g_select_quit_fd);

		ret = select(maxfd + 1, &select_fds, NULL, NULL, NULL);

		if (ret > 0)
		{
			if (FD_ISSET(sock, &select_fds))
			{

				addrLen = sizeof(toAddr);
				TIM_DEBUG("In:%s(%d)\r\n", __FUNCTION__, __LINE__);
				if ((recvLen = recvfrom(sock, recvBuffer, 127, 0, (struct sockaddr *)&toAddr, &addrLen)) < 0)
				{
					TIM_ERROR("ERROR:%s(%d)\r\n", __FUNCTION__, __LINE__);
					//close(sock);
					//pthread_exit(1);
					break;
				}
				TIM_DEBUG("In:%s(%d) %d\r\n", __FUNCTION__, __LINE__, recvLen);
				recvBuffer[recvLen] = 0;

				if (0 == strcmp(recvBuffer, DIS_STR))
				{
					{
						uchar out[128];
						out[0] = 0;
						AT_version(" ?", out);
						if (0 == strcmp("", nvram_get(SER2NET_NVRAM, "p2p3")))
						{
							sprintf(recvBuffer, "%s(%s)", g_AT_discovery_udp_name, out);
						}
						else
						{
							uchar buf_tmp[128];
							uchar *p;
							sprintf(buf_tmp, nvram_get(SER2NET_NVRAM, "p2p3"));
							p = strstr(buf_tmp, "@");
							if (p)
								*p = 0;
							sprintf(recvBuffer, "%s(%s)(%s)", g_AT_discovery_udp_name, out, buf_tmp);
						}
					}
					srand((unsigned)time(NULL));
					usleep(rand() % 1000000);
					TIM_DEBUG("In:%s(%d) %d\r\n", __FUNCTION__, __LINE__, (rand() % 1000000));
					TIM_DEBUG("In:%s(%d)\r\n", __FUNCTION__, __LINE__);
					recvLen = strlen(recvBuffer);
					if (sendto(sock, recvBuffer, recvLen, 0, (struct sockaddr *)&toAddr, sizeof(toAddr)) != recvLen)
					{
						TIM_ERROR("ERROR:%s(%d)\r\n", __FUNCTION__, __LINE__);
						//close(sock);
						break;
					}
				}
				else if (0 == strcmp(recvBuffer, "HLK_MAC"))
				{
					{
						uchar out[128];
						out[0] = 0;
						AT_version(" ?", out);
						sprintf(recvBuffer, "%s(%s)", g_AT_discovery_udp_name, out);
					}

					{
						uchar out[128];
						int len;
						out[0] = 0;
						config_command("at+Get_MAC=?\r\n", out);
						out[sizeof(out) - 1] = 0;

						strcat(recvBuffer, out);
					}
					srand((unsigned)time(NULL));
					usleep(rand() % 1000000);
					TIM_DEBUG("In:%s(%d) %d\r\n", __FUNCTION__, __LINE__, (rand() % 1000000));
					TIM_DEBUG("In:%s(%d)\r\n", __FUNCTION__, __LINE__);
					recvLen = strlen(recvBuffer);
					if (sendto(sock, recvBuffer, recvLen, 0, (struct sockaddr *)&toAddr, sizeof(toAddr)) != recvLen)
					{
						TIM_ERROR("ERROR:%s(%d)\r\n", __FUNCTION__, __LINE__);
						//close(sock);
						break;
					}
				}
				else
				{
					//			continue;
					uchar *p = nvram_get(SER2NET_NVRAM, "AT_UdpAtEn");

					if (p && (*p == '1'))
						if (recvBuffer == strstr(recvBuffer, AT_START_STR))
						{
							uchar out[128];
							int len;
							out[0] = 0;
							config_command(recvBuffer + strlen(AT_START_STR), out);
							out[sizeof(out) - 1] = 0;
							len = strlen(out);
							TIM_DEBUG("In:%s(%d) %s\r\n", __FUNCTION__, __LINE__, out);
							if (sendto(sock, out, len, 0, (struct sockaddr *)&toAddr, sizeof(toAddr)) != len)
							{
								TIM_ERROR("ERROR:%s(%d)\r\n", __FUNCTION__, __LINE__);
								//close(sock);
								//pthread_exit(0);
								break;
							}
						}
				}
			}

			if (FD_ISSET(g_select_quit_fd, &select_fds))
			{
				uchar buf[64];
				TIM_DEBUG("in:%s(%d)\r\n", __FUNCTION__, __LINE__);
				read(g_select_quit_fd, buf, sizeof(buf));
				break;
			}
		}
		else
		{
			TIM_WARN("WARN:%s(%d)\r\n", __FUNCTION__, __LINE__);
		}

		//		return 0;
	}

	if (g_select_quit_fd)
		close(g_select_quit_fd);

	if (sock)
		close(sock);

	//	pthread_exit(0);
	//pthread_detach(pthread_self());
	return ((void *)0);
}
