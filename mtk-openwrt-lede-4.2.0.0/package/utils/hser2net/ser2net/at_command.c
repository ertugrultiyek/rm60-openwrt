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
//#include "ring.h"
//#include "defines.h"
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

//#include "nvram.h"

#include <signal.h>

//#include	"ralink_gpio.h"

#include <net/route.h>

#include "ser2net.h"
#include <uci.h>

#define gtolower(a) (a)

extern uchar *g_serial_config;
extern uchar g_network_mode;
extern uchar *g_remoteip_domain;
extern int g_locale_remote_port;
extern int g_client_locale_port;

extern uchar g_network_protocol; //0 none,1 tcp,2 udp
extern uint g_network_timeout;

extern uint g_serial_package_len;
extern uint g_serial_package_time;
extern uint g_serial_package_interval_time;

extern uchar g_transparent_escape_en;
extern uchar g_transparent_escape2_en;

extern uint g_transparent_escape2_time;

extern uchar g_AT_netmode;	  //0-default 1-eth 2-client 3-ap 4-3g 5-netwebconfig
extern uchar *g_AT_wifi_conf; //none wep wpa_tkip wpa_aes wpa2_tkip wpa2_aes wpawpa2_tkip wpawpa2_aes
extern uchar g_AT_dhcpd;

extern uchar *g_AT_dhcpd_ip;
extern uchar *g_AT_dhcpd_dns;
extern uint g_AT_dhcpd_time;

extern uchar g_AT_dhcpc;
extern uchar *g_AT_net_ip;
extern uchar *g_AT_net_dns;

extern ushort g_AT_pm;

extern uchar *g_AT_discovery_udp_name;

extern ushort g_AT_gpio_mode;
extern ushort g_AT_gpio_data;

extern uchar g_SMaxConn;
extern uchar *g_SRemoteIp;
extern uchar g_SRemotePort;
extern uchar *g_SRemoteIpPort;

extern uchar g_tcp_realtime;

void goahead_initInternet(void);

void power_save(void);

//=?
uchar AT_version(uchar *comm, uchar *out)
{
	if ('?' == *(comm + 1))
	{
		sprintf(out, "V%d.%d(%s)", AT_VERSION / 100, AT_VERSION % 100, __DATE__);
	}
}

uchar AT_mode(uchar *comm, uchar *out)
{
	uint ret = 0;

	if ('?' == *(comm + 1))
	{
		if (1 == g_network_mode)
		{
			sprintf(out, "server");
		}
		else if (2 == g_network_mode)
		{
			sprintf(out, "client");
		}
		else
		{
			sprintf(out, "none");
		}
	}
	else if ('=' == *(comm + 0))
	{
		uchar *p;
		if (strstr(comm, "server"))
		{
			g_network_mode = 1;
			sprintf(out, "ok");
		}
		else if (strstr(comm, "client"))
		{
			g_network_mode = 2;
			sprintf(out, "ok");
		}
		else if (strstr(comm, "none"))
		{
			g_network_mode = 0;
			sprintf(out, "ok");
		}
		else
		{
			sprintf(out, "error");
		}
	}

	return ret;
}

uchar AT_remoteip(uchar *comm, uchar *out)
{
	uint ret = 0;

	if ('?' == *(comm + 1))
	{
		sprintf(out, "%s", g_remoteip_domain);
	}
	else if ('=' == *(comm + 0))
	{
		g_remoteip_domain = at_set_value_(g_remoteip_domain, comm + 1);
		sprintf(out, "ok");
	}
	else
	{
		sprintf(out, "error");
	}

	return ret;
}

uchar AT_remoteport(uchar *comm, uchar *out)
{
	uint ret = 0;

	if ('?' == *(comm + 1))
	{
		sprintf(out, "%d", g_locale_remote_port);
	}
	else if ('=' == *(comm + 0))
	{
		g_locale_remote_port = atoi(comm + 1);
		sprintf(out, "ok");
	}
	else
	{
		sprintf(out, "error");
	}

	return ret;
}
uchar AT_client_localeport(uchar *comm, uchar *out)
{
	uint ret = 0;

	if ('?' == *(comm + 1))
	{
		sprintf(out, "%d", g_client_locale_port);
	}
	else if ('=' == *(comm + 0))
	{
		g_client_locale_port = atoi(comm + 1);
		sprintf(out, "ok");
	}
	else
	{
		sprintf(out, "error");
	}

	return ret;
}

uchar AT_remotepro(uchar *comm, uchar *out)
{
	uint ret = 0;

	if ('?' == *(comm + 1))
	{
		if (1 == g_network_protocol)
		{
			sprintf(out, "tcp");
		}
		else if (2 == g_network_protocol)
		{
			sprintf(out, "udp");
		}
		else
		{
			sprintf(out, "none");
		}
	}
	else if ('=' == *(comm + 0))
	{
		uchar *p;
		if (strstr(comm, "tcp"))
		{
			g_network_protocol = 1;
			sprintf(out, "ok");
		}
		else if (strstr(comm, "udp"))
		{
			g_network_protocol = 2;
			sprintf(out, "ok");
		}
		else if (strstr(comm, "none"))
		{
			g_network_protocol = 0;
			sprintf(out, "ok");
		}
		else
		{
			sprintf(out, "error");
		}
	}

	return ret;
}

uchar AT_SMaxConn(uchar *comm, uchar *out)
{
	uint ret = 0;

	if ('?' == *(comm + 1))
	{
		sprintf(out, "%d", g_SMaxConn);
	}
	else if ('=' == *(comm + 0))
	{
		g_SMaxConn = atoi(comm + 1);
		if (g_SMaxConn > MAX_CONNECTS)
			g_SMaxConn = MAX_CONNECTS;
		sprintf(out, "ok");
	}

	return ret;
}

uchar AT_TCPRealTime(uchar *comm, uchar *out)
{
	uint ret = 0;

	if ('?' == *(comm + 1))
	{
		sprintf(out, "%d", g_tcp_realtime);
	}
	else if ('=' == *(comm + 0))
	{
		g_tcp_realtime = atoi(comm + 1);
		sprintf(out, "ok");
	}

	return ret;
}

uchar AT_SRemoteIp(uchar *comm, uchar *out)
{
	uint ret = 0;

	if ('?' == *(comm + 1))
	{
		sprintf(out, "%s", g_SRemoteIp);
	}
	else if ('=' == *(comm + 0))
	{
		g_SRemoteIp = at_set_value_(g_SRemoteIp, (comm + 1));
		sprintf(out, "ok");
	}

	return ret;
}

uchar AT_SRemoteIpPort(uchar *comm, uchar *out)
{
	uint ret = 0;

	if ('?' == *(comm + 1))
	{
		sprintf(out, "%s", g_SRemoteIpPort);
	}
	else if ('=' == *(comm + 0))
	{
		g_SRemoteIpPort = at_set_value_(g_SRemoteIpPort, (comm + 1));
		sprintf(out, "ok");
	}

	return ret;
}

uchar AT_SRemotePort(uchar *comm, uchar *out)
{
	uint ret = 0;

	if ('?' == *(comm + 1))
	{
		sprintf(out, "%d", g_SRemotePort);
	}
	else if ('=' == *(comm + 0))
	{
		g_SRemotePort = atoi(comm + 1);
		sprintf(out, "ok");
	}

	return ret;
}

extern uint statistics_serial_send;
extern uint statistics_serial_tx;
extern uint statistics_serial_tx2;
extern uint statistics_serial_tx3;

#if 0
extern uint tim_test1;
extern uint tim_test2;
extern uint tim_test3;
extern uint tim_test4;
extern uint tim_test5;
extern uint tim_test6;
extern uint tim_test7;


extern uint tim_testA[20];
#endif

uchar AT_uart(uchar *comm, uchar *out)
{
	uint ret = 0;

	if ('?' == *(comm + 1))
	{
		sprintf(out, "%s", g_serial_config);
	}
	else if ('=' == *(comm + 0))
	{
		g_serial_config = at_set_value_(g_serial_config, comm + 1);
		sprintf(out, "ok");
	}
	else
	{
		sprintf(out, "error");
	}

	return ret;
}

uchar AT_timeout(uchar *comm, uchar *out)
{
	uint ret = 0;

	if ('?' == *(comm + 1))
	{
		sprintf(out, "%d", g_network_timeout);
	}
	else if ('=' == *(comm + 0))
	{
		g_network_timeout = atoi(comm + 1);
		sprintf(out, "ok");
	}
	else
	{
		sprintf(out, "error");
	}

	return ret;
}

extern uchar g_tcp_auto_connec;
extern uchar g_tcp_client_stat_check;

uchar AT_tcp_auto_connec(uchar *comm, uchar *out)
{
	uint ret = 0;

	if ('?' == *(comm + 1))
	{
		sprintf(out, "%d", g_tcp_auto_connec);
	}
	else if ('=' == *(comm + 0))
	{
		g_tcp_auto_connec = atoi(comm + 1);
		sprintf(out, "ok");
	}
	else
	{
		sprintf(out, "error");
	}

	return ret;
}

uchar AT_tcp_client_stat_check(uchar *comm, uchar *out)
{
	uint ret = 0;

	if ('?' == *(comm + 1))
	{
		sprintf(out, "%d", g_tcp_client_stat_check);
	}
	else if ('=' == *(comm + 0))
	{
		g_tcp_client_stat_check = atoi(comm + 1);
		sprintf(out, "ok");
	}
	else
	{
		sprintf(out, "error");
	}

	return ret;
}

uchar AT_Channel(uchar *comm, uchar *out)
{
	uint ret = 0;

	if ('?' == *(comm + 1))
	{
		if (0 == strcmp(nvram_get(SER2NET_NVRAM, "AutoChannelSelect"), "1"))
		{
			sprintf(out, "auto");
		}
		else
			sprintf(out, "%s", nvram_get(SER2NET_NVRAM, "Channel"));
	}
	else if ('=' == *(comm + 0))
	{
		uchar buf[64];
		int channel = atoi(comm + 1);
		if (channel)
		{
			nvram_set(SER2NET_NVRAM, "AutoChannelSelect", "0");
			sprintf(buf, "%d", channel);
			nvram_set(SER2NET_NVRAM, "Channel", buf);
		}
		else
		{
			nvram_set(SER2NET_NVRAM, "AutoChannelSelect", "1");
		}
		sprintf(out, "ok");
	}
	else
	{
		sprintf(out, "error");
	}

	return ret;
}

uchar suspend = 0;

void wifi_power_save(uchar on);

uchar wifi_suspend(uchar *comm, uchar *out)
{
	uint ret = 0;

	if ('?' == *(comm + 1))
	{
		sprintf(out, "%d", suspend);
	}
	else if ('=' == *(comm + 0))
	{
		suspend = atoi(comm + 1);
		if (suspend)
		{
			system("(config-powersave.sh ethernet 1 3;config-powersave.sh ethernet 1 4;config-powersave.sh usb 1)");
			//			wifi_power_save(0);

			system("(brctl delif br0 ra0;brctl delif br0 apcli0;ifconfig ra0 down;ifconfig apcli0 down)");

			system("(config-powersave.sh cpu 1;config-powersave.sh sdr 1;config-powersave.sh ddr 1;)");
		}
		else
		{
			system("(config-powersave.sh cpu 0;config-powersave.sh sdr 0;config-powersave.sh ddr 0;)");
			//		power_save();

			switch (g_AT_netmode)
			{
			case 1:
				system("(config-powersave.sh ethernet 1 3;config-powersave.sh ethernet 0 4;config-powersave.sh usb 1)");
				system("ifconfig br0 down");
				break;
			case 2:
				system("(config-powersave.sh ethernet 1 3;config-powersave.sh ethernet 1 4;config-powersave.sh usb 1)");
				system("(ifconfig ra0 up;)");
				system("ifconfig br0 down");
				system("ifconfig eth2 down");
				break;
			case 3:
				system("(config-powersave.sh ethernet 1 3;config-powersave.sh ethernet 1 4;config-powersave.sh usb 1)");
				system("(ifconfig ra0 up;brctl addif br0 ra0;)");
				break;
			case 4:
				system("(config-powersave.sh ethernet 1 3;config-powersave.sh ethernet 1 4;config-powersave.sh usb 0)");
				break;
			default:
				if (!strcmp(nvram_bufget(RT2860_NVRAM, "NoEth"), "1"))
					system("(config-powersave.sh ethernet 1 3;config-powersave.sh ethernet 1 4;config-powersave.sh usb 1)");
				else
					system("(config-powersave.sh ethernet 0 3;config-powersave.sh ethernet 0 4;config-powersave.sh usb 0)");
				{
					int OperationMode = atoi(nvram_bufget(RT2860_NVRAM, "OperationMode"));

					switch (OperationMode)
					{
					case 0:
						if (strcmp(nvram_bufget(RT2860_NVRAM, "HLK_APOnOff"), "0"))
							system("(ifconfig ra0 up;brctl addif br0 ra0;)");
						system("(ifconfig apcli0 up;brctl addif br0 apcli0;)");
						break;
					case 1:
						if (strcmp(nvram_bufget(RT2860_NVRAM, "HLK_APOnOff"), "0"))
							system("(ifconfig ra0 up;brctl addif br0 ra0;)");
						break;
					case 2:
						if (strcmp(nvram_bufget(RT2860_NVRAM, "HLK_APOnOff"), "0"))
							system("(ifconfig ra0 up;)");
						break;
					case 3:
						if (strcmp(nvram_bufget(RT2860_NVRAM, "HLK_APOnOff"), "0"))
							system("(ifconfig ra0 up;brctl addif br0 ra0;)");
						system("(ifconfig apcli0 up;)");
						break;
					case 4:
						break;
					default:
						break;
					}
				}
				break;
			}
		}
		sprintf(out, "ok");
	}
	else
	{
		sprintf(out, "error");
	}

	return ret;
}

uchar password_(uchar *comm, uchar *out)
{
	uint ret = 0;

	if ('?' == *(comm + 1))
	{
		//sprintf(out,"%s",nvram_get(SER2NET_NVRAM,"Password"));
		sprintf(out, "");
	}
	else if ('=' == *(comm + 0))
	{
		nvram_set(SER2NET_NVRAM, "Password", (comm + 1));
		sprintf(out, "ok");
	}
	else
	{
		sprintf(out, "error");
	}

	return ret;
}

uchar username_(uchar *comm, uchar *out)
{
	uint ret = 0;

	if ('?' == *(comm + 1))
	{
		sprintf(out, "%s", nvram_get(SER2NET_NVRAM, "Login"));
	}
	else if ('=' == *(comm + 0))
	{
		nvram_set(SER2NET_NVRAM, "Login", (comm + 1));
		sprintf(out, "ok");
	}
	else
	{
		sprintf(out, "error");
	}

	return ret;
}

uchar p2pkey(uchar *comm, uchar *out)
{
	uint ret = 0;

	if ('?' == *(comm + 1))
	{
		sprintf(out, "%s", nvram_get(SER2NET_NVRAM, "p2p5"));
	}
	else if ('=' == *(comm + 0))
	{
		nvram_set(SER2NET_NVRAM, "p2p5", (comm + 1));
		system("killall T2uSvr;killall -9 T2uSvr;");
		sprintf(out, "ok");
	}
	else
	{
		sprintf(out, "error");
	}

	return ret;
}

uchar AT_RBrDhcpc(uchar *comm, uchar *out)
{
	uint ret = 0;

	if ('?' == *(comm + 1))
	{
		sprintf(out, "%s", nvram_get(SER2NET_NVRAM, "brdhcpc"));
		if (!strcmp(out, ""))
		{
			sprintf(out, "0");
		}
	}
	else if ('=' == *(comm + 0))
	{
		nvram_set_(SER2NET_NVRAM, "brdhcpc", (comm + 1));
		sprintf(out, "ok");
	}
	else
	{
		sprintf(out, "error");
	}

	return ret;
}

uchar AT_RNoti(uchar *comm, uchar *out)
{
	uint ret = 0;

	if ('?' == *(comm + 1))
	{
		sprintf(out, "%s", nvram_get(SER2NET_NVRAM, "RNoti"));
		if (!strcmp(out, ""))
		{
			sprintf(out, "0");
		}
	}
	else if ('=' == *(comm + 0))
	{
		nvram_set_(SER2NET_NVRAM, "RNoti", (comm + 1));
		sprintf(out, "ok");
	}
	else
	{
		sprintf(out, "error");
	}

	return ret;
}

uchar AT_RNoti_(uchar *comm, uchar *out)
{
	uint ret = 0;

	if ('?' == *(comm + 1))
	{
		sprintf(out, "");
	}
	else if ('=' == *(comm + 0))
	{
		if (!strcmp(nvram_get(SER2NET_NVRAM, "RNoti"), "1"))
			if (G_serial_mode_get())
				if ('1' == *(comm + 1))
				{
					serial_buf_send("Notice:STA Connected.\r\n", sizeof("Notice:STA Connected.\r\n"));
				}
				else
				{
					serial_buf_send("Notice:STA Disconnected.\r\n", sizeof("Notice:STA Disconnected.\r\n"));
				}

		sprintf(out, "ok");
	}
	else
	{
		sprintf(out, "error");
	}

	return ret;
}

uchar AT_RBrCurIp(uchar *comm, uchar *out)
{
	uint ret = 0;

	if ('?' == *(comm + 1))
	{
		char if_addr[16];
		char if_mask[16];
		char if_gw[16];

		if (-1 == getIfIp("br0", if_addr))
		{
			if_addr[0] = 0;
		}

		if (-1 == getIfNetmask("br0", if_mask))
		{
			if_mask[0] = 0;
		}

		if (-1 == getWanGateway(if_gw))
		{
			if_gw[0] = 0;
		}

		sprintf(out, "%s,%s,%s", if_addr, if_mask, if_gw);
	}
	else if ('=' == *(comm + 0))
	{
		sprintf(out, "error");
	}
	else
	{
		sprintf(out, "error");
	}

	return ret;
}

uchar AT_Rdhcpd(uchar *comm, uchar *out)
{
	uint ret = 0;

	if ('?' == *(comm + 1))
	{
		sprintf(out, "%s", nvram_get(SER2NET_NVRAM, "dhcpEnabled"));
		if (!strcmp(out, ""))
		{
			sprintf(out, "0");
		}
	}
	else if ('=' == *(comm + 0))
	{
		nvram_set_(SER2NET_NVRAM, "dhcpEnabled", (comm + 1));
		sprintf(out, "ok");
	}
	else
	{
		sprintf(out, "error");
	}

	return ret;
}

uchar AT_Raphide(uchar *comm, uchar *out)
{
	uint ret = 0;

	if ('?' == *(comm + 1))
	{
		sprintf(out, "%s", nvram_get(SER2NET_NVRAM, "HideSSID"));
		if (!strcmp(out, ""))
		{
			sprintf(out, "0");
		}
	}
	else if ('=' == *(comm + 0))
	{
		nvram_set_(SER2NET_NVRAM, "HideSSID", (comm + 1));
		sprintf(out, "ok");
	}
	else
	{
		sprintf(out, "error");
	}

	return ret;
}
uchar AT_Rapclient(uchar *comm, uchar *out)
{
	uint ret = 0;

	if ('?' == *(comm + 1))
	{
		sprintf(out, "%s", nvram_get(SER2NET_NVRAM, "ApCliEnable"));
		if (!strcmp(out, ""))
		{
			sprintf(out, "0");
		}
	}
	else if ('=' == *(comm + 0))
	{
		nvram_set_(SER2NET_NVRAM, "ApCliEnable", (comm + 1));
		nvram_set_(SER2NET_NVRAM, "apClient", (comm + 1));
		sprintf(out, "ok");
	}
	else
	{
		sprintf(out, "error");
	}

	return ret;
}

uchar AT_RadioOff(uchar *comm, uchar *out)
{
	uint ret = 0;

	if ('?' == *(comm + 1))
	{
		sprintf(out, "%s", nvram_get(SER2NET_NVRAM, "RadioOff"));
		if (!strcmp(out, ""))
		{
			sprintf(out, "0");
		}
	}
	else if ('=' == *(comm + 0))
	{
		nvram_set(SER2NET_NVRAM, "RadioOff", (comm + 1));
		if ('0' == *(comm + 1))
		{
			system("iwpriv ra0 set RadioOn=1");
		}
		else
		{
			system("iwpriv ra0 set RadioOn=0");
		}
		sprintf(out, "ok");
	}
	else
	{
		sprintf(out, "error");
	}

	return ret;
}

uchar AT_WiFiMode(uchar *comm, uchar *out)
{
	uint ret = 0;

	if ('?' == *(comm + 1))
	{
		sprintf(out, "%s", nvram_get(SER2NET_NVRAM, "WirelessMode"));
	}
	else if ('=' == *(comm + 0))
	{
		uchar *wirelessmode = (comm + 1);

		nvram_set(SER2NET_NVRAM, "WirelessMode", wirelessmode);

		if (!strncmp(wirelessmode, "4", 2) || !strncmp(wirelessmode, "7", 2)) //g, gn
			nvram_bufset(RT2860_NVRAM, "BasicRate", "351");
		else if (!strncmp(wirelessmode, "1", 2)) //b
			nvram_bufset(RT2860_NVRAM, "BasicRate", "3");
		else //bg,bgn,n
			nvram_bufset(RT2860_NVRAM, "BasicRate", "15");
		{
			uchar buf[512];

			sprintf(buf, "iwpriv ra0 set WirelessMode=%s", nvram_get(SER2NET_NVRAM, "WirelessMode"));
			system(buf);
			sprintf(buf, "iwpriv ra0 set BasicRate=%s", nvram_get(SER2NET_NVRAM, "BasicRate"));
			system(buf);
		}

		sprintf(out, "ok");
	}
	else
	{
		sprintf(out, "error");
	}

	return ret;
}

#ifdef SER2NET_COM2
extern nvram_t nvram_2[];
extern int nvram_2_sizeof;

uchar AT_COM2(uchar *comm, uchar *out)
{
	uint ret = 0;

	if ('?' == *(comm + 1))
	{
		uchar buf[128];
		int i;
		int len = nvram_2_sizeof / sizeof(nvram_t);
		out[0] = 0;
		for (i = 0; i < len; i++)
		{
			if (nvram_2[i].type == 's')
			{
				sprintf(buf, "%s=%s\r\n", nvram_2[i].nvram_key, *(uchar **)nvram_2[i].p);
				strcat(out, buf);
			}
			else if (nvram_2[i].type == 'i')
			{
				sprintf(buf, "%s=%d\r\n", nvram_2[i].nvram_key, *(int *)nvram_2[i].p);
				strcat(out, buf);
			}
			else if (nvram_2[i].type == 'u')
			{
				sprintf(buf, "%s=%d\r\n", nvram_2[i].nvram_key, *(uchar *)nvram_2[i].p);
				strcat(out, buf);
			}
		}
	}
	else if ('=' == *(comm + 0))
	{

		sprintf(out, "ok");
	}
	else
	{
		sprintf(out, "error");
	}

	return ret;
}

uchar COM2_ATs(uchar *comm, uchar *out)
{
	uchar ret = 1;
	int i, j;
	uchar *p, *comm_;

	int len = nvram_2_sizeof / sizeof(nvram_t);

	comm_ = comm + 3;
	p = comm_;
	for (j = 0; j < 128; j++, p++)
		if ((0 == *p) || ('=' == *p))
			break;

	for (i = 0; i < len; i++)
	{
		if ((j == strlen(nvram_2[i].nvram_key)) && (0 == memcmp(nvram_2[i].nvram_key, comm_, j)))
		{
			out[0] = 0;
			if ('?' == *(p + 1))
			{
				uchar buf[128];
				if (nvram_2[i].type == 's')
				{
					sprintf(buf, "%s", *(uchar **)nvram_2[i].p);
					strcat(out, buf);
				}
				else if (nvram_2[i].type == 'i')
				{
					sprintf(buf, "%d", *(int *)nvram_2[i].p);
					strcat(out, buf);
				}
				else if (nvram_2[i].type == 'u')
				{
					sprintf(buf, "%d", *(uchar *)nvram_2[i].p);
					strcat(out, buf);
				}
				else if (nvram_2[i].type == 'T')
				{ //tmp not save.
					sprintf(buf, "%d", *(int *)nvram_2[i].p);
					strcat(out, buf);
				}
			}
			else if ('=' == *(p + 0))
			{

				if (nvram_2[i].type == 's')
				{
					*(uchar **)nvram_2[i].p = at_set_value_(*(uchar **)nvram_2[i].p, p + 1);
				}
				else if (nvram_2[i].type == 'i')
				{
					*(int *)nvram_2[i].p = atoi(p + 1);
				}
				else if (nvram_2[i].type == 'u')
				{
					*(uchar *)nvram_2[i].p = atoi(p + 1);
				}
				sprintf(out, "ok");
			}
			else
			{
				sprintf(out, "error");
			}
			ret = 0;
			break;
		}
	}

	return ret;
}

#endif

uchar AT_packlen(uchar *comm, uchar *out)
{
	uint ret = 0;

	if ('?' == *(comm + 1))
	{
		sprintf(out, "%d", g_serial_package_len);
	}
	else if ('=' == *(comm + 0))
	{
		g_serial_package_len = atoi(comm + 1);
		sprintf(out, "ok");
	}
	else
	{
		sprintf(out, "error");
	}

	return ret;
}
uchar AT_packtimeout(uchar *comm, uchar *out)
{
	uint ret = 0;

	if ('?' == *(comm + 1))
	{
		sprintf(out, "%d", g_serial_package_time);
	}
	else if ('=' == *(comm + 0))
	{
		g_serial_package_time = atoi(comm + 1);
		sprintf(out, "ok");
	}
	else
	{
		sprintf(out, "error");
	}

	return ret;
}
uchar AT_packintertimeout(uchar *comm, uchar *out)
{
	uint ret = 0;

	if ('?' == *(comm + 1))
	{
		sprintf(out, "%d", g_serial_package_interval_time);
	}
	else if ('=' == *(comm + 0))
	{
		g_serial_package_interval_time = atoi(comm + 1);
		sprintf(out, "ok");
	}
	else
	{
		sprintf(out, "error");
	}

	return ret;
}

uchar AT_escap(uchar *comm, uchar *out)
{
	uint ret = 0;

	if ('?' == *(comm + 1))
	{
		sprintf(out, "%d", g_transparent_escape_en);
	}
	else if ('=' == *(comm + 0))
	{
		g_transparent_escape_en = atoi(comm + 1);
		sprintf(out, "ok");
	}
	else
	{
		sprintf(out, "error");
	}

	return ret;
}

uchar AT_escap2(uchar *comm, uchar *out)
{
	uint ret = 0;

	if ('?' == *(comm + 1))
	{
		sprintf(out, "%d", g_transparent_escape2_en);
	}
	else if ('=' == *(comm + 0))
	{
		g_transparent_escape2_en = atoi(comm + 1);
		sprintf(out, "ok");
	}
	else
	{
		sprintf(out, "error");
	}

	return ret;
}

uchar AT_escap2_time(uchar *comm, uchar *out)
{
	uint ret = 0;

	if ('?' == *(comm + 1))
	{
		sprintf(out, "%d", g_transparent_escape2_time);
	}
	else if ('=' == *(comm + 0))
	{
		g_transparent_escape2_time = atoi(comm + 1);
		sprintf(out, "ok");
	}
	else
	{
		sprintf(out, "error");
	}

	return ret;
}

extern uchar FLOW_CONTROL_;

uchar AT_FLOW_CONTROL_RTS(uchar *comm, uchar *out)
{
	uint ret = 0;

	if ('?' == *(comm + 1))
	{
		sprintf(out, "%d", FLOW_CONTROL_);
	}
	else if ('=' == *(comm + 0))
	{
		FLOW_CONTROL_ = atoi(comm + 1);
		sprintf(out, "ok");
	}
	else
	{
		sprintf(out, "error");
	}

	return ret;
}

extern uchar XON_XOFF;

uchar AT_XON_XOFF(uchar *comm, uchar *out)
{
	uint ret = 0;

	if ('?' == *(comm + 1))
	{
		sprintf(out, "%d", XON_XOFF);
	}
	else if ('=' == *(comm + 0))
	{
		XON_XOFF = atoi(comm + 1);
		sprintf(out, "ok");
	}
	else
	{
		sprintf(out, "error");
	}

	return ret;
}

uchar config_do();

uchar config_all_save_nvram();

extern uchar Need_config_do;

uchar AT_save(uchar *comm, uchar *out)
{
	if ('1' != *(comm + 1))
	{
		sprintf(out, "");
		//		return;
	}
	TIM_DEBUG("in:%s(%d)\r\n", __FUNCTION__, __LINE__);
	config_all_save_nvram();
	TIM_DEBUG("in:%s(%d)\r\n", __FUNCTION__, __LINE__);
	//config_do();
	Need_config_do = 1;
	TIM_DEBUG("in:%s(%d)\r\n", __FUNCTION__, __LINE__);
	sprintf(out, "ok");
}

uchar AT_restart(uchar *comm, uchar *out)
{
	TIM_DEBUG("in:%s(%d)\r\n", __FUNCTION__, __LINE__);
	//config_do();
	Need_config_do = 1;
	TIM_DEBUG("in:%s(%d)\r\n", __FUNCTION__, __LINE__);
	sprintf(out, "ok");
}

extern uint tim_all_len;
extern uint tim_all_len2;

uchar AT_status(uchar *comm, uchar *out)
{
	uint ret = 0;

	TIM_DEBUG("in:%s(%d)(%d)\r\n", __FUNCTION__, __LINE__, statistics_serial_send);
	TIM_DEBUG("in:%s(%d)(%d)\r\n", __FUNCTION__, __LINE__, tim_all_len);
	TIM_DEBUG("in:%s(%d)(%d)\r\n", __FUNCTION__, __LINE__, tim_all_len2);
}

#ifdef SER2NET_COM2

extern uchar g_s2n_stat;

uchar AT_s2n_stat(uchar *comm, uchar *out)
{
	uint ret = 0;

	if ('?' == *(comm + 1))
	{
		sprintf(out, "%d", g_s2n_stat);
	}
	else if ('=' == *(comm + 0))
	{
		sprintf(out, "error");
	}
	else
	{
		sprintf(out, "error");
	}

	return ret;
}

extern uchar C2_g_s2n_stat;

uchar AT_s2n2_stat(uchar *comm, uchar *out)
{
	uint ret = 0;

	if ('?' == *(comm + 1))
	{
		sprintf(out, "%d", C2_g_s2n_stat);
	}
	else if ('=' == *(comm + 0))
	{
		sprintf(out, "error");
	}
	else
	{
		sprintf(out, "error");
	}

	return ret;
}

#endif

void get_mac1_(uchar *p)
{

	{
		uint base = 0;
		uchar *px;

		volatile uint x = 4;

		base = ((x << 16) + 0x2e); //0x40000

		px = flash_read2(base + 0, 1);
		if (px)
		{
			p[0] = *px;
			free(px);
		}
		px = flash_read2(base + 1, 1);
		if (px)
		{
			p[1] = *px;
			free(px);
		}
		px = flash_read2(base + 2, 1);
		if (px)
		{
			p[2] = *px;
			free(px);
		}
		px = flash_read2(base + 3, 1);
		if (px)
		{
			p[3] = *px;
			free(px);
		}
		px = flash_read2(base + 4, 1);
		if (px)
		{
			p[4] = *px;
			free(px);
		}
		px = flash_read2(base + 5, 1);
		if (px)
		{
			p[5] = *px;
			free(px);
		}
	}
}

void get_mac2_(uchar *p)
{
	{
		uint base = 0;
		uchar *px;

		volatile uint x = 4;

		base = ((x << 16) + 0x28); //0x40000

		px = flash_read2(base + 0, 1);
		if (px)
		{
			p[0] = *px;
			free(px);
		}
		px = flash_read2(base + 1, 1);
		if (px)
		{
			p[1] = *px;
			free(px);
		}
		px = flash_read2(base + 2, 1);
		if (px)
		{
			p[2] = *px;
			free(px);
		}
		px = flash_read2(base + 3, 1);
		if (px)
		{
			p[3] = *px;
			free(px);
		}
		px = flash_read2(base + 4, 1);
		if (px)
		{
			p[4] = *px;
			free(px);
		}
		px = flash_read2(base + 5, 1);
		if (px)
		{
			p[5] = *px;
			free(px);
		}
	}
}

uchar AT_Get_MAC(uchar *comm, uchar *out)
{
	uint ret = 0;

	if ('?' == *(comm + 1))
	{
		uchar mac1[6];
		uchar mac2[6];
		get_mac1_(mac1);
		get_mac2_(mac2);
		sprintf(out, "%02X:%02X:%02X:%02X:%02X:%02X,%02X:%02X:%02X:%02X:%02X:%02X", mac1[0], mac1[1], mac1[2], mac1[3], mac1[4], mac1[5], mac2[0], mac2[1], mac2[2], mac2[3], mac2[4], mac2[5]);
	}
	else if ('=' == *(comm + 0))
	{
		sprintf(out, "error");
	}
	else
	{
		sprintf(out, "error");
	}

	return ret;
}

uchar wifi_ConState(uchar *comm, uchar *out)
{
	uint ret = 0;

	if ('?' == *(comm + 1))
	{
		do
		{
			FILE *fp;
			char buf[512], *p;
			int len;

			sprintf(out, "error");
			if (((int)(fp = popen("iwconfig apcli0 |grep ESSID", "r"))) <= 0)
			{
				TIM_ERROR("ERROR:%s(%d)\r\n", __FUNCTION__, __LINE__);
				break;
			}
			len = fread(buf, 1, sizeof(buf), fp);
			if ((len < 10) || (len >= sizeof(buf)))
			{
				TIM_ERROR("ERROR:%s(%d) %d\r\n", __FUNCTION__, __LINE__, len);
				pclose(fp);
				break;
			}
			pclose(fp);
			buf[len] = 0;
			if (strstr(buf, "ESSID:\"\""))
			{
				sprintf(out, "Disconnected");
				TIM_DEBUG("in:%s(%d)\r\n", __FUNCTION__, __LINE__);
			}
			else
			{
				sprintf(out, "Connected");
				TIM_DEBUG("in:%s(%d)\r\n", __FUNCTION__, __LINE__);
			}
		} while (0);
	}
	else if ('=' == *(comm + 0))
	{
		sprintf(out, "error");
	}
	else
	{
		sprintf(out, "error");
	}

	return ret;
}

uchar wifi_ConStateold(uchar *comm, uchar *out)
{
	uint ret = 0;

	if ('?' == *(comm + 1))
	{
		do
		{
			FILE *fp;
			char buf[512], *p;
			int len;

			sprintf(out, "error");
			if (((int)(fp = popen("iwpriv apcli0 show connStatus", "r"))) <= 0)
			{
				TIM_ERROR("ERROR:%s(%d)\r\n", __FUNCTION__, __LINE__);
				break;
			}
			len = fread(buf, 1, sizeof(buf), fp);
			if ((len < 10) || (len >= sizeof(buf)))
			{
				TIM_ERROR("ERROR:%s(%d) %d\r\n", __FUNCTION__, __LINE__, len);
				pclose(fp);
				break;
			}
			pclose(fp);
			buf[len] = 0;
			if (strstr(buf, "Disconnected"))
			{
				sprintf(out, "Disconnected");
				TIM_DEBUG("in:%s(%d)\r\n", __FUNCTION__, __LINE__);
			}
			else if (strstr(buf, "Connected"))
			{
				sprintf(out, "Connected");
				TIM_DEBUG("in:%s(%d)\r\n", __FUNCTION__, __LINE__);
			}
			else if (strstr(buf, "Invalid command"))
			{
				sprintf(out, "No Connection.");
				TIM_DEBUG("in:%s(%d)\r\n", __FUNCTION__, __LINE__);
			}
			else
			{
				sprintf(out, buf);
				TIM_DEBUG("in:%s(%d)(%s)\r\n", __FUNCTION__, __LINE__, buf);
			}
		} while (0);
	}
	else if ('=' == *(comm + 0))
	{
		sprintf(out, "error");
	}
	else
	{
		sprintf(out, "error");
	}

	return ret;
}
uchar wifi_ConState58(uchar *comm, uchar *out)
{
	uint ret = 0;

	if ('?' == *(comm + 1))
	{
		do
		{
			FILE *fp;
			char buf[512], *p;
			int len;

			sprintf(out, "error");
			if (((int)(fp = popen("iwconfig apclix0 |grep ESSID", "r"))) <= 0)
			{
				TIM_ERROR("ERROR:%s(%d)\r\n", __FUNCTION__, __LINE__);
				break;
			}
			len = fread(buf, 1, sizeof(buf), fp);
			if ((len < 10) || (len >= sizeof(buf)))
			{
				TIM_ERROR("ERROR:%s(%d) %d\r\n", __FUNCTION__, __LINE__, len);
				pclose(fp);
				break;
			}
			pclose(fp);
			buf[len] = 0;
			if (strstr(buf, "ESSID:\"\""))
			{
				sprintf(out, "Disconnected");
				TIM_DEBUG("in:%s(%d)\r\n", __FUNCTION__, __LINE__);
			}
			else
			{
				sprintf(out, "Connected");
				TIM_DEBUG("in:%s(%d)\r\n", __FUNCTION__, __LINE__);
			}
		} while (0);
	}
	else if ('=' == *(comm + 0))
	{
		sprintf(out, "error");
	}
	else
	{
		sprintf(out, "error");
	}

	return ret;
}

uchar wifi_Scan(uchar *comm, uchar *out)
{
	uint ret = 0;

	if ('?' == *(comm + 1))
	{
		sprintf(out, "error");
		do
		{

			FILE *fp;
			char buf[4096], *ptr;
			char channel[4], ssid[186], bssid[20], security[23];
			char signal[9], mode[7], ext_ch[7], net_type[3];
			int i, space_start;

			system("iwpriv apcli0 set SiteSurvey=1");
			sleep(4);
			if (((int)(fp = popen("iwpriv apcli0 get_site_survey", "r"))) <= 0)
			{
				fprintf(stderr, "%s: execute get_site_survey failed !\n", __func__);
				break;
			}
			/*

					iwpriv apcli0 set SiteSurvey=1;iwpriv apcli0 get_site_survey

					
# iwpriv ra0 set SiteSurvey=1;iwpriv ra0 get_site_survey
					ra0 	  get_site_survey:
					Ch	SSID							 BSSID				 Security				Siganl(%)W-Mode  ExtCH	NT WPS DPID
					4	HI-LINK 						 5c:63:bf:d9:3e:da	 WPA2PSK/AES			15		 11b/g/n NONE	In YES	   
					10	ChinaNet-5Yyp					 00:25:12:74:39:09	 WPAPSK/TKIP			0		 11b/g	 NONE	In YES	   
					1	HI-LINK_098C					 48:02:2a:00:09:8b	 WPA1PSKWPA2PSK/AES 	5		 11b/g/n NONE	In YES	   
			
					*/

			memset(buf, 0, sizeof(buf));
			fgets(buf, sizeof(buf), fp);
			//			fgets(buf, sizeof(buf), fp);
			//sprintf(out,1024*3,);
			fread(buf, 1, SERIAL_BUF_LEN - 1, fp);

			pclose(fp);

			sprintf(out, "");
			{

				uchar *p1, *p2;
				p1 = buf;
				while (1)
				{
					p2 = strstr(p1, "\n");
					if (p2)
					{
						*p2 = 0;
						if ((strlen(out) + strlen(p1) + 2) > (SERIAL_BUF_LEN - 1))
							break;
						strcat(out, p1);
						strcat(out, "\r\n");
						p1 = p2 + 1;
					}
					else
					{
						strcat(out, p1);
						break;
					}
				}
			}

		} while (0);
	}
	else if ('=' == *(comm + 0))
	{
		sprintf(out, "error");
	}
	else
	{
		sprintf(out, "error");
	}

	return ret;
}

uchar wifi_Scan58(uchar *comm, uchar *out)
{
	uint ret = 0;

	if ('?' == *(comm + 1))
	{
		sprintf(out, "error");
		do
		{

			FILE *fp;
			char buf[4096], *ptr;
			char channel[4], ssid[186], bssid[20], security[23];
			char signal[9], mode[7], ext_ch[7], net_type[3];
			int i, space_start;

			system("iwpriv apclix0 set SiteSurvey=1");
			sleep(4);
			if (((int)(fp = popen("iwpriv apclix0 get_site_survey", "r"))) <= 0)
			{
				fprintf(stderr, "%s: execute get_site_survey failed !\n", __func__);
				break;
			}
			/*
# iwpriv ra0 set SiteSurvey=1;iwpriv ra0 get_site_survey
					ra0 	  get_site_survey:
					Ch	SSID							 BSSID				 Security				Siganl(%)W-Mode  ExtCH	NT WPS DPID
					4	HI-LINK 						 5c:63:bf:d9:3e:da	 WPA2PSK/AES			15		 11b/g/n NONE	In YES	   
					10	ChinaNet-5Yyp					 00:25:12:74:39:09	 WPAPSK/TKIP			0		 11b/g	 NONE	In YES	   
					1	HI-LINK_098C					 48:02:2a:00:09:8b	 WPA1PSKWPA2PSK/AES 	5		 11b/g/n NONE	In YES	   
			
					*/

			memset(buf, 0, sizeof(buf));
			fgets(buf, sizeof(buf), fp);
			//			fgets(buf, sizeof(buf), fp);
			//sprintf(out,1024*3,);
			fread(buf, 1, SERIAL_BUF_LEN - 1, fp);

			pclose(fp);

			sprintf(out, "");
			{

				uchar *p1, *p2;
				p1 = buf;
				while (1)
				{
					p2 = strstr(p1, "\n");
					if (p2)
					{
						*p2 = 0;
						if ((strlen(out) + strlen(p1) + 2) > (SERIAL_BUF_LEN - 1))
							break;
						strcat(out, p1);
						strcat(out, "\r\n");
						p1 = p2 + 1;
					}
					else
					{
						strcat(out, p1);
						break;
					}
				}
			}

		} while (0);
	}
	else if ('=' == *(comm + 0))
	{
		sprintf(out, "error");
	}
	else
	{
		sprintf(out, "error");
	}

	return ret;
}

uchar AT_error(uchar *comm, uchar *out)
{
	sprintf(out, "ERROR");
}

uchar AT_netmode(uchar *comm, uchar *out)
{
	uint ret = 0;

	if ('?' == *(comm + 1))
	{
		sprintf(out, "%d", g_AT_netmode);
	}
	else if ('=' == *(comm + 0))
	{
		TIM_DEBUG("in:%s(%d)(%s\r\n", __FUNCTION__, __LINE__, comm);
		g_AT_netmode = atoi(comm + 1);
		TIM_DEBUG("in:%s(%d)(%d\r\n", __FUNCTION__, __LINE__, g_AT_netmode);
		sprintf(out, "ok");
	}
	else
	{
		sprintf(out, "error");
	}

	return ret;
}

uchar AT_wifi_conf(uchar *comm, uchar *out)
{
	uint ret = 0;

	if ('?' == *(comm + 1))
	{
		if (g_AT_wifi_conf)
			sprintf(out, "%s", g_AT_wifi_conf);
	}
	else if ('=' == *(comm + 0))
	{
		g_AT_wifi_conf = at_set_value_(g_AT_wifi_conf, (comm + 1));
		TIM_DEBUG("in:%s(%d) (%s)\r\n", __FUNCTION__, __LINE__, g_AT_wifi_conf);

		sprintf(out, "ok");
	}
	else
	{
		sprintf(out, "error");
	}

	return ret;
}

uchar AT_discovery_udp_name(uchar *comm, uchar *out)
{
	uint ret = 0;

	if ('?' == *(comm + 1))
	{
		if (g_AT_discovery_udp_name)
			sprintf(out, "%s", g_AT_discovery_udp_name);
	}
	else if ('=' == *(comm + 0))
	{
		g_AT_discovery_udp_name = at_set_value_(g_AT_discovery_udp_name, (comm + 1));
		TIM_DEBUG("in:%s(%d) (%s)\r\n", __FUNCTION__, __LINE__, g_AT_discovery_udp_name);

		sprintf(out, "ok");
	}
	else
	{
		sprintf(out, "error");
	}

	return ret;
}

uchar AT_web_port(uchar *comm, uchar *out)
{
	uint ret = 0;

	if ('?' == *(comm + 1))
	{
		char *web_port = nvram_get(SER2NET_NVRAM, "web_port");
		if (0 == strcmp(web_port, ""))
		{
			sprintf(out, "80");
		}
		else
		{
			sprintf(out, "%s", web_port);
		}
	}
	else if ('=' == *(comm + 0))
	{

		nvram_set(SER2NET_NVRAM, "web_port", (comm + 1));
		sprintf(out, "ok");
	}
	else
	{
		sprintf(out, "error");
	}

	return ret;
}

uchar AT_flash1(uchar *comm, uchar *out)
{
	uint ret = 0;

	if ('?' == *(comm + 1))
	{
		char *web_port = nvram_get(SER2NET_NVRAM, "flash1");
		sprintf(out, "%s", web_port);
	}
	else if ('=' == *(comm + 0))
	{
		nvram_set(SER2NET_NVRAM, "flash1", (comm + 1));
		sprintf(out, "ok");
	}
	else
	{
		sprintf(out, "error");
	}

	return ret;
}

uchar AT_flash2(uchar *comm, uchar *out)
{
	uint ret = 0;

	if ('?' == *(comm + 1))
	{
		char *web_port = nvram_get(SER2NET_NVRAM, "flash2");
		sprintf(out, "%s", web_port);
	}
	else if ('=' == *(comm + 0))
	{
		nvram_set(SER2NET_NVRAM, "flash2", (comm + 1));
		sprintf(out, "ok");
	}
	else
	{
		sprintf(out, "error");
	}

	return ret;
}

uchar AT_flash3(uchar *comm, uchar *out)
{
	uint ret = 0;

	if ('?' == *(comm + 1))
	{
		char *web_port = nvram_get(SER2NET_NVRAM, "flash3");
		sprintf(out, "%s", web_port);
	}
	else if ('=' == *(comm + 0))
	{
		nvram_set(SER2NET_NVRAM, "flash3", (comm + 1));
		sprintf(out, "ok");
	}
	else
	{
		sprintf(out, "error");
	}

	return ret;
}

uchar AT_ROperMode(uchar *comm, uchar *out)
{
	uint ret = 0;

	if ('?' == *(comm + 1))
	{
		sprintf(out, "%s", nvram_get(SER2NET_NVRAM, "OperationMode"));
	}
	else if ('=' == *(comm + 0))
	{
		uchar buf[64];
		int mode = atoi(comm + 1);

		int oldmode = atoi(nvram_get(SER2NET_NVRAM, "OperationMode"));

		if (mode != oldmode)
		{
			switch (mode)
			{
			case 0:
				system("ralink_init renew 2860 /etc_ro/Wireless/RT2860AP/AP_bridge_sta");
				break;
			case 1:
				break;
			case 2:
				system("ralink_init renew 2860 /etc_ro/Wireless/RT2860AP/AP_sta");
				break;
			case 3:
				system("ralink_init renew 2860 /etc_ro/Wireless/RT2860AP/AP_apclient_sta");
				break;
			default:
				break;
			}

			if ((mode >= 0) && (mode <= 3))
			{
				nvram_close(SER2NET_NVRAM);
				nvram_init(SER2NET_NVRAM);
				sprintf(buf, "%d", mode);
				nvram_set(SER2NET_NVRAM, "OperationMode", buf);
			}
		}

		sprintf(out, "ok");
	}
	else
	{
		sprintf(out, "error");
	}

	return ret;
}

uchar AT_RstaChe(uchar *comm, uchar *out)
{
	uint ret = 0;

	if ('?' == *(comm + 1))
	{
		sprintf(out, "error");
	}
	else if ('=' == *(comm + 0))
	{

		if ('1' == *(comm + 1))
			system("echo 1 >/tmp/hlk_sta_check");
		else
			system("rm /tmp/hlk_sta_check");

		sprintf(out, "ok");
	}
	else
	{
		sprintf(out, "error");
	}

	return ret;
}

uchar AT_RstaRec(uchar *comm, uchar *out)
{
	uint ret = 0;

	if ('?' == *(comm + 1))
	{
		sprintf(out, "error");
	}
	else if ('=' == *(comm + 0))
	{

		system("echo 1 >/tmp/hlk_wifi_reconn");

		sprintf(out, "ok");
	}
	else
	{
		sprintf(out, "error");
	}

	return ret;
}

uchar AT_RstaRec2(uchar *comm, uchar *out)
{
	uint ret = 0;

	if ('?' == *(comm + 1))
	{
		sprintf(out, "error");
	}
	else if ('=' == *(comm + 0))
	{

		system("iwpriv apcli0 set ApCliEnable=1");

		sprintf(out, "ok");
	}
	else
	{
		sprintf(out, "error");
	}

	return ret;
}

uchar AT_RLANIp(uchar *comm, uchar *out)
{
	uint ret = 0;

	if ('?' == *(comm + 1))
	{
		sprintf(out, "%s,%s", nvram_get(SER2NET_NVRAM, "lan_ipaddr"), nvram_get(SER2NET_NVRAM, "lan_netmask"));
	}
	else if ('=' == *(comm + 0))
	{
		uchar buf[64];

		do
		{ //net ip
			uchar ip[32];
			uchar mask[32];
			uchar gw[32];
			uchar *p1, *p2;

			TIM_DEBUG("in:%s(%d)\r\n", __FUNCTION__, __LINE__);

			p1 = (comm + 1);

			p2 = strchr(p1, ',');
			if (0 == p2)
			{
				TIM_ERROR("in:%s(%d)\r\n", __FUNCTION__, __LINE__);
				break;
			}
			snprintf(ip, MIN_(p2 - p1 + 1, sizeof(ip) - 1), p1);

			p1 = p2 + 1;
			snprintf(mask, sizeof(mask) - 1, p1);

			TIM_ERROR("in:%s(%d) (%s) (%s) (%s)\r\n", __FUNCTION__, __LINE__, ip, mask, gw);

			{
				nvram_set_(SER2NET_NVRAM, "lan_ipaddr", ip);
				nvram_set_(SER2NET_NVRAM, "lan_netmask", mask);
				//			nvram_set_(SER2NET_NVRAM,"wan_gateway",gw);
			}

			TIM_DEBUG("in:%s(%d)\r\n", __FUNCTION__, __LINE__);
		} while (0);

		sprintf(out, "ok");
	}
	else
	{
		sprintf(out, "error");
	}

	return ret;
}

uchar AT_Rdhcpd_ip(uchar *comm, uchar *out)
{
	uint ret = 0;

	if ('?' == *(comm + 1))
	{
		sprintf(out, "%s,%s,%s,%s", nvram_get(SER2NET_NVRAM, "dhcpStart"), nvram_get(SER2NET_NVRAM, "dhcpEnd"), nvram_get(SER2NET_NVRAM, "dhcpMask"), nvram_get(SER2NET_NVRAM, "dhcpGateway"));
	}
	else if ('=' == *(comm + 0))
	{
		uchar buf[64];

		do
		{ //dhcpd ip
			uchar ip[32];
			uchar ip2[32];
			uchar mask[32];
			uchar gw[32];
			uchar *p1, *p2;

			TIM_DEBUG("in:%s(%d)\r\n", __FUNCTION__, __LINE__);

			p1 = (comm + 1);

			p2 = strchr(p1, ',');
			if (0 == p2)
			{
				TIM_ERROR("in:%s(%d)\r\n", __FUNCTION__, __LINE__);
				break;
			}
			snprintf(ip, MIN_(p2 - p1 + 1, sizeof(ip) - 1), p1);

			p1 = p2 + 1;
			p2 = strchr(p1, ',');
			if (0 == p2)
			{
				TIM_ERROR("in:%s(%d)\r\n", __FUNCTION__, __LINE__);
				break;
			}
			snprintf(ip2, MIN_(p2 - p1 + 1, sizeof(ip2) - 1), p1);

			p1 = p2 + 1;
			p2 = strchr(p1, ',');
			if (0 == p2)
			{
				TIM_ERROR("in:%s(%d)\r\n", __FUNCTION__, __LINE__);
				break;
			}
			snprintf(mask, MIN_(p2 - p1 + 1, sizeof(mask) - 1), p1);

			p1 = p2 + 1;
			snprintf(gw, sizeof(gw) - 1, p1);

			TIM_DEBUG("in:%s(%d) (%s) (%s) (%s)\r\n", __FUNCTION__, __LINE__, ip, mask, gw);

			{
				nvram_set_(SER2NET_NVRAM, "dhcpStart", ip);
				nvram_set_(SER2NET_NVRAM, "dhcpEnd", ip2);
				nvram_set_(SER2NET_NVRAM, "dhcpMask", mask);
				nvram_set_(SER2NET_NVRAM, "dhcpGateway", gw);
			}

			TIM_DEBUG("in:%s(%d)\r\n", __FUNCTION__, __LINE__);
		} while (0);

		sprintf(out, "ok");
	}
	else
	{
		sprintf(out, "error");
	}

	return ret;
}

uchar AT_RapWIFI(uchar *comm, uchar *out)
{
	uint ret = 0;

	if ('?' == *(comm + 1))
	{
		uchar ty[32];

		uchar *auth;
		uchar *enc;

		auth = nvram_get(SER2NET_NVRAM, "AuthMode");
		enc = nvram_get(SER2NET_NVRAM, "EncrypType");

		sprintf(ty, "");

		if (0 == strcmp(auth, "OPEN"))
		{
			sprintf(ty, "none");
		}
		else if (0 == strcmp(enc, "WEP"))
		{
			if (0 == strcmp(auth, "OPEN"))
			{
				sprintf(ty, "wep_open");
			}
			else if (0 == strcmp(enc, "SHARED"))
			{
				sprintf(ty, "wep");
			}
		}
		else if (0 == strcmp(auth, "WPAPSK"))
		{
			if (0 == strcmp(enc, "TKIP"))
			{
				sprintf(ty, "wpa_tkip");
			}
			else if (0 == strcmp(enc, "AES"))
			{
				sprintf(ty, "wpa_aes");
			}
		}
		else if (0 == strcmp(auth, "WPA2PSK"))
		{
			if (0 == strcmp(enc, "TKIP"))
			{
				sprintf(ty, "wpa2_tkip");
			}
			else if (0 == strcmp(enc, "AES"))
			{
				sprintf(ty, "wpa2_aes");
			}
		}
		else if (0 == strcmp(auth, "WPAPSKWPA2PSK"))
		{
			if (0 == strcmp(enc, "TKIP"))
			{
				sprintf(ty, "wpawpa2_tkip");
			}
			else if (0 == strcmp(enc, "AES"))
			{
				sprintf(ty, "wpawpa2_aes");
			}
		}

		sprintf(out, "%s,%s,%s", nvram_get(SER2NET_NVRAM, "SSID1"), ty, nvram_get(SER2NET_NVRAM, "WPAPSK1"));
	}
	else if ('=' == *(comm + 0))
	{
		uchar buf[64];

		do
		{ //wifi conf
			uchar ssid[32];
			uchar keytype[32];
			uchar key[32];
			uchar *p1, *p2;

			TIM_DEBUG("in:%s(%d)\r\n", __FUNCTION__, __LINE__);

			p1 = (comm + 1);

			p2 = strchr(p1, ',');
			if (0 == p2)
			{
				TIM_ERROR("in:%s(%d)\r\n", __FUNCTION__, __LINE__);
				break;
			}
			snprintf(ssid, MIN_(p2 - p1 + 1, sizeof(ssid)), p1);

			p1 = p2 + 1;
			p2 = strchr(p1, ',');
			if (0 == p2)
			{
				TIM_ERROR("in:%s(%d)\r\n", __FUNCTION__, __LINE__);
				break;
			}
			snprintf(keytype, MIN_(p2 - p1 + 1, sizeof(keytype)), p1);

			p1 = p2 + 1;
			snprintf(key, sizeof(key) - 1, p1);

			TIM_DEBUG("in:%s(%d) (%s)  (%s) (%s) (%s)\r\n", __FUNCTION__, __LINE__, g_AT_wifi_conf, ssid, keytype, key);

			{
				nvram_set_(SER2NET_NVRAM, "SSID1", ssid);

				if (0 == strcmp(keytype, "none"))
				{
					nvram_set_(SER2NET_NVRAM, "AuthMode", "OPEN");
					nvram_set_(SER2NET_NVRAM, "EncrypType", "NONE");
				}
				else if (0 == strcmp(keytype, "wep"))
				{

					nvram_set_(SER2NET_NVRAM, "AuthMode", "SHARED");
					nvram_set_(SER2NET_NVRAM, "EncrypType", "WEP");
					nvram_set_(SER2NET_NVRAM, "DefaultKeyID", "1");
					nvram_set_(SER2NET_NVRAM, "Key1Type", "1");
					nvram_set_(SER2NET_NVRAM, "Key1Str1", key);
				}
				else if (0 == strcmp(keytype, "wep_open"))
				{

					nvram_set_(SER2NET_NVRAM, "AuthMode", "OPEN");
					nvram_set_(SER2NET_NVRAM, "EncrypType", "WEP");
					nvram_set_(SER2NET_NVRAM, "DefaultKeyID", "1");
					nvram_set_(SER2NET_NVRAM, "Key1Type", "1");
					nvram_set_(SER2NET_NVRAM, "Key1Str1", key);
				}
				else if (0 == strcmp(keytype, "wpa_tkip"))
				{
					nvram_set_(SER2NET_NVRAM, "AuthMode", "WPAPSK");
					nvram_set_(SER2NET_NVRAM, "EncrypType", "TKIP");
					nvram_set_(SER2NET_NVRAM, "WPAPSK1", key);
				}
				else if (0 == strcmp(keytype, "wpa_aes"))
				{
					nvram_set_(SER2NET_NVRAM, "AuthMode", "WPAPSK");
					nvram_set_(SER2NET_NVRAM, "EncrypType", "AES");
					nvram_set_(SER2NET_NVRAM, "WPAPSK1", key);
				}
				else if (0 == strcmp(keytype, "wpa2_tkip"))
				{
					nvram_set_(SER2NET_NVRAM, "AuthMode", "WPA2PSK");
					nvram_set_(SER2NET_NVRAM, "EncrypType", "TKIP");
					nvram_set_(SER2NET_NVRAM, "WPAPSK1", key);
				}
				else if (0 == strcmp(keytype, "wpa2_aes"))
				{
					nvram_set_(SER2NET_NVRAM, "AuthMode", "WPA2PSK");
					nvram_set_(SER2NET_NVRAM, "EncrypType", "AES");
					nvram_set_(SER2NET_NVRAM, "WPAPSK1", key);
				}
				else if (0 == strcmp(keytype, "wpawpa2_tkip"))
				{
					nvram_set_(SER2NET_NVRAM, "AuthMode", "WPAPSKWPA2PSK");
					nvram_set_(SER2NET_NVRAM, "EncrypType", "TKIP");
					nvram_set_(SER2NET_NVRAM, "WPAPSK1", key);
				}
				else if (0 == strcmp(keytype, "wpawpa2_aes"))
				{
					nvram_set_(SER2NET_NVRAM, "AuthMode", "WPAPSKWPA2PSK");
					nvram_set_(SER2NET_NVRAM, "EncrypType", "AES");
					nvram_set_(SER2NET_NVRAM, "WPAPSK1", key);
				}
				else
				{
					nvram_set_(SER2NET_NVRAM, "AuthMode", "WPAPSKWPA2PSK");
					nvram_set_(SER2NET_NVRAM, "EncrypType", "AES");
					nvram_set_(SER2NET_NVRAM, "WPAPSK1", key);
				}

				nvram_set_(SER2NET_NVRAM, "RekeyMethod", "TIME");
			}

			TIM_DEBUG("in:%s(%d)\r\n", __FUNCTION__, __LINE__);
		} while (0);

		sprintf(out, "ok");
	}
	else
	{
		sprintf(out, "error");
	}

	return ret;
}

uchar AT_RapWIFI2(uchar *comm, uchar *out)
{
	uint ret = 0;

	if ('?' == *(comm + 1))
	{
		uchar ty[32];

		uchar *auth;
		uchar *enc;

		auth = nvram_get(SER2NET_NVRAM, "AuthMode");
		enc = nvram_get(SER2NET_NVRAM, "EncrypType");

		sprintf(ty, "");

		if (0 == strcmp(auth, "OPEN"))
		{
			sprintf(ty, "none");
		}
		else if (0 == strcmp(enc, "WEP"))
		{
			if (0 == strcmp(auth, "OPEN"))
			{
				sprintf(ty, "wep_open");
			}
			else if (0 == strcmp(enc, "SHARED"))
			{
				sprintf(ty, "wep");
			}
		}
		else if (0 == strcmp(auth, "WPAPSK"))
		{
			if (0 == strcmp(enc, "TKIP"))
			{
				sprintf(ty, "wpa_tkip");
			}
			else if (0 == strcmp(enc, "AES"))
			{
				sprintf(ty, "wpa_aes");
			}
		}
		else if (0 == strcmp(auth, "WPA2PSK"))
		{
			if (0 == strcmp(enc, "TKIP"))
			{
				sprintf(ty, "wpa2_tkip");
			}
			else if (0 == strcmp(enc, "AES"))
			{
				sprintf(ty, "wpa2_aes");
			}
		}
		else if (0 == strcmp(auth, "WPAPSKWPA2PSK"))
		{
			if (0 == strcmp(enc, "TKIP"))
			{
				sprintf(ty, "wpawpa2_tkip");
			}
			else if (0 == strcmp(enc, "AES"))
			{
				sprintf(ty, "wpawpa2_aes");
			}
		}

		sprintf(out, "%s,%s,%s", nvram_get(SER2NET_NVRAM, "SSID1"), ty, nvram_get(SER2NET_NVRAM, "WPAPSK1"));
	}
	else if ('=' == *(comm + 0))
	{
		uchar buf[64];

		do
		{ //wifi conf
			uchar ssid[32];
			uchar keytype[32];
			uchar key[32];
			uchar *p1, *p2;
			uchar s_len;

			TIM_DEBUG("in:%s(%d)\r\n", __FUNCTION__, __LINE__);

			p1 = (comm + 1);
			s_len = atoi(p1);

			p2 = strchr(p1, ',');
			if (0 == p2)
			{
				TIM_ERROR("in:%s(%d)\r\n", __FUNCTION__, __LINE__);
				break;
			}
			p1 = p2 + 1;

			if ((s_len + 2) > strlen(p1))
			{
				TIM_ERROR("in:%s(%d)\r\n", __FUNCTION__, __LINE__);
				break;
			}

			p2 = p1 + s_len;
			snprintf(ssid, MIN_(p2 - p1 + 1, sizeof(ssid)), p1);

			p1 = p2 + 1;
			p2 = strchr(p1, ',');
			if (0 == p2)
			{
				TIM_ERROR("in:%s(%d)\r\n", __FUNCTION__, __LINE__);
				break;
			}
			snprintf(keytype, MIN_(p2 - p1 + 1, sizeof(keytype)), p1);

			p1 = p2 + 1;
			snprintf(key, sizeof(key) - 1, p1);

			TIM_DEBUG("in:%s(%d) (%s)  (%s) (%s) (%s)\r\n", __FUNCTION__, __LINE__, g_AT_wifi_conf, ssid, keytype, key);

			{
				nvram_set_(SER2NET_NVRAM, "SSID1", ssid);

				if (0 == strcmp(keytype, "none"))
				{
					nvram_set_(SER2NET_NVRAM, "AuthMode", "OPEN");
					nvram_set_(SER2NET_NVRAM, "EncrypType", "NONE");
				}
				else if (0 == strcmp(keytype, "wep"))
				{

					nvram_set_(SER2NET_NVRAM, "AuthMode", "SHARED");
					nvram_set_(SER2NET_NVRAM, "EncrypType", "WEP");
					nvram_set_(SER2NET_NVRAM, "DefaultKeyID", "1");
					nvram_set_(SER2NET_NVRAM, "Key1Type", "1");
					nvram_set_(SER2NET_NVRAM, "Key1Str1", key);
				}
				else if (0 == strcmp(keytype, "wep_open"))
				{

					nvram_set_(SER2NET_NVRAM, "AuthMode", "OPEN");
					nvram_set_(SER2NET_NVRAM, "EncrypType", "WEP");
					nvram_set_(SER2NET_NVRAM, "DefaultKeyID", "1");
					nvram_set_(SER2NET_NVRAM, "Key1Type", "1");
					nvram_set_(SER2NET_NVRAM, "Key1Str1", key);
				}
				else if (0 == strcmp(keytype, "wpa_tkip"))
				{
					nvram_set_(SER2NET_NVRAM, "AuthMode", "WPAPSK");
					nvram_set_(SER2NET_NVRAM, "EncrypType", "TKIP");
					nvram_set_(SER2NET_NVRAM, "WPAPSK1", key);
				}
				else if (0 == strcmp(keytype, "wpa_aes"))
				{
					nvram_set_(SER2NET_NVRAM, "AuthMode", "WPAPSK");
					nvram_set_(SER2NET_NVRAM, "EncrypType", "AES");
					nvram_set_(SER2NET_NVRAM, "WPAPSK1", key);
				}
				else if (0 == strcmp(keytype, "wpa2_tkip"))
				{
					nvram_set_(SER2NET_NVRAM, "AuthMode", "WPA2PSK");
					nvram_set_(SER2NET_NVRAM, "EncrypType", "TKIP");
					nvram_set_(SER2NET_NVRAM, "WPAPSK1", key);
				}
				else if (0 == strcmp(keytype, "wpa2_aes"))
				{
					nvram_set_(SER2NET_NVRAM, "AuthMode", "WPA2PSK");
					nvram_set_(SER2NET_NVRAM, "EncrypType", "AES");
					nvram_set_(SER2NET_NVRAM, "WPAPSK1", key);
				}
				else if (0 == strcmp(keytype, "wpawpa2_tkip"))
				{
					nvram_set_(SER2NET_NVRAM, "AuthMode", "WPAPSKWPA2PSK");
					nvram_set_(SER2NET_NVRAM, "EncrypType", "TKIP");
					nvram_set_(SER2NET_NVRAM, "WPAPSK1", key);
				}
				else if (0 == strcmp(keytype, "wpawpa2_aes"))
				{
					nvram_set_(SER2NET_NVRAM, "AuthMode", "WPAPSKWPA2PSK");
					nvram_set_(SER2NET_NVRAM, "EncrypType", "AES");
					nvram_set_(SER2NET_NVRAM, "WPAPSK1", key);
				}
				else
				{
					nvram_set_(SER2NET_NVRAM, "AuthMode", "WPAPSKWPA2PSK");
					nvram_set_(SER2NET_NVRAM, "EncrypType", "AES");
					nvram_set_(SER2NET_NVRAM, "WPAPSK1", key);
				}

				nvram_set_(SER2NET_NVRAM, "RekeyMethod", "TIME");
			}

			TIM_DEBUG("in:%s(%d)\r\n", __FUNCTION__, __LINE__);
		} while (0);

		sprintf(out, "ok");
	}
	else
	{
		sprintf(out, "error");
	}

	return ret;
}

uchar AT_RstaStatus(uchar *comm, uchar *out)
{
	uint ret = 0;

	if ('?' == *(comm + 1))
	{
		int mode = atoi(nvram_get(SER2NET_NVRAM, "OperationMode"));

		switch (mode)
		{
		case 2:
			out[0] = 0;
			wifi_ConState(comm, out);
			{

				if (strstr(out, "Connected"))
				{
					sprintf(out, "Connected");
					TIM_DEBUG("in:%s(%d)\r\n", __FUNCTION__, __LINE__);

					/*
ra0       Ralink STA  ESSID:"HI-LINK_D879"  Nickname:"RT2860STA"
          Mode:Managed  Frequency=2.412 GHz  Access Point: 44:33:4C:B1:D8:78   
          Bit Rate=58.5 Mb/s   
          RTS thr:off   Fragment thr:off
          Encryption key:5621-EACF-2C40-6E6B-3D61-FE96-59A8-8D20 [3]   Security mode:restricted   Security mode:open
          Link Quality=67/100  Signal level:-72 dBm  Noise level:-78 dBm
          Rx invalid nwid:0  Rx invalid crypt:0  Rx invalid frag:0
          Tx excessive retries:0  Invalid misc:0   Missed beacon:0


*/
					{
						uchar mac[64];
						uchar i;

						mac[0] = 0;
						do
						{
							FILE *fp;
							char buf[512], *p;
							int len;

							//		sprintf(out,"error");
							if (((int)(fp = popen("iwconfig ra0|grep \"Access Point: \"", "r"))) <= 0)
							{
								TIM_ERROR("ERROR:%s(%d)\r\n", __FUNCTION__, __LINE__);
								break;
							}
							len = fread(buf, 1, sizeof(buf), fp);
							if ((len < 10) || (len >= sizeof(buf)))
							{
								TIM_ERROR("ERROR:%s(%d)\r\n", __FUNCTION__, __LINE__);
								pclose(fp);
								break;
							}
							pclose(fp);
							buf[len] = 0;
							if (strstr(buf, "Access Point: "))
							{
								sprintf(mac, "%s", (strstr(buf, "Access Point: ") + strlen("Access Point: ")));
							}
							else
							{
								//			sprintf(out,"Disconnected");
								TIM_DEBUG("in:%s(%d)\r\n", __FUNCTION__, __LINE__);
							}
						} while (0);

						mac[sizeof(mac) - 1] = 0;

						for (i = 0; i < sizeof(mac); i++)
						{
							if ((mac[i] >= 'A') && (mac[i] <= 'Z'))
							{
								mac[i] = mac[i] - 'A' + 'a';
							}
							if (((mac[i] >= '0') && (mac[i] <= '9')) || ((mac[i] >= 'a') && (mac[i] <= 'f')) || ((mac[i] >= 'A') && (mac[i] <= 'F')) || ((mac[i] == ':')))
							{
							}
							else
							{
								mac[i] = 0;
								break;
							}
							if (0 == mac[i])
								break;
						}
						TIM_ERROR("ERROR:%s(%d)  (%s)\r\n", __FUNCTION__, __LINE__, mac);
						TIM_ERROR("ERROR:%s(%d)  (%s)\r\n", __FUNCTION__, __LINE__, out);

						system("iwpriv ra0 set SiteSurvey=1");

						do
						{
							FILE *fp;
							char buf[512], *p;
							int len;

							if (!(fp = popen("iwpriv ra0 get_site_survey", "r")))
							{
								TIM_ERROR("in:%s(%d)\r\n", __FUNCTION__, __LINE__);
								break;
							}

							memset(buf, 0, sizeof(buf));
							fgets(buf, sizeof(buf), fp);
							fgets(buf, sizeof(buf), fp);
							while (fgets(buf, sizeof(buf), fp))
							{
								if (strlen(buf) < 4)
									break;
								/*
# iwpriv ra0 set SiteSurvey=1;iwpriv ra0 get_site_survey
ra0       get_site_survey:
Ch  SSID                             BSSID               Security               Siganl(%)W-Mode  ExtCH  NT WPS DPID
4   HI-LINK                          5c:63:bf:d9:3e:da   WPA2PSK/AES            15       11b/g/n NONE   In YES     
10  ChinaNet-5Yyp                    00:25:12:74:39:09   WPAPSK/TKIP            0        11b/g   NONE   In YES     
1   HI-LINK_098C                     48:02:2a:00:09:8b   WPA1PSKWPA2PSK/AES     5        11b/g/n NONE   In YES     

*/
								if (strstr(buf, mac))
								{
									TIM_ERROR("ERROR:%s(%d)  (%s)\r\n", __FUNCTION__, __LINE__, out);
									strcat(out, "\r\n");
									TIM_ERROR("ERROR:%s(%d)  (%s)\r\n", __FUNCTION__, __LINE__, out);
									strcat(out, buf);
									TIM_ERROR("ERROR:%s(%d)  (%s)\r\n", __FUNCTION__, __LINE__, out);
									break;
								}
							}
							pclose(fp);

						} while (0);
					}
				}
				else
				{
				}
			}

			break;
		case 0:
		case 3:
		{
			char buf[4096];
			char buf2[4096];

			system("iwpriv apcli0 show connStatus");

			{
				FILE *fp;

				uchar connected = 0;

				uchar *p;
				int len;
				if (!(fp = popen("logread", "r")))
				{
					TIM_ERROR("in:%s(%d)\r\n", __FUNCTION__, __LINE__);
					return 0;
				}

				memset(buf, 0, sizeof(buf));

				while (fgets(buf, sizeof(buf), fp))
				{
					if (strstr(buf, "ApCli0 Connected AP : "))
					{
						if (strstr(buf, "ApCli0 Connected AP : Disconnect"))
						{
							connected = 0;
						}
						else
						{
							connected = 1;
							sprintf(buf2, buf);
						}
					}
				}

				pclose(fp);

				if (connected)
				{
					sprintf(out, "Connected");

					{
						uchar mac[64];
						uchar i;

						mac[0] = 0;

						TIM_ERROR("in:%s(%d) (%s)\r\n", __FUNCTION__, __LINE__, buf2);
						TIM_ERROR("in:%s(%d) (%s)\r\n", __FUNCTION__, __LINE__, mac);

						snprintf(mac, sizeof(mac) - 1, "%s", (strstr(buf2, "ApCli0 Connected AP : ") + strlen("ApCli0 Connected AP : ")));

						TIM_ERROR("in:%s(%d) (%s)\r\n", __FUNCTION__, __LINE__, buf2);

						mac[sizeof(mac) - 1] = 0;

						for (i = 0; i < sizeof(mac); i++)
						{
							if ((mac[i] >= 'A') && (mac[i] <= 'Z'))
							{
								mac[i] = mac[i] - 'A' + 'a';
							}
							if (((mac[i] >= '0') && (mac[i] <= '9')) || ((mac[i] >= 'a') && (mac[i] <= 'f')) || ((mac[i] >= 'A') && (mac[i] <= 'F')) || ((mac[i] == ':')))
							{
							}
							else
							{
								mac[i] = 0;
								break;
							}
							if (0 == mac[i])
								break;
						}
						TIM_ERROR("ERROR:%s(%d)  (%s)\r\n", __FUNCTION__, __LINE__, mac);
						TIM_ERROR("ERROR:%s(%d)  (%s)\r\n", __FUNCTION__, __LINE__, out);

						system("iwpriv ra0 set SiteSurvey=1");

						do
						{
							FILE *fp;
							char buf[512], *p;
							int len;

							if (!(fp = popen("iwpriv ra0 get_site_survey", "r")))
							{
								TIM_ERROR("in:%s(%d)\r\n", __FUNCTION__, __LINE__);
								break;
							}

							memset(buf, 0, sizeof(buf));
							fgets(buf, sizeof(buf), fp);
							fgets(buf, sizeof(buf), fp);
							while (fgets(buf, sizeof(buf), fp))
							{
								if (strlen(buf) < 4)
									break;
								/*
# iwpriv ra0 set SiteSurvey=1;iwpriv ra0 get_site_survey
	ra0 	  get_site_survey:
	Ch	SSID							 BSSID				 Security				Siganl(%)W-Mode  ExtCH	NT WPS DPID
	4	HI-LINK 						 5c:63:bf:d9:3e:da	 WPA2PSK/AES			15		 11b/g/n NONE	In YES	   
	10	ChinaNet-5Yyp					 00:25:12:74:39:09	 WPAPSK/TKIP			0		 11b/g	 NONE	In YES	   
	1	HI-LINK_098C					 48:02:2a:00:09:8b	 WPA1PSKWPA2PSK/AES 	5		 11b/g/n NONE	In YES	   
	
	*/
								if (strstr(buf, mac))
								{
									TIM_ERROR("ERROR:%s(%d)  (%s)\r\n", __FUNCTION__, __LINE__, out);
									strcat(out, "\r\n");
									TIM_ERROR("ERROR:%s(%d)  (%s)\r\n", __FUNCTION__, __LINE__, out);
									strcat(out, buf);
									TIM_ERROR("ERROR:%s(%d)  (%s)\r\n", __FUNCTION__, __LINE__, out);
									break;
								}
							}
							pclose(fp);

						} while (0);
					}
				}
				else
				{
					sprintf(out, "Disconnected");
				}
			}
		}
		break;
		default:
			sprintf(out, "error:no sta connect");
			break;
		}

		//		sprintf(out,"%s",nvram_get(SER2NET_NVRAM,"OperationMode"));
	}
	else if ('=' == *(comm + 0))
	{
		sprintf(out, "error");
	}
	else
	{
		sprintf(out, "error");
	}

	return ret;
}

uchar AT_RstaStatus2(uchar *comm, uchar *out)
{
	uint ret = 0;

	if ('?' == *(comm + 1))
	{
		int mode = atoi(nvram_get(SER2NET_NVRAM, "OperationMode"));

		switch (mode)
		{
		case 2:
			out[0] = 0;
			wifi_ConState(comm, out);

			break;
		case 0:
		case 3:
		{
			char buf[4096];
			char buf2[4096];

			system("iwpriv apcli0 show connStatus");

			{
				FILE *fp;

				uchar connected = 0;

				uchar *p;
				int len;
				if (!(fp = popen("logread", "r")))
				{
					TIM_ERROR("in:%s(%d)\r\n", __FUNCTION__, __LINE__);
					return 0;
				}

				memset(buf, 0, sizeof(buf));

				while (fgets(buf, sizeof(buf), fp))
				{
					if (strstr(buf, "ApCli0 Connected AP : "))
					{
						if (strstr(buf, "ApCli0 Connected AP : Disconnect"))
						{
							connected = 0;
						}
						else
						{
							connected = 1;
							sprintf(buf2, buf);
						}
					}
				}

				pclose(fp);

				if (connected)
				{
					sprintf(out, "Connected");
				}
				else
				{
					sprintf(out, "Disconnected");
				}
			}
		}
		break;
		default:
			sprintf(out, "error:no sta connect");
			break;
		}

		//		sprintf(out,"%s",nvram_get(SER2NET_NVRAM,"OperationMode"));
	}
	else if ('=' == *(comm + 0))
	{
		sprintf(out, "error");
	}
	else
	{
		sprintf(out, "error");
	}

	return ret;
}

uchar AT_RstaWIFI(uchar *comm, uchar *out)
{
	uint ret = 0;

	if ('?' == *(comm + 1))
	{
		if (0 == strcmp("2", nvram_get(SER2NET_NVRAM, "OperationMode")))
		{

			uchar ty[32];

			uchar *auth;
			uchar *enc;

			enc = nvram_get(SER2NET_NVRAM, "staEncrypt");
			auth = nvram_get(SER2NET_NVRAM, "staAuth");

			sprintf(ty, "");

			if (0 == strcmp(enc, "0"))
			{
				sprintf(ty, "none");
			}
			else if (0 == strcmp(enc, "1"))
			{
				if (0 == strcmp(auth, "0"))
				{
					sprintf(ty, "wep_open");
				}
				else if (0 == strcmp(auth, "1"))
				{
					sprintf(ty, "wep");
				}
			}
			else if (0 == strcmp(auth, "4"))
			{
				if (0 == strcmp(enc, "4"))
				{
					sprintf(ty, "wpa_tkip");
				}
				else if (0 == strcmp(enc, "6"))
				{
					sprintf(ty, "wpa_aes");
				}
			}
			else if (0 == strcmp(auth, "7"))
			{
				if (0 == strcmp(enc, "4"))
				{
					sprintf(ty, "wpa2_tkip");
				}
				else if (0 == strcmp(enc, "6"))
				{
					sprintf(ty, "wpa2_aes");
				}
			}

			sprintf(out, "%s,%s,%s", nvram_get(SER2NET_NVRAM, "staSSID"), ty, nvram_get(SER2NET_NVRAM, "staWpaPsk"));
		}
		else
		{

			uchar ty[32];

			uchar *auth;
			uchar *enc;

			enc = nvram_get(SER2NET_NVRAM, "ApCliEncrypType");
			auth = nvram_get(SER2NET_NVRAM, "ApCliAuthMode");

			sprintf(ty, "");

			if (0 == strcmp(enc, "NONE"))
			{
				sprintf(ty, "none");
			}
			else if (0 == strcmp(enc, "WEP"))
			{
				if (0 == strcmp(auth, "OPEN"))
				{
					sprintf(ty, "wep_open");
				}
				else if (0 == strcmp(auth, "SHARED"))
				{
					sprintf(ty, "wep");
				}
			}
			else if (0 == strcmp(auth, "WPAPSK"))
			{
				if (0 == strcmp(enc, "TKIP"))
				{
					sprintf(ty, "wpa_tkip");
				}
				else if (0 == strcmp(enc, "AES"))
				{
					sprintf(ty, "wpa_aes");
				}
			}
			else if (0 == strcmp(auth, "WPA2PSK"))
			{
				if (0 == strcmp(enc, "TKIP"))
				{
					sprintf(ty, "wpa2_tkip");
				}
				else if (0 == strcmp(enc, "AES"))
				{
					sprintf(ty, "wpa2_aes");
				}
			}

			sprintf(out, "%s,%s,%s", nvram_get(SER2NET_NVRAM, "ApCliSsid"), ty, nvram_get(SER2NET_NVRAM, "ApCliWPAPSK"));
		}
	}
	else if ('=' == *(comm + 0))
	{
		uchar buf[64];

		do
		{ //wifi conf
			uchar ssid[32];
			uchar keytype[32];
			uchar key[32];
			uchar *p1, *p2;

			TIM_DEBUG("in:%s(%d)\r\n", __FUNCTION__, __LINE__);

			p1 = (comm + 1);

			p2 = strchr(p1, ',');
			if (0 == p2)
			{
				TIM_ERROR("in:%s(%d)\r\n", __FUNCTION__, __LINE__);
				break;
			}
			snprintf(ssid, MIN_(p2 - p1 + 1, sizeof(ssid)), p1);

			p1 = p2 + 1;
			p2 = strchr(p1, ',');
			if (0 == p2)
			{
				TIM_ERROR("in:%s(%d)\r\n", __FUNCTION__, __LINE__);
				break;
			}
			snprintf(keytype, MIN_(p2 - p1 + 1, sizeof(keytype)), p1);

			p1 = p2 + 1;
			snprintf(key, sizeof(key) - 1, p1);

			TIM_DEBUG("in:%s(%d) (%s)  (%s) (%s) (%s)\r\n", __FUNCTION__, __LINE__, g_AT_wifi_conf, ssid, keytype, key);

			if (0 == strcmp("2", nvram_get(SER2NET_NVRAM, "OperationMode")))
			{
				nvram_set_(SER2NET_NVRAM, "staSSID", ssid);

				if (0 == strcmp(keytype, "none"))
				{

					nvram_set_(SER2NET_NVRAM, "staEncrypt", "0");
					nvram_set_(SER2NET_NVRAM, "staAuth", "0");
					nvram_set_(SER2NET_NVRAM, "staWpaPsk", "0");
					nvram_set_(SER2NET_NVRAM, "staKey1", "0");
				}
				else if ((0 == strcmp(keytype, "wep")) || (0 == strcmp(keytype, "wep_shared")))
				{

					nvram_set_(SER2NET_NVRAM, "staAuth", "1");
					nvram_set_(SER2NET_NVRAM, "staEncrypt", "1");
					nvram_set_(SER2NET_NVRAM, "staWpaPsk", "0");
					nvram_set_(SER2NET_NVRAM, "staKey1Type", "1");
					nvram_set_(SER2NET_NVRAM, "staKey1", key);

					//hex
					//		nvram_set_(SER2NET_NVRAM,"staKey1Type","0");
					//		nvram_set_(SER2NET_NVRAM,"staKey1",key);

					//128bit
					//staKey1Length=1
				}
				else if ((0 == strcmp(keytype, "wep_open")))
				{

					nvram_set_(SER2NET_NVRAM, "staAuth", "0");
					nvram_set_(SER2NET_NVRAM, "staEncrypt", "1");
					nvram_set_(SER2NET_NVRAM, "staWpaPsk", "0");
					nvram_set_(SER2NET_NVRAM, "staKey1Type", "1");
					nvram_set_(SER2NET_NVRAM, "staKey1", key);

					//hex
					//		nvram_set_(SER2NET_NVRAM,"staKey1Type","0");
					//		nvram_set_(SER2NET_NVRAM,"staKey1",key);

					//128bit
					//staKey1Length=1
				}
				else if (0 == strcmp(keytype, "wpa_tkip"))
				{
					nvram_set_(SER2NET_NVRAM, "staEncrypt", "4");
					nvram_set_(SER2NET_NVRAM, "staAuth", "4");
					nvram_set_(SER2NET_NVRAM, "staWpaPsk", key);
				}
				else if (0 == strcmp(keytype, "wpa_aes"))
				{
					nvram_set_(SER2NET_NVRAM, "staEncrypt", "6");
					nvram_set_(SER2NET_NVRAM, "staAuth", "4");
					nvram_set_(SER2NET_NVRAM, "staWpaPsk", key);
				}
				else if (0 == strcmp(keytype, "wpa2_tkip"))
				{
					nvram_set_(SER2NET_NVRAM, "staEncrypt", "4");
					nvram_set_(SER2NET_NVRAM, "staAuth", "7");
					nvram_set_(SER2NET_NVRAM, "staWpaPsk", key);
				}
				else if (0 == strcmp(keytype, "wpa2_aes"))
				{
					nvram_set_(SER2NET_NVRAM, "staEncrypt", "6");
					nvram_set_(SER2NET_NVRAM, "staAuth", "7");
					nvram_set_(SER2NET_NVRAM, "staWpaPsk", key);
				}
				else if (0 == strcmp(keytype, "auto"))
				{
					nvram_set_(SER2NET_NVRAM, "g_AT_wifi_autoed", "0");
				}
				else
				{
					nvram_set_(SER2NET_NVRAM, "staEncrypt", "6");
					nvram_set_(SER2NET_NVRAM, "staAuth", "7");
					nvram_set_(SER2NET_NVRAM, "staWpaPsk", key);
				}

				nvram_set_(SER2NET_NVRAM, "RekeyMethod", "TIME");
			}
			else
			{
				nvram_set_(SER2NET_NVRAM, "ApCliSsid", ssid);

				if (0 == strcmp(keytype, "none"))
				{
					nvram_set_(SER2NET_NVRAM, "ApCliAuthMode", "OPEN");
					nvram_set_(SER2NET_NVRAM, "ApCliEncrypType", "NONE");
				}
				else if (0 == strcmp(keytype, "wep"))
				{

					nvram_set_(SER2NET_NVRAM, "ApCliAuthMode", "SHARED");
					nvram_set_(SER2NET_NVRAM, "ApCliEncrypType", "WEP");
					nvram_set_(SER2NET_NVRAM, "ApCliDefaultKeyID", "1");
					nvram_set_(SER2NET_NVRAM, "ApCliKey1Type", "1");
					nvram_set_(SER2NET_NVRAM, "ApCliKey1Str", key);
				}
				else if (0 == strcmp(keytype, "wep_open"))
				{

					nvram_set_(SER2NET_NVRAM, "ApCliAuthMode", "OPEN");
					nvram_set_(SER2NET_NVRAM, "ApCliEncrypType", "WEP");
					nvram_set_(SER2NET_NVRAM, "ApCliDefaultKeyID", "1");
					nvram_set_(SER2NET_NVRAM, "ApCliKey1Type", "1");
					nvram_set_(SER2NET_NVRAM, "ApCliKey1Str", key);
				}
				else if (0 == strcmp(keytype, "wpa_tkip"))
				{
					nvram_set_(SER2NET_NVRAM, "ApCliAuthMode", "WPAPSK");
					nvram_set_(SER2NET_NVRAM, "ApCliEncrypType", "TKIP");
					nvram_set_(SER2NET_NVRAM, "ApCliWPAPSK", key);
				}
				else if (0 == strcmp(keytype, "wpa_aes"))
				{
					nvram_set_(SER2NET_NVRAM, "ApCliAuthMode", "ApCliWPAPSK");
					nvram_set_(SER2NET_NVRAM, "ApCliEncrypType", "AES");
					nvram_set_(SER2NET_NVRAM, "ApCliWPAPSK", key);
				}
				else if (0 == strcmp(keytype, "wpa2_tkip"))
				{
					nvram_set_(SER2NET_NVRAM, "ApCliAuthMode", "WPA2PSK");
					nvram_set_(SER2NET_NVRAM, "ApCliEncrypType", "TKIP");
					nvram_set_(SER2NET_NVRAM, "ApCliWPAPSK", key);
				}
				else if (0 == strcmp(keytype, "wpa2_aes"))
				{
					nvram_set_(SER2NET_NVRAM, "ApCliAuthMode", "WPA2PSK");
					nvram_set_(SER2NET_NVRAM, "ApCliEncrypType", "AES");
					nvram_set_(SER2NET_NVRAM, "ApCliWPAPSK", key);
				}
				else if (0 == strcmp(keytype, "wpawpa2_tkip"))
				{
					nvram_set_(SER2NET_NVRAM, "ApCliAuthMode", "WPAPSKWPA2PSK");
					nvram_set_(SER2NET_NVRAM, "ApCliEncrypType", "TKIP");
					nvram_set_(SER2NET_NVRAM, "ApCliWPAPSK", key);
				}
				else if (0 == strcmp(keytype, "wpawpa2_aes"))
				{
					nvram_set_(SER2NET_NVRAM, "ApCliAuthMode", "WPAPSKWPA2PSK");
					nvram_set_(SER2NET_NVRAM, "ApCliEncrypType", "AES");
					nvram_set_(SER2NET_NVRAM, "ApCliWPAPSK", key);
				}
				else
				{
					nvram_set_(SER2NET_NVRAM, "ApCliAuthMode", "WPAPSKWPA2PSK");
					nvram_set_(SER2NET_NVRAM, "ApCliEncrypType", "AES");
					nvram_set_(SER2NET_NVRAM, "ApCliWPAPSK", key);
				}

				nvram_set_(SER2NET_NVRAM, "RekeyMethod", "TIME");
			}

			TIM_DEBUG("in:%s(%d)\r\n", __FUNCTION__, __LINE__);
		} while (0);

		sprintf(out, "ok");
	}
	else
	{
		sprintf(out, "error");
	}

	return ret;
}

uchar AT_RstaWIFI2(uchar *comm, uchar *out)
{
	uint ret = 0;

	if ('?' == *(comm + 1))
	{
		if (0 == strcmp("2", nvram_get(SER2NET_NVRAM, "OperationMode")))
		{

			uchar ty[32];

			uchar *auth;
			uchar *enc;

			enc = nvram_get(SER2NET_NVRAM, "staEncrypt");
			auth = nvram_get(SER2NET_NVRAM, "staAuth");

			sprintf(ty, "");

			if (0 == strcmp(enc, "0"))
			{
				sprintf(ty, "none");
			}
			else if (0 == strcmp(enc, "1"))
			{
				if (0 == strcmp(auth, "0"))
				{
					sprintf(ty, "wep_open");
				}
				else if (0 == strcmp(auth, "1"))
				{
					sprintf(ty, "wep");
				}
			}
			else if (0 == strcmp(auth, "4"))
			{
				if (0 == strcmp(enc, "4"))
				{
					sprintf(ty, "wpa_tkip");
				}
				else if (0 == strcmp(enc, "6"))
				{
					sprintf(ty, "wpa_aes");
				}
			}
			else if (0 == strcmp(auth, "7"))
			{
				if (0 == strcmp(enc, "4"))
				{
					sprintf(ty, "wpa2_tkip");
				}
				else if (0 == strcmp(enc, "6"))
				{
					sprintf(ty, "wpa2_aes");
				}
			}

			sprintf(out, "%s,%s,%s", nvram_get(SER2NET_NVRAM, "staSSID"), ty, nvram_get(SER2NET_NVRAM, "staWpaPsk"));
		}
		else
		{

			uchar ty[32];

			uchar *auth;
			uchar *enc;

			enc = nvram_get(SER2NET_NVRAM, "ApCliEncrypType");
			auth = nvram_get(SER2NET_NVRAM, "ApCliAuthMode");

			sprintf(ty, "");

			if (0 == strcmp(enc, "NONE"))
			{
				sprintf(ty, "none");
			}
			else if (0 == strcmp(enc, "WEP"))
			{
				if (0 == strcmp(auth, "OPEN"))
				{
					sprintf(ty, "wep_open");
				}
				else if (0 == strcmp(auth, "SHARED"))
				{
					sprintf(ty, "wep");
				}
			}
			else if (0 == strcmp(auth, "WPAPSK"))
			{
				if (0 == strcmp(enc, "TKIP"))
				{
					sprintf(ty, "wpa_tkip");
				}
				else if (0 == strcmp(enc, "AES"))
				{
					sprintf(ty, "wpa_aes");
				}
			}
			else if (0 == strcmp(auth, "WPA2PSK"))
			{
				if (0 == strcmp(enc, "TKIP"))
				{
					sprintf(ty, "wpa2_tkip");
				}
				else if (0 == strcmp(enc, "AES"))
				{
					sprintf(ty, "wpa2_aes");
				}
			}

			sprintf(out, "%s,%s,%s", nvram_get(SER2NET_NVRAM, "ApCliSsid"), ty, nvram_get(SER2NET_NVRAM, "ApCliWPAPSK"));
		}
	}
	else if ('=' == *(comm + 0))
	{
		uchar buf[64];

		do
		{ //wifi conf
			uchar ssid[32];
			uchar keytype[32];
			uchar key[32];
			uchar *p1, *p2;

			uchar s_len;

			TIM_DEBUG("in:%s(%d)\r\n", __FUNCTION__, __LINE__);

			p1 = (comm + 1);
			s_len = atoi(p1);

			p2 = strchr(p1, ',');
			if (0 == p2)
			{
				TIM_ERROR("in:%s(%d)\r\n", __FUNCTION__, __LINE__);
				break;
			}
			p1 = p2 + 1;

			if ((s_len + 2) > strlen(p1))
			{
				TIM_ERROR("in:%s(%d)\r\n", __FUNCTION__, __LINE__);
				break;
			}

			p2 = p1 + s_len;
			snprintf(ssid, MIN_(p2 - p1 + 1, sizeof(ssid)), p1);

			p1 = p2 + 1;
			p2 = strchr(p1, ',');
			if (0 == p2)
			{
				TIM_ERROR("in:%s(%d)\r\n", __FUNCTION__, __LINE__);
				break;
			}
			snprintf(keytype, MIN_(p2 - p1 + 1, sizeof(keytype)), p1);

			p1 = p2 + 1;
			snprintf(key, sizeof(key) - 1, p1);

			TIM_DEBUG("in:%s(%d) (%s)  (%s) (%s) (%s)\r\n", __FUNCTION__, __LINE__, g_AT_wifi_conf, ssid, keytype, key);

			if (0 == strcmp("2", nvram_get(SER2NET_NVRAM, "OperationMode")))
			{
				nvram_set_(SER2NET_NVRAM, "staSSID", ssid);

				if (0 == strcmp(keytype, "none"))
				{

					nvram_set_(SER2NET_NVRAM, "staEncrypt", "0");
					nvram_set_(SER2NET_NVRAM, "staAuth", "0");
					nvram_set_(SER2NET_NVRAM, "staWpaPsk", "0");
					nvram_set_(SER2NET_NVRAM, "staKey1", "0");
				}
				else if ((0 == strcmp(keytype, "wep")) || (0 == strcmp(keytype, "wep_shared")))
				{

					nvram_set_(SER2NET_NVRAM, "staAuth", "1");
					nvram_set_(SER2NET_NVRAM, "staEncrypt", "0");
					nvram_set_(SER2NET_NVRAM, "staWpaPsk", "0");
					nvram_set_(SER2NET_NVRAM, "staKey1Type", "1");
					nvram_set_(SER2NET_NVRAM, "staKey1", key);

					//hex
					//		nvram_set_(SER2NET_NVRAM,"staKey1Type","0");
					//		nvram_set_(SER2NET_NVRAM,"staKey1",key);

					//128bit
					//staKey1Length=1
				}
				else if ((0 == strcmp(keytype, "wep_open")))
				{

					nvram_set_(SER2NET_NVRAM, "staAuth", "0");
					nvram_set_(SER2NET_NVRAM, "staEncrypt", "1");
					nvram_set_(SER2NET_NVRAM, "staWpaPsk", "0");
					nvram_set_(SER2NET_NVRAM, "staKey1Type", "1");
					nvram_set_(SER2NET_NVRAM, "staKey1", key);

					//hex
					//		nvram_set_(SER2NET_NVRAM,"staKey1Type","0");
					//		nvram_set_(SER2NET_NVRAM,"staKey1",key);

					//128bit
					//staKey1Length=1
				}
				else if (0 == strcmp(keytype, "wpa_tkip"))
				{
					nvram_set_(SER2NET_NVRAM, "staEncrypt", "4");
					nvram_set_(SER2NET_NVRAM, "staAuth", "4");
					nvram_set_(SER2NET_NVRAM, "staWpaPsk", key);
				}
				else if (0 == strcmp(keytype, "wpa_aes"))
				{
					nvram_set_(SER2NET_NVRAM, "staEncrypt", "6");
					nvram_set_(SER2NET_NVRAM, "staAuth", "4");
					nvram_set_(SER2NET_NVRAM, "staWpaPsk", key);
				}
				else if (0 == strcmp(keytype, "wpa2_tkip"))
				{
					nvram_set_(SER2NET_NVRAM, "staEncrypt", "4");
					nvram_set_(SER2NET_NVRAM, "staAuth", "7");
					nvram_set_(SER2NET_NVRAM, "staWpaPsk", key);
				}
				else if (0 == strcmp(keytype, "wpa2_aes"))
				{
					nvram_set_(SER2NET_NVRAM, "staEncrypt", "6");
					nvram_set_(SER2NET_NVRAM, "staAuth", "7");
					nvram_set_(SER2NET_NVRAM, "staWpaPsk", key);
				}
				else if (0 == strcmp(keytype, "auto"))
				{
					nvram_set_(SER2NET_NVRAM, "g_AT_wifi_autoed", "0");
				}
				else
				{
					nvram_set_(SER2NET_NVRAM, "staEncrypt", "6");
					nvram_set_(SER2NET_NVRAM, "staAuth", "7");
					nvram_set_(SER2NET_NVRAM, "staWpaPsk", key);
				}

				nvram_set_(SER2NET_NVRAM, "RekeyMethod", "TIME");
			}
			else
			{
				nvram_set_(SER2NET_NVRAM, "ApCliSsid", ssid);

				if (0 == strcmp(keytype, "none"))
				{
					nvram_set_(SER2NET_NVRAM, "ApCliAuthMode", "OPEN");
					nvram_set_(SER2NET_NVRAM, "ApCliEncrypType", "NONE");
				}
				else if (0 == strcmp(keytype, "wep"))
				{

					nvram_set_(SER2NET_NVRAM, "ApCliAuthMode", "SHARED");
					nvram_set_(SER2NET_NVRAM, "ApCliEncrypType", "WEP");
					nvram_set_(SER2NET_NVRAM, "ApCliDefaultKeyID", "1");
					nvram_set_(SER2NET_NVRAM, "ApCliKey1Type", "1");
					nvram_set_(SER2NET_NVRAM, "ApCliKey1Str", key);
				}
				else if (0 == strcmp(keytype, "wep_open"))
				{

					nvram_set_(SER2NET_NVRAM, "ApCliAuthMode", "OPEN");
					nvram_set_(SER2NET_NVRAM, "ApCliEncrypType", "WEP");
					nvram_set_(SER2NET_NVRAM, "ApCliDefaultKeyID", "1");
					nvram_set_(SER2NET_NVRAM, "ApCliKey1Type", "1");
					nvram_set_(SER2NET_NVRAM, "ApCliKey1Str", key);
				}
				else if (0 == strcmp(keytype, "wpa_tkip"))
				{
					nvram_set_(SER2NET_NVRAM, "ApCliAuthMode", "WPAPSK");
					nvram_set_(SER2NET_NVRAM, "ApCliEncrypType", "TKIP");
					nvram_set_(SER2NET_NVRAM, "ApCliWPAPSK", key);
				}
				else if (0 == strcmp(keytype, "wpa_aes"))
				{
					nvram_set_(SER2NET_NVRAM, "ApCliAuthMode", "ApCliWPAPSK");
					nvram_set_(SER2NET_NVRAM, "ApCliEncrypType", "AES");
					nvram_set_(SER2NET_NVRAM, "ApCliWPAPSK", key);
				}
				else if (0 == strcmp(keytype, "wpa2_tkip"))
				{
					nvram_set_(SER2NET_NVRAM, "ApCliAuthMode", "WPA2PSK");
					nvram_set_(SER2NET_NVRAM, "ApCliEncrypType", "TKIP");
					nvram_set_(SER2NET_NVRAM, "ApCliWPAPSK", key);
				}
				else if (0 == strcmp(keytype, "wpa2_aes"))
				{
					nvram_set_(SER2NET_NVRAM, "ApCliAuthMode", "WPA2PSK");
					nvram_set_(SER2NET_NVRAM, "ApCliEncrypType", "AES");
					nvram_set_(SER2NET_NVRAM, "ApCliWPAPSK", key);
				}
				else if (0 == strcmp(keytype, "wpawpa2_tkip"))
				{
					nvram_set_(SER2NET_NVRAM, "ApCliAuthMode", "WPAPSKWPA2PSK");
					nvram_set_(SER2NET_NVRAM, "ApCliEncrypType", "TKIP");
					nvram_set_(SER2NET_NVRAM, "ApCliWPAPSK", key);
				}
				else if (0 == strcmp(keytype, "wpawpa2_aes"))
				{
					nvram_set_(SER2NET_NVRAM, "ApCliAuthMode", "WPAPSKWPA2PSK");
					nvram_set_(SER2NET_NVRAM, "ApCliEncrypType", "AES");
					nvram_set_(SER2NET_NVRAM, "ApCliWPAPSK", key);
				}
				else
				{
					nvram_set_(SER2NET_NVRAM, "ApCliAuthMode", "WPAPSKWPA2PSK");
					nvram_set_(SER2NET_NVRAM, "ApCliEncrypType", "AES");
					nvram_set_(SER2NET_NVRAM, "ApCliWPAPSK", key);
				}

				nvram_set_(SER2NET_NVRAM, "RekeyMethod", "TIME");
			}

			TIM_DEBUG("in:%s(%d)\r\n", __FUNCTION__, __LINE__);
		} while (0);

		sprintf(out, "ok");
	}
	else
	{
		sprintf(out, "error");
	}

	return ret;
}

uchar AT_RstaBssid(uchar *comm, uchar *out)
{
	uint ret = 0;

	if ('?' == *(comm + 1))
	{
		sprintf(out, "%s", nvram_get(SER2NET_NVRAM, "ApCliBssid"));
	}
	else if ('=' == *(comm + 0))
	{
		uchar buf[64];

		do
		{ //wifi conf

			if (0 == strcmp("2", nvram_get(SER2NET_NVRAM, "OperationMode")))
			{
				//	nvram_set_(SER2NET_NVRAM,"staBSSID",(comm+1));
				nvram_set_(SER2NET_NVRAM, "ApCliBssid", (comm + 1));
			}
			else
			{
				nvram_set_(SER2NET_NVRAM, "ApCliBssid", (comm + 1));
			}

			TIM_DEBUG("in:%s(%d)\r\n", __FUNCTION__, __LINE__);
		} while (0);

		sprintf(out, "ok");
	}
	else
	{
		sprintf(out, "error");
	}

	return ret;
}

uchar AT_dhcpd(uchar *comm, uchar *out)
{
	uint ret = 0;

	if ('?' == *(comm + 1))
	{
		sprintf(out, "%d", g_AT_dhcpd);
	}
	else if ('=' == *(comm + 0))
	{
		g_AT_dhcpd = atoi(comm + 1);
		sprintf(out, "ok");
	}
	else
	{
		sprintf(out, "error");
	}

	return ret;
}

uchar AT_dhcpd_ip(uchar *comm, uchar *out)
{
	uint ret = 0;

	if ('?' == *(comm + 1))
	{
		if (g_AT_dhcpd_ip)
			sprintf(out, "%s", g_AT_dhcpd_ip);
	}
	else if ('=' == *(comm + 0))
	{
		g_AT_dhcpd_ip = at_set_value_(g_AT_dhcpd_ip, (comm + 1));
		sprintf(out, "ok");
	}
	else
	{
		sprintf(out, "error");
	}

	return ret;
}

uchar AT_dhcpd_dns(uchar *comm, uchar *out)
{
	uint ret = 0;

	if ('?' == *(comm + 1))
	{
		if (g_AT_dhcpd_dns)
			sprintf(out, "%s", g_AT_dhcpd_dns);
	}
	else if ('=' == *(comm + 0))
	{
		g_AT_dhcpd_dns = at_set_value_(g_AT_dhcpd_dns, (comm + 1));
		sprintf(out, "ok");
	}
	else
	{
		sprintf(out, "error");
	}

	return ret;
}

uchar AT_dhcpd_time(uchar *comm, uchar *out)
{
	uint ret = 0;

	if ('?' == *(comm + 1))
	{
		sprintf(out, "%d", g_AT_dhcpd_time);
	}
	else if ('=' == *(comm + 0))
	{
		g_AT_dhcpd_time = atoi(comm + 1);
		sprintf(out, "ok");
	}
	else
	{
		sprintf(out, "error");
	}

	return ret;
}

uchar AT_dhcpc(uchar *comm, uchar *out)

{
	uint ret = 0;

	if ('?' == *(comm + 1))
	{
		sprintf(out, "%d", g_AT_dhcpc);
	}
	else if ('=' == *(comm + 0))
	{
		if (0 == strcmp((comm + 1), "1"))
		{
			g_AT_dhcpc = 1;
		}
		else
		{
			g_AT_dhcpc = 0;
		}

		sprintf(out, "ok");
	}
	else
	{
		sprintf(out, "error");
	}

	return ret;
}

uchar AT_net_ip(uchar *comm, uchar *out)
{
	uint ret = 0;

	if ('?' == *(comm + 1))
	{
		if (g_AT_net_ip)
			sprintf(out, "%s", g_AT_net_ip);
	}
	else if ('=' == *(comm + 0))
	{
		g_AT_net_ip = at_set_value_(g_AT_net_ip, (comm + 1));
		sprintf(out, "ok");
	}
	else
	{
		sprintf(out, "error");
	}

	return ret;
}

uchar AT_net_dns(uchar *comm, uchar *out)
{
	uint ret = 0;

	if ('?' == *(comm + 1))
	{
		//		sprintf(out,"%s",g_AT_wifi_conf);
		if (g_AT_net_dns)
			sprintf(out, "%s", g_AT_net_dns);
	}
	else if ('=' == *(comm + 0))
	{

		g_AT_net_dns = at_set_value_(g_AT_net_dns, (comm + 1));

		sprintf(out, "ok");
	}
	else
	{
		sprintf(out, "error");
	}

	return ret;
}

uchar AT_default(uchar *comm, uchar *out)
{
	sprintf(out, "load default...\r\n");
	//	system("ralink_init clear 2860;ralink_init renew 2860 /etc_ro/Wireless/RT2860AP/RT2860_default_vlan;reboot");
	//	system("(sleep 2;ralink_init clear 2860;ralink_init renew 2860 /etc_ro/Wireless/RT2860AP/RT2860_default_vlan;reboot)&");
	system("ralink_init clear 2860;ralink_init renew 2860 /etc_ro/Wireless/RT2860AP/RT2860_default_vlan;");

	sprintf(out, "ok");

	return 0;
}

uchar AT_net_done(uchar *comm, uchar *out)
{
	if ('1' != *(comm + 1))
	{
		sprintf(out, "");
		return 0;
	}

	config_all_save_nvram_2();
	goahead_initInternet();
	power_save();
	sprintf(out, "ok");

	return 0;
}

uchar AT_reboot(uchar *comm, uchar *out)
{
	sprintf(out, "ok\r\n");
	return system("(sleep 1;reboot)&");
}

uchar AT_resetdefault(uchar *comm, uchar *out)
{
	sprintf(out, "ok\r\n");
	return system("(sleep 1; killall dropbear uhttpd; sleep 1; jffs2reset -y && reboot)&");
}

uchar AT_out_trans(uchar *comm, uchar *out)
{
	uint ret = 0;

	if ('?' == *(comm + 1))
	{
		sprintf(out, "");
	}
	else if ('=' == *(comm + 0))
	{
		uchar value = atoi(comm + 1);
		out_transparent_do(value);
		sprintf(out, "ok");
	}
	else
	{
		sprintf(out, "error");
	}

	return ret;
}

uchar AT_ip_max_conn_ctrl(uchar *comm, uchar *out)
{
	int hfd;
	int ret = 0;
	char buff[256] = {0};
	long old_max_conn_cnt = 65535;
	long max_conn_cnt;
	int iptables_num;
	//iptables -I FORWARD -p tcp -m connlimit --connlimit-above 50 -j REJECT
	//判断 /etc/hlk_firewall 是否存在
	if (0 == access(HLK_FIREWALL_FILE, R_OK)) {
		//获取配置数据
		ret = get_cmd_ret_str(buff, "cat /etc/hlk_firewall |awk '{print $9}'");
		if (0 == ret) {
			ret = hlk_strtol(buff, 10, &old_max_conn_cnt);
			if (ret < 0) {
				goto fail;
			}
		}
	}

	if ('?' == *(comm + 1)) {
		sprintf(out, "%d", old_max_conn_cnt);
	} else if ('=' == *(comm + 0)) {
		ret = hlk_strtol(comm+1, 10, &max_conn_cnt);
		if (ret < 0) {
			goto fail;
		}
		char hlk_cmd[256] = {0};
		sprintf(hlk_cmd, "iptables -L FORWARD --line-numbers|grep \"#conn src/32 > %d\"|awk '{print $1}'", old_max_conn_cnt);
		//先删后加？？

		memset(buff, 0, sizeof(buff));
		ret = get_cmd_ret_str(buff, hlk_cmd);
		if (0 == ret) {
			ret = hlk_strtol(buff, 10, &iptables_num);
			if (0 == ret && iptables_num != 0) {
				sprintf(hlk_cmd, "iptables -D FORWARD %d", iptables_num);
				system(hlk_cmd);
			}
		}
		do_system("rm -f %s;sync", HLK_FIREWALL_FILE);

		//max_conn_cnt == 0 解除限制
		if (0 == max_conn_cnt) {
			//删除iptable规则
			//获取 对应的iptables规则 行号
		} else {
			//立即生效
			//将规则写入/etc/hlk_firewall文件中			
			do_system("iptables -I FORWARD -p tcp -m connlimit --connlimit-above %d -j REJECT", max_conn_cnt);
			do_system("rm -f %s;sync", HLK_FIREWALL_FILE);
			do_system("echo \"iptables -I FORWARD -p tcp -m connlimit --connlimit-above %d -j REJECT\" >%s ", max_conn_cnt, HLK_FIREWALL_FILE);
		}
		//怎么删除限制？？
		sprintf(out, "ok");
	} else {
		goto fail;
	}

	return 0;

fail:

	if (hfd > 0) {
		clsoe(hfd);
	}

	sprintf(out, "error");
	return 0;
}

uchar AT_sta_conn_ctrl(uchar *comm, uchar *out)
{
	//设置STA最大接入数
	//参数 2.4g at+sta_max_cnt=?
	//因为配置只能针对2.4G 或5G AP 是/2 平均设置还是 只设置一个
	int i = 0;
	int ret = 0;
	int Old_MbssMaxStaNum, MbssMaxStaNum;

	char *endptr = NULL;

	//加载wifi6 配置
	if (NULL == wifi6_conf ) {
		wifi6_conf = load_wifi6_profile("/etc/wireless/mediatek/mt7915.dbdc.b1.dat");
		if (NULL == wifi6_conf) {
			debug_log("get wifi6 config failed");
			goto fail;
		}
	}

	//判断是否有 MbssMaxStaNum 参数 没有 默认配置是多少
	//怎么建索引??? 无法建索引
	for (i = 0; i < wifi6_keyval_cnt; i++) {
		if (!strcmp(wifi6_conf[i].key, "MbssMaxStaNum") ) {
			break;
		}
	}

	if (i < wifi6_keyval_cnt) {
		//keyval_conf[i].val
		Old_MbssMaxStaNum = strtol(wifi6_conf[i].val, &endptr, 10);
	} else {
		//配置文件中没有配置， 使用默认参数？
		Old_MbssMaxStaNum = 16;//default;
	}

	if ('?' == *(comm + 1)) {
		sprintf(out, "%d", Old_MbssMaxStaNum);
	} else if ('=' == *(comm + 0)) {
		ret = hlk_strtol(comm+1, 10, &MbssMaxStaNum);
		if (ret < 0) {
			debug_log("param failed");
			goto fail;
		}

		ret = wifi6_set_param(wifi6_conf, "MbssMaxStaNum", comm + 1);
		if (ret != 0) {
			debug_log("param failed");
			goto fail;
		} 
		//立即生效
		{
			char cmd[512] = {0};
			sprintf(cmd, "iwpriv rax0 set MbssMaxStaNum=%d", MbssMaxStaNum);
			system(cmd);
		}
		//保存参数到 /etc/wireless/mediatek/mt7915.dbdc.b1.dat
		wifi6_param_store("/etc/wireless/mediatek/mt7915.dbdc.b1.dat");
		sprintf(out, "ok");
	} else {
		goto fail;
	}

	return 0;

fail:
	sprintf(out, "error");
	return 0;
}

extern uchar socket_buf_empty_flush();

uchar AT_serial_flush(uchar *comm, uchar *out)
{
	uint ret = 0;

	if ('?' == *(comm + 1))
	{
		sprintf(out, "");
	}
	else if ('=' == *(comm + 0))
	{
		uchar value = atoi(comm + 1);
		if (value)
		{
			serial_buf_empty_flush();
			socket_buf_empty_flush();
		}
		sprintf(out, "ok");
	}
	else
	{
		sprintf(out, "error");
	}

	return ret;
}

uchar AT_pm(uchar *comm, uchar *out)
{
	uint ret = 0;

	if ('?' == *(comm + 1))
	{
		sprintf(out, "%d", g_AT_pm);
	}
	else if ('=' == *(comm + 0))
	{
		g_AT_pm = atoi(comm + 1);
		power_save();
		sprintf(out, "ok");
	}
	else
	{
		sprintf(out, "error");
	}

	return ret;
}

uchar AT_settings(uchar *comm, uchar *out)
{
	uint ret = 0;

	if ('?' == *(comm + 1))
	{
		sprintf(out, "%s", nvram_get(SER2NET_NVRAM, (comm + 2)));
	}
	else if ('=' == *(comm + 0))
	{

		do
		{ //
			uchar name[64];
			uchar value[64];
			uchar *p1, *p2;

			TIM_DEBUG("in:%s(%d)\r\n", __FUNCTION__, __LINE__);

			p1 = (comm + 1);

			p2 = strchr(p1, '=');
			if (0 == p2)
			{
				TIM_ERROR("in:%s(%d)\r\n", __FUNCTION__, __LINE__);
				break;
			}
			snprintf(name, MIN_(p2 - p1 + 1, sizeof(name)), p1);

			p1 = p2 + 1;
			snprintf(value, sizeof(value) - 1, p1);

			TIM_DEBUG("in:%s(%d) (%s)  (%s) (%s) (%s)\r\n", __FUNCTION__, __LINE__, (comm + 1), name, value);

			nvram_bufset(SER2NET_NVRAM, name, value);

		} while (0);

		sprintf(out, "ok");
	}
	else
	{
		sprintf(out, "error");
	}

	return ret;
}

uchar AT_RCommitAll(uchar *comm, uchar *out)
{
	uint ret = 0;

	if ('?' == *(comm + 1))
	{
		sprintf(out, "ok");
	}
	else if ('=' == *(comm + 0))
	{
		config_all_save_nvram_3(0);
		goahead_initInternet();
		power_save();
		sprintf(out, "ok");
	}
	else
	{
		sprintf(out, "error");
	}

	return ret;
}

uchar AT_settings_done(uchar *comm, uchar *out)
{
	uint ret = 0;

	if ('?' == *(comm + 1))
	{
		sprintf(out, "ok");
	}
	else if ('=' == *(comm + 0))
	{
		nvram_commit(SER2NET_NVRAM);
		goahead_initInternet();
		power_save();
		sprintf(out, "ok");
	}
	else
	{
		sprintf(out, "error");
	}

	return ret;
}

uchar AT_settings_done2(uchar *comm, uchar *out)
{
	uint ret = 0;

	if ('?' == *(comm + 1))
	{
		sprintf(out, "ok");
	}
	else if ('=' == *(comm + 0))
	{
		goahead_initInternet();
		power_save();
		sprintf(out, "ok");
	}
	else
	{
		sprintf(out, "error");
	}

	return ret;
}

uchar AT_CfgSave(uchar *comm, uchar *out)
{
	uint ret = 0;

	if ('?' == *(comm + 1))
	{
		sprintf(out, "ok");
	}
	else if ('=' == *(comm + 0))
	{
		nvram_commit(SER2NET_NVRAM);
		sprintf(out, "ok");
	}
	else
	{
		sprintf(out, "error");
	}

	return ret;
}

void gpio_mode_(ushort mode)
{
	int fd;
	uchar i;

	fd = open("/dev/gpio", O_RDONLY);
	if (fd < 0)
	{
		perror("/dev/gpio");
		return;
	}
	for (i = 0; i <= 8; i++)
	{
	}

	close(fd);

	return;

ioctl_err:
	perror("ioctl");
	close(fd);
	return;
}

uchar gpio_read_(ushort data)
{
}

void gpio_write_(ushort data)
{
}

uchar AT_gpio_mode(uchar *comm, uchar *out)
{
	uint ret = 0;

	if ('?' == *(comm + 1))
	{
		sprintf(out, "%d", g_AT_gpio_mode);
	}
	else if ('=' == *(comm + 0))
	{

		g_AT_gpio_mode = atoi(comm + 1);
		gpio_mode_(g_AT_gpio_mode);

		sprintf(out, "ok");
	}
	else
	{
		sprintf(out, "error");
	}

	return ret;
}

uchar AT_gpio_read(uchar *comm, uchar *out)
{
	uint ret = 0;

	if ('?' == *(comm + 1))
	{
		sprintf(out, "%d", gpio_read_(atoi(comm + 2)));
	}
	else if ('=' == *(comm + 0))
	{
		sprintf(out, "ok");
	}
	else
	{
		sprintf(out, "error");
	}

	return ret;
}

uchar AT_gpio_write(uchar *comm, uchar *out)
{
	uint ret = 0;

	if ('?' == *(comm + 1))
	{
	}
	else if ('=' == *(comm + 0))
	{

		do
		{ //
			uchar name[64];
			uchar value[64];
			uchar *p1, *p2;
			uchar i, j;

			TIM_DEBUG("in:%s(%d)\r\n", __FUNCTION__, __LINE__);

			p1 = (comm + 1);

			p2 = strchr(p1, '=');
			if (0 == p2)
			{
				TIM_ERROR("in:%s(%d)\r\n", __FUNCTION__, __LINE__);
				break;
			}
			snprintf(name, MIN_(p2 - p1 + 1, sizeof(name)), p1);

			p1 = p2 + 1;
			snprintf(value, sizeof(value) - 1, p1);

			TIM_DEBUG("in:%s(%d) (%s)  (%s) (%s) (%s)\r\n", __FUNCTION__, __LINE__, (comm + 1), name, value);

			i = atoi(name);
			j = atoi(value);

			if ((i < 0) || (i > 8) || (j > 1) || (j < 0))
			{
				break;
			}

			if (j)
				g_AT_gpio_data |= (1 << i);
			else
				g_AT_gpio_data &= ~(1 << i);

			gpio_write_(g_AT_gpio_data);

		} while (0);

		sprintf(out, "ok");
	}
	else
	{
		sprintf(out, "error");
	}

	return ret;
}

uchar AT_gpio_save(uchar *comm, uchar *out)
{
	uint ret = 0;

	if ('?' == *(comm + 1))
	{
	}
	else if ('=' == *(comm + 0))
	{
		if ('1' == *(comm + 1))
		{

			uchar buf[32];
			sprintf(buf, "%d", g_AT_gpio_mode);
			nvram_set_(SER2NET_NVRAM, "gpio_mode", buf);
			sprintf(buf, "%d", g_AT_gpio_data);
			nvram_set_(SER2NET_NVRAM, "gpio_data", buf);
			nvram_commit(SER2NET_NVRAM);
		}

		sprintf(out, "ok");
	}
	else
	{
		sprintf(out, "error");
	}

	return ret;
}

static uchar gpio2_status = 0;

void gpio2_set(uchar value);
uchar gpio2_get();

uchar AT_gpio2(uchar *comm, uchar *out)
{
	uint ret = 0;

	if ('?' == *(comm + 1))
	{
		sprintf(out, "%d", gpio2_get());
	}
	else if ('=' == *(comm + 0))
	{
		sprintf(out, "ok");
		if ('1' == *(comm + 1))
		{
			gpio2_set(0);
		}
		else if ('0' == *(comm + 1))
		{
			gpio2_set(1);
		}
		else
		{
			sprintf(out, "value error");
		}
	}
	else
	{
		sprintf(out, "error");
	}

	return ret;
}

#define RT2860_NVRAM SER2NET_NVRAM

/*
 * arguments: ifname  - interface name
 *            if_addr - a 16-byte buffer to store ip address
 * description: fetch ip address, netmask associated to given interface name
 */
int getIfIp(char *ifname, char *if_addr)
{
	struct ifreq ifr;
	int skfd = 0;

	if ((skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		TIM_ERROR("ERROR:%s(%d)\r\n", __FUNCTION__, __LINE__);
		return -1;
	}

	strncpy(ifr.ifr_name, ifname, IF_NAMESIZE);
	if (ioctl(skfd, SIOCGIFADDR, &ifr) < 0)
	{
		close(skfd);
		//error(E_L, E_LOG, T("getIfIp: ioctl SIOCGIFADDR error for %s"), ifname);
		return -1;
	}
	strcpy(if_addr, inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));

	close(skfd);
	return 0;
}

/*
 * description: return WAN interface name
 *              0 = bridge, 1 = gateway, 2 = wirelss isp
 */
char *getWanIfName(void)
{
	const char *mode = nvram_bufget(RT2860_NVRAM, "OperationMode");
	static char *if_name = "br0";

	if (NULL == mode)
		return if_name;
	if (!strncmp(mode, "0", 2))
		if_name = "br0";
	else if (!strncmp(mode, "1", 2))
	{
		if_name = "eth2.2";
	}
	else if (!strncmp(mode, "2", 2))
		if_name = "ra0";
	else if (!strncmp(mode, "3", 2))
		if_name = "apcli0";
	return if_name;
}

char *getWanIfNamePPP(void)
{
	const char *cm = nvram_bufget(RT2860_NVRAM, "wanConnectionMode");
	if (!strncmp(cm, "PPPOE", 6) || !strncmp(cm, "L2TP", 5) || !strncmp(cm, "PPTP", 5)
#ifdef CONFIG_USER_3G
		|| !strncmp(cm, "3G", 3)
#endif
	)
	{
		return "ppp0";
	}

	return getWanIfName();
}

/*
 * arguments: ifname - interface name
 *            if_net - a 16-byte buffer to store subnet mask
 * description: fetch subnet mask associated to given interface name
 *              0 = bridge, 1 = gateway, 2 = wirelss isp
 */
int getIfNetmask(char *ifname, char *if_net)
{
	struct ifreq ifr;
	int skfd = 0;

	if ((skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		TIM_ERROR("ERROR:%s(%d)\r\n", __FUNCTION__, __LINE__);
		return -1;
	}

	strncpy(ifr.ifr_name, ifname, IF_NAMESIZE);
	if (ioctl(skfd, SIOCGIFNETMASK, &ifr) < 0)
	{
		close(skfd);
		return -1;
	}
	strcpy(if_net, inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
	close(skfd);
	return 0;
}

/*
 * description: write WAN default gateway accordingly
 */
int getWanGateway(char *if_net)
{
	char buff[256];
	int nl = 0;
	struct in_addr dest;
	struct in_addr gw;
	int flgs, ref, use, metric;
	unsigned long int d, g, m;
	int find_default_flag = 0;

	char sgw[16];

	FILE *fp = fopen("/proc/net/route", "r");

	while (fgets(buff, sizeof(buff), fp) != NULL)
	{
		if (nl)
		{
			int ifl = 0;
			while (buff[ifl] != ' ' && buff[ifl] != '\t' && buff[ifl] != '\0')
				ifl++;
			buff[ifl] = 0; /* interface */
			if (sscanf(buff + ifl + 1, "%lx%lx%X%d%d%d%lx",
					   &d, &g, &flgs, &ref, &use, &metric, &m) != 7)
			{
				fclose(fp);
				return sprintf(if_net, "");
			}

			if (flgs & RTF_UP)
			{
				dest.s_addr = d;
				gw.s_addr = g;
				strcpy(sgw, (gw.s_addr == 0 ? "" : inet_ntoa(gw)));

				if (dest.s_addr == 0)
				{
					find_default_flag = 1;
					break;
				}
			}
		}
		nl++;
	}
	fclose(fp);

	if (find_default_flag == 1)
		return sprintf(if_net, "%s", sgw);
	else
		return sprintf(if_net, "");
}

uchar AT_netip(uchar *comm, uchar *out)
{
	uint ret = 0;

	if ('?' == *(comm + 1))
	{
		char if_addr[16];
		char if_mask[16];
		char if_gw[16];

		if (-1 == getIfIp(getWanIfNamePPP(), if_addr))
		{
			if_addr[0] = 0;
		}

		if (-1 == getIfNetmask(getWanIfNamePPP(), if_mask))
		{
			if_mask[0] = 0;
		}

		if (-1 == getWanGateway(if_gw))
		{
			if_gw[0] = 0;
		}

		sprintf(out, "%s,%s,%s", if_addr, if_mask, if_gw);
	}
	else if ('=' == *(comm + 0))
	{
		sprintf(out, "error");
	}
	else
	{
		sprintf(out, "error");
	}

	return ret;
}

uchar AT_exc(uchar *comm, uchar *out)
{
	uint ret = 0;

	if ('?' == *(comm + 1))
	{
		sprintf(out, "error");
	}
	else if ('=' == *(comm + 0))
	{
		system((comm + 1));
		sprintf(out, "error");
	}
	else
	{
		sprintf(out, "error");
	}

	return ret;
}

uchar *netnvram_get(uchar *a, uchar *b);
void netnvram_set(uchar *a, uchar *b, uchar *c);

uchar filereadn(uchar *file, uchar *p, uint len)
{
	FILE *fp;
	//	char buf[512], *p;
	//	int len;

	//		sprintf(out,"error");
	if (((int)(fp = fopen(file, "r"))) <= 0)
	{
		TIM_ERROR("ERROR:%s(%d)\r\n", __FUNCTION__, __LINE__);
		return 2;
	}
	len = fread(p, 1, len, fp);
	if (len < 0)
	{
		TIM_ERROR("ERROR:%s(%d)\r\n", __FUNCTION__, __LINE__);
		fclose(fp);
		return 3;
	}
	fclose(fp);

	return 0;
}

uchar AT_para(uchar *cmd, uchar *comm, uchar *out)
{
	uint ret = 0;

	if ('?' == *(comm + 1))
	{
		sprintf(out, netnvram_get(0, cmd));
	}
	else if ('=' == *(comm + 0))
	{
		netnvram_set(0, cmd, (comm + 1));
		sprintf(out, "ok");
	}
	else
	{
		sprintf(out, "error");
	}

	return ret;
}

uchar AT_sta2_4(uchar *comm, uchar *out)
{
	return AT_para("sta2_4", comm, out);
}
uchar AT_sta5_8(uchar *comm, uchar *out)
{
	return AT_para("sta5_8", comm, out);
}

uchar AT_ap2_4(uchar *comm, uchar *out)
{
	return AT_para("ap2_4", comm, out);
}

uchar AT_ap5_8(uchar *comm, uchar *out)
{
	return AT_para("ap5_8", comm, out);
}
#if 0
uchar AT_lanip(uchar *comm, uchar *out)
{
       return AT_para("lanip", comm, out);
}
#else
struct network_conf {
       char *ip;
       char *netmask;
       char *proto;    //dhcp static
       char *ifname;   //eth0 ra0 rax0
};

struct uci_network_conf {
       struct network_conf lan;
       struct network_conf wan;
};

void free_network(struct uci_network_conf *network_conf)
{
       if (network_conf->lan.ip != NULL) free(network_conf->lan.ip);
       if (network_conf->lan.netmask != NULL) free(network_conf->lan.netmask);
       if (network_conf->lan.proto != NULL) free(network_conf->lan.proto);
       if (network_conf->lan.ifname != NULL) free(network_conf->lan.ifname);
}

int is_netmask_valid(uint32_t mask)
{
    if (mask == 0) return 0;

    if (mask & (~mask >> 1)) {
        return 0;
    } else {
        return 1;
    }
}

int set_uci(char *section, char *option, char *value)
{
    struct uci_ptr ptr;
    memset(&ptr, 0, sizeof(ptr));

    struct uci_context * _ctx = uci_alloc_context(); //申请上下文
    if (UCI_OK != uci_load(_ctx, "/etc/config/network", &ptr.p)) {
        syslog(LOG_DEBUG, "open set_uci UCI_CONFIG_FILE fail exit\n");
        goto set_cleanup; //如果打开UCI文件失败,则跳到末尾 清理 UCI 上下文. 
    }

    ptr.package = "config";
    ptr.section = section;
    ptr.option = option;
    ptr.value = value;

    uci_set(_ctx, &ptr); //写入配置 

    uci_commit(_ctx, &ptr.p, false); //提交保存更改

    uci_unload(_ctx, ptr.p); //卸载包

set_cleanup:

    //printf("uci set_uci exit\n");

    uci_free_context(_ctx);  //释放上下文

    return 0;
}

int set_lan_ip(char *input)
{
       int ret = -1;
       int comma = 0;
       char *ip_netmask = NULL;
       char *token, *saveptr, *entry;
       char *ip = NULL, *netmask = NULL;

       //判断参数是否正确
       ip_netmask = strdup(input);
       entry = ip_netmask;
       struct in_addr addr;

       comma = 0;
       for (token = strtok_r(entry, ",", &saveptr); token != NULL; token = strtok_r(NULL, ",", &saveptr)) {
               comma++;
               switch (comma) {
                       case 1:
                               //ip
                               ret = inet_aton(token, &addr);
                               if (ret != 0 ) {
                                       ip = strdup(token);
                               }
                               break;
                       case 2:
                               //netmask
                               ret = inet_aton(token, &addr);
                               if (ret != 0 ) {
                                       if (is_netmask_valid(ntohl(addr.s_addr) )) {
                                               netmask = strdup(token);
                                               ret = 0;
                                       }
                               }
                               break;
                       default:
                       ret = -1;
                       break;
               }
       }
       if (ret != 0 ) {
               goto end;
       }

       syslog(LOG_DEBUG, "set ip:%s netmask :%s\n", ip, netmask);
       //设置LAN口IP
       set_uci("lan", "ipaddr", ip);

       set_uci("lan", "netmask", netmask);

       free(ip);

       free(netmask);

end:
       free(ip_netmask);
       return ret;
}

uchar AT_lanip(uchar *comm, uchar *out)
{
       uint ret = 0;

       struct uci_network_conf network_conf;
	   memset(&network_conf, 0, sizeof(struct uci_network_conf));

       load_network_profile(&network_conf);
       syslog(LOG_DEBUG, "LAN ip :%s netmask :%s", network_conf.lan.ip, network_conf.lan.netmask);

       if ('?' == *(comm + 1)) {
               sprintf(out, "%s,%s", network_conf.lan.ip, network_conf.lan.netmask);
       } else if ('=' == *(comm + 0)) {
               //设置LAN IP
               ret = set_lan_ip(comm+1);
               if (0 == ret) {
                       sprintf(out, "ok");
               } else {
                       sprintf(out, "error");
               }
       } else {
               sprintf(out, "error");
       }

       free_network(&network_conf);
}

#endif

uchar AT_dhcpd0(uchar *comm, uchar *out)
{
	return AT_para("dhcpd", comm, out);
}
uchar AT_dhcpc0(uchar *comm, uchar *out)
{
	return AT_para("dhcpc", comm, out);
}

/********************************************* 
*   载入配置文件,并遍历Section. 
*/ 


int load_network_profile(struct uci_network_conf *uci_network)
{
       int rc = -1;
    struct uci_package * pkg = NULL;  
    struct uci_element *e, *e1;
       struct uci_ptr ptr;
    char *value = NULL;
       struct uci_option *o;

    struct uci_context *ctx = uci_alloc_context(); // 申请一个UCI上下文.

    if (UCI_OK != uci_load(ctx, "/etc/config/network", &pkg)) {
        printf("open file UCI_CONFIG_FILE fail exit\n");
        goto cleanup; //如果打开UCI文件失败,则跳到末尾 清理 UCI 上下文. 
    }
 
    /*遍历UCI的每一个节*/  
    uci_foreach_element(&pkg->sections, e) {
        struct uci_section *s = uci_to_section(e);
        // 将一个 element 转换为 section类型, 如果节点有名字,则 s->anonymous 为 false.  
        // 此时通过 s->e.name 来获取.
        if (s->anonymous == true) {    //匿名
            continue;
        }

               uci_foreach_element(&s->options, e1) {
                       o = uci_to_option(e1);
                       if (!strcmp(o->section->e.name, "lan")) {
                               //lan口配置
                               if (!strcmp(o->e.name, "ipaddr")) {
                                       uci_network->lan.ip = strdup(o->v.string);
                               }
                               if (!strcmp(o->e.name, "netmask")) {
                                       uci_network->lan.netmask = strdup(o->v.string);
                               }
                               if (!strcmp(o->e.name, "proto")) {
                                       uci_network->lan.proto = strdup(o->v.string);
                               }
                               if (!strcmp(o->e.name, "ifname")) {
                                       uci_network->lan.ifname = strdup(o->v.string);
                               }
                       } else if (!strcmp(o->section->e.name, "wan")) {
                               //wan口配置
                               if (!strcmp(o->e.name, "ipaddr")) {
                                       uci_network->wan.ip = strdup(o->v.string);
                               }
                               if (!strcmp(o->e.name, "netmask")) {
                                       uci_network->wan.netmask = strdup(o->v.string);
                               }
                               if (!strcmp(o->e.name, "proto")) {
                                       uci_network->wan.proto = strdup(o->v.string);
                               }
                               if (!strcmp(o->e.name, "ifname")) {
                                       uci_network->wan.ifname = strdup(o->v.string);
                               }
                       }
               }
    }

    uci_unload(ctx, pkg); // 释放 pkg  

cleanup:  
    //printf("uci load_config exit\n");
    uci_free_context(ctx);  
    ctx = NULL;

    return 0;
}

uchar AT_wanip(uchar *comm, uchar *out)
{
	//判断是否是 动态获取还是 静态分配
	#if 1
	uchar *wanip = netnvram_get(0, "wanip");
	debug_log("record interface :%s", wanip);
	if (  ('?' == *(comm + 1)) && (0 == strcmpT(split(wanip, ",", 0), "dhcp")) ) {
		//动态获取WAN口IP
		char ip[17];
		char bip[17];
		char netmask[17];
		char gateway[17];
		char dns[17];
		uint ret = 0;

		//uci get network.wan.ifname
		//查询wan口 
		//getIfIp();
		char *uci_res = uci_get_option_val("/etc/config/network", "wan", "ifname");
		if (uci_res != NULL) {
			debug_log("interface :%s", uci_res);
			ret = getipinfo(ip, bip, netmask, uci_res);
			
			ret += GetGatewayForInterface(uci_res, gateway);
			if (ret < 0) {
				free(uci_res);
				sprintf(out, "error");
				return 0;
			}

			//获取DNS
			ret = GetDNSForInterface(uci_res, dns);
			if (ret < 0) {
				free(uci_res);
				sprintf(out, "error");
				return 0;
			}

			sprintf(out, "dhcp,%s,%s,%s,%s", ip, netmask, gateway, dns);
			free(uci_res);
		} else {
			sprintf(out, "error");
		}
		return 0;
	}
	#endif
	
	return AT_para("wanip", comm, out);
}

uchar AT_netmode0(uchar *comm, uchar *out)
{
	return AT_para("netmode", comm, out);
}
uchar AT_waninterface(uchar *comm, uchar *out)
{
	return AT_para("waninterface", comm, out);
}
uchar AT_laninterface(uchar *comm, uchar *out)
{
	return AT_para("laninterface", comm, out);
}
uchar AT_eth_endis(uchar *comm, uchar *out)
{
	return AT_para("eth_endis", comm, out);
}
uchar AT_wifi_endis(uchar *comm, uchar *out)
{
	return AT_para("wifi_endis", comm, out);
}

void netapply();

uchar AT_netsaveapply(uchar *comm, uchar *out)
{
	uint ret = 0;

	if ('?' == *(comm + 1))
	{
		sprintf(out, "error");
	}
	else if ('=' == *(comm + 0))
	{

		uchar *p = comm + 1;
		if (strcmp(p, "save") == 0)
		{

			system("uci commit");
			sprintf(out, "ok");
		}
		else if (strcmp(p, "saveapply") == 0)
		{
			netapply();
			sprintf(out, "ok");
			out[256 - 1] = 0;
		}
		else
		{
			sprintf(out, "error");
		}
	}
	else
	{
		sprintf(out, "error");
	}

	return ret;
}

uchar *uci_cmd(uchar *cmd);



void setuci2(uchar *a, uchar *b)
{
	uchar buf[512];
	uchar buf2[512];
	strcpyT(buf, b, sizeof(buf) - 1);
	snprintf(buf2, sizeof(buf2) - 1, a, buf);
	snprintf(buf, sizeof(buf) - 1, "uci set %s", buf2);


	if (strstr(buf2, "='"))
	{
		system(buf);
	}
	else
	{
		uchar tt[256];
		uchar needexec = 1;

		strcpyT2(tt, buf2, sizeof(tt) - 1, '=');

		uchar cmd[256];

		snprintf(cmd, sizeof(cmd) - 1, "uci show %s", tt);
		uchar *cret, *p, *p2;

		cret = uci_cmd(cmd);

		if (cret == 0)
		{
		}
		else
		{
			if (strstr(cret, buf2))
			{
				needexec = 0;
			}
			else
			{
				uchar *p1, *p2;
				p1 = strstr(cret, "=");
				p2 = strstr(buf2, "=");
				if (p1 && p2 && strstr(p1, p2))
				{
					needexec = 0;
				}
			}
		}

		if (needexec)
		{
			system(buf);
		}
	}
}

void setuci(uchar *str)
{
	uchar *p;
	uint i = 0;
	while (strcmp((p = split(str, "\r\n", i++)), "") != 0)
	{
		setuci2("%s", p);
	}
}

void cfgwanip()
{
	uchar *wanip = netnvram_get(0, "wanip");
	if (strcmpT(split(wanip, ",", 0), "dhcp") == 0)
	{
		setuci2("network.wan.proto='%s'", "dhcp");
	}
	else if (strcmpT(split(wanip, ",", 0), "static") == 0)
	{
		setuci2("network.wan.proto='%s'", "static");
		setuci2("network.wan.proto='%s'", split(wanip, ",", 0));
		setuci2("network.wan.ipaddr='%s'", split(wanip, ",", 1));
		setuci2("network.wan.netmask='%s'", split(wanip, ",", 2));
		setuci2("network.wan.gateway='%s'", split(wanip, ",", 3));
		setuci2("network.wan.dns='%s'", split(wanip, ",", 4));
	}
}

void setwififile(uchar *file, uchar *a, uchar *b, uchar *c)
{
	uchar buf[512];
	uchar buf2[512];

	strcpyT(buf, c, sizeof(buf) - 1);

	snprintf(buf2, sizeof(buf2) - 1, b, buf);
	snprintf(buf, sizeof(buf) - 1, "wificonf -f %s set %s \"%s\"", file, a, buf2);

	system(buf);
}

void cfgdhcpd()
{
	uchar *dhcpd = netnvram_get(0, "dhcpd");

	setuci(

		//"dhcp.lan=dhcp\r\n"\


		"dhcp.lan.interface='lan'\r\n"
		"dhcp.lan.dhcpv6='server'\r\n"
		"dhcp.lan.ra='server'\r\n"
		"dhcp.lan.ra_management='1'\r\n"
		"dhcp.lan.start='100'\r\n"
		"dhcp.lan.limit='150'\r\n"
		"dhcp.lan.leasetime='12h'\r\n"

	);

	setuci2("dhcp.lan.ignore='%s'", split(dhcpd, ",", 0));
	setuci2("dhcp.lan.force='%s'", "1");
}

void cfglanip()
{
	uchar *lanip = netnvram_get(0, "lanip");

	setuci(

		"network.lan.proto='static'\r\n"
		"network.lan.ip6assign='60'\r\n");

	setuci2("network.lan.ipaddr='%s'", split(lanip, ",", 0));
	setuci2("network.lan.netmask='%s'", split(lanip, ",", 1));

	cfgdhcpd();
}

uchar cfgwifi_hs2nwifi() //obsolete
{
	/*

wificonf -f /etc/wireless/mediatek/mt7915.dbdc.b0.dat get SSID1

wificonf -f /etc/wireless/mediatek/mt7915.dbdc.b0.dat set SSID1 "dkejksdf"

*/

	system("rm -f /tmp/hs2nwifi_0.dat;cp /etc/wireless/mediatek/mt7915.dbdc.b0.dat /tmp/hs2nwifi_0.dat");
	system("rm -f /tmp/hs2nwifi_1.dat;cp /etc/wireless/mediatek/mt7915.dbdc.b1.dat /tmp/hs2nwifi_1.dat");

	uchar *p;

	TIM_DEBUG("In:%s(%d)\r\n", __FUNCTION__, __LINE__);

	p = netnvram_get(0, "ap2_4");

	setwififile("/tmp/hs2nwifi_0.dat", "SSID1", "%s", split(p, ",", 0));

	uchar *pfile = "/tmp/hs2nwifi_0.dat";

	uchar *authtype = split(p, ",", 1);
	if (strcmpT(authtype, "none") == 0)
	{
		setwififile(pfile, "AuthMode", "%s", "OPEN");
		setwififile(pfile, "EncrypType", "%s", "NONE");
	}
	else if (strcmpT(authtype, "wep") == 0)
	{
		setwififile(pfile, "AuthMode", "%s", "SHARED");
		setwififile(pfile, "EncrypType", "%s", "WEP");
		setwififile(pfile, "DefaultKeyID", "%s", "1");
		setwififile(pfile, "Key1Type", "%s", "1");
		setwififile(pfile, "Key1Str1", "%s", split(p, ",", 2));
	}
	else if (strcmpT(authtype, "wep_open") == 0)
	{
		setwififile(pfile, "AuthMode", "%s", "OPEN");
		setwififile(pfile, "EncrypType", "%s", "WEP");
		setwififile(pfile, "DefaultKeyID", "%s", "1");
		setwififile(pfile, "Key1Type", "%s", "1");
		setwififile(pfile, "Key1Str1", "%s", split(p, ",", 2));
	}
	else if (strcmpT(authtype, "wpa_tkip") == 0)
	{
		setwififile(pfile, "AuthMode", "%s", "WPAPSK");
		setwififile(pfile, "EncrypType", "%s", "TKIP");
		setwififile(pfile, "WPAPSK1", "%s", split(p, ",", 2));
	}
	else if (strcmpT(authtype, "wpa_aes") == 0)
	{
		setwififile(pfile, "AuthMode", "%s", "WPAPSK");
		setwififile(pfile, "EncrypType", "%s", "AES");
		setwififile(pfile, "WPAPSK1", "%s", split(p, ",", 2));
	}
	else if (strcmpT(authtype, "wpa2_tkip") == 0)
	{
		setwififile(pfile, "AuthMode", "%s", "WPA2PSK");
		setwififile(pfile, "EncrypType", "%s", "TKIP");
		setwififile(pfile, "WPAPSK1", "%s", split(p, ",", 2));
	}
	else if (strcmpT(authtype, "wpa2_aes") == 0)
	{
		setwififile(pfile, "AuthMode", "%s", "WPA2PSK");
		setwififile(pfile, "EncrypType", "%s", "AES");
		setwififile(pfile, "WPAPSK1", "%s", split(p, ",", 2));
	}
	else if (strcmpT(authtype, "wpawpa2_tkip") == 0)
	{
		setwififile(pfile, "AuthMode", "%s", "WPAPSKWPA2PSK");
		setwififile(pfile, "EncrypType", "%s", "TKIP");
		setwififile(pfile, "WPAPSK1", "%s", split(p, ",", 2));
	}
	else if (strcmpT(authtype, "wpawpa2_aes") == 0)
	{
		setwififile(pfile, "AuthMode", "%s", "WPAPSKWPA2PSK");
		setwififile(pfile, "EncrypType", "%s", "AES");
		setwififile(pfile, "WPAPSK1", "%s", split(p, ",", 2));
	}
	else if (strcmpT(authtype, "wpa2wpa3_aes") == 0)
	{
		setwififile(pfile, "AuthMode", "%s", "WPA2PSKWPA3PSK");
		setwififile(pfile, "EncrypType", "%s", "AES");
		setwififile(pfile, "WPAPSK1", "%s", split(p, ",", 2));
	}

	if ((strcmpT(split(p, ",", 4), "0") == 0) || (strcmpT(split(p, ",", 4), "") == 0))
	{
		setwififile(pfile, "AutoChannelSelect", "%s", "3");
	}
	else
	{
		setwififile(pfile, "AutoChannelSelect", "%s", "0");
		setwififile(pfile, "Channel", "%s", split(p, ",", 4));
	}

	{
		uchar *p;

		uchar *pfile = "/tmp/hs2nwifi_1.dat";

		TIM_DEBUG("In:%s(%d)\r\n", __FUNCTION__, __LINE__);

		p = netnvram_get(0, "ap5_8");

		setwififile(pfile, "SSID1", "%s", split(p, ",", 0));

		uchar *authtype = split(p, ",", 1);
		if (strcmpT(authtype, "none") == 0)
		{
			setwififile(pfile, "AuthMode", "%s", "OPEN");
			setwififile(pfile, "EncrypType", "%s", "NONE");
		}
		else if (strcmpT(authtype, "wep") == 0)
		{
			setwififile(pfile, "AuthMode", "%s", "SHARED");
			setwififile(pfile, "EncrypType", "%s", "WEP");
			setwififile(pfile, "DefaultKeyID", "%s", "1");
			setwififile(pfile, "Key1Type", "%s", "1");
			setwififile(pfile, "Key1Str1", "%s", split(p, ",", 2));
		}
		else if (strcmpT(authtype, "wep_open") == 0)
		{
			setwififile(pfile, "AuthMode", "%s", "OPEN");
			setwififile(pfile, "EncrypType", "%s", "WEP");
			setwififile(pfile, "DefaultKeyID", "%s", "1");
			setwififile(pfile, "Key1Type", "%s", "1");
			setwififile(pfile, "Key1Str1", "%s", split(p, ",", 2));
		}
		else if (strcmpT(authtype, "wpa_tkip") == 0)
		{
			setwififile(pfile, "AuthMode", "%s", "WPAPSK");
			setwififile(pfile, "EncrypType", "%s", "TKIP");
			setwififile(pfile, "WPAPSK1", "%s", split(p, ",", 2));
		}
		else if (strcmpT(authtype, "wpa_aes") == 0)
		{
			setwififile(pfile, "AuthMode", "%s", "WPAPSK");
			setwififile(pfile, "EncrypType", "%s", "AES");
			setwififile(pfile, "WPAPSK1", "%s", split(p, ",", 2));
		}
		else if (strcmpT(authtype, "wpa2_tkip") == 0)
		{
			setwififile(pfile, "AuthMode", "%s", "WPA2PSK");
			setwififile(pfile, "EncrypType", "%s", "TKIP");
			setwififile(pfile, "WPAPSK1", "%s", split(p, ",", 2));
		}
		else if (strcmpT(authtype, "wpa2_aes") == 0)
		{
			setwififile(pfile, "AuthMode", "%s", "WPA2PSK");
			setwififile(pfile, "EncrypType", "%s", "AES");
			setwififile(pfile, "WPAPSK1", "%s", split(p, ",", 2));
		}
		else if (strcmpT(authtype, "wpawpa2_tkip") == 0)
		{
			setwififile(pfile, "AuthMode", "%s", "WPAPSKWPA2PSK");
			setwififile(pfile, "EncrypType", "%s", "TKIP");
			setwififile(pfile, "WPAPSK1", "%s", split(p, ",", 2));
		}
		else if (strcmpT(authtype, "wpawpa2_aes") == 0)
		{
			setwififile(pfile, "AuthMode", "%s", "WPAPSKWPA2PSK");
			setwififile(pfile, "EncrypType", "%s", "AES");
			setwififile(pfile, "WPAPSK1", "%s", split(p, ",", 2));
		}
		else if (strcmpT(authtype, "wpa2wpa3_aes") == 0)
		{
			setwififile(pfile, "AuthMode", "%s", "WPA2PSKWPA3PSK");
			setwififile(pfile, "EncrypType", "%s", "AES");
			setwififile(pfile, "WPAPSK1", "%s", split(p, ",", 2));
		}

		if ((strcmpT(split(p, ",", 4), "0") == 0) || (strcmpT(split(p, ",", 4), "") == 0))
		{
			setwififile(pfile, "AutoChannelSelect", "%s", "3");
		}
		else
		{
			setwififile(pfile, "AutoChannelSelect", "%s", "0");
			setwififile(pfile, "Channel", "%s", split(p, ",", 4));
		}
	}

	{

		uchar *pfile = "/tmp/hs2nwifi_0.dat";

		p = netnvram_get(0, "sta2_4");

		setwififile(pfile, "ApCliSsid", "%s", split(p, ",", 0));

		uchar *authtype = split(p, ",", 1);
		if (strcmpT(authtype, "none") == 0)
		{
			setwififile(pfile, "ApCliAuthMode", "%s", "OPEN");
			setwififile(pfile, "ApCliEncrypType", "%s", "NONE");
		}
		else if (strcmpT(authtype, "wep") == 0)
		{
			setwififile(pfile, "ApCliAuthMode", "%s", "SHARED");
			setwififile(pfile, "ApCliEncrypType", "%s", "WEP");
			setwififile(pfile, "ApCliDefaultKeyID", "%s", "1");
			setwififile(pfile, "ApCliKey1Type", "%s", "1");
			setwififile(pfile, "ApCliKey1Str1", "%s", split(p, ",", 2));
		}
		else if (strcmpT(authtype, "wep_open") == 0)
		{
			setwififile(pfile, "ApCliAuthMode", "%s", "OPEN");
			setwififile(pfile, "ApCliEncrypType", "%s", "WEP");
			setwififile(pfile, "ApCliDefaultKeyID", "%s", "1");
			setwififile(pfile, "ApCliKey1Type", "%s", "1");
			setwififile(pfile, "ApCliKey1Str1", "%s", split(p, ",", 2));
		}
		else if (strcmpT(authtype, "wpa_tkip") == 0)
		{
			setwififile(pfile, "ApCliAuthMode", "%s", "WPAPSK");
			setwififile(pfile, "ApCliEncrypType", "%s", "TKIP");
			setwififile(pfile, "ApCliWPAPSK", "%s", split(p, ",", 2));
		}
		else if (strcmpT(authtype, "wpa_aes") == 0)
		{
			setwififile(pfile, "ApCliAuthMode", "%s", "WPAPSK");
			setwififile(pfile, "ApCliEncrypType", "%s", "AES");
			setwififile(pfile, "ApCliWPAPSK", "%s", split(p, ",", 2));
		}
		else if (strcmpT(authtype, "wpa2_tkip") == 0)
		{
			setwififile(pfile, "ApCliAuthMode", "%s", "WPA2PSK");
			setwififile(pfile, "ApCliEncrypType", "%s", "TKIP");
			setwififile(pfile, "ApCliWPAPSK", "%s", split(p, ",", 2));
		}
		else if (strcmpT(authtype, "wpa2_aes") == 0)
		{
			setwififile(pfile, "ApCliAuthMode", "%s", "WPA2PSK");
			setwififile(pfile, "ApCliEncrypType", "%s", "AES");
			setwififile(pfile, "ApCliWPAPSK", "%s", split(p, ",", 2));
		}
		else if (strcmpT(authtype, "wpawpa2_tkip") == 0)
		{
			setwififile(pfile, "ApCliAuthMode", "%s", "WPAPSKWPA2PSK");
			setwififile(pfile, "ApCliEncrypType", "%s", "TKIP");
			setwififile(pfile, "ApCliWPAPSK", "%s", split(p, ",", 2));
		}
		else if (strcmpT(authtype, "wpawpa2_aes") == 0)
		{
			setwififile(pfile, "ApCliAuthMode", "%s", "WPAPSKWPA2PSK");
			setwififile(pfile, "ApCliEncrypType", "%s", "AES");
			setwififile(pfile, "ApCliWPAPSK", "%s", split(p, ",", 2));
		}
		else if (strcmpT(authtype, "wpa2wpa3_aes") == 0)
		{
			setwififile(pfile, "ApCliAuthMode", "%s", "WPA2PSKWPA3PSK");
			setwififile(pfile, "ApCliEncrypType", "%s", "AES");
			setwififile(pfile, "ApCliWPAPSK", "%s", split(p, ",", 2));
		}

		if ((strcmpT(split(p, ",", 4), "0") == 0) || (strcmpT(split(p, ",", 4), "") == 0))
		{
			setwififile(pfile, "AutoChannelSelect", "%s", "3");
		}
		else
		{
			setwififile(pfile, "AutoChannelSelect", "%s", "0");
			setwififile(pfile, "Channel", "%s", split(p, ",", 4));
		}
	}
	{

		uchar *pfile = "/tmp/hs2nwifi_1.dat";

		p = netnvram_get(0, "sta5_8");

		setwififile(pfile, "ApCliSsid", "%s", split(p, ",", 0));

		uchar *authtype = split(p, ",", 1);
		if (strcmpT(authtype, "none") == 0)
		{
			setwififile(pfile, "ApCliAuthMode", "%s", "OPEN");
			setwififile(pfile, "ApCliEncrypType", "%s", "NONE");
		}
		else if (strcmpT(authtype, "wep") == 0)
		{
			setwififile(pfile, "ApCliAuthMode", "%s", "SHARED");
			setwififile(pfile, "ApCliEncrypType", "%s", "WEP");
			setwififile(pfile, "ApCliDefaultKeyID", "%s", "1");
			setwififile(pfile, "ApCliKey1Type", "%s", "1");
			setwififile(pfile, "ApCliKey1Str1", "%s", split(p, ",", 2));
		}
		else if (strcmpT(authtype, "wep_open") == 0)
		{
			setwififile(pfile, "ApCliAuthMode", "%s", "OPEN");
			setwififile(pfile, "ApCliEncrypType", "%s", "WEP");
			setwififile(pfile, "ApCliDefaultKeyID", "%s", "1");
			setwififile(pfile, "ApCliKey1Type", "%s", "1");
			setwififile(pfile, "ApCliKey1Str1", "%s", split(p, ",", 2));
		}
		else if (strcmpT(authtype, "wpa_tkip") == 0)
		{
			setwififile(pfile, "ApCliAuthMode", "%s", "WPAPSK");
			setwififile(pfile, "ApCliEncrypType", "%s", "TKIP");
			setwififile(pfile, "ApCliWPAPSK", "%s", split(p, ",", 2));
		}
		else if (strcmpT(authtype, "wpa_aes") == 0)
		{
			setwififile(pfile, "ApCliAuthMode", "%s", "WPAPSK");
			setwififile(pfile, "ApCliEncrypType", "%s", "AES");
			setwififile(pfile, "ApCliWPAPSK", "%s", split(p, ",", 2));
		}
		else if (strcmpT(authtype, "wpa2_tkip") == 0)
		{
			setwififile(pfile, "ApCliAuthMode", "%s", "WPA2PSK");
			setwififile(pfile, "ApCliEncrypType", "%s", "TKIP");
			setwififile(pfile, "ApCliWPAPSK", "%s", split(p, ",", 2));
		}
		else if (strcmpT(authtype, "wpa2_aes") == 0)
		{
			setwififile(pfile, "ApCliAuthMode", "%s", "WPA2PSK");
			setwififile(pfile, "ApCliEncrypType", "%s", "AES");
			setwififile(pfile, "ApCliWPAPSK", "%s", split(p, ",", 2));
		}
		else if (strcmpT(authtype, "wpawpa2_tkip") == 0)
		{
			setwififile(pfile, "ApCliAuthMode", "%s", "WPAPSKWPA2PSK");
			setwififile(pfile, "ApCliEncrypType", "%s", "TKIP");
			setwififile(pfile, "ApCliWPAPSK", "%s", split(p, ",", 2));
		}
		else if (strcmpT(authtype, "wpawpa2_aes") == 0)
		{
			setwififile(pfile, "ApCliAuthMode", "%s", "WPAPSKWPA2PSK");
			setwififile(pfile, "ApCliEncrypType", "%s", "AES");
			setwififile(pfile, "ApCliWPAPSK", "%s", split(p, ",", 2));
		}
		else if (strcmpT(authtype, "wpa2wpa3_aes") == 0)
		{
			setwififile(pfile, "ApCliAuthMode", "%s", "WPA2PSKWPA3PSK");
			setwififile(pfile, "ApCliEncrypType", "%s", "AES");
			setwififile(pfile, "ApCliWPAPSK", "%s", split(p, ",", 2));
		}

		if ((strcmpT(split(p, ",", 4), "0") == 0) || (strcmpT(split(p, ",", 4), "") == 0))
		{
			setwififile(pfile, "AutoChannelSelect", "%s", "3");
		}
		else
		{
			setwififile(pfile, "AutoChannelSelect", "%s", "0");
			setwififile(pfile, "Channel", "%s", split(p, ",", 4));
		}
	}

	uchar uret = 0;

	{
		uchar *cret = uci_cmd("cmp /etc/wireless/mediatek/mt7915.dbdc.b0.dat /tmp/hs2nwifi_0.dat&& echo 'files are same' || echo 'files are different'");
		if (cret && strstr(cret, "differ"))
		{
			TIM_DEBUG("In:%s(%d)\r\n", __FUNCTION__, __LINE__);
			system("cp /tmp/hs2nwifi_0.dat /etc/wireless/mediatek/mt7915.dbdc.b0.dat");
			uret = 1;
		}
	}
	{
		uchar *cret = uci_cmd("cmp /etc/wireless/mediatek/mt7915.dbdc.b1.dat /tmp/hs2nwifi_1.dat&& echo 'files are same' || echo 'files are different'");
		if (cret && strstr(cret, "differ"))
		{
			TIM_DEBUG("In:%s(%d)\r\n", __FUNCTION__, __LINE__);
			system("cp /tmp/hs2nwifi_1.dat /etc/wireless/mediatek/mt7915.dbdc.b1.dat");
			uret = 1;
		}
	}

	return uret;
}

uchar cfgwifienable(uchar *s) //obsolete
{
	/*

wificonf -f /etc/wireless/mediatek/mt7915.dbdc.b0.dat set ApEnable 0

*/

	system("rm -f /tmp/hs2nwifi_0.dat;cp /etc/wireless/mediatek/mt7915.dbdc.b0.dat /tmp/hs2nwifi_0.dat");
	system("rm -f /tmp/hs2nwifi_1.dat;cp /etc/wireless/mediatek/mt7915.dbdc.b1.dat /tmp/hs2nwifi_1.dat");

	setwififile("/tmp/hs2nwifi_0.dat", "ApEnable", "%s", split(s, ",", 0));
	setwififile("/tmp/hs2nwifi_0.dat", "ApCliEnable", "%s", split(s, ",", 1));

	setwififile("/tmp/hs2nwifi_1.dat", "ApEnable", "%s", split(s, ",", 2));
	setwififile("/tmp/hs2nwifi_1.dat", "ApCliEnable", "%s", split(s, ",", 3));

	uchar uret = 0;

	{
		uchar *cret = uci_cmd("cmp /etc/wireless/mediatek/mt7915.dbdc.b0.dat /tmp/hs2nwifi_0.dat&& echo 'files are same' || echo 'files are different'");
		if (cret && strstr(cret, "differ"))
		{
			TIM_DEBUG("In:%s(%d)\r\n", __FUNCTION__, __LINE__);
			system("cp /tmp/hs2nwifi_0.dat /etc/wireless/mediatek/mt7915.dbdc.b0.dat");
			uret = 1;
		}
	}
	{
		uchar *cret = uci_cmd("cmp /etc/wireless/mediatek/mt7915.dbdc.b1.dat /tmp/hs2nwifi_1.dat&& echo 'files are same' || echo 'files are different'");
		if (cret && strstr(cret, "differ"))
		{
			TIM_DEBUG("In:%s(%d)\r\n", __FUNCTION__, __LINE__);
			system("cp /tmp/hs2nwifi_1.dat /etc/wireless/mediatek/mt7915.dbdc.b1.dat");
			uret = 1;
		}
	}

	return uret;
}

uchar cfgwifienable2(uchar *s)
{
	/*

wificonf -f /etc/wireless/mediatek/mt7915.dbdc.b0.dat set ApEnable 0

*/

	system("rm -f /tmp/hs2nwifi_0.dat;cp /etc/wireless/mediatek/mt7915.dbdc.b0.dat /tmp/hs2nwifi_0.dat");
	system("rm -f /tmp/hs2nwifi_1.dat;cp /etc/wireless/mediatek/mt7915.dbdc.b1.dat /tmp/hs2nwifi_1.dat");

	setwififile("/tmp/hs2nwifi_0.dat", "ApEnable", "%s", split(s, ",", 0));
	setwififile("/tmp/hs2nwifi_0.dat", "ApCliEnable", "%s", split(s, ",", 1));

	setwififile("/tmp/hs2nwifi_1.dat", "ApEnable", "%s", split(s, ",", 2));
	setwififile("/tmp/hs2nwifi_1.dat", "ApCliEnable", "%s", split(s, ",", 3));

	uchar ss[32];
	snprintf(ss, sizeof(ss) - 1, s);

	if (strcmpT(split(ss, ",", 0), "1") == 0)
	{
		uchar *p;

		p = netnvram_get(0, "ap2_4");

		setwififile("/tmp/hs2nwifi_0.dat", "SSID1", "%s", split(p, ",", 0));

		uchar *pfile = "/tmp/hs2nwifi_0.dat";

		uchar *authtype = split(p, ",", 1);
		if (strcmpT(authtype, "none") == 0)
		{
			setwififile(pfile, "AuthMode", "%s", "OPEN");
			setwififile(pfile, "EncrypType", "%s", "NONE");
		}
		else if (strcmpT(authtype, "wep") == 0)
		{
			setwififile(pfile, "AuthMode", "%s", "SHARED");
			setwififile(pfile, "EncrypType", "%s", "WEP");
			setwififile(pfile, "DefaultKeyID", "%s", "1");
			setwififile(pfile, "Key1Type", "%s", "1");
			setwififile(pfile, "Key1Str1", "%s", split(p, ",", 2));
		}
		else if (strcmpT(authtype, "wep_open") == 0)
		{
			setwififile(pfile, "AuthMode", "%s", "OPEN");
			setwififile(pfile, "EncrypType", "%s", "WEP");
			setwififile(pfile, "DefaultKeyID", "%s", "1");
			setwififile(pfile, "Key1Type", "%s", "1");
			setwififile(pfile, "Key1Str1", "%s", split(p, ",", 2));
		}
		else if (strcmpT(authtype, "wpa_tkip") == 0)
		{
			setwififile(pfile, "AuthMode", "%s", "WPAPSK");
			setwififile(pfile, "EncrypType", "%s", "TKIP");
			setwififile(pfile, "WPAPSK1", "%s", split(p, ",", 2));
		}
		else if (strcmpT(authtype, "wpa_aes") == 0)
		{
			setwififile(pfile, "AuthMode", "%s", "WPAPSK");
			setwififile(pfile, "EncrypType", "%s", "AES");
			setwififile(pfile, "WPAPSK1", "%s", split(p, ",", 2));
		}
		else if (strcmpT(authtype, "wpa2_tkip") == 0)
		{
			setwififile(pfile, "AuthMode", "%s", "WPA2PSK");
			setwififile(pfile, "EncrypType", "%s", "TKIP");
			setwififile(pfile, "WPAPSK1", "%s", split(p, ",", 2));
		}
		else if (strcmpT(authtype, "wpa2_aes") == 0)
		{
			setwififile(pfile, "AuthMode", "%s", "WPA2PSK");
			setwififile(pfile, "EncrypType", "%s", "AES");
			setwififile(pfile, "WPAPSK1", "%s", split(p, ",", 2));
		}
		else if (strcmpT(authtype, "wpawpa2_tkip") == 0)
		{
			setwififile(pfile, "AuthMode", "%s", "WPAPSKWPA2PSK");
			setwififile(pfile, "EncrypType", "%s", "TKIP");
			setwififile(pfile, "WPAPSK1", "%s", split(p, ",", 2));
		}
		else if (strcmpT(authtype, "wpawpa2_aes") == 0)
		{
			setwififile(pfile, "AuthMode", "%s", "WPAPSKWPA2PSK");
			setwififile(pfile, "EncrypType", "%s", "AES");
			setwififile(pfile, "WPAPSK1", "%s", split(p, ",", 2));
		}
		else if (strcmpT(authtype, "wpa2wpa3_aes") == 0)
		{
			setwififile(pfile, "AuthMode", "%s", "WPA2PSKWPA3PSK");
			setwififile(pfile, "EncrypType", "%s", "AES");
			setwififile(pfile, "WPAPSK1", "%s", split(p, ",", 2));
		}

		if ((strcmpT(split(p, ",", 4), "0") == 0) || (strcmpT(split(p, ",", 4), "") == 0))
		{
			setwififile(pfile, "AutoChannelSelect", "%s", "3");
		}
		else
		{
			setwififile(pfile, "AutoChannelSelect", "%s", "0");
			setwififile(pfile, "Channel", "%s", split(p, ",", 4));
		}
	}

	if (strcmpT(split(ss, ",", 1), "1") == 0)
	{

		uchar *pfile = "/tmp/hs2nwifi_0.dat";
		uchar *p = netnvram_get(0, "sta2_4");

		setwififile(pfile, "ApCliSsid", "%s", split(p, ",", 0));

		uchar *authtype = split(p, ",", 1);
		if (strcmpT(authtype, "none") == 0)
		{
			setwififile(pfile, "ApCliAuthMode", "%s", "OPEN");
			setwififile(pfile, "ApCliEncrypType", "%s", "NONE");
		}
		else if (strcmpT(authtype, "wep") == 0)
		{
			setwififile(pfile, "ApCliAuthMode", "%s", "SHARED");
			setwififile(pfile, "ApCliEncrypType", "%s", "WEP");
			setwififile(pfile, "ApCliDefaultKeyID", "%s", "1");
			setwififile(pfile, "ApCliKey1Type", "%s", "1");
			setwififile(pfile, "ApCliKey1Str1", "%s", split(p, ",", 2));
		}
		else if (strcmpT(authtype, "wep_open") == 0)
		{
			setwififile(pfile, "ApCliAuthMode", "%s", "OPEN");
			setwififile(pfile, "ApCliEncrypType", "%s", "WEP");
			setwififile(pfile, "ApCliDefaultKeyID", "%s", "1");
			setwififile(pfile, "ApCliKey1Type", "%s", "1");
			setwififile(pfile, "ApCliKey1Str1", "%s", split(p, ",", 2));
		}
		else if (strcmpT(authtype, "wpa_tkip") == 0)
		{
			setwififile(pfile, "ApCliAuthMode", "%s", "WPAPSK");
			setwififile(pfile, "ApCliEncrypType", "%s", "TKIP");
			setwififile(pfile, "ApCliWPAPSK", "%s", split(p, ",", 2));
		}
		else if (strcmpT(authtype, "wpa_aes") == 0)
		{
			setwififile(pfile, "ApCliAuthMode", "%s", "WPAPSK");
			setwififile(pfile, "ApCliEncrypType", "%s", "AES");
			setwififile(pfile, "ApCliWPAPSK", "%s", split(p, ",", 2));
		}
		else if (strcmpT(authtype, "wpa2_tkip") == 0)
		{
			setwififile(pfile, "ApCliAuthMode", "%s", "WPA2PSK");
			setwififile(pfile, "ApCliEncrypType", "%s", "TKIP");
			setwififile(pfile, "ApCliWPAPSK", "%s", split(p, ",", 2));
		}
		else if (strcmpT(authtype, "wpa2_aes") == 0)
		{
			setwififile(pfile, "ApCliAuthMode", "%s", "WPA2PSK");
			setwififile(pfile, "ApCliEncrypType", "%s", "AES");
			setwififile(pfile, "ApCliWPAPSK", "%s", split(p, ",", 2));
		}
		else if (strcmpT(authtype, "wpawpa2_tkip") == 0)
		{
			setwififile(pfile, "ApCliAuthMode", "%s", "WPAPSKWPA2PSK");
			setwififile(pfile, "ApCliEncrypType", "%s", "TKIP");
			setwififile(pfile, "ApCliWPAPSK", "%s", split(p, ",", 2));
		}
		else if (strcmpT(authtype, "wpawpa2_aes") == 0)
		{
			setwififile(pfile, "ApCliAuthMode", "%s", "WPAPSKWPA2PSK");
			setwififile(pfile, "ApCliEncrypType", "%s", "AES");
			setwififile(pfile, "ApCliWPAPSK", "%s", split(p, ",", 2));
		}
		else if (strcmpT(authtype, "wpa2wpa3_aes") == 0)
		{
			setwififile(pfile, "ApCliAuthMode", "%s", "WPA2PSKWPA3PSK");
			setwififile(pfile, "ApCliEncrypType", "%s", "AES");
			setwififile(pfile, "ApCliWPAPSK", "%s", split(p, ",", 2));
		}

		if ((strcmpT(split(p, ",", 4), "0") == 0) || (strcmpT(split(p, ",", 4), "") == 0))
		{
			setwififile(pfile, "AutoChannelSelect", "%s", "3");
		}
		else
		{
			setwififile(pfile, "AutoChannelSelect", "%s", "0");
			setwififile(pfile, "Channel", "%s", split(p, ",", 4));
		}
	}

	if (strcmpT(split(ss, ",", 2), "1") == 0)
	{
		uchar *p;

		uchar *pfile = "/tmp/hs2nwifi_1.dat";

		TIM_DEBUG("In:%s(%d)\r\n", __FUNCTION__, __LINE__);

		p = netnvram_get(0, "ap5_8");

		setwififile(pfile, "SSID1", "%s", split(p, ",", 0));

		uchar *authtype = split(p, ",", 1);
		if (strcmpT(authtype, "none") == 0)
		{
			setwififile(pfile, "AuthMode", "%s", "OPEN");
			setwififile(pfile, "EncrypType", "%s", "NONE");
		}
		else if (strcmpT(authtype, "wep") == 0)
		{
			setwififile(pfile, "AuthMode", "%s", "SHARED");
			setwififile(pfile, "EncrypType", "%s", "WEP");
			setwififile(pfile, "DefaultKeyID", "%s", "1");
			setwififile(pfile, "Key1Type", "%s", "1");
			setwififile(pfile, "Key1Str1", "%s", split(p, ",", 2));
		}
		else if (strcmpT(authtype, "wep_open") == 0)
		{
			setwififile(pfile, "AuthMode", "%s", "OPEN");
			setwififile(pfile, "EncrypType", "%s", "WEP");
			setwififile(pfile, "DefaultKeyID", "%s", "1");
			setwififile(pfile, "Key1Type", "%s", "1");
			setwififile(pfile, "Key1Str1", "%s", split(p, ",", 2));
		}
		else if (strcmpT(authtype, "wpa_tkip") == 0)
		{
			setwififile(pfile, "AuthMode", "%s", "WPAPSK");
			setwififile(pfile, "EncrypType", "%s", "TKIP");
			setwififile(pfile, "WPAPSK1", "%s", split(p, ",", 2));
		}
		else if (strcmpT(authtype, "wpa_aes") == 0)
		{
			setwififile(pfile, "AuthMode", "%s", "WPAPSK");
			setwififile(pfile, "EncrypType", "%s", "AES");
			setwififile(pfile, "WPAPSK1", "%s", split(p, ",", 2));
		}
		else if (strcmpT(authtype, "wpa2_tkip") == 0)
		{
			setwififile(pfile, "AuthMode", "%s", "WPA2PSK");
			setwififile(pfile, "EncrypType", "%s", "TKIP");
			setwififile(pfile, "WPAPSK1", "%s", split(p, ",", 2));
		}
		else if (strcmpT(authtype, "wpa2_aes") == 0)
		{
			setwififile(pfile, "AuthMode", "%s", "WPA2PSK");
			setwififile(pfile, "EncrypType", "%s", "AES");
			setwififile(pfile, "WPAPSK1", "%s", split(p, ",", 2));
		}
		else if (strcmpT(authtype, "wpawpa2_tkip") == 0)
		{
			setwififile(pfile, "AuthMode", "%s", "WPAPSKWPA2PSK");
			setwififile(pfile, "EncrypType", "%s", "TKIP");
			setwififile(pfile, "WPAPSK1", "%s", split(p, ",", 2));
		}
		else if (strcmpT(authtype, "wpawpa2_aes") == 0)
		{
			setwififile(pfile, "AuthMode", "%s", "WPAPSKWPA2PSK");
			setwififile(pfile, "EncrypType", "%s", "AES");
			setwififile(pfile, "WPAPSK1", "%s", split(p, ",", 2));
		}
		else if (strcmpT(authtype, "wpa2wpa3_aes") == 0)
		{
			setwififile(pfile, "AuthMode", "%s", "WPA2PSKWPA3PSK");
			setwififile(pfile, "EncrypType", "%s", "AES");
			setwififile(pfile, "WPAPSK1", "%s", split(p, ",", 2));
		}

		if ((strcmpT(split(p, ",", 4), "0") == 0) || (strcmpT(split(p, ",", 4), "") == 0))
		{
			setwififile(pfile, "AutoChannelSelect", "%s", "3");
		}
		else
		{
			setwififile(pfile, "AutoChannelSelect", "%s", "0");
			setwififile(pfile, "Channel", "%s", split(p, ",", 4));
		}
	}
	if (strcmpT(split(ss, ",", 3), "1") == 0)
	{
		uchar *p;

		uchar *pfile = "/tmp/hs2nwifi_1.dat";

		p = netnvram_get(0, "sta5_8");

		setwififile(pfile, "ApCliSsid", "%s", split(p, ",", 0));

		uchar *authtype = split(p, ",", 1);
		if (strcmpT(authtype, "none") == 0)
		{
			setwififile(pfile, "ApCliAuthMode", "%s", "OPEN");
			setwififile(pfile, "ApCliEncrypType", "%s", "NONE");
		}
		else if (strcmpT(authtype, "wep") == 0)
		{
			setwififile(pfile, "ApCliAuthMode", "%s", "SHARED");
			setwififile(pfile, "ApCliEncrypType", "%s", "WEP");
			setwififile(pfile, "ApCliDefaultKeyID", "%s", "1");
			setwififile(pfile, "ApCliKey1Type", "%s", "1");
			setwififile(pfile, "ApCliKey1Str1", "%s", split(p, ",", 2));
		}
		else if (strcmpT(authtype, "wep_open") == 0)
		{
			setwififile(pfile, "ApCliAuthMode", "%s", "OPEN");
			setwififile(pfile, "ApCliEncrypType", "%s", "WEP");
			setwififile(pfile, "ApCliDefaultKeyID", "%s", "1");
			setwififile(pfile, "ApCliKey1Type", "%s", "1");
			setwififile(pfile, "ApCliKey1Str1", "%s", split(p, ",", 2));
		}
		else if (strcmpT(authtype, "wpa_tkip") == 0)
		{
			setwififile(pfile, "ApCliAuthMode", "%s", "WPAPSK");
			setwififile(pfile, "ApCliEncrypType", "%s", "TKIP");
			setwififile(pfile, "ApCliWPAPSK", "%s", split(p, ",", 2));
		}
		else if (strcmpT(authtype, "wpa_aes") == 0)
		{
			setwififile(pfile, "ApCliAuthMode", "%s", "WPAPSK");
			setwififile(pfile, "ApCliEncrypType", "%s", "AES");
			setwififile(pfile, "ApCliWPAPSK", "%s", split(p, ",", 2));
		}
		else if (strcmpT(authtype, "wpa2_tkip") == 0)
		{
			setwififile(pfile, "ApCliAuthMode", "%s", "WPA2PSK");
			setwififile(pfile, "ApCliEncrypType", "%s", "TKIP");
			setwififile(pfile, "ApCliWPAPSK", "%s", split(p, ",", 2));
		}
		else if (strcmpT(authtype, "wpa2_aes") == 0)
		{
			setwififile(pfile, "ApCliAuthMode", "%s", "WPA2PSK");
			setwififile(pfile, "ApCliEncrypType", "%s", "AES");
			setwififile(pfile, "ApCliWPAPSK", "%s", split(p, ",", 2));
		}
		else if (strcmpT(authtype, "wpawpa2_tkip") == 0)
		{
			setwififile(pfile, "ApCliAuthMode", "%s", "WPAPSKWPA2PSK");
			setwififile(pfile, "ApCliEncrypType", "%s", "TKIP");
			setwififile(pfile, "ApCliWPAPSK", "%s", split(p, ",", 2));
		}
		else if (strcmpT(authtype, "wpawpa2_aes") == 0)
		{
			setwififile(pfile, "ApCliAuthMode", "%s", "WPAPSKWPA2PSK");
			setwififile(pfile, "ApCliEncrypType", "%s", "AES");
			setwififile(pfile, "ApCliWPAPSK", "%s", split(p, ",", 2));
		}
		else if (strcmpT(authtype, "wpa2wpa3_aes") == 0)
		{
			setwififile(pfile, "ApCliAuthMode", "%s", "WPA2PSKWPA3PSK");
			setwififile(pfile, "ApCliEncrypType", "%s", "AES");
			setwififile(pfile, "ApCliWPAPSK", "%s", split(p, ",", 2));
		}

		if ((strcmpT(split(p, ",", 4), "0") == 0) || (strcmpT(split(p, ",", 4), "") == 0))
		{
			setwififile(pfile, "AutoChannelSelect", "%s", "3");
		}
		else
		{
			setwififile(pfile, "AutoChannelSelect", "%s", "0");
			setwififile(pfile, "Channel", "%s", split(p, ",", 4));
		}
	}

	uchar uret = 0;

	{
		uchar *cret = uci_cmd("cmp /etc/wireless/mediatek/mt7915.dbdc.b0.dat /tmp/hs2nwifi_0.dat&& echo 'files are same' || echo 'files are different'");
		if (cret && strstr(cret, "differ"))
		{
			system("cp /tmp/hs2nwifi_0.dat /etc/wireless/mediatek/mt7915.dbdc.b0.dat");
			uret = 1;
		}
	}
	{
		uchar *cret = uci_cmd("cmp /etc/wireless/mediatek/mt7915.dbdc.b1.dat /tmp/hs2nwifi_1.dat&& echo 'files are same' || echo 'files are different'");
		if (cret && strstr(cret, "differ"))
		{
			system("cp /tmp/hs2nwifi_1.dat /etc/wireless/mediatek/mt7915.dbdc.b1.dat");
			uret = 1;
		}
	}

	return uret;
}

void cfgethenable(uchar *s)
{
}

uchar ucichanges(uchar *v)
{
	if (strstr(v, "='"))
	{

		return 1;
	}
	return 0;
}

void netapply()
{
	uchar *p;

	uchar wifiNeedReload = 0;
	uchar wifiNeedReload2 = 0;

	p = netnvram_get(0, "netmode");

	if (strcmp(p, "sta2_4") == 0)
	{
		setuci("network.wan=interface\r\n"
			   "network.wan._orig_ifname='eth1'\r\n"
			   "network.wan._orig_bridge='false'\r\n"
			   "network.wan.ifname='apcli0'\r\n"
			   "network.wan6=interface\r\n"
			   "network.wan6.ifname='apcli0'\r\n"
			   "network.wan6.proto='dhcpv6'\r\n"
			   "network.wan6.reqaddress='none'\r\n"
			   "network.wan6.reqprefix='auto'\r\n"
			   "network.@switch[0]=switch\r\n"
			   "network.@switch[0].name='switch0'\r\n"
			   "network.@switch[0].reset='1'\r\n"
			   "network.@switch[0].enable_vlan='1'\r\n"
			   "network.@switch_vlan[0]=switch_vlan\r\n"
			   "network.@switch_vlan[0].device='switch0'\r\n"
			   "network.@switch_vlan[0].vlan='1'\r\n"
			   "network.@switch_vlan[0].ports='0 1 2 3 6'\r\n"
			   "network.@switch_vlan[1]=switch_vlan\r\n"
			   "network.@switch_vlan[1].device='switch0'\r\n"
			   "network.@switch_vlan[1].vlan='2'\r\n"
			   "network.@switch_vlan[1].ports='4 5'\r\n"
			   "network.lan=interface\r\n"
			   "network.lan.type='bridge'\r\n"
			   "network.lan.ifname='eth0'\r\n"
			   );

		cfgwanip();
		cfglanip();

		wifiNeedReload2 = cfgwifienable2("0,1,0,0");
	}
	else if (strcmp(p, "sta5_8") == 0)
	{

		//"network.wan.proto='dhcp'\r\n"\

		setuci("network.wan=interface\r\n"
			   "network.wan.ifname='apclix0'\r\n"
			   "network.wan6=interface\r\n"
			   "network.wan6.ifname='apclix0'\r\n"
			   "network.wan6.proto='dhcpv6'\r\n"
			   "network.wan6.reqaddress='none'\r\n"
			   "network.wan6.reqprefix='auto'\r\n"
			   "network.@switch[0]=switch\r\n"
			   "network.@switch[0].name='switch0'\r\n"
			   "network.@switch[0].reset='1'\r\n"
			   "network.@switch[0].enable_vlan='1'\r\n"
			   "network.@switch_vlan[0]=switch_vlan\r\n"
			   "network.@switch_vlan[0].device='switch0'\r\n"
			   "network.@switch_vlan[0].vlan='1'\r\n"
			   "network.@switch_vlan[0].ports='0 1 2 3 6'\r\n"
			   "network.@switch_vlan[1]=switch_vlan\r\n"
			   "network.@switch_vlan[1].device='switch0'\r\n"
			   "network.@switch_vlan[1].vlan='2'\r\n"
			   "network.@switch_vlan[1].ports='4 5'\r\n"
			   "network.lan=interface\r\n"
			   "network.lan.type='bridge'\r\n"
			   "network.lan.ifname='eth0'\r\n");

		//	system("uci del network.lan");

		cfgwanip();
		cfglanip();

		wifiNeedReload2 = cfgwifienable2("0,0,0,1");
	}
	else if (strcmp(p, "ap2_4") == 0)
	{

		setuci("network.wan=interface\r\n"
			   "network.wan._orig_ifname='eth1'\r\n"
			   "network.wan._orig_bridge='false'\r\n"
			   "network.wan.proto='dhcp'\r\n"
			   "network.wan.ifname='eth1'\r\n"
			   "network.wan6=interface\r\n"
			   "network.wan6.ifname='eth1'\r\n"
			   "network.wan6.proto='dhcpv6'\r\n"
			   "network.wan6.reqaddress='none'\r\n"
			   "network.wan6.reqprefix='auto'\r\n"
			   "network.@switch[0]=switch\r\n"
			   "network.@switch[0].name='switch0'\r\n"
			   "network.@switch[0].reset='1'\r\n"
			   "network.@switch[0].enable_vlan='1'\r\n"
			   "network.@switch_vlan[0]=switch_vlan\r\n"
			   "network.@switch_vlan[0].device='switch0'\r\n"
			   "network.@switch_vlan[0].vlan='1'\r\n"
			   "network.@switch_vlan[0].ports='0 1 2 3 6'\r\n"
			   "network.@switch_vlan[1]=switch_vlan\r\n"
			   "network.@switch_vlan[1].device='switch0'\r\n"
			   "network.@switch_vlan[1].vlan='2'\r\n"
			   "network.@switch_vlan[1].ports='4 5'\r\n"
			   "network.lan=interface\r\n"
			   "network.lan.type='bridge'\r\n"
			   "network.lan.ifname='eth0 ra0'\r\n");

		cfglanip();
		//	wifiNeedReload=cfgwifi_hs2nwifi();

		//	wifiNeedReload2=cfgwifienable("1,0,0,0");
		wifiNeedReload2 = cfgwifienable2("1,0,0,0");
		//	cfgethenable("0,0,0,0");
	}
	else if (strcmp(p, "ap5_8") == 0)
	{

		setuci("network.wan=interface\r\n"
			   "network.wan._orig_ifname='eth1'\r\n"
			   "network.wan._orig_bridge='false'\r\n"
			   "network.wan.proto='dhcp'\r\n"
			   "network.wan.ifname='eth1'\r\n"
			   "network.wan6=interface\r\n"
			   "network.wan6.ifname='eth1'\r\n"
			   "network.wan6.proto='dhcpv6'\r\n"
			   "network.wan6.reqaddress='none'\r\n"
			   "network.wan6.reqprefix='auto'\r\n"
			   "network.@switch[0]=switch\r\n"
			   "network.@switch[0].name='switch0'\r\n"
			   "network.@switch[0].reset='1'\r\n"
			   "network.@switch[0].enable_vlan='1'\r\n"
			   "network.@switch_vlan[0]=switch_vlan\r\n"
			   "network.@switch_vlan[0].device='switch0'\r\n"
			   "network.@switch_vlan[0].vlan='1'\r\n"
			   "network.@switch_vlan[0].ports='0 1 2 3 6'\r\n"
			   "network.@switch_vlan[1]=switch_vlan\r\n"
			   "network.@switch_vlan[1].device='switch0'\r\n"
			   "network.@switch_vlan[1].vlan='2'\r\n"
			   "network.@switch_vlan[1].ports='4 5'\r\n"
			   "network.lan=interface\r\n"
			   "network.lan.type='bridge'\r\n"
			   "network.lan.ifname='eth0 rax0'\r\n");

		cfglanip();
		wifiNeedReload2 = cfgwifienable2("0,0,1,0");
	}
	else if (strcmp(p, "ap2_4ap5_8") == 0)
	{

		setuci("network.wan=interface\r\n"
			   "network.wan._orig_ifname='eth1'\r\n"
			   "network.wan._orig_bridge='false'\r\n"
			   "network.wan.proto='dhcp'\r\n"
			   "network.wan.ifname='eth1'\r\n"
			   "network.wan6=interface\r\n"
			   "network.wan6.ifname='eth1'\r\n"
			   "network.wan6.proto='dhcpv6'\r\n"
			   "network.wan6.reqaddress='none'\r\n"
			   "network.wan6.reqprefix='auto'\r\n"
			   "network.@switch[0]=switch\r\n"
			   "network.@switch[0].name='switch0'\r\n"
			   "network.@switch[0].reset='1'\r\n"
			   "network.@switch[0].enable_vlan='1'\r\n"
			   "network.@switch_vlan[0]=switch_vlan\r\n"
			   "network.@switch_vlan[0].device='switch0'\r\n"
			   "network.@switch_vlan[0].vlan='1'\r\n"
			   "network.@switch_vlan[0].ports='0 1 2 3 6'\r\n"
			   "network.@switch_vlan[1]=switch_vlan\r\n"
			   "network.@switch_vlan[1].device='switch0'\r\n"
			   "network.@switch_vlan[1].vlan='2'\r\n"
			   "network.@switch_vlan[1].ports='4 5'\r\n"
			   "network.lan=interface\r\n"
			   "network.lan.type='bridge'\r\n"
			   "network.lan.ifname='eth0 ra0 rax0'\r\n");

		cfglanip();
		wifiNeedReload2 = cfgwifienable2("1,0,1,0");
	}
	else if (strcmp(p, "eth") == 0)
	{

		//"network.wan.proto='dhcp'\r\n"\

		setuci("network.wan=interface\r\n"
			   "network.wan.ifname='eth1'\r\n"
			   "network.wan6=interface\r\n"
			   "network.wan6.ifname='eth1'\r\n"
			   "network.wan6.proto='dhcpv6'\r\n"
			   "network.wan6.reqaddress='none'\r\n"
			   "network.wan6.reqprefix='auto'\r\n"
			   "network.@switch[0]=switch\r\n"
			   "network.@switch[0].name='switch0'\r\n"
			   "network.@switch[0].reset='1'\r\n"
			   "network.@switch[0].enable_vlan='1'\r\n"
			   "network.@switch_vlan[0]=switch_vlan\r\n"
			   "network.@switch_vlan[0].device='switch0'\r\n"
			   "network.@switch_vlan[0].vlan='1'\r\n"
			   "network.@switch_vlan[0].ports='0 1 2 3 6'\r\n"
			   "network.@switch_vlan[1]=switch_vlan\r\n"
			   "network.@switch_vlan[1].device='switch0'\r\n"
			   "network.@switch_vlan[1].vlan='2'\r\n"
			   "network.@switch_vlan[1].ports='4 5'\r\n"
			   "network.lan=interface\r\n"
			   "network.lan.type='bridge'\r\n"
			   "network.lan.ifname='eth0'\r\n");

		//	system("uci del network.lan");

		cfgwanip();
		cfglanip();
		wifiNeedReload2 = cfgwifienable2("0,0,0,0");
	}
	else if (strcmp(p, "aprouter") == 0)
	{

		//		"network.wan.proto='dhcp'\r\n"


		setuci("network.wan=interface\r\n"
			   "network.wan._orig_ifname='eth1'\r\n"
			   "network.wan._orig_bridge='false'\r\n"
			   "network.wan.ifname='eth1'\r\n"
			   "network.wan6=interface\r\n"
			   "network.wan6.proto='dhcpv6'\r\n"
			   "network.wan6.reqaddress='none'\r\n"
			   "network.wan6.reqprefix='auto'\r\n"
			   "network.@switch[0]=switch\r\n"
			   "network.@switch[0].name='switch0'\r\n"
			   "network.@switch[0].reset='1'\r\n"
			   "network.@switch[0].enable_vlan='1'\r\n"
			   "network.@switch_vlan[0]=switch_vlan\r\n"
			   "network.@switch_vlan[0].device='switch0'\r\n"
			   "network.@switch_vlan[0].vlan='1'\r\n"
			   "network.@switch_vlan[0].ports='0 1 2 3 6'\r\n"
			   "network.@switch_vlan[1]=switch_vlan\r\n"
			   "network.@switch_vlan[1].device='switch0'\r\n"
			   "network.@switch_vlan[1].vlan='2'\r\n"
			   "network.@switch_vlan[1].ports='4 5'\r\n"
			   "network.lan=interface\r\n"
			   "network.lan.type='bridge'\r\n"
			   "network.lan.ifname='eth0 ra0 rax0'\r\n");

		cfgwanip();
		cfglanip();

		uchar *waninterface = netnvram_get(0, "waninterface");
		if (strcmpT(waninterface, "sta2_4") == 0)
		{
			setuci2("network.wan.ifname='%s'", "apcli0");
		}
		else if (strcmpT(waninterface, "sta5_8") == 0)
		{
			setuci2("network.wan.ifname='%s'", "apclix0");
		}
		else if (strcmpT(waninterface, "apcli0") == 0)
		{
			setuci2("network.wan.ifname='%s'", "apcli0");
		}
		else if (strcmpT(waninterface, "apclix0") == 0)
		{
			setuci2("network.wan.ifname='%s'", "apclix0");
		}
		else
		{
			setuci2("network.wan.ifname='%s'", "eth1");
		}

		uchar *laninterface = netnvram_get(0, "laninterface");
		if (strcmpT(laninterface, "") != 0)
		{
			setuci2("network.lan.ifname='%s'", laninterface);
		}

		wifiNeedReload2 = cfgwifienable2(netnvram_get(0, "wifi_endis"));
		cfgethenable(netnvram_get(0, "eth_endis"));
	}

	{
		uchar *cret = uci_cmd("uci changes");
		//if ((wifiNeedReload) || (wifiNeedReload2) || (cret && ucichanges(cret)))
		{
			if (0 == is_udp_at_cmd ) {
				//是串口 AT命令 直接执行
				system("sleep 1;uci commit;/etc/init.d/network restart");
			} else {
				//延时执行 网络重启功能
				delay_network_restart = 1;
			}
			//怎么延时执行 
		}
	}
}

#define MAX_SOCKET_NUMS 20
#define MAX_SOCKET_SERVERC_NUMS 20

typedef struct
{
	uchar cmd;
#define CMD_IDLE 0
#define CMD_OPEN 1
#define CMD_CLOSE 2

	uchar mode;
#define SMODE_NULL 0
#define SMODE_TCPS 1
#define SMODE_TCPC 2
#define SMODE_UDPS 3
#define SMODE_UDPC 4

	uchar state;
#define SSTATE_IDLE 0
#define SSTATE_CRESOVLEIP 1
#define SSTATE_CCONNECT 2
#define SSTATE_CCONNECTED 3
#define SSTATE_SBIND 4
#define SSTATE_SLISTEN 5

	uchar ip[4];
	ushort port;
	ushort localport;
	int fd;

	uchar clifd[MAX_SOCKET_SERVERC_NUMS];
	uchar clistate[MAX_SOCKET_SERVERC_NUMS];
} sockets_t;

static sockets_t g_socket[MAX_SOCKET_NUMS];

typedef struct
{
	uchar *command;
	uchar (*handle)(uchar *comm, uchar *out);
} AT_Handle_t;

#define TTT_ser2net_ser "ser2net_ser"
#define TTT_ser2net_mode "ser2net_mode"
#define TTT_ser2net_ipdomain "ser2net_ipdomain"
#define TTT_ser2net_port "ser2net_port"
#define TTT_ser2net_localeport "ser2net_localeport"
#define TTT_ser2net_proto "ser2net_proto"
#define TTT_ser2net_time_out "ser2net_time_out"
#define TTT_ser2net_tcp_autoc "ser2net_tcp_autoc"
#define TTT_ser2net_ser_pack_len "ser2net_ser_pack_len"
#define TTT_ser2net_ser_pack_time "ser2net_ser_pack_time"
#define TTT_ser2net_ser_pack_inter_time "ser2net_ser_pack_inter_time"
#define TTT_ser2net_escape_en "ser2net_escape_en"
#define TTT_ser2net_escape2_en "ser2net_escape2_en"
#define TTT_ser2net_escape2_time "ser2net_escape2_time"
#define TTT_ser2net_RTS "ser2net_RTS"
#define TTT_ser2net_XON "ser2net_XON"
#define TTT_AT_DisName "AT_DisName"
#define TTT_ser2net_SMaxConn "ser2net_SMaxConn"
#define TTT_ser2net_TCPRT "ser2net_TCPRT"
#define TTT_ser2net_tcp_c_check "ser2net_tcp_c_check"
#define TTT_ser2net_SRemoteIpPort "ser2net_SRemoteIpPort"

AT_Handle_t at_handler[] = {
	{"ver", AT_version},
	{"remoteip", AT_remoteip},
	{"remoteport", AT_remoteport},
	{"CLport", AT_client_localeport}, //client locale port.
	{"remotepro", AT_remotepro},
	{"timeout", AT_timeout},
	{"SMaxConn", AT_SMaxConn},
	{"SRemoteIpPort", AT_SRemoteIpPort},

	{"TCPRealTime", AT_TCPRealTime},
	{"tcp_client_check", AT_tcp_client_stat_check},

	{"DisName", AT_discovery_udp_name},

	{"mode", AT_mode},
	{"uart", AT_uart},
	{"uartpacklen", AT_packlen},
	{"uartpacktimeout", AT_packtimeout},
	{"uartpackintertimeout", AT_packintertimeout},
	{"save", AT_save},
	{"reconn", AT_restart},

	{"escap", AT_escap},
	{"escap2", AT_escap2},
	{"escap2_time", AT_escap2_time},

	{"status", AT_status},
	//network

	{"sta2_4state", wifi_ConState},
	{"scan2_4", wifi_Scan},
	{"sta5_8state", wifi_ConState58},
	{"scan5_8", wifi_Scan58},

	{"sta2_4", AT_sta2_4},
	{"sta5_8", AT_sta5_8},
	{"ap2_4", AT_ap2_4},
	{"ap5_8", AT_ap5_8},
	{"lanip", AT_lanip},
	{"dhcpd", AT_dhcpd0},
	{"dhcpc", AT_dhcpc},
	{"wanip", AT_wanip},
	{"netmode", AT_netmode0},
	{"waninterface", AT_waninterface},
	{"laninterface", AT_laninterface},

	{"eth_endis", AT_eth_endis},
	{"wifi_endis", AT_wifi_endis},

	{"netsaveapply", AT_netsaveapply},

	{"excxxx", AT_exc},

	{"resetdefault", AT_resetdefault},

	{"reboot", AT_reboot},

	{"out_trans", AT_out_trans},

	{"sta_max_conn", AT_sta_conn_ctrl},
	{"ip_max_conn", AT_ip_max_conn_ctrl},
};

uchar at(uchar *comm, uchar *out)
{
	if ((NULL == comm) || (NULL == out))
		return -1;

	out[0] = 0;
	if (('a' == gtolower(*comm)) && ('t' == gtolower(*(comm + 1))) && ('+' == (*(comm + 2))))
	{

		{
			if ('\0' == (*(comm + 3)))
			{
				sprintf(out, "+ok");
				return 0;
			}
		}

		{
			int i, j;
			uchar *p, *comm_;

			comm_ = comm + 3;
			p = comm_;
			for (j = 0; j < 128; j++, p++)
				if ((0 == *p) || ('=' == *p))
					break;

			for (i = 0; i < (sizeof(at_handler) / sizeof(AT_Handle_t)); i++)
			{
				if ((j == strlen(at_handler[i].command)) && (0 == memcmp(at_handler[i].command, comm_, j)))
				{
					at_handler[i].handle(p, out);
					break;
				}
			}

			if (i == (sizeof(at_handler) / sizeof(AT_Handle_t)))
			{
#ifdef SER2NET_COM2
				if (COM2_ATs(comm, out))
#endif
					AT_error(comm, out);
			}

			return 0;
		}
	}
}

static volatile uchar goahead_usr2 = 0;

void ser2netSigHandler2(int signum)
{
	if (signum != SIGUSR2)
		return;

	goahead_usr2 = 1;
}

void goahead_initInternet(void)
{
	{
		int gopid;
		FILE *fp = fopen("/var/run/goahead.pid", "r");

		if (NULL == fp)
		{
			printf("ser2net: goahead is not running\n");
			return;
		}
		fscanf(fp, "%d", &gopid);
		if (gopid < 2)
		{
			printf("ser2net: goahead pid(%d) <= 1\n", gopid);
			return;
		}

		//send SIGUSR1 signal to goAhead for WPSPBCStart();
		printf("notify goahead to initInternet..\n");
		kill(gopid, SIGUSR2);
		fclose(fp);
	}

	{
		uchar time = 0;
		goahead_usr2 = 0;

		while (1)
		{
			if (goahead_usr2)
			{
				break;
			}

			time++;
			if (time > 30)
			{
				TIM_ERROR("ERROR:Not receive USR2%s(%d)\r\n", __FUNCTION__, __LINE__);
				break;
			}

			sleep(1);
		}
	}
}

#define PM_ETH0 0
#define PM_ETH1 1
#define PM_WIFI 2
#define PM_CPU 3
#define PM_RESERVE1 4
#define PM_RESERVE2 5
#define PM_RESERVE3 6
#define PM_RESERVE4 7

/*
 * arguments: ifname  - interface name
 *			  if_addr - a 16-byte buffer to store ip address
 * description: fetch ip address, netmask associated to given interface name
 */
int getIfIp2(char *ifname, char *if_addr)
{
	struct ifreq ifr;
	int skfd = 0;

	if ((skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		TIM_ERROR("ERROR:%s(%d)\r\n", __FUNCTION__, __LINE__);
		return -1;
	}

	strncpy(ifr.ifr_name, ifname, IF_NAMESIZE);
	if (ioctl(skfd, SIOCGIFFLAGS, &ifr) < 0)
	{
		close(skfd);
		//error(E_L, E_LOG, T("getIfIp: ioctl SIOCGIFADDR error for %s"), ifname);
		return -1;
	}

	close(skfd);

	if ((ifr.ifr_flags) & IFF_UP)
	{
		return 0;
	}
	else
	{
		return -1;
	}

	return 0;
}

void wifi_power_save(uchar on)

{

	char if_addr[16];
	uchar wifi_on = 1;

	if (-1 == getIfIp2("ra0", if_addr))
	{
		wifi_on = 0;
	}
	else
	{
		wifi_on = 1;
	}

	TIM_DEBUG("DEBUG:%s(%d)(%d)(%d)\r\n", __FUNCTION__, __LINE__, on, wifi_on);
	if (on != wifi_on)
	{
		if (on)
		{
			system("config-powersave.sh wireless 0;");
		}
		else
		{
			system("config-powersave.sh wireless 1;");
		}
		wifi_on = on;
	}

	{
		if (!strcmp(nvram_bufget(RT2860_NVRAM, "HLK_APOnOff"), "0"))
		{
			system("brctl delif br0 ra0");
			system("ifconfig ra0 down");
		}
	}

}

void power_save(void)
{

	return;

	system("(config-powersave.sh ethernet 1 0;config-powersave.sh ethernet 1 1;config-powersave.sh ethernet 1 2;)");

	{
		switch (g_AT_netmode)
		{
		case 1:
			system("(config-powersave.sh ethernet 1 3;config-powersave.sh ethernet 0 4;config-powersave.sh usb 1)");
			wifi_power_save(0);
			system("ifconfig br0 down");
			break;
		case 2:
			system("(config-powersave.sh ethernet 1 3;config-powersave.sh ethernet 1 4;config-powersave.sh usb 1)");
			wifi_power_save(1);
			system("ifconfig br0 down");
			system("ifconfig eth2 down");
			break;
		case 3:
			system("(config-powersave.sh ethernet 1 3;config-powersave.sh ethernet 1 4;config-powersave.sh usb 1)");
			wifi_power_save(1);
			break;
		case 4:
			system("(config-powersave.sh ethernet 1 3;config-powersave.sh ethernet 1 4;config-powersave.sh usb 0)");
			wifi_power_save(0);
			break;
		default:
			if (!strcmp(nvram_bufget(RT2860_NVRAM, "NoEth"), "1"))
				system("(config-powersave.sh ethernet 1 3;config-powersave.sh ethernet 1 4;config-powersave.sh usb 1)");
			else
				system("(config-powersave.sh ethernet 0 3;config-powersave.sh ethernet 0 4;config-powersave.sh usb 0)");
			{
				if (!strcmp(nvram_bufget(RT2860_NVRAM, "RadioOff"), "0"))
				{
					wifi_power_save(1);
				}
				else if (!strcmp(nvram_bufget(RT2860_NVRAM, "RadioOff"), "1"))
				{
					wifi_power_save(0);
				}
				else
				{
					wifi_power_save(1);
				}
			}
			break;
		}
	}

	if (g_AT_pm & (1 << (8 + PM_ETH0)))
	{
		if (g_AT_pm & (1 << (0 + PM_ETH0)))
		{
			system("config-powersave.sh ethernet 1 4;");
		}
		else
		{
			system("config-powersave.sh ethernet 0 4;");
		}
	}
	if (g_AT_pm & (1 << (8 + PM_ETH1)))
	{
		if (g_AT_pm & (1 << (0 + PM_ETH1)))
		{
			system("config-powersave.sh ethernet 1 3;");
		}
		else
		{
			system("config-powersave.sh ethernet 0 3;");
		}
	}
	if (g_AT_pm & (1 << (8 + PM_WIFI)))
	{
		if (g_AT_pm & (1 << (0 + PM_WIFI)))
		{
			wifi_power_save(0);
		}
		else
		{
			wifi_power_save(1);
		}
	}
	if (g_AT_pm & (1 << (8 + PM_CPU)))
	{
		if (g_AT_pm & (1 << (0 + PM_CPU)))
		{
			system("config-powersave.sh sdr 1;");
		}
		else
		{
			system("config-powersave.sh sdr 0;");
		}
	}

#if 1 //for debug.

	system(nvram_bufget(RT2860_NVRAM, "HLK_rcs0"));
	system(nvram_bufget(RT2860_NVRAM, "HLK_rcs1"));
	system(nvram_bufget(RT2860_NVRAM, "HLK_rcs2"));
	system(nvram_bufget(RT2860_NVRAM, "HLK_rcs3"));
	system(nvram_bufget(RT2860_NVRAM, "HLK_rcs4"));

#endif
}

void print_wait_exit(int status)
{
	/* shell正常退出，即便命令执行异常，shell也有可能是正常退出的 */
	if (WIFEXITED(status))
	{
		printf("normal termination, exit status = %d\n", WEXITSTATUS(status));
	}
	/* shell异常退出 */
	else if (WIFSIGNALED(status))
	{
		printf("abnormal terminatio, signal number = %d\n", WTERMSIG(status));
	}
}

uchar *uci_cmd(uchar *cmd)
{
	FILE *fp;
	static char buf[512];
	char *p;
	int len;

	if (((int)(fp = popen(cmd, "r"))) <= 0)
	{
		return 0;
	}
	len = fread(buf, 1, sizeof(buf), fp);
	if ((len < 5) || (len >= sizeof(buf)))
	{
		int ret = pclose(fp);

		return 0;
	}
	int ret = pclose(fp);

	buf[len] = 0;
	buf[sizeof(buf) - 1] = 0;

	return buf;
}

void popentest()
{
	uchar *cret;

	cret = uci_cmd("uci show s2ncfg");
	if (cret)
	{
	}

	cret = uci_cmd("uci show s2ncfg");
	if (cret)
	{
	}

	uchar *p;

	p = nvram_get(0, "dersdf");

	nvram_set(0, "dersdf", "dkfjowe233");

	p = nvram_get(0, "dersdf");
}

uchar *nvram_get(uchar *a, uchar *b)
{
	uchar cmd[256];

	snprintf(cmd, sizeof(cmd) - 1, "uci show s2ncfg.@com[0].%s", b);
	uchar *cret, *p, *p2;

	cret = uci_cmd(cmd);
	if (cret)
	{
		if (p = strstr(cret, "='"))
		{
			if (p2 = strstr(p + 2, "'"))
			{
				p2[0] = 0;
				//syslog(LOG_DEBUG, "%s val:%s", b, p+2);
				return p + 2;
			}
		}
	}
	return 0;
}

void nvram_set(uchar *a, uchar *b, uchar *c)
{
	uchar cmd[256];

	snprintf(cmd, sizeof(cmd) - 1, "uci set s2ncfg.@com[0].%s=%s", b, c);
	uchar *cret, *p, *p2;

	cret = uci_cmd(cmd);
	if (cret)
	{
	}
}

uchar *netnvram_get(uchar *a, uchar *b)
{
	uchar cmd[256];

	snprintf(cmd, sizeof(cmd) - 1, "uci show s2ncfg.@net[0].%s", b);
	uchar *cret, *p, *p2;

	cret = uci_cmd(cmd);
	if (cret)
	{
		if (p = strstr(cret, "='"))
		{
			if (p2 = strstr(p + 2, "'"))
			{
				p2[0] = 0;
				return p + 2;
			}
		}
	}
	return "";
}

void netnvram_set(uchar *a, uchar *b, uchar *c)
{
	uchar cmd[256];

	snprintf(cmd, sizeof(cmd) - 1, "uci set s2ncfg.@net[0].%s=%s", b, c);
	uchar *cret, *p, *p2;

	cret = uci_cmd(cmd);
	if (cret)
	{
		TIM_DEBUG("DEBUG:%s(%d)(%s)\r\n", __FUNCTION__, __LINE__, cret);
	}
}

uchar *nvram_get_ori(uchar *b)
{
	uchar cmd[256];

	snprintf(cmd, sizeof(cmd) - 1, "uci show %s", b);
	char *cret, *p, *p2;

	cret = uci_cmd(cmd);
	if (cret)
	{
		if (p = strstr(cret, "='"))
		{
			if (p2 = strstr(p + 2, "'"))
			{
				p2[0] = 0;
				return p + 2;
			}
		}
		else if (p = strstr(cret, "=\""))
		{
			if (p2 = strstr(p + 2, "\""))
			{
				p2[0] = 0;
				return p + 2;
			}
		}
	}
	return 0;
}

void nvram_set_ori(uchar *a, uchar *b, uchar *c)
{
	uchar cmd[256];

	snprintf(cmd, sizeof(cmd) - 1, "uci set %s=%s", b, c);
	uchar *cret, *p, *p2;

	cret = uci_cmd(cmd);
	if (cret)
	{
		TIM_DEBUG("DEBUG:%s(%d)(%s)\r\n", __FUNCTION__, __LINE__, cret);
	}
}

uchar *nvram_bufget(uchar *a, uchar *b)
{
}

void nvram_bufset(uchar *a, uchar *b, uchar *c)
{
}

void nvram_close(int i)
{
}

void nvram_init(int i)
{
}
void nvram_commit(int i)
{

	netapply();
	return;

	uchar cmd[256];

	snprintf(cmd, sizeof(cmd) - 1, "uci commit");
	uchar *cret, *p, *p2;

	cret = uci_cmd(cmd);
	if (cret)
	{
		TIM_DEBUG("DEBUG:%s(%d)(%s)\r\n", __FUNCTION__, __LINE__, cret);
	}
}

uchar AT_stalist(uchar *comm, uchar *out)
{
}
