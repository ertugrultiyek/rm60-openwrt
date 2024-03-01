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

#include "pthread.h"

#include <sys/socket.h>
#include <sys/un.h>

#include <unistd.h>

#include <sys/msg.h>

#include <netinet/tcp.h>

#include <linux/socket.h>
#include <linux/socket.h>
#include <linux/serial.h>
#include <linux/if_ether.h>
#include <linux/wireless.h>

#include "ser2net.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>

#include <linux/serial.h>

struct tcp_info
{
	u_int8_t tcpi_state;
	u_int8_t tcpi_ca_state;
	u_int8_t tcpi_retransmits;
	u_int8_t tcpi_probes;
	u_int8_t tcpi_backoff;
	u_int8_t tcpi_options;
	u_int8_t tcpi_snd_wscale : 4, tcpi_rcv_wscale : 4;

	u_int32_t tcpi_rto;
	u_int32_t tcpi_ato;
	u_int32_t tcpi_snd_mss;
	u_int32_t tcpi_rcv_mss;

	u_int32_t tcpi_unacked;
	u_int32_t tcpi_sacked;
	u_int32_t tcpi_lost;
	u_int32_t tcpi_retrans;
	u_int32_t tcpi_fackets;

	/* Times. */
	u_int32_t tcpi_last_data_sent;
	u_int32_t tcpi_last_ack_sent; /* Not remembered, sorry.  */
	u_int32_t tcpi_last_data_recv;
	u_int32_t tcpi_last_ack_recv;

	/* Metrics. */
	u_int32_t tcpi_pmtu;
	u_int32_t tcpi_rcv_ssthresh;
	u_int32_t tcpi_rtt;
	u_int32_t tcpi_rttvar;
	u_int32_t tcpi_snd_ssthresh;
	u_int32_t tcpi_snd_cwnd;
	u_int32_t tcpi_advmss;
	u_int32_t tcpi_reordering;

	u_int32_t tcpi_rcv_rtt;
	u_int32_t tcpi_rcv_space;

	u_int32_t tcpi_total_retrans;
};

#define TTT_ser2net_ser "uart"
#define TTT_ser2net_mode "mode"
#define TTT_ser2net_ipdomain "remoteip"
#define TTT_ser2net_port "remoteport"
#define TTT_ser2net_localeport "CLport"
#define TTT_ser2net_proto "remotepro"
#define TTT_ser2net_time_out "timeout"
#define TTT_ser2net_tcp_autoc "ser2net_tcp_autoc"
#define TTT_ser2net_ser_pack_len "uartpacklen"
#define TTT_ser2net_ser_pack_time "uartpacktimeout"
#define TTT_ser2net_ser_pack_inter_time "uartpackintertimeout"
#define TTT_ser2net_escape_en "escap"
#define TTT_ser2net_escape2_en "escap2"
#define TTT_ser2net_escape2_time "escap2_time"
#define TTT_ser2net_RTS "ser2net_RTS"
#define TTT_ser2net_XON "ser2net_XON"
#define TTT_AT_DisName "DisName"
#define TTT_ser2net_SMaxConn "SMaxConn"
#define TTT_ser2net_TCPRT "TCPRealTime"
#define TTT_ser2net_tcp_c_check "tcp_client_check"
#define TTT_ser2net_SRemoteIpPort "SRemoteIpPort"

pthread_t serial_thread_id;
pthread_t server_thread_id;
pthread_t client_thread_id;
pthread_t udp_thread_id;

int is_udp_at_cmd = 0;
int delay_network_restart = 0;

void led_init();
void led_blink();
void led_set(uchar value);

#define TIM_TRACE() TIM_DEBUG("In:%s(%d)\r\n", __FUNCTION__, __LINE__)

#define perror printf

#define dprintf

struct termios oldtio, options;

//#define MAX_(A, B) ((A > B) ? A : B)
//#define MIN_(A, B) ((A < B) ? A : B)

uchar g_select_status = 0;

#define UART_DEV "/dev/ttyS2"

enum
{
	MAIN_STATE_IDLE,
	MAIN_STATE_DOMAIN_RESOLVE,
	MAIN_STATE_IP_CONNECT,
	MAIN_STATE_WORK,
};

enum
{
	SERVER_MAIN_STATE_IDLE,
	SERVER_MAIN_STATE_IP_LISTEN,
	SERVER_MAIN_STATE_WORK,
};

//uchar *g_serial_config="115200,8,n,1";
uchar *g_serial_config = 0;

uchar g_network_mode = 0; //0 ,1 server,2 client
//uchar *g_remoteip_domain="192.168.11.245";
uchar *g_remoteip_domain = 0;
int g_locale_remote_port = 8080;
int g_client_locale_port = 0;
uchar g_network_protocol = 1; //0 none,1 tcp,2 udp
uint g_network_timeout = 0;

uint g_serial_package_len = 64;
uint g_serial_package_time = 10;
uint g_serial_package_interval_time = 255;

uchar g_transparent_mode = 0;

uchar g_transparent_escape_en = 1;
uchar g_transparent_escape2_en = 1;

uint g_transparent_escape2_time = 2000;

uchar g_AT_netmode = 0; //0-default 1-eth 2-client 3-ap 4-3g 5-netwebconfig
//uchar *g_AT_wifi_conf="Hi-Link_,wpa2_aes,0000000000";//none wep wpa_tkip wpa_aes wpa2_tkip wpa2_aes wpawpa2_tkip wpawpa2_aes
uchar *g_AT_wifi_conf = 0; //none wep_open wep wpa_tkip wpa_aes wpa2_tkip wpa2_aes wpawpa2_tkip wpawpa2_aes auto
uchar g_AT_dhcpd = 0;
uchar *g_AT_dhcpd_ip = 0;
uchar *g_AT_dhcpd_dns = 0;
uint g_AT_dhcpd_time = 86400;

uchar g_AT_dhcpc = 1;
//uchar *g_AT_net_ip="192.168.11.254,255.255.255.0,192.168.11.1";
//uchar *g_AT_net_dns="192.168.11.1,8.8.8.8";
uchar *g_AT_net_ip = 0;
uchar *g_AT_net_dns = 0;

ushort g_AT_pm = 0;

uchar g_tcp_auto_connec = 1;

uchar g_tcp_client_stat_check = 1;

uchar FLOW_CONTROL_ = 1;
uchar XON_XOFF = 0;

uchar g_s2n_stat = 0;

uchar *g_AT_discovery_udp_name = 0;

ushort g_AT_gpio_mode = 0;
ushort g_AT_gpio_data = 0;

uchar g_SMaxConn = 100;
uchar *g_SRemoteIp = 0;
uchar g_SRemotePort = 0;
uchar *g_SRemoteIpPort = 0;

uchar g_tcp_realtime = 0;

#define PM_ETH0 0
#define PM_ETH1 1
#define PM_WIFI 2
#define PM_CPU 3
#define PM_RESERVE1 4
#define PM_RESERVE2 5
#define PM_RESERVE3 6
#define PM_RESERVE4 7

pthread_mutex_t g_mutex;

uchar g_network_failed = 0;

uchar g_serial_at = 0;

void restart_app();

void out_transparent(uchar value);

void ser2network_notify_serial();
void ser2network_notify_socket();

uint statistics_serial_send = 0;

uint statistics_serial_tx = 0;

uint statistics_serial_tx2 = 0;

uint statistics_serial_tx3 = 0;

const uchar *g_default_serial_config = "";

int fd_write(int fd, unsigned char *buffer, int dwBytesToWrite)
{
	uint j, k;
	uint sended = 0;
	for (j = 0; j < 3; j++)
	{
		if (sended < dwBytesToWrite)
		{
			k = write(fd, &buffer[sended], dwBytesToWrite - sended); //!> ��������д��ȥ
			if (k < 0)
			{
				TIM_ERROR("ERROR:%s(%d)\r\n", __FUNCTION__, __LINE__);
				break;
			}
			else
			{
				sended += k;
			}
		}
		else
			break;
	}

	if (sended != dwBytesToWrite)
	{
		TIM_ERROR("1");
	}
	return sended;
}

int serial_set_speci_baud(int fd, int baud)
{
	struct serial_struct ss, ss_set;

	tcgetattr(fd, &options);

	cfsetispeed(&options, baud);
	cfsetospeed(&options, baud);

	tcflush(fd, TCIFLUSH); /*handle unrecevie char*/
	tcsetattr(fd, TCSANOW, &options);

	if ((ioctl(fd, TIOCGSERIAL, &ss)) < 0)
	{
		TIM_ERROR("BAUD: error to get the serial_struct info:%s\r\n", strerror(errno));
		return -1;
	}

	ss.flags = ASYNC_SPD_CUST;
	//    ss.custom_divisor = ss.baud_base / baud;

	TIM_ERROR("In:%s(%d) (%d)(%d)(%d)\r\n", __FUNCTION__, __LINE__,
			  baud, ss.baud_base, ss.custom_divisor);

	if ((ioctl(fd, TIOCSSERIAL, &ss)) < 0)
	{
		TIM_ERROR("BAUD: error to set serial_struct:%s\r\n", strerror(errno));
		return -2;
	}

	ioctl(fd, TIOCGSERIAL, &ss_set);
	TIM_ERROR("BAUD: success set baud to %d,custom_divisor=%d,baud_base=%d\r\n",
			  baud, ss_set.custom_divisor, ss_set.baud_base);

	return 0;
}

//"115200,8,n,1"
int Serial_Config(int fd, uchar *para)
{

	speed_t speed;

	int Baud = 115200;
	uchar Data_size = 8;
	uchar parity = 'n';
	uchar stop_bit = 1;

	if (fd < 0)
	{
		TIM_ERROR("ERROR:%s(%d)\r\n", __FUNCTION__, __LINE__);
	}

	{
		uchar *p1, *p2;

		p1 = para;

		if (p1)
		{
			Baud = atoi(p1);
			p2 = strchr(p1, ',');
			if (p2)
			{
				p1 = (p2 + 1);
				Data_size = atoi(p1);
				p2 = strchr(p1, ',');
				if (p2)
				{
					p1 = (p2 + 1);
					parity = *p1;

					p2 = strchr(p1, ',');
					if (p2)
					{
						p1 = (p2 + 1);
						stop_bit = atoi(p1);
					}
				}
			}
		}
	}

	TIM_DEBUG("In:%s(%d) (%d)(%d)(%c)(%d)\r\n", __FUNCTION__, __LINE__,
			  Baud, Data_size, parity, stop_bit);

	tcgetattr(fd, &oldtio);
	tcgetattr(fd, &options);

	TIM_DEBUG("In:%s(%d)(%d)(%d)\r\n", __FUNCTION__, __LINE__, cfgetospeed(&options), cfgetospeed(&oldtio));

	fcntl(fd, F_SETFL, 0);
	fcntl(fd, F_SETFL, FNDELAY);

	options.c_iflag = 0;
	options.c_oflag = 0;
	options.c_lflag = 0;
	if (0 == 0)
		options.c_cc[VTIME] = 20;
	else
		options.c_cc[VTIME] = 0; //20;  //inter-character timer use 2s
								 //	options.c_cc[VTIME]    = 20;
	options.c_cc[VMIN] = 0;		 //blocking read until 1 chars received
	options.c_cflag = CS8 | CLOCAL | CREAD;

	if (5 == Data_size)
	{
		options.c_cflag &= ~CSIZE;
		options.c_cflag |= CS5;
	}
	else if (6 == Data_size)
	{
		options.c_cflag &= ~CSIZE;
		options.c_cflag |= CS6;
	}
	else if (7 == Data_size)
	{
		options.c_cflag &= ~CSIZE;
		options.c_cflag |= CS7;
	}
	else if (8 == Data_size)
	{

		options.c_cflag &= ~CSIZE;
		options.c_cflag |= CS8;
	}

	if ('n' == parity)
	{
	}
	else if ('o' == parity)
	{

		options.c_cflag |= PARENB;
		options.c_cflag |= PARODD;
		options.c_iflag |= (INPCK);
	}
	else if ('e' == parity)
	{
		options.c_cflag |= PARENB;
		options.c_cflag &= ~PARODD;
		options.c_iflag |= (INPCK);
	}

	if (1 == stop_bit)
	{
		options.c_cflag &= ~CSTOPB;
	}
	else if (2 == stop_bit)
	{
		options.c_cflag |= CSTOPB;
	}
	else
	{
		options.c_cflag |= CSTOPB;
	}

	TIM_WARN("In:%s(%d)(%08x)(%d)\r\n", __FUNCTION__, __LINE__, options.c_cflag, cfgetospeed(&oldtio));

	if (FLOW_CONTROL_)
	{
		options.c_cflag |= CIBAUD;
	}
	else
	{
		options.c_cflag &= ~CIBAUD;
	}

	TIM_WARN("In:%s(%d)(%08x)(%d)\r\n", __FUNCTION__, __LINE__, options.c_cflag, cfgetospeed(&oldtio));

	switch (Baud)
	{
	case 50:
		speed = B50;
		break;
	case 75:
		speed = B75;
		break;
	case 110:
		speed = B110;
		break;
	case 134:
		speed = B134;
		break;
	case 150:
		speed = B150;
		break;
	case 200:
		speed = B200;
		break;
	case 300:
		speed = B300;
		break;
	case 600:
		speed = B600;
		break;
	case 1200:
		speed = B1200;
		break;
	case 1800:
		speed = B1800;
		break;
	case 2400:
		speed = B2400;
		break;
	case 4800:
		speed = B4800;
		break;
	case 9600:
		speed = B9600;
		break;
	case 19200:
		speed = B19200;
		break;
	case 38400:
		speed = B38400;
		break;
	case 57600:
		speed = B57600;
		break;
	case 115200:
		speed = B115200;
		break;
	case 230400:
		speed = B230400;
		break;
	default:
		speed = B115200;
		TIM_WARN("WARN:%s(%d)(%d)\r\n", __FUNCTION__, __LINE__, Baud);
		break;
	}

	TIM_WARN("in:%s(%d) %d %d %d\r\n", __FUNCTION__, __LINE__, B115200, speed, B57600);

	TIM_DEBUG(":%s(%d)(%d)(%d)\r\n", __FUNCTION__, __LINE__, cfgetospeed(&options), cfgetospeed(&oldtio));

	if (XON_XOFF)
	{
		options.c_iflag |= IXON;
		options.c_iflag |= IXOFF;
	}

	if (cfsetispeed(&options, speed))
	{
		TIM_ERROR("ERROR:%s(%d)\r\n", __FUNCTION__, __LINE__);
	}
	if (cfsetispeed(&options, speed))
	{
		TIM_ERROR("ERROR:%s(%d)\r\n", __FUNCTION__, __LINE__);
	}

	if (tcsetattr(fd, TCSAFLUSH, &options))
	{
		TIM_ERROR("ERROR:%s(%d)\r\n", __FUNCTION__, __LINE__);
	}

	tcflush(fd, TCIOFLUSH);

	serial_set_speci_baud(fd, Baud);

	return 0;
}

uint get_serial_wait_time() //us
{
	int Baud = 115200;
	uchar Data_size = 8;
	uchar parity = 'n';
	uchar stop_bit = 1;

	uint time;

	{
		uchar *p1, *p2;

		p1 = g_serial_config;

		if (p1)
		{
			Baud = atoi(p1);
		}
	}

#define SERIAL_WAITETIME_LEN 4 * 1024 / 10 //  1/5

	time = 1000000 * (SERIAL_WAITETIME_LEN) / Baud;
	TIM_DEBUG("in:%s(%d)(%d)\r\n", __FUNCTION__, __LINE__, time);
	return time;
}

uint get_serial_wait_time2(uint count) //us
{
	int Baud = 115200;
	uchar Data_size = 8;
	uchar parity = 'n';
	uchar stop_bit = 1;

	uint time;

	{
		uchar *p1, *p2;

		p1 = g_serial_config;

		if (p1)
		{
			Baud = atoi(p1);
		}
	}

#define SERIAL_WAITETIME_LEN 4 * 1024 / 10 //  1/5

	time = 1000000 * (count * 10) / Baud;
	TIM_DEBUG("in:%s(%d)(%d)\r\n", __FUNCTION__, __LINE__, time);
	return time;
}

pthread_mutex_t G_serial_mode_mutex;

static volatile uchar G_serial_mode = 0;

uchar G_serial_mode_get()
{
	uchar x;
	pthread_mutex_lock(&G_serial_mode_mutex);
	x = G_serial_mode;
	pthread_mutex_unlock(&G_serial_mode_mutex);
	return x;
}

void G_serial_mode_set(uchar x)
{
	pthread_mutex_lock(&G_serial_mode_mutex);
	G_serial_mode = x;
	pthread_mutex_unlock(&G_serial_mode_mutex);
}

pthread_mutex_t serial_buf_mutex;
pthread_mutex_t socket_buf_mutex;

#define SERIAL_REQUEST() pthread_mutex_lock(&serial_buf_mutex);
#define SERIAL_RELEASE() pthread_mutex_unlock(&serial_buf_mutex);

uchar serial_buf_tx[SERIAL_BUF_LEN];
uchar *serial_buf_tx_in = serial_buf_tx;
uchar *serial_buf_tx_out = serial_buf_tx;

uchar Serial_Flush_now = 0;

uchar serial_buf_empty_flush()
{
	SERIAL_REQUEST();
	serial_buf_tx_out = serial_buf_tx;
	serial_buf_tx_in = serial_buf_tx;
	SERIAL_RELEASE();

	Serial_Flush_now = 1;
	ser2network_notify_serial();

	return 0;
}

uint serial_buf_empty_len()
{
	uint len;

	if (G_serial_mode_get())
	{
		return 0;
	}

	SERIAL_REQUEST();
	TIM_DEBUG("in:%s(%d) %08x %08x \r\n", __FUNCTION__, __LINE__, serial_buf_tx_out, serial_buf_tx_in);
	if (serial_buf_tx_out <= serial_buf_tx_in)
	{
		len = sizeof(serial_buf_tx) - (serial_buf_tx_in - serial_buf_tx_out) - 1;
	}
	else
	{
		len = serial_buf_tx_out - serial_buf_tx_in - 1;
	}
	SERIAL_RELEASE();
	return len;
}

uchar serial_buf_is_empty()
{
	uint ret;
	SERIAL_REQUEST();
	if (serial_buf_tx_out != serial_buf_tx_in)
	{
		ret = 0;
	}
	else
		ret = 1;
	SERIAL_RELEASE();
	return ret;
}

uint serial_buf_send(uchar *value, uint len)
{
	uint len_;
	SERIAL_REQUEST();
	if (serial_buf_tx_out <= serial_buf_tx_in)
	{
		len_ = sizeof(serial_buf_tx) - (serial_buf_tx_in - serial_buf_tx_out) - 1;
		if (len < len_)
			len_ = len;

		if ((serial_buf_tx_in + len_) < (serial_buf_tx + sizeof(serial_buf_tx)))
		{
			memcpy(serial_buf_tx_in, value, len_);
			serial_buf_tx_in += len_;
		}
		else
		{
			uint len1;
			len1 = (serial_buf_tx + sizeof(serial_buf_tx)) - serial_buf_tx_in;
			memcpy(serial_buf_tx_in, value, len1);
			memcpy(serial_buf_tx, value + len1, len_ - len1);
			serial_buf_tx_in = serial_buf_tx + (len_ - len1);
		}
	}
	else
	{
		len_ = serial_buf_tx_out - serial_buf_tx_in - 1;
		if (len < len_)
			len_ = len;
		memcpy(serial_buf_tx_in, value, len_);
		serial_buf_tx_in += len_;
	}
	SERIAL_RELEASE();

	ser2network_notify_serial();

	if (0 == len)
		return 0;
	if (0 == G_serial_mode_get())
		led_blink();
	return len;
}

uint serial_buf_get(uchar *value, uint len)
{

	SERIAL_REQUEST();
	SERIAL_RELEASE();

	return 0;
}

uint serial_buf_tx_(int fd)
{
	uint len;
	int iret = 0;
	int ret;

	SERIAL_REQUEST();
	if (serial_buf_tx_out <= serial_buf_tx_in)
	{
		len = (serial_buf_tx_in - serial_buf_tx_out);
		ret = write(fd, serial_buf_tx_out, len);
		if (ret >= 0)
		{
			serial_buf_tx_out += ret;

			statistics_serial_send += ret;
		}
		else
		{
			iret = -1;
		}
	}
	else
	{
		int len1;
		len = sizeof(serial_buf_tx) - (serial_buf_tx_out - serial_buf_tx_in);
		len1 = ((serial_buf_tx + sizeof(serial_buf_tx)) - serial_buf_tx_out);
		ret = write(fd, serial_buf_tx_out, len1);
		if (ret >= 0)
		{
			if (ret == len1)
			{
				int len2;
				len2 = (uint)serial_buf_tx_in - (uint)serial_buf_tx;

				ret = write(fd, serial_buf_tx, len2);
				if (ret >= 0)
				{
					serial_buf_tx_out = serial_buf_tx + ret;
					statistics_serial_send += ret;
					statistics_serial_send += len1;
				}
				else
				{
					iret = -1;
					serial_buf_tx_out = serial_buf_tx + 0;
					statistics_serial_send += len1;
				}
			}
			else
			{
				serial_buf_tx_out += ret;
				statistics_serial_send += ret;
			}
		}
		else
		{
			iret = -1;
		}
	}
	SERIAL_RELEASE();

	return iret;
}

#define SOCKET_REQUEST() pthread_mutex_lock(&socket_buf_mutex);
#define SOCKET_RELEASE() pthread_mutex_unlock(&socket_buf_mutex);

uchar socket_buf_tx[socket_buf_LEN];
uint socket_buf_tx_in = (uint)socket_buf_tx;
uint socket_buf_tx_out = (uint)socket_buf_tx;

uchar socket_buf_empty_flush()
{
	SOCKET_REQUEST();
	socket_buf_tx_out = (uint)socket_buf_tx;
	socket_buf_tx_in = (uint)socket_buf_tx;
	SOCKET_RELEASE();
	return 0;
}

uint socket_buf_empty_len()
{
	uint len;
	SOCKET_REQUEST();
	if (socket_buf_tx_out <= socket_buf_tx_in)
	{
		len = socket_buf_LEN - (socket_buf_tx_in - socket_buf_tx_out) - 1;
	}
	else
	{
		len = socket_buf_tx_out - socket_buf_tx_in - 1;
	}
	SOCKET_RELEASE();
	return len;
}

uchar socket_buf_is_empty()
{
	uint ret;
	SOCKET_REQUEST();
	if (socket_buf_tx_out != socket_buf_tx_in)
	{
		ret = 0;
	}
	else
		ret = 1;
	SOCKET_RELEASE();
	return ret;
}

uint socket_buf_send(uchar *value, uint len)
{
	uint len_;
	SOCKET_REQUEST();
	if (socket_buf_tx_out <= socket_buf_tx_in)
	{
		len_ = socket_buf_LEN - (socket_buf_tx_in - socket_buf_tx_out) - 1;
		if (len < len_)
			len_ = len;

		if ((socket_buf_tx_in + len_) < ((uint)socket_buf_tx + sizeof(socket_buf_tx)))
		{
			memcpy((void *)socket_buf_tx_in, value, len_);
			socket_buf_tx_in += len_;
		}
		else
		{
			uint len1;
			len1 = ((uint)socket_buf_tx + sizeof(socket_buf_tx)) - socket_buf_tx_in;
			memcpy((void *)socket_buf_tx_in, value, len1);
			memcpy(socket_buf_tx, value + len1, len_ - len1);
			socket_buf_tx_in = (uint)socket_buf_tx + (len_ - len1);
		}
	}
	else
	{
		len_ = socket_buf_tx_out - socket_buf_tx_in - 1;
		if (len < len_)
			len_ = len;
		memcpy((void *)socket_buf_tx_in, value, len_);
		socket_buf_tx_in += len_;
	}
	SOCKET_RELEASE();
	ser2network_notify_socket();
	led_blink();
}

uint socket_buf_get(uchar *value, uint len)
{

	SOCKET_REQUEST();
	SOCKET_RELEASE();
}

uint socket_buf_tx_(uchar *buf, uint *len_)
{
	uint len;
	int iret = 0;
	int ret;

	SOCKET_REQUEST();
	if (socket_buf_tx_out <= socket_buf_tx_in)
	{
		len = (socket_buf_tx_in - socket_buf_tx_out);
		memcpy(buf, (void *)socket_buf_tx_out, len);
		buf += len;
		ret = len;

		if (ret >= 0)
		{
			socket_buf_tx_out += ret;
		}
		else
		{
			TIM_ERROR("ERROR:%s(%d)(%d)(%s)\r\n", __FUNCTION__, __LINE__, ret, strerror(ret));
			iret = -1;
		}
	}
	else
	{
		int len1;
		len = socket_buf_LEN - (socket_buf_tx_out - socket_buf_tx_in);
		len1 = (((uint)socket_buf_tx + sizeof(socket_buf_tx)) - socket_buf_tx_out);

		memcpy(buf, (void *)socket_buf_tx_out, len1);
		buf += len1;
		ret = len1;

		if (ret >= 0)
		{
			if (ret == len1)
			{
				int len2;
				len2 = socket_buf_tx_in - (uint)socket_buf_tx;

				memcpy(buf, socket_buf_tx, len2);
				buf += len2;
				ret = len2;
				if (ret >= 0)
				{
					socket_buf_tx_out = (uint)socket_buf_tx + ret;
				}
				else
				{
					TIM_ERROR("ERROR:%s(%d)(%d)(%s)\r\n", __FUNCTION__, __LINE__, ret, strerror(ret));
					iret = -1;
					socket_buf_tx_out += ret;
				}
			}
			else
			{
				socket_buf_tx_out += ret;
			}
		}
		else
		{
			TIM_ERROR("ERROR:%s(%d)(%d)(%s)\r\n", __FUNCTION__, __LINE__, ret, strerror(ret));
			iret = -1;
		}
	}
	SOCKET_RELEASE();
	*len_ = len;
	return iret;
}

uint timeval_subtract(struct timeval *x, struct timeval *y)
{
	unsigned long long nsec1, nsec2;

	nsec1 = x->tv_sec * 1000000 + x->tv_usec;
	nsec2 = y->tv_sec * 1000000 + y->tv_usec;

	if (nsec1 < nsec2)
		return (nsec2 - nsec1);
	else
		return (0x0fffffff);
}
uint timeval_subtract2(struct timeval *x, struct timeval *y)
{
	unsigned long long nsec1, nsec2;

	nsec1 = x->tv_sec * 1000000 + x->tv_usec;
	nsec2 = y->tv_sec * 1000000 + y->tv_usec;

	if (nsec1 <= nsec2)
		return (nsec2 - nsec1);
	else
		return (0x0fffffff);
}

void out_transparent2(uchar *p, int len);

void *serial_thread(void *arg)
{
	int ret;
	uchar buf[COMM_BUF_LEN];
	uchar buf_serial[SERIAL_BUF_LEN];
	uint buf_serial_len = 0;

	struct timeval tv;
	fd_set select_fds;

	int g_serial_fd = 0;
	int g_select_quit_fd = 0;
	int g_notify_serial_fd = 0;
	uchar serial_opened = 0;

	uint tx_wait_time = 0;			  //us
	uint serial_timeout = 0;		  //us
	uint serial_interval_timeout = 0; //us

	struct timeval serial_timeout_start;
	struct timeval serial_interval_timeout_start;

	if ((mkfifo(SERIAL_QUIT_PIPE_DEV, 0777) < 0) && (errno != EEXIST))
	{
		TIM_DEBUG("ERROR:%s(%d)\r\n", __FUNCTION__, __LINE__);
	}

	g_select_quit_fd = open(SERIAL_QUIT_PIPE_DEV, O_NONBLOCK | O_RDWR);
	if (g_select_quit_fd <= 0)
	{
		TIM_ERROR("ERROR:%s(%d)\r\n", __FUNCTION__, __LINE__);
		restart_app();
	}


	if ((mkfifo(NOTIFY_SERIAL_PIPE_DEV, 0777) < 0) && (errno != EEXIST))
	{
	}

	g_notify_serial_fd = open(NOTIFY_SERIAL_PIPE_DEV, O_NONBLOCK | O_RDWR);
	if (g_notify_serial_fd <= 0)
	{
		TIM_ERROR("ERROR:%s(%d)\r\n", __FUNCTION__, __LINE__);
		restart_app();
	}

	debug_log("serial thread start");

	while (1)
	{
		int maxfd = 0;
		uchar time_need = 0;

		FD_ZERO(&select_fds);

		if (serial_opened)
		{
			if (g_serial_fd)
			{
				FD_SET(g_serial_fd, &select_fds);
				maxfd = MAX_(maxfd, g_serial_fd);
			}
			else
			{
				TIM_ERROR("ERROR:%s(%d)\r\n", __FUNCTION__, __LINE__);
			}
		}

		FD_SET(g_select_quit_fd, &select_fds);
		maxfd = MAX_(maxfd, g_select_quit_fd);

		FD_SET(g_notify_serial_fd, &select_fds);
		maxfd = MAX_(maxfd, g_notify_serial_fd);

		if (0 == serial_opened)
		{
#if 1
			{
				TIM_ERROR("g\r\n");
				g_serial_fd = open(UART_DEV, O_RDWR | O_NOCTTY | O_NDELAY);
				if (g_serial_fd < 0)
				{
					TIM_ERROR("ERROR:%s(%d)\r\n", __FUNCTION__, __LINE__);
				}
				else
				{
					Serial_Config(g_serial_fd, g_serial_config);
					serial_opened = 1;
				}
			}
#endif

			tv.tv_sec = 500 / 1000;
			tv.tv_usec = (500 % 1000) * 1000;
			time_need = 1;
		}
		else
		{
			{
				uint serial_time = 0;

				tx_wait_time = 0;
				if (0 == serial_buf_is_empty())
				{
					serial_buf_tx_(g_serial_fd);
					if (0 == serial_buf_is_empty())
					{
						tx_wait_time = get_serial_wait_time();
					}
				}

				if (serial_timeout || serial_interval_timeout)
				{
					uint serial_time1 = 0xfffffff;
					uint serial_time2 = 0xfffffff;
					struct timeval now;
					gettimeofday(&now, NULL);
					if (serial_timeout)
					{
						serial_time1 = timeval_subtract(&serial_timeout_start, &now);
						if (serial_time1 >= serial_timeout)
							serial_time1 = 1;
						else
							serial_time1 = serial_timeout - serial_time1;
					}
					if (serial_interval_timeout)
					{
						serial_time2 = timeval_subtract(&serial_interval_timeout_start, &now);
						if (serial_time2 >= serial_interval_timeout)
							serial_time2 = 1;
						else
							serial_time2 = serial_interval_timeout - serial_time2;
					}
					serial_time = MIN_(serial_time1, serial_time2);
				}

				if (tx_wait_time || serial_time)
				{
					uint t;
					if (tx_wait_time > serial_time)
					{
						if (serial_time)
						{
							t = serial_time;
							time_need = 2;
						}
						else
						{
							t = tx_wait_time;
							time_need = 3;
						}
					}
					else
					{
						if (tx_wait_time)
						{
							t = tx_wait_time;
							time_need = 3;
						}
						else
						{
							t = serial_time;
							time_need = 2;
						}
					}
					tv.tv_sec = t / 1000000;
					tv.tv_usec = t % 1000000;
				}
			}
		}

		if (time_need)
		{
			ret = select(maxfd + 1, &select_fds, NULL, NULL, &tv);
		}
		else
		{
			ret = select(maxfd + 1, &select_fds, NULL, NULL, NULL);
		}
		if (ret > 0)
		{

			if (FD_ISSET(g_notify_serial_fd, &select_fds))
			{
				uint x;
				uchar buf1[64];
				x = read(g_notify_serial_fd, buf1, sizeof(buf1));

				if (Serial_Flush_now)
				{
					Serial_Flush_now = 0;

					if (serial_opened)
					{
						if (g_serial_fd)
							tcflush(g_serial_fd, TCOFLUSH);
					}
				}
				else
				{
					serial_buf_tx_(g_serial_fd);
				}
			}

			if (FD_ISSET(g_select_quit_fd, &select_fds))
			{
				read(g_select_quit_fd, buf, sizeof(buf));
				break;
			}

			if (FD_ISSET(g_serial_fd, &select_fds))
			{
				if (G_serial_mode_get())	//AT模式
				{
					uint z;
					z = read(g_serial_fd, &buf_serial[buf_serial_len], 1);

					if (1 == z)
					{

						if ((0x00 == buf_serial[buf_serial_len]) || (0x0a == buf_serial[buf_serial_len]))
						{
							continue;
						}

						buf_serial_len += z;

						if (buf_serial_len >= sizeof(buf))
						{
							TIM_ERROR("ERROR:%s(%d)\r\n", __FUNCTION__, __LINE__);
							buf_serial_len = 0;
							serial_buf_send("\r\nERROR:command too long!\r\n", sizeof("\r\nERROR:command too long!\r\n"));
							continue;
						}

						serial_buf_send(&buf_serial[buf_serial_len - 1], 1);

						if ('\r' == buf_serial[buf_serial_len - 1])
						{
							buf_serial[buf_serial_len - 1] = 0;
							buf[0] = 0;
							if (buf_serial[0])
								config_command(buf_serial, buf);
							buf[sizeof(buf) - 1] = 0;
							serial_buf_send("\r\n", 2);
							serial_buf_send(buf, strlen(buf));
							serial_buf_send("\r\n", 2);
							buf_serial_len = 0;
						}
					}
					else
					{
						TIM_ERROR("ERROR:%s(%d)\r\n", __FUNCTION__, __LINE__);
						buf_serial_len = 0;
					}
				}
				else	
				{
					//透传模式
					uint z;
					z = read(g_serial_fd, &buf_serial[buf_serial_len], MIN_(g_serial_package_len, sizeof(buf_serial) - buf_serial_len));
					if (0 == buf_serial_len)
					{
						uint x = get_serial_wait_time2(z);
						uint y = g_serial_package_time * 1000;
						if (y > x)
						{
							serial_timeout = y - x;
						}
						else
						{
							serial_timeout = 1;
						}
						gettimeofday(&serial_timeout_start, NULL);
					}
					serial_interval_timeout = g_serial_package_interval_time * 1000;
					gettimeofday(&serial_interval_timeout_start, NULL);
					if (1 == g_transparent_escape_en)
					{
						if (1 == z)
							out_transparent(buf_serial[buf_serial_len]);
						else
							out_transparent(0xff);
					}

					if (1 == g_transparent_escape2_en)
					{
						if (3 == z)
							out_transparent2(&buf_serial[buf_serial_len], z);
						else
							out_transparent2(0, 0);
					}

					buf_serial_len += z;

					if ((buf_serial_len >= MIN_(g_serial_package_len, sizeof(buf_serial))) || (serial_timeout == 0) || (serial_interval_timeout == 0))
					{
						socket_buf_send(buf_serial, buf_serial_len);
						statistics_serial_tx += buf_serial_len;

						buf_serial_len = 0;
						serial_timeout = 0;
						serial_interval_timeout = 0;
					}
					else
					{

						{

							uint serial_time;
							{
								serial_time = timeval_subtract2(&serial_timeout_start, &serial_interval_timeout_start);
								if (serial_time >= serial_timeout)
								{
									socket_buf_send(buf_serial, buf_serial_len);
									statistics_serial_tx += buf_serial_len;

									buf_serial_len = 0;
									serial_timeout = 0;
									serial_interval_timeout = 0;

								}
							}
						}
					}
				}
			}
		}
		else if (ret == 0)
		{
			if ((0 != serial_opened) && (0 == tx_wait_time) && (0 == serial_timeout) && (0 == serial_interval_timeout))
			{

				TIM_ERROR("ERROR:%s(%d)\r\n", __FUNCTION__, __LINE__);
			}

			if (0 == serial_opened)
			{
				g_serial_fd = open(UART_DEV, O_RDWR | O_NOCTTY | O_NDELAY);
				if (g_serial_fd < 0)
				{
					TIM_ERROR("ERROR:%s(%d)\r\n", __FUNCTION__, __LINE__);
				}
				else
				{
					Serial_Config(g_serial_fd, g_serial_config);
					serial_opened = 1;
				}
			}
			else
			{
				if (0 == serial_buf_is_empty())
				{
					serial_buf_tx_(g_serial_fd);
				}

				if (serial_timeout || serial_interval_timeout)
				{
					uint serial_time;
					struct timeval now;
					gettimeofday(&now, NULL);
					if (serial_timeout)
					{
						serial_time = timeval_subtract(&serial_timeout_start, &now);
						if (serial_time >= serial_timeout)
						{
							socket_buf_send(buf_serial, buf_serial_len);
							statistics_serial_tx += buf_serial_len;
							buf_serial_len = 0;
							serial_timeout = 0;
							serial_interval_timeout = 0;
						}
					}
					if (buf_serial_len && serial_interval_timeout)
					{

						serial_time = timeval_subtract(&serial_interval_timeout_start, &now);
						if (serial_time >= serial_interval_timeout)
						{
							socket_buf_send(buf_serial, buf_serial_len);
							statistics_serial_tx += buf_serial_len;

							buf_serial_len = 0;
							serial_timeout = 0;
							serial_interval_timeout = 0;
						}
					}
				}
			}

			continue;
		}
		else
		{
			TIM_ERROR("ERROR:%s(%d)\r\n", __FUNCTION__, __LINE__);
		}
	}

	if (g_serial_fd > 0)
	{
		close(g_serial_fd);
	}

	if (g_notify_serial_fd > 0)
	{
		close(g_notify_serial_fd);
	}

	if (g_select_quit_fd > 0)
	{
		close(g_select_quit_fd);
	}

	pthread_detach(pthread_self());
	return ((void *)0);
}

#define abs_(a, b) ((a >= b) ? (a - b) : (b - a))

void out_transparent(uchar value)
{
	static uchar state = 0;
	struct timeval tv;
	static struct timeval tv_last;

	uint time;
	static uint time_diff;

	switch (state)
	{
	case 0:
		if ('+' == value)
			state = 1;
		else
			state = 0;
		break;
	case 1:
		if ('+' == value)
			state = 3;
		else
			state = 0;
		break;
	case 2:
		if ('+' == value)
			state = 3;
		else
			state = 0;
		break;
	case 3:
		if ('+' == value)
		{
			state = 4;
			gettimeofday(&tv_last, NULL);
		}
		else
			state = 0;
		break;
	case 4:
		if (0x1b == value)
		{
			gettimeofday(&tv, NULL);
			time = (tv.tv_sec - tv_last.tv_sec) * 1000000 + (tv.tv_usec - tv_last.tv_usec);

			if ((time > 400000) && (time < 600000))
			{
				state = 5;
				time_diff = time;
				gettimeofday(&tv_last, NULL);
			}
			else
			{
				state = 0;
				time_diff = 0;
			}
		}
		else
			state = 0;
		break;
	case 5:
		if (0x1b == value)
		{
			state = 7;
		}
		else
			state = 0;
		break;
	case 6:
		if (0x1b == value)
		{
			state = 7;
		}
		else
			state = 0;
		break;
	case 7:
		if (0x1b == value)
		{
			G_serial_mode_set(2);
		}
		state = 0;
		break;
	default:
		state = 0;
		break;
	}
}

void error_check_1s()
{
	struct timeval tv;
	static struct timeval tv_last = {0, 0};

	static uchar count = 0;
	uint i;

	gettimeofday(&tv, NULL);

	i = timeval_subtract(&tv_last, &tv);
	if ((i < (1000 * 1000)) && (tv_last.tv_sec) && (tv_last.tv_usec))
	{
		count++;
		if (count > 20)
		{
			if (0 == G_serial_mode_get())
			{
				TIM_ERROR("ERROR!!!!:%s(%d)\r\n", __FUNCTION__, __LINE__);
				restart_app();
			}
		}
	}
	else
	{
		count = 0;
	}
	gettimeofday(&tv_last, NULL);
}

void error_check_1s_2()
{
	struct timeval tv;
	static struct timeval tv_last = {0, 0};

	static uchar count = 0;
	uint i;

	gettimeofday(&tv, NULL);

	i = timeval_subtract(&tv_last, &tv);
	if ((i < (2000 * 1000)) && (tv_last.tv_sec) && (tv_last.tv_usec))
	{
		count++;
		if (count > 20)
		{
			if (0 == G_serial_mode_get())
			{
				TIM_ERROR("ERROR!!!!:%s(%d)\r\n", __FUNCTION__, __LINE__);
				restart_app();
			}
		}
	}
	else
	{
		count = 0;
	}
	gettimeofday(&tv_last, NULL);
}

void out_transparent2(uchar *p, int len)
{
	struct timeval tv;
	static struct timeval tv_last = {
		0, 0};

	if (len == 0)
	{
		gettimeofday(&tv_last, NULL);
	}
	else
	{
		uint i;
		gettimeofday(&tv, NULL);
		i = timeval_subtract(&tv_last, &tv);
		TIM_DEBUG("%s(%d) (%d) \r\n", __FUNCTION__, __LINE__, i);
		if ((p[0] == '+') && (p[1] == '+') && (p[2] == '+') && (i > (g_transparent_escape2_time * 1000)) && (tv_last.tv_sec) && (tv_last.tv_usec))
		{
			G_serial_mode_set(2);
			tv_last.tv_sec = 0;
			tv_last.tv_usec = 0;
		}
		else
		{
			gettimeofday(&tv_last, NULL);
		}
	}
}

void restart_app()
{
	TIM_ERROR("ERROR!!!!:%s(%d)\r\n", __FUNCTION__, __LINE__);
	TIM_ERROR("App restart again:%s(%d)\r\n", __FUNCTION__, __LINE__);
	exit(-1);
	system("(killall ser2net;killall -9 ser2net;)&");
	sleep(10);
	system("(killall ser2net;killall -9 ser2net;sleep 1;ser2net)&");
	while (1)
		sleep(30);
}

uint error_times = 0;
#define MAX_ERROR_TIMES 10

void ERROR_TIMES_RESET()
{
	error_times = 0;
}

void ERROR_TIMES_AGAIN()
{
	if (error_times++ > MAX_ERROR_TIMES)
	{
		restart_app();
	}
}

struct sockaddr_in toAddr;
struct sockaddr_in *udp_toAddr_p;

void led_tcp_check(int *client)
{
	int i;
	uchar found = 0;
	for (i = 0; i < MAX_CONNECTS; i++)
	{
		if (client[i] != (-1))
		{
			found = 1;
			break;
		}
	}
	led_set(found);
}

int shutdown2(int sockfd, int how)
{
	setsockopt(sockfd, IPPROTO_TCP, TCP_QUICKACK, (int[]){1}, sizeof(int));

	return shutdown(sockfd, how);
}

void *ser2network_server_thread(void *arg)
{
	int ret;
	uchar first_run = 1;
	uchar listened = 0;

	uint rx_need_wait = 0;
	struct timeval rx_need_wait_start;

	int g_select_quit_fd = 0;
	int g_notify_socket_fd = 0;

	struct timeval tv;
	fd_set select_fds;

	int listenfd = 0, connfd = 0, sockfd = 0, maxfd, maxi, i;
	int nready, client[MAX_CONNECTS];
	int lens;
	socklen_t clilen;
	struct sockaddr_in servaddr, chiaddr;

	ssize_t n;
	uchar buf[SERIAL_BUF_LEN];

	uint client_tick[MAX_CONNECTS];

	led_set(0);

	ERROR_TIMES_RESET();

again1:
	if ((mkfifo(SERVER_QUIT_PIPE_DEV, 0777) < 0) && (errno != EEXIST))
	{
		TIM_ERROR("ERROR:%s(%d)\r\n", __FUNCTION__, __LINE__);
	}

	g_select_quit_fd = open(SERVER_QUIT_PIPE_DEV, O_NONBLOCK | O_RDWR);
	if (g_select_quit_fd <= 0)
	{
		TIM_ERROR("ERROR:%s(%d)\r\n", __FUNCTION__, __LINE__);
		ERROR_TIMES_AGAIN();
		usleep(50 * 1000);
		goto again1;
	}

	ERROR_TIMES_RESET();

again2:
	if ((mkfifo(NOTIFY_SOCKET_PIPE_DEV, 0777) < 0) && (errno != EEXIST))
	{
		TIM_ERROR("ERROR:%s(%d)\r\n", __FUNCTION__, __LINE__);
	}

	g_notify_socket_fd = open(NOTIFY_SOCKET_PIPE_DEV, O_NONBLOCK | O_RDWR);
	if (g_notify_socket_fd <= 0)
	{
		TIM_ERROR("ERROR:%s(%d)\r\n", __FUNCTION__, __LINE__);
		ERROR_TIMES_AGAIN();
		usleep(50 * 1000);
		goto again2;
	}

	int g_notify_fd = 0;

	ERROR_TIMES_RESET();

again3:
	if ((mkfifo(NOTIFY_PIPE_DEV, 0777) < 0) && (errno != EEXIST))
	{
		TIM_ERROR("ERROR:%s(%d)\r\n", __FUNCTION__, __LINE__);
	}

	g_notify_fd = open(NOTIFY_PIPE_DEV, O_NONBLOCK | O_RDWR);
	if (g_notify_fd <= 0)
	{
		TIM_ERROR("ERROR:%s(%d)\r\n", __FUNCTION__, __LINE__);
		ERROR_TIMES_AGAIN();
		usleep(50 * 1000);
		goto again3;
	}

out1:

	for (i = 0; i < MAX_CONNECTS; i++)
	{
		client[i] = -1;
	}

	while (1)
	{
		int maxfd = 0;

		FD_ZERO(&select_fds);

		if (g_select_quit_fd > 0)
		{
			FD_SET(g_select_quit_fd, &select_fds);
			maxfd = MAX_(maxfd, g_select_quit_fd);
		}

		if (1 == listened)
		{
			FD_SET(listenfd, &select_fds);
			maxfd = MAX_(maxfd, listenfd);
		}

		if (rx_need_wait)
		{
			uint serial_time;
			struct timeval now;
			gettimeofday(&now, NULL);
			serial_time = timeval_subtract(&rx_need_wait_start, &now);
			if (serial_time >= rx_need_wait)
			{
				rx_need_wait = 0;
			}
			else
			{
				rx_need_wait = rx_need_wait - serial_time;
				TIM_DEBUG("in:%s(%d)(%d)\r\n", __FUNCTION__, __LINE__, rx_need_wait);
			}
		}

		if (0 == rx_need_wait)
		{
			uchar have_connect = 0;
			for (i = 0; i < MAX_CONNECTS; i++)
			{
				if (client[i] > 0)
				{
					FD_SET(client[i], &select_fds);
					maxfd = MAX_(maxfd, client[i]);
					have_connect = 1;
				}
			}
			if (have_connect)
				if (g_notify_fd > 0)
				{
					FD_SET(g_notify_fd, &select_fds);
					maxfd = MAX_(maxfd, g_notify_fd);
				}
		}

		TIM_DEBUG("in:%s(%d) %d %d\r\n", __FUNCTION__, __LINE__, maxfd, select_fds);

		if (0 == listened)
		{
			if (first_run == 1)
			{
				first_run = 0;
				tv.tv_sec = 0;
				tv.tv_usec = 1;
			}
			else
			{
				tv.tv_sec = 0;
				tv.tv_usec = 500 * 1000;
			}
			ret = select(maxfd + 1, &select_fds, NULL, NULL, &tv);
		}
		else
		{

			if (g_notify_socket_fd > 0)
			{
				FD_SET(g_notify_socket_fd, &select_fds);
				maxfd = MAX_(maxfd, g_notify_socket_fd);
			}

			if (rx_need_wait)
			{
				tv.tv_sec = rx_need_wait / 1000000;
				tv.tv_usec = rx_need_wait % 1000000;
				ret = select(maxfd + 1, &select_fds, NULL, NULL, &tv);
			}
			else
			{
				ret = select(maxfd + 1, &select_fds, NULL, NULL, NULL);
			}
		}
		if (ret > 0)
		{
			if (FD_ISSET(g_notify_socket_fd, &select_fds))
			{
				uint x;
				x = read(g_notify_socket_fd, buf, sizeof(buf));

				uchar buf_serial[socket_buf_LEN];
				uint buf_serial_len = 0;

				socket_buf_tx_(buf_serial, &buf_serial_len);
				statistics_serial_tx2 += buf_serial_len;

				if (1 == g_network_protocol)
				{

					for (i = 0; i < MAX_CONNECTS; i++)
					{
						if ((sockfd = client[i]) > 0)
						{
							fd_write(sockfd, buf_serial, buf_serial_len); //!> ��������д��ȥ
						}
					}

				}
				else
				{
					if (udp_toAddr_p)
					{
						if (sendto(listenfd, buf_serial, buf_serial_len, 0, (struct sockaddr *)udp_toAddr_p, sizeof(struct sockaddr)) != buf_serial_len)
						{
							close(listenfd);
							listened = 0;
							udp_toAddr_p = 0;
							led_set(0);
						}
					}
				}
			}

			time_t now;
			time(&now);

			if ((listenfd) && FD_ISSET(listenfd, &select_fds))
			{
				if (1 == g_network_protocol)
				{
					clilen = sizeof(chiaddr);

					if ((connfd = accept(listenfd, (struct sockaddr *)&chiaddr, &clilen)) == -1)
					{ 
						TIM_ERROR("Error : %d(%s)\n", errno, strerror(errno));
						TIM_ERROR("ERROR:%s(%d)\r\n", __FUNCTION__, __LINE__);
						continue;
					}
					{
						int flags;
						flags = fcntl(connfd, F_GETFL, 0);
						fcntl(connfd, F_SETFL, flags | O_NONBLOCK);
					}

					{
						int nRecvBuf = 1024 * 4;
						setsockopt(connfd, SOL_SOCKET, SO_RCVBUF, (const char *)&nRecvBuf, sizeof(int));
						int nSendBuf = 1024 * 4;
						setsockopt(connfd, SOL_SOCKET, SO_SNDBUF, (const char *)&nSendBuf, sizeof(int));
					}
					if (1)
					{
						struct timeval timeout;
						timeout.tv_sec = 33;
						timeout.tv_usec = 0;

						setsockopt(connfd, SOL_SOCKET, SO_SNDTIMEO, (const char *)&timeout, sizeof(int));
						setsockopt(connfd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout, sizeof(int));
					}

#if 1 //test
					if (g_tcp_client_stat_check)
					{
						{
							int s = connfd;
							int optval;
							socklen_t optlen = sizeof(optval);

							if (getsockopt(s, SOL_SOCKET, SO_KEEPALIVE, &optval, &optlen) < 0)
							{
								perror("getsockopt()");
								close(s);
								exit(EXIT_FAILURE);
							}
							printf("SO_KEEPALIVE is %s\n", (optval ? "ON" : "OFF"));

							optval = 1;
							optlen = sizeof(optval);
							if (setsockopt(s, SOL_SOCKET, SO_KEEPALIVE, &optval, optlen) < 0)
							{
								perror("setsockopt()");
								close(s);
								exit(EXIT_FAILURE);
							}
							printf("SO_KEEPALIVE set on socket\n");

							if (getsockopt(s, SOL_SOCKET, SO_KEEPALIVE, &optval, &optlen) < 0)
							{
								perror("getsockopt()");
								close(s);
								exit(EXIT_FAILURE);
							}
							printf("SO_KEEPALIVE is %s\n", (optval ? "ON" : "OFF"));
						}

						{

							int s = connfd;
							int optval;
							socklen_t optlen = sizeof(optval);

							optval = 1;
							optlen = sizeof(optval);
							if (setsockopt(s, SOL_SOCKET, SO_KEEPALIVE, &optval, optlen) < 0)
							{
								TIM_ERROR("ERROR:%s(%d)\r\n", __FUNCTION__, __LINE__);
							}

#define SOL_TCP 6

							optval = 5;
							optlen = sizeof(optval);
							if (setsockopt(s, SOL_TCP, TCP_KEEPIDLE, &optval, optlen) < 0)
							{
								TIM_ERROR("ERROR:%s(%d)\r\n", __FUNCTION__, __LINE__);
							}
							optval = 3;
							optlen = sizeof(optval);
							if (setsockopt(s, SOL_TCP, TCP_KEEPINTVL, &optval, optlen) < 0)
							{
								TIM_ERROR("ERROR:%s(%d)\r\n", __FUNCTION__, __LINE__);
							}
							optval = 3;
							optlen = sizeof(optval);
							if (setsockopt(s, SOL_TCP, TCP_KEEPCNT, &optval, optlen) < 0)
							{
								TIM_ERROR("ERROR:%s(%d)\r\n", __FUNCTION__, __LINE__);
							}
						}
					}
#endif

#if 1

					if (0 != g_tcp_realtime)
					{
						int s = connfd;
						int optval;
						socklen_t optlen = sizeof(optval);

						optval = 1;
						optlen = sizeof(optval);
						if (setsockopt(s, IPPROTO_TCP, TCP_NODELAY, &optval, optlen) < 0)
						{
							TIM_ERROR("ERROR:%s(%d)\r\n", __FUNCTION__, __LINE__);
						}
					}
#endif

					if (*g_SRemoteIpPort)
					{
						uchar tmp[64];

						sprintf(tmp, "|%s:0|", inet_ntoa(chiaddr.sin_addr));

						TIM_DEBUG("ERROR:%s(%d)(%s) g_SRemoteIpPort:%s\r\n", __FUNCTION__, __LINE__, tmp, g_SRemoteIpPort);

						if (0 == strstr(g_SRemoteIpPort, tmp))
						{

							sprintf(tmp, "|%s:%d|", inet_ntoa(chiaddr.sin_addr), ntohs(chiaddr.sin_port));

							TIM_DEBUG("ERROR:%s(%d)(%s)\r\n", __FUNCTION__, __LINE__, tmp);

							if (0 == strstr(g_SRemoteIpPort, tmp))
							{
								TIM_ERROR("ERROR:%s(%d)(%s)\r\n", __FUNCTION__, __LINE__, tmp);
								close(connfd);
								continue;
							}
						}
					}

					for (i = 0; i < MAX_CONNECTS; i++)
					{

						if ((g_network_timeout) && ((now - client_tick[i]) > g_network_timeout))
						{
							shutdown2(client[i], SHUT_RDWR);
							close(client[i]);
							client[i] = -1;
						}

						if (client[i] < 0)
						{
							client[i] = connfd;
							client_tick[i] = now;
							TIM_DEBUG("in:%s(%d) %d %d\r\n", __FUNCTION__, __LINE__, i, client[i]);
							break;
						}
					}

					if (i >= g_SMaxConn)
					{

						TIM_ERROR("To many ... ");
						TIM_ERROR("ERROR:%s(%d)(%d)(%d)\r\n", __FUNCTION__, __LINE__, i, g_SMaxConn);
						close(connfd);
						client[i] = -1;
						continue;
					}

					if (1 == G_serial_mode_get())
					{
						G_serial_mode_set(0);
					}
					led_set(1);
				}
				else
				{
					uint recvLen = 0;

					unsigned int addrLen;

					addrLen = sizeof(struct sockaddr_in);

					if (1 == G_serial_mode_get())
					{
						G_serial_mode_set(0);
					}

					udp_toAddr_p = &toAddr;

					led_set(1);

					if ((recvLen = recvfrom(listenfd, buf, serial_buf_empty_len(), 0, (struct sockaddr *)udp_toAddr_p, &addrLen)) < 0)
					{
						TIM_ERROR("ERROR:%s(%d)\r\n", __FUNCTION__, __LINE__);
						close(listenfd);
						listened = 0;
						udp_toAddr_p = 0;
						led_set(0);
					}

					serial_buf_send(buf, recvLen);
				}
			}

			if (0 == rx_need_wait)
				for (i = 0; i < MAX_CONNECTS; i++)
				{
					if ((sockfd = client[i]) > 0)
					{
						if (FD_ISSET(sockfd, &select_fds) || FD_ISSET(g_notify_fd, &select_fds))
						{
							uint lenx = serial_buf_empty_len();
							if (FD_ISSET(g_notify_fd, &select_fds))
							{
								lenx = 0;
								read(g_notify_fd, buf, sizeof(buf));
							}
							else
							{
								client_tick[i] = now;
							}

							n = read(sockfd, buf, lenx);

							if (n < 0)
							{
								TIM_ERROR("Error : %d(%s)\n", errno, strerror(errno));
								TIM_ERROR("ERROR:%s(%d)(%d)(%d)\r\n", __FUNCTION__, __LINE__, n, lenx);

								shutdown2(sockfd, SHUT_RDWR);
								close(sockfd);
								client[i] = -1;
								led_tcp_check(client);
								continue;
							}

							else if (n == 0)
							{
								if (lenx)
								{
									TIM_ERROR("ERROR:%s(%d)\r\n", __FUNCTION__, __LINE__);
									shutdown2(sockfd, SHUT_RDWR);
									close(sockfd);
									client[i] = -1;
									led_tcp_check(client);
									continue;
								}
								else if (FD_ISSET(g_notify_fd, &select_fds))
								{
									buf[0] = 0;
								}
								else if (G_serial_mode_get())
								{
									TIM_ERROR("ERROR:%s(%d)\r\n", __FUNCTION__, __LINE__);
									buf[0] = 0;
									rx_need_wait = 500 * 1000;
									gettimeofday(&rx_need_wait_start, NULL);
								}
								else
								{
									buf[0] = 0;
									rx_need_wait = get_serial_wait_time();
									gettimeofday(&rx_need_wait_start, NULL);
								}
							}

							TIM_DEBUG("Server Recv: %s\n", buf);

							TIM_DEBUG("Server send : %s\n", buf);
							serial_buf_send(buf, n);
						}
						else if ((g_network_timeout) && ((now - client_tick[i]) > g_network_timeout))
						{
							shutdown2(sockfd, SHUT_RDWR);
							close(sockfd);
							client[i] = -1;
							led_tcp_check(client);
						}
					}
				}

			if (FD_ISSET(g_select_quit_fd, &select_fds))
			{
				read(g_select_quit_fd, buf, sizeof(buf));

				for (i = 0; i < MAX_CONNECTS; i++)
				{
					if (client[i] > 0)
					{
						if (shutdown2(client[i], SHUT_RDWR) < 0)
						{
							TIM_ERROR("ERROR:%s(%d)\r\n", __FUNCTION__, __LINE__);
						}
						TIM_DEBUG("in:%s(%d) %d %d\r\n", __FUNCTION__, __LINE__, i, client[i]);
						if (close(client[i]) < 0)
						{
							TIM_ERROR("ERROR:%s(%d)\r\n", __FUNCTION__, __LINE__);
						}
					}
				}

				shutdown2(listenfd, SHUT_RDWR);
				close(listenfd);
				close(g_select_quit_fd);
				close(g_notify_socket_fd);
				close(g_notify_fd);

				led_set(0);

				break;
			}
		}
		else if (ret == 0)
		{
			if ((0 != listened) && (0 == rx_need_wait))
			{

				TIM_ERROR("ERROR:%s(%d)\r\n", __FUNCTION__, __LINE__);
			}

			if (0 == listened)
			{

				if ((listenfd = socket(AF_INET, ((1 == g_network_protocol) ? SOCK_STREAM : (SOCK_DGRAM)), ((1 == g_network_protocol) ? 0 : (IPPROTO_UDP)))) == -1)
				{
					goto out2;
				}

				{
					int on = 1;
					ret = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
				}

				bzero(&servaddr, sizeof(servaddr));
				servaddr.sin_family = AF_INET;
				servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
				servaddr.sin_port = htons(g_locale_remote_port);

				if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1)
				{
					TIM_ERROR("BindError : %d(%s)\n", errno, strerror(errno));
					TIM_ERROR("in:%s(%d)(%d)\r\n", __FUNCTION__, __LINE__, g_client_locale_port);
					close(listenfd);
					error_check_1s();

					listenfd = 0;
					goto out2;
				}

				if (1 == g_network_protocol)
				{

					if (listen(listenfd, g_SMaxConn) == -1)
					{
						close(listenfd);
						listenfd = 0;
						goto out2;
					}
				}
				else if (2 == g_network_protocol)
				{
					int yes = 1;
					setsockopt(listenfd, SOL_SOCKET, SO_BROADCAST, &yes, sizeof(yes));
				}

				listened = 1;
			out2:
				continue;
			}
			else
			{
				if (rx_need_wait)
				{
				}
			}
		}
		else
		{
			TIM_ERROR("Error : %d(%s)\n", errno, strerror(errno));
			TIM_ERROR("ERROR:%s(%d)\r\n", __FUNCTION__, __LINE__);

			static uchar x = 0;
			static uint lasttime = 0;
			if (lasttime)
			{
				struct timeval tv;
				gettimeofday(&tv, NULL);
				if (lasttime == tv.tv_sec)
				{
					x++;
					if (x > 5)
					{
						TIM_ERROR("ERROR:%s(%d)reboot\r\n", __FUNCTION__, __LINE__);
						restart_app();
						while (1)
							sleep(30);
					}
				}
				else
				{
					x = 0;
				}
				lasttime = tv.tv_sec;
			}
			else
			{
				struct timeval tv;
				gettimeofday(&tv, NULL);
				lasttime = tv.tv_sec;
			}
		}
	}
out:

	pthread_detach(pthread_self());
	return ((void *)0);
}

uint tim_all_len = 0;
uint tim_all_len2 = 0;

//因为tcp连接时非阻塞的， 所以 分为了三种状态 -1：未连接 0：连接中 1：已连接 （已连接）对于UDP 没有连接状态
enum ser2net_conn_staus
{
	SER2NET_NOT_CONNECT = -1,
	SER2NET_CONNECTING,
	SER2NET_CONNECTED,
	SER2NET_CONNECT_UNKNOWN,
};

int hlk_is_connected(int *sockfd)
{
	int ret;
	int len;
	struct tcp_info info;

	if (*sockfd <= 0)
	{
		debug_log("TCP Probe Connected SockFd <= 0");
		return -1;
	}

	len = sizeof(info);
	ret = getsockopt(*sockfd, IPPROTO_TCP, TCP_INFO, &info, (socklen_t *)&len);
	if (ret != 0)
	{
		debug_log("GetSockOpt TCP_INFO Fail ");
		return -1;
	}

	if ((TCP_SYN_RECV == info.tcpi_state) || (TCP_SYN_SENT == info.tcpi_state))
	{
		debug_log("TCP Connecting State:%d", info.tcpi_state);
		return 0;
	}
	else if (info.tcpi_state != TCP_ESTABLISHED)
	{
		close(*sockfd);
		*sockfd = 0;
		return -1;
	}


	return 1;
}

void *ser2network_client_thread(void *arg)
{
	uchar first_run = 1;
	int connected = SER2NET_NOT_CONNECT;
	uchar remote_closed = 0;
	int ret;

	uint rx_need_wait = 0;
	struct timeval rx_need_wait_start;

	int g_select_quit_fd = 0;
	int g_notify_socket_fd = 0;

	struct timeval tv;
	fd_set select_fds;

	int sockfd = 0, maxfd, maxi, i;
	struct sockaddr_in their_addr;

	ssize_t n; //!> read�ֽ���
	uchar buf[SERIAL_BUF_LEN];
	uint buf_serial_len = 0;

	struct hostent *he;

	led_set(0);

	ERROR_TIMES_RESET();
again1:
	if ((mkfifo(CLIENT_QUIT_PIPE_DEV, 0777) < 0) && (errno != EEXIST))
	{
	}

	g_select_quit_fd = open(CLIENT_QUIT_PIPE_DEV, O_NONBLOCK | O_RDWR);
	if (g_select_quit_fd <= 0)
	{
		ERROR_TIMES_AGAIN();
		usleep(50 * 1000);
		goto again1;
	}

	ERROR_TIMES_RESET();
again2:
	if ((mkfifo(NOTIFY_SOCKET_PIPE_DEV, 0777) < 0) && (errno != EEXIST))
	{
	}

	g_notify_socket_fd = open(NOTIFY_SOCKET_PIPE_DEV, O_NONBLOCK | O_RDWR);
	if (g_notify_socket_fd <= 0)
	{
		ERROR_TIMES_AGAIN();
		usleep(50 * 1000);
		goto again2;
	}

	int g_notify_fd = 0;

	ERROR_TIMES_RESET();

again3:
	if ((mkfifo(NOTIFY_PIPE_DEV, 0777) < 0) && (errno != EEXIST))
	{
	}

	g_notify_fd = open(NOTIFY_PIPE_DEV, O_NONBLOCK | O_RDWR);
	if (g_notify_fd <= 0)
	{
		ERROR_TIMES_AGAIN();
		usleep(50 * 1000);
		goto again3;
	}

	uchar need_connect_now = 0;

	while (1)
	{
		int maxfd = 0;

		FD_ZERO(&select_fds);
		if (g_select_quit_fd > 0)
		{
			FD_SET(g_select_quit_fd, &select_fds);
			maxfd = MAX_(maxfd, g_select_quit_fd);
		}

		if (connected > SER2NET_CONNECTING)
		{ //TCP/UDP 已连接
			if (rx_need_wait)
			{
				uint serial_time;
				struct timeval now;
				gettimeofday(&now, NULL);
				serial_time = timeval_subtract(&rx_need_wait_start, &now);
				if (serial_time >= rx_need_wait)
				{
					rx_need_wait = 0;
				}
				else
				{
					rx_need_wait = rx_need_wait - serial_time;
					TIM_DEBUG("in:%s(%d)(%d)\r\n", __FUNCTION__, __LINE__, rx_need_wait);
				}
			}

			if (0 == rx_need_wait)
			{
				maxfd = MAX_(maxfd, sockfd);
				FD_SET(sockfd, &select_fds);

				if (g_notify_fd > 0)
				{
					FD_SET(g_notify_fd, &select_fds);
					maxfd = MAX_(maxfd, g_notify_fd);
				}
			}
		}

		if (g_notify_socket_fd > 0)
		{
			FD_SET(g_notify_socket_fd, &select_fds);
			maxfd = MAX_(maxfd, g_notify_socket_fd);
		}

		if ((connected <= SER2NET_CONNECTING) && ((g_tcp_auto_connec) || (0 == remote_closed)))
		{
			if ((first_run == 1) || (need_connect_now))
			{
				first_run = 0;
				tv.tv_sec = 0;
				tv.tv_usec = 1;
				need_connect_now = 0;
			}
			else
			{
				tv.tv_sec = 0;
				tv.tv_usec = 500 * 1000;
			}
			ret = select(maxfd + 1, &select_fds, NULL, NULL, &tv);
		}
		else
		{
			if (rx_need_wait)
			{
				tv.tv_sec = rx_need_wait / 1000000;
				tv.tv_usec = rx_need_wait % 1000000;
				ret = select(maxfd + 1, &select_fds, NULL, NULL, &tv);
			}
			else
			{				
				tv.tv_sec = 1; 
				tv.tv_usec = 0;

				ret = select(maxfd + 1, &select_fds, NULL, NULL, &tv);
			}
		}

		if (ret > 0)
		{
			if (connected < SER2NET_CONNECTING)
			{
				need_connect_now = 1;
			}
			if (FD_ISSET(g_notify_socket_fd, &select_fds))
			{
				uint x;
				x = read(g_notify_socket_fd, buf, sizeof(buf));

				uchar buf_serial[socket_buf_LEN];
				uint buf_serial_len = 0;

				if (connected < SER2NET_CONNECTING)
				{
					remote_closed = 0;
					continue;
				}

				socket_buf_tx_(buf_serial, &buf_serial_len);
				statistics_serial_tx2 += buf_serial_len;

				if (1 == g_network_protocol)
				{
					int sended = fd_write(sockfd, buf_serial, buf_serial_len); //!> ��������д��ȥ
					if (sended < buf_serial_len)
					{
						TIM_WARN("warn:%s(%d) %d %d\r\n", __FUNCTION__, __LINE__, buf_serial_len, sended);
					}

					TIM_DEBUG(":%s(%d) %d %d\r\n", __FUNCTION__, __LINE__, buf_serial_len, sended);
				}
				else if (2 == g_network_protocol)
				{
					if (udp_toAddr_p)
					{
						if (buf_serial_len)
							if (sendto(sockfd, buf_serial, buf_serial_len, 0, (struct sockaddr *)udp_toAddr_p, sizeof(struct sockaddr)) != buf_serial_len)
							{
								close(sockfd);
								sockfd = 0;
								connected = SER2NET_NOT_CONNECT; //连接断开
								led_set(0);
								udp_toAddr_p = 0;
							}
					}
				}
			}

			if (0 == rx_need_wait)
				if (FD_ISSET(sockfd, &select_fds) || FD_ISSET(g_notify_fd, &select_fds))
				{
					uint lenx = serial_buf_empty_len();
					if (FD_ISSET(g_notify_fd, &select_fds))
					{
						lenx = 0;
						read(g_notify_fd, buf, sizeof(buf));
					}

					if (1 == g_network_protocol)
					{
						n = read(sockfd, buf, lenx);
					}
					else if (2 == g_network_protocol)
					{
						unsigned int addrLen;
						struct sockaddr_in udp_toAddr;

						addrLen = sizeof(struct sockaddr_in);
						if (lenx)
							n = recvfrom(sockfd, buf, lenx, 0, (struct sockaddr *)&udp_toAddr, &addrLen);
						else
							n = 0;
					}

					if (n < 0)
					{
						connected = SER2NET_NOT_CONNECT; //连接断开
						led_set(0);
						shutdown2(sockfd, SHUT_RDWR);
						close(sockfd);
						sockfd = 0;
						continue;
					}
					else if (n == 0)
					{
						if (lenx)
						{
							remote_closed = 1;
							connected = SER2NET_NOT_CONNECT; //连接断开
							led_set(0);
							shutdown2(sockfd, SHUT_RDWR);
							close(sockfd);
							sockfd = 0;
							continue;
						}
						else if (FD_ISSET(g_notify_fd, &select_fds))
						{
							buf[0] = 0;
						}
						else
						{
							buf[0] = 0;
							//usleep(1000);
							rx_need_wait = get_serial_wait_time();
							gettimeofday(&rx_need_wait_start, NULL);
						}
					}

					serial_buf_send(buf, n);
					tim_all_len2 += n;
				}

			if (FD_ISSET(g_select_quit_fd, &select_fds))
			{
				read(g_select_quit_fd, buf, sizeof(buf));
				break;
			}
		}
		else if (ret == 0)
		{
			if (connected == SER2NET_CONNECTING)
			{ 
				//TCP 处于正在连接中
				debug_log("NetMode 2 TCP Connecting To Server");
				connected = hlk_is_connected(&sockfd);
			}

			if ((connected < SER2NET_CONNECTING) && ((g_tcp_auto_connec) || (0 == remote_closed)))
			{
				if ((he = gethostbyname(g_remoteip_domain)) == NULL)
				{
					debug_log("remote domain:%s", g_remoteip_domain);
					goto out2;
				}

				if ((sockfd = socket(AF_INET, ((1 == g_network_protocol) ? SOCK_STREAM : (SOCK_DGRAM)), ((1 == g_network_protocol) ? 0 : (IPPROTO_UDP)))) == -1)
				{
					goto out2;
				}

				if (g_client_locale_port)
				{

					{
						int on = 1;
						ret = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
					}

					if (1 == g_network_protocol)
					{
						struct linger linger;
						linger.l_onoff = 1;
						linger.l_linger = 0;
						setsockopt(sockfd, SOL_SOCKET, SO_LINGER, (const char *)&linger, sizeof(linger));
					}

					struct sockaddr_in servaddr;
					bzero(&servaddr, sizeof(servaddr));
					servaddr.sin_family = AF_INET;
					servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
					servaddr.sin_port = htons(g_client_locale_port);

					if (bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1)
					{
						close(sockfd);
						led_set(0);
						error_check_1s();

						sockfd = 0;
						goto out2;
					}
				}

				bzero(&their_addr, sizeof(their_addr));
				their_addr.sin_family = AF_INET;
				their_addr.sin_addr = *((struct in_addr *)he->h_addr);
				their_addr.sin_port = htons(g_locale_remote_port);

				fcntl(sockfd, F_SETFL, O_NONBLOCK);

				if (1 == g_network_protocol)
				{

					{
						int nRecvBuf = 1024 * 4; 
						setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, (const char *)&nRecvBuf, sizeof(int));
						int nSendBuf = 1024 * 4;
						setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, (const char *)&nSendBuf, sizeof(int));
					}
					if (0)
					{
						struct timeval timeout;
						timeout.tv_sec = 1;
						timeout.tv_usec = 0;
						setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (const char *)&timeout, sizeof(int));
						setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout, sizeof(int));
					}

#if 1 //test
					if (g_tcp_client_stat_check)
					{
						{
							int s = sockfd;
							int optval;
							socklen_t optlen = sizeof(optval);

							/* Check the status for the keepalive option */
							if (getsockopt(s, SOL_SOCKET, SO_KEEPALIVE, &optval, &optlen) < 0)
							{
								perror("getsockopt()");
								close(s);
								exit(EXIT_FAILURE);
							}
							printf("SO_KEEPALIVE is %s\n", (optval ? "ON" : "OFF"));

							/* Set the option active */
							optval = 1;
							optlen = sizeof(optval);
							if (setsockopt(s, SOL_SOCKET, SO_KEEPALIVE, &optval, optlen) < 0)
							{
								perror("setsockopt()");
								close(s);
								exit(EXIT_FAILURE);
							}
							printf("SO_KEEPALIVE set on socket\n");

							/* Check the status again */
							if (getsockopt(s, SOL_SOCKET, SO_KEEPALIVE, &optval, &optlen) < 0)
							{
								perror("getsockopt()");
								close(s);
								exit(EXIT_FAILURE);
							}
							printf("SO_KEEPALIVE is %s\n", (optval ? "ON" : "OFF"));
						}

						{
							int s = sockfd;
							int optval;
							socklen_t optlen = sizeof(optval);

							optval = 1;
							optlen = sizeof(optval);
							if (setsockopt(s, SOL_SOCKET, SO_KEEPALIVE, &optval, optlen) < 0)
							{
							}

#define SOL_TCP 6

							optval = 5;
							optlen = sizeof(optval);
							if (setsockopt(s, SOL_TCP, TCP_KEEPIDLE, &optval, optlen) < 0)
							{
							}
							optval = 3;
							optlen = sizeof(optval);
							if (setsockopt(s, SOL_TCP, TCP_KEEPINTVL, &optval, optlen) < 0)
							{
							}
							optval = 3;
							optlen = sizeof(optval);
							if (setsockopt(s, SOL_TCP, TCP_KEEPCNT, &optval, optlen) < 0)
							{
							}
						}
					}

#if 1
					if (0 != g_tcp_realtime)
					{
						int s = sockfd;
						int optval;
						socklen_t optlen = sizeof(optval);

						optval = 1;
						optlen = sizeof(optval);
						if (setsockopt(s, IPPROTO_TCP, TCP_NODELAY, &optval, optlen) < 0)
						{
						}
					}
#endif

#endif
				}
				else if (2 == g_network_protocol)
				{
					int yes = 1;
					setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &yes, sizeof(yes));
				}

				if (1 == g_network_protocol)
				{

					if (connect(sockfd, (struct sockaddr *)&their_addr, sizeof(struct sockaddr)) == -1)
					{
						if (errno == EINPROGRESS) // it is in the connect process
						{
							struct timeval tv;
							fd_set writefds;
							tv.tv_sec = 0;
							{
								uchar *p;
								p = nvram_get(SER2NET_NVRAM, "tcpsyn_time");
								if ((p == NULL) || (*p == 0))
								{
									tv.tv_usec = 1000 * 1000;
								}
								else
								{
									tv.tv_usec = atoi(p) * 1000;
								}
							}
							FD_ZERO(&writefds);
							FD_SET(sockfd, &writefds);
							if (select(sockfd + 1, NULL, &writefds, NULL, &tv) > 0)
							{
								int error;
								int len = sizeof(int);
								getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len);
								if (error == 0)
									ret = 0;
								else
									ret = 1;
								debug_log("SO_ERROR res:%d", error);
							}
							else
								ret = 1; //timeout or error happen
						} else {
							ret = 1;
						}
					}
					else
						ret = 0;
				}
				else if (2 == g_network_protocol)
				{
					bzero(&toAddr, sizeof(toAddr));
					toAddr.sin_family = AF_INET;
					toAddr.sin_addr = *((struct in_addr *)he->h_addr);
					toAddr.sin_port = htons(g_locale_remote_port);
					udp_toAddr_p = &toAddr;
					ret = 0;
				}

				if (ret)
				{
					close(sockfd);
					connected = SER2NET_NOT_CONNECT;
					led_set(0);
				}
				else
				{
					{
						int flags;
						flags = fcntl(sockfd, F_GETFL, 0);
						fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
					}
					if (1 == g_network_protocol)
					{
						debug_log("NetMode 2 TCP Connect To Server: ");
						connected = SER2NET_CONNECTING; //仍然需要TCP 连接仍需确认是否到达 ESTABLISH状态
					}
					else if (2 == g_network_protocol)
					{
						debug_log("NetMode 2 UDP Connecting To Server");
						connected = SER2NET_CONNECTED; //UDP 连接状态直接变为已连接
					}
					led_set(1);
					remote_closed = 0;
					ser2network_notify_socket();
				}
			out2:
				continue;
			}
			else
			{

				if (rx_need_wait)
				{
				}
			}
		}
		else
		{
			TIM_ERROR("ERROR:%s(%d)\r\n", __FUNCTION__, __LINE__);
			static uchar x = 0;
			static uint lasttime = 0;
			if (lasttime)
			{
				struct timeval tv;
				gettimeofday(&tv, NULL);
				if (lasttime == tv.tv_sec)
				{
					x++;
					if (x > 5)
					{
						TIM_ERROR("ERROR:%s(%d)reboot\r\n", __FUNCTION__, __LINE__);
						restart_app();

						while (1)
							sleep(30);
					}
				}
				else
				{
					x = 0;
				}
				lasttime = tv.tv_sec;
			}
			else
			{
				struct timeval tv;
				gettimeofday(&tv, NULL);
				lasttime = tv.tv_sec;
			}
		}
	}

out:
	if (sockfd)
	{
		shutdown2(sockfd, SHUT_RDWR);
		close(sockfd);
	}

	led_set(0);

	if (g_select_quit_fd)
		close(g_select_quit_fd);
	if (g_notify_socket_fd)
		close(g_notify_socket_fd);

	if (g_notify_fd)
		close(g_notify_fd);

	pthread_detach(pthread_self());
	return ((void *)0);
}

#define SER2NET_CFG_DOMAIN "/tmp/SER2NET_CFG_HLK"

uchar Need_config_do = 0;

uchar config_do();

uchar config_listen_handle()
{
	fd_set select_fds;

	int listen_fd;
	struct sockaddr_un clt_addr;
	struct sockaddr_un srv_addr;

	int ret;
	int i;
	int len;

	uchar buf_in[SERIAL_BUF_LEN];
	uchar buf_out[SERIAL_BUF_LEN];

	int n;

	int sockfd;

	listen_fd = socket(PF_UNIX, SOCK_STREAM, 0);
	if (listen_fd < 0)
	{
		TIM_ERROR("Create socket Error : %d\n", errno);
		TIM_ERROR("ERROR:%s(%d)\r\n", __FUNCTION__, __LINE__);
		return -1;
	}

	bzero(&srv_addr, sizeof(srv_addr));
	srv_addr.sun_family = AF_UNIX;
	strncpy(srv_addr.sun_path, SER2NET_CFG_DOMAIN, sizeof(srv_addr.sun_path) - 1);
	unlink(SER2NET_CFG_DOMAIN);

	ret = bind(listen_fd, (struct sockaddr *)&srv_addr, sizeof(srv_addr));
	if (ret == -1)
	{
		close(listen_fd);
		TIM_ERROR("BindError : %d(%s)\n", errno, strerror(errno));
		TIM_ERROR("ERROR:%s(%d)\r\n", __FUNCTION__, __LINE__);
		return -1;
	}

	ret = listen(listen_fd, 5);

	if (ret)
	{
		TIM_ERROR("ERROR:%s(%d)\r\n", __FUNCTION__, __LINE__);
		close(listen_fd);
		unlink(SER2NET_CFG_DOMAIN);
		return 1;
	}

	while (1)
	{

		if (Need_config_do)
		{
			//system("killall hser2net");
			//if (config_do())
				Need_config_do = 0;
		}

		FD_ZERO(&select_fds);
		FD_SET(listen_fd, &select_fds);

		if (Need_config_do)
		{

			struct timeval tv;
			tv.tv_sec = 0;
			tv.tv_usec = 1000;
			ret = select(listen_fd + 1, &select_fds, NULL, NULL, &tv);
		}
		else
		{

			ret = select(listen_fd + 1, &select_fds, NULL, NULL, NULL);
		}


		if (ret > 0)
		{

			if (FD_ISSET(listen_fd, &select_fds))
			{

				sockfd = accept(listen_fd, (struct sockaddr *)&clt_addr, &len);
				if (sockfd < 0)
				{
					TIM_ERROR("LastError : %d(%s)\n", errno, strerror(errno));
					TIM_ERROR("ERROR:%s(%d)\r\n", __FUNCTION__, __LINE__);
					error_check_1s_2();
					sleep(1);
					continue;
				}
				{
					struct timeval timeout;
					timeout.tv_sec = 1;
					timeout.tv_usec = 0;
					setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (const char *)&timeout, sizeof(int));
					setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout, sizeof(int));
				}

				usleep(10 * 1000);
				n = read(sockfd, buf_in, SERIAL_BUF_LEN - 1);

				if (n < 0)
				{
					TIM_ERROR("ERROR:%s(%d)\r\n", __FUNCTION__, __LINE__);
					close(sockfd);
					continue;
				}
				if (n == 0)
				{
					TIM_ERROR("ERROR:%s(%d)\r\n", __FUNCTION__, __LINE__);
					close(sockfd);
					continue;
				}

				buf_in[n] = 0;
				TIM_DEBUG("Server Recv: %s\r\n", buf_in);
				{
					pthread_mutex_lock(&g_mutex);
					is_udp_at_cmd = 1;
					at(buf_in, buf_out);
					is_udp_at_cmd = 0;
					pthread_mutex_unlock(&g_mutex);
				}
				buf_out[sizeof(buf_out) - 1] = 0;

				hlk_check2();
				TIM_DEBUG("Server send : %s\r\n", buf_out);
				fd_write(sockfd, buf_out, strlen(buf_out) + 1); //!> ��������д��ȥ

				//udp 支持 saveapply命令返回
				//延时执行函数
				shutdown2(sockfd, SHUT_RDWR);
				close(sockfd);
				if (delay_network_restart) {
					debug_log("Delay Network Restart");
					system("uci commit;/etc/init.d/network restart");
					delay_network_restart = 0;
					//system("killall hser2net");
				}
			}
		}
		else if (ret == 0)
		{
			TIM_ERROR("ERROR:%s(%d)\r\n", __FUNCTION__, __LINE__);
		}
		else
		{
			if (errno == EINTR)
			{
			}
			else
			{
				TIM_ERROR("ERROR:%s(%d)\r\n", __FUNCTION__, __LINE__);

				TIM_ERROR("gethostbyname : %d(%s)\n", errno, strerror(errno));
			}
		}
	}

	close(listen_fd);
	unlink(SER2NET_CFG_DOMAIN);
	return 0;
}

uchar *at_set_value_(uchar *key, const uchar *value)
{
	uint i;
	if (0 == value)
		return key;
	i = strlen(value);
	if (i < 64)
	{
		if (key)
			free(key);
		key = malloc(i + 1);
		sprintf(key, value);
	}
	return key;
}

uchar config_all_get_nvram()
{
	int i ;
	const uchar *p;
	char *endptr = NULL;


	p = nvram_get(SER2NET_NVRAM, TTT_AT_DisName);
	if ((0 != p) && (0 != *p))
		g_AT_discovery_udp_name = at_set_value_(g_AT_discovery_udp_name, p);
	else
		g_AT_discovery_udp_name = at_set_value_(g_AT_discovery_udp_name, "HLK-RM60");
	
	debug_log("====== g_AT_discovery_udp_name:%s", g_AT_discovery_udp_name);


	//加载wifi6 配置
	int MbssMaxStaNum = 48;
	if (NULL == wifi6_conf ) {
		wifi6_conf = load_wifi6_profile("/etc/wireless/mediatek/mt7915.dbdc.b1.dat");
		if (wifi6_conf != NULL) {
			for (i = 0; i < wifi6_keyval_cnt; i++) {
				if (!strcmp(wifi6_conf[i].key, "MbssMaxStaNum") ) {
					break;
				}
			}
		}
	}

	//判断是否有 MbssMaxStaNum 参数 没有 默认配置是多少
	//怎么建索引??? 无法建索引

	if (i < wifi6_keyval_cnt) {
		//keyval_conf[i].val
		MbssMaxStaNum = strtol(wifi6_conf[i].val, &endptr, 10);
	} else {
		//配置文件中没有配置， 使用默认参数？
		MbssMaxStaNum = 16;//default;
	}
	//立即生效
	{
		char cmd[128] = {0};
		sprintf(cmd, "iwpriv rax0 set MbssMaxStaNum=%d", MbssMaxStaNum);
		system(cmd);
	}


#if 0
	p = nvram_get(SER2NET_NVRAM, "AT_netmode");
	if (p)
		g_AT_netmode = atoi(p);

	p = nvram_get(SER2NET_NVRAM, "AT_wifi");
	if (p)
		g_AT_wifi_conf = at_set_value_(g_AT_wifi_conf, p);


	p = nvram_get(SER2NET_NVRAM, "AT_dhcpd");
	if (p)
		g_AT_dhcpd = atoi(p);

	p = nvram_get(SER2NET_NVRAM, "AT_dhcpd_ip");
	if (p)
		g_AT_dhcpd_ip = at_set_value_(g_AT_dhcpd_ip, p);

	p = nvram_get(SER2NET_NVRAM, "AT_dhcpd_dns");
	if (p)
		g_AT_dhcpd_dns = at_set_value_(g_AT_dhcpd_dns, p);

	p = nvram_get(SER2NET_NVRAM, "AT_dhcpd_time");
	if ((0 != p) && (0 != *p))
	{
		g_AT_dhcpd_time = atoi(p);
	}

	p = nvram_get(SER2NET_NVRAM, "AT_dhcpc");
	if (p)
		g_AT_dhcpc = atoi(p);

	p = nvram_get(SER2NET_NVRAM, "AT_net_ip");
	if (p)
		g_AT_net_ip = at_set_value_(g_AT_net_ip, p);

	p = nvram_get(SER2NET_NVRAM, "AT_net_dns");
	if (p)
		g_AT_net_dns = at_set_value_(g_AT_net_dns, p);

	p = nvram_get(SER2NET_NVRAM, "AT_pm");
	if (p)
		g_AT_pm = atoi(p);
#endif

	hlk_check1();

	p = nvram_get(SER2NET_NVRAM, TTT_ser2net_ser);
	if ((0 == p) || (0 == *p))
	{
		g_serial_config = at_set_value_(g_serial_config, "115200,8,n,1");
		g_remoteip_domain = at_set_value_(g_remoteip_domain, "192.168.11.245");
	} else {
		g_serial_config = at_set_value_(g_serial_config, p);
	}

	debug_log("====== g_serial_config:%s", g_serial_config);

	p = nvram_get(SER2NET_NVRAM, TTT_ser2net_mode);
	if (p)
		g_network_mode = atoi(p);
	debug_log("====== g_network_mode:%d", g_network_mode);

	p = nvram_get(SER2NET_NVRAM, TTT_ser2net_ipdomain);
	if (p)
		g_remoteip_domain = at_set_value_(g_remoteip_domain, p);
	else
		g_remoteip_domain = at_set_value_(g_remoteip_domain, "192.168.11.245");
	debug_log("====== g_remoteip_domain:%s", g_remoteip_domain);

	p = nvram_get(SER2NET_NVRAM, TTT_ser2net_SRemoteIpPort);
	if (p && (*p))
		g_SRemoteIpPort = at_set_value_(g_SRemoteIpPort, p);
	else
		g_SRemoteIpPort = at_set_value_(g_SRemoteIpPort, "");
	debug_log("====== g_SRemoteIpPort:%s", g_SRemoteIpPort);

	p = nvram_get(SER2NET_NVRAM, TTT_ser2net_port);
	if (p)
		g_locale_remote_port = atoi(p);
	debug_log("====== g_locale_remote_port:%d", g_locale_remote_port);

	p = nvram_get(SER2NET_NVRAM, TTT_ser2net_localeport);
	if (p && *p)
		g_client_locale_port = atoi(p);
	debug_log("====== g_client_locale_port:%d", g_client_locale_port);

	p = nvram_get(SER2NET_NVRAM, TTT_ser2net_SMaxConn);
	if (p && *p)
		g_SMaxConn = atoi(p);
	debug_log("====== g_SMaxConn:%d", g_SMaxConn);

	p = nvram_get(SER2NET_NVRAM, TTT_ser2net_TCPRT);
	if (p && *p)
		g_tcp_realtime = atoi(p);
	debug_log("====== g_tcp_realtime:%d", g_tcp_realtime);

	p = nvram_get(SER2NET_NVRAM, TTT_ser2net_proto);
	if (p)
		g_network_protocol = atoi(p);
	debug_log("====== g_network_protocol:%d", g_network_protocol);

	p = nvram_get(SER2NET_NVRAM, TTT_ser2net_time_out);
	if (p)
		g_network_timeout = atoi(p);
	debug_log("====== g_network_timeout:%d", g_network_timeout);

	p = nvram_get(SER2NET_NVRAM, TTT_ser2net_tcp_autoc);
	if (p && (*p != 0x0))
	{
		g_tcp_auto_connec = atoi(p);
	}
	debug_log("====== g_tcp_auto_connec:%d", g_tcp_auto_connec);

	p = nvram_get(SER2NET_NVRAM, TTT_ser2net_tcp_c_check);
	if (p && (*p != 0x0))
	{
		g_tcp_client_stat_check = atoi(p);
	}
	debug_log("====== g_tcp_client_stat_check:%d", g_tcp_client_stat_check);

	p = nvram_get(SER2NET_NVRAM, TTT_ser2net_ser_pack_len);
	if (p)
		g_serial_package_len = atoi(p);
	debug_log("====== g_serial_package_len:%d", g_serial_package_len);

	hlk_check1();

	p = nvram_get(SER2NET_NVRAM, TTT_ser2net_ser_pack_time);
	if (p)
		g_serial_package_time = atoi(p);
	debug_log("====== g_serial_package_time:%d", g_serial_package_time);

	p = nvram_get(SER2NET_NVRAM, TTT_ser2net_ser_pack_inter_time);
	if (p && (*p))
		g_serial_package_interval_time = atoi(p);
	debug_log("====== g_serial_package_interval_time:%d", g_serial_package_interval_time);

	p = nvram_get(SER2NET_NVRAM, TTT_ser2net_escape_en);
	if (p && (*p != 0x0))
		g_transparent_escape_en = atoi(p);
	debug_log("====== g_transparent_escape_en:%d", g_transparent_escape_en);

	p = nvram_get(SER2NET_NVRAM, TTT_ser2net_escape2_en);
	if (p && (*p != 0x0))
		g_transparent_escape2_en = atoi(p);
	debug_log("====== g_transparent_escape2_en:%d", g_transparent_escape2_en);

	p = nvram_get(SER2NET_NVRAM, TTT_ser2net_escape2_time);
	if (p && (*p != 0x0))
		g_transparent_escape2_time = atoi(p);
	debug_log("====== g_transparent_escape2_time:%d", g_transparent_escape2_time);

	p = nvram_get(SER2NET_NVRAM, TTT_ser2net_RTS);
	if (p && (*p != 0x0))
		FLOW_CONTROL_ = atoi(p);
	debug_log("====== FLOW_CONTROL_:%d", FLOW_CONTROL_);

	p = nvram_get(SER2NET_NVRAM, TTT_ser2net_XON);
	if (p && (*p != 0x0))
		XON_XOFF = atoi(p);
	debug_log("====== XON_XOFF:%d", XON_XOFF);

	return 0;
}

int nvram_set_(int index, char *name, char *value)
{
	nvram_set(index, name, value);
	return 0;
}

char B16ToB10(const char *pszNum)
{
	int r = 0;
	if (pszNum)
		sscanf(pszNum, "%X", &r);
	return r;
}

void *config_all_save_nvram_(void *x)
{
	uchar buf[512] = {0};

	nvram_close(SER2NET_NVRAM);
	nvram_init(SER2NET_NVRAM);

	nvram_set_(SER2NET_NVRAM, TTT_ser2net_ser, g_serial_config);
	memset(buf, 0, sizeof(buf));
	sprintf(buf, "%d", g_network_mode);
	nvram_set_(SER2NET_NVRAM, TTT_ser2net_mode, buf);

	nvram_set_(SER2NET_NVRAM, TTT_ser2net_ipdomain, g_remoteip_domain);
	memset(buf, 0, sizeof(buf));
	sprintf(buf, "%d", g_locale_remote_port);
	nvram_set_(SER2NET_NVRAM, TTT_ser2net_port, buf);
	memset(buf, 0, sizeof(buf));
	sprintf(buf, "%d", g_client_locale_port);
	nvram_set_(SER2NET_NVRAM, TTT_ser2net_localeport, buf);
	memset(buf, 0, sizeof(buf));
	sprintf(buf, "%d", g_network_protocol);
	nvram_set_(SER2NET_NVRAM, TTT_ser2net_proto, buf);
	memset(buf, 0, sizeof(buf));
	sprintf(buf, "%d", g_network_timeout);
	nvram_set_(SER2NET_NVRAM, TTT_ser2net_time_out, buf);
	memset(buf, 0, sizeof(buf));
	sprintf(buf, "%d", g_tcp_auto_connec);
	nvram_set_(SER2NET_NVRAM, TTT_ser2net_tcp_autoc, buf);
	memset(buf, 0, sizeof(buf));
	sprintf(buf, "%d", g_serial_package_len);
	nvram_set_(SER2NET_NVRAM, TTT_ser2net_ser_pack_len, buf);
	memset(buf, 0, sizeof(buf));
	sprintf(buf, "%d", g_serial_package_time);
	nvram_set_(SER2NET_NVRAM, TTT_ser2net_ser_pack_time, buf);
	memset(buf, 0, sizeof(buf));
	sprintf(buf, "%d", g_serial_package_interval_time);
	nvram_set_(SER2NET_NVRAM, TTT_ser2net_ser_pack_inter_time, buf);
	memset(buf, 0, sizeof(buf));
	sprintf(buf, "%d", g_transparent_escape_en);
	nvram_set_(SER2NET_NVRAM, TTT_ser2net_escape_en, buf);
	memset(buf, 0, sizeof(buf));
	sprintf(buf, "%d", g_transparent_escape2_en);
	nvram_set_(SER2NET_NVRAM, TTT_ser2net_escape2_en, buf);
	memset(buf, 0, sizeof(buf));
	sprintf(buf, "%d", g_transparent_escape2_time);
	nvram_set_(SER2NET_NVRAM, TTT_ser2net_escape2_time, buf);
	memset(buf, 0, sizeof(buf));
	sprintf(buf, "%d", FLOW_CONTROL_);
	nvram_set_(SER2NET_NVRAM, TTT_ser2net_RTS, buf);
	memset(buf, 0, sizeof(buf));
	sprintf(buf, "%d", XON_XOFF);
	nvram_set_(SER2NET_NVRAM, TTT_ser2net_XON, buf);

	nvram_set_(SER2NET_NVRAM, TTT_AT_DisName, g_AT_discovery_udp_name);

	nvram_set_(SER2NET_NVRAM, TTT_ser2net_SRemoteIpPort, g_SRemoteIpPort);

	memset(buf, 0, sizeof(buf));
	sprintf(buf, "%d", g_SMaxConn);
	nvram_set_(SER2NET_NVRAM, TTT_ser2net_SMaxConn, buf);

	memset(buf, 0, sizeof(buf));
	sprintf(buf, "%d", g_tcp_realtime);
	nvram_set_(SER2NET_NVRAM, TTT_ser2net_TCPRT, buf);

	memset(buf, 0, sizeof(buf));
	sprintf(buf, "%d", g_tcp_client_stat_check);
	nvram_set_(SER2NET_NVRAM, TTT_ser2net_tcp_c_check, buf);

	nvram_commit(SER2NET_NVRAM);
	system("sync");

	hlk_check1();
	pthread_detach(pthread_self());

	Need_config_do = 1;

	return ((void *)0);
}

uchar *p2penable = 0;
uchar *p2p1 = 0;
uchar *p2p2 = 0;
uchar *p2p3 = 0;
uchar *p2p4 = 0;
uchar *p2p5 = 0;

uchar *Language = 0;

uchar config_all_save_nvram_2()
{
	uchar buf[32];

	p2penable = at_set_value_(p2penable, nvram_get(SER2NET_NVRAM, "p2penable"));
	p2p1 = at_set_value_(p2p1, nvram_get(SER2NET_NVRAM, "p2p1"));
	p2p2 = at_set_value_(p2p2, nvram_get(SER2NET_NVRAM, "p2p2"));
	p2p3 = at_set_value_(p2p3, nvram_get(SER2NET_NVRAM, "p2p3"));
	p2p4 = at_set_value_(p2p4, nvram_get(SER2NET_NVRAM, "p2p4"));
	p2p5 = at_set_value_(p2p5, nvram_get(SER2NET_NVRAM, "p2p5"));
	Language = at_set_value_(Language, nvram_get(SER2NET_NVRAM, "Language"));

#if 1
	{
		static uchar last_mode = 0;

		TIM_DEBUG("in:%s(%d)  %d\r\n", __FUNCTION__, __LINE__, g_AT_netmode);

		if (last_mode != g_AT_netmode)
		{
			switch (g_AT_netmode)
			{
			case 0:
				system("ralink_init clear 2860;ralink_init renew 2860 /etc_ro/Wireless/RT2860AP/RT2860_uart_default");
				break;
			case 1:
				system("ralink_init clear 2860;ralink_init renew 2860 /etc_ro/Wireless/RT2860AP/RT2860_uart_eth");
				break;
			case 2:
				system("ralink_init clear 2860;ralink_init renew 2860 /etc_ro/Wireless/RT2860AP/RT2860_uart_client");
				break;
			case 3:
				system("ralink_init clear 2860;ralink_init renew 2860 /etc_ro/Wireless/RT2860AP/RT2860_uart_ap");
				break;
			case 4:
				system("ralink_init clear 2860;ralink_init renew 2860 /etc_ro/Wireless/RT2860AP/RT2860_uart_3g");
				break;
			case 5:
				break;
			default:
				break;
			}
			last_mode = g_AT_netmode;
		}
	}
#endif

	TIM_DEBUG("in:%s(%d)  %d\r\n", __FUNCTION__, __LINE__, g_AT_netmode);

	nvram_close(SER2NET_NVRAM);

	nvram_init(SER2NET_NVRAM);

	TIM_DEBUG("in:%s(%d)  %d\r\n", __FUNCTION__, __LINE__, g_AT_netmode);

	nvram_set_(SER2NET_NVRAM, TTT_ser2net_ser, g_serial_config);
	sprintf(buf, "%d", g_network_mode);
	nvram_set_(SER2NET_NVRAM, TTT_ser2net_mode, buf);

	nvram_set_(SER2NET_NVRAM, TTT_ser2net_ipdomain, g_remoteip_domain);
	sprintf(buf, "%d", g_locale_remote_port);
	nvram_set_(SER2NET_NVRAM, TTT_ser2net_port, buf);
	sprintf(buf, "%d", g_client_locale_port);
	nvram_set_(SER2NET_NVRAM, TTT_ser2net_localeport, buf);
	sprintf(buf, "%d", g_network_protocol);
	nvram_set_(SER2NET_NVRAM, TTT_ser2net_proto, buf);
	sprintf(buf, "%d", g_network_timeout);
	nvram_set_(SER2NET_NVRAM, TTT_ser2net_time_out, buf);
	sprintf(buf, "%d", g_tcp_auto_connec);
	nvram_set_(SER2NET_NVRAM, TTT_ser2net_tcp_autoc, buf);
	sprintf(buf, "%d", g_serial_package_len);
	nvram_set_(SER2NET_NVRAM, TTT_ser2net_ser_pack_len, buf);
	sprintf(buf, "%d", g_serial_package_time);
	nvram_set_(SER2NET_NVRAM, TTT_ser2net_ser_pack_time, buf);
	sprintf(buf, "%d", g_serial_package_interval_time);
	nvram_set_(SER2NET_NVRAM, TTT_ser2net_ser_pack_inter_time, buf);
	sprintf(buf, "%d", g_transparent_escape_en);
	nvram_set_(SER2NET_NVRAM, TTT_ser2net_escape_en, buf);
	sprintf(buf, "%d", g_transparent_escape2_en);
	nvram_set_(SER2NET_NVRAM, TTT_ser2net_escape2_en, buf);
	sprintf(buf, "%d", g_transparent_escape2_time);
	nvram_set_(SER2NET_NVRAM, TTT_ser2net_escape2_time, buf);
	sprintf(buf, "%d", FLOW_CONTROL_);
	nvram_set_(SER2NET_NVRAM, TTT_ser2net_RTS, buf);
	sprintf(buf, "%d", XON_XOFF);
	nvram_set_(SER2NET_NVRAM, TTT_ser2net_XON, buf);

	sprintf(buf, "%d", g_tcp_client_stat_check);
	nvram_set_(SER2NET_NVRAM, TTT_ser2net_tcp_c_check, buf);

#if 1

	sprintf(buf, "%d", g_AT_netmode);
	nvram_set_(SER2NET_NVRAM, "AT_netmode", buf);

	nvram_set_(SER2NET_NVRAM, "AT_wifi", g_AT_wifi_conf);
	sprintf(buf, "%d", g_AT_dhcpd);
	nvram_set_(SER2NET_NVRAM, "AT_dhcpd", buf);
	nvram_set_(SER2NET_NVRAM, "AT_dhcpd_ip", g_AT_dhcpd_ip);
	nvram_set_(SER2NET_NVRAM, "AT_dhcpd_dns", g_AT_dhcpd_dns);
	sprintf(buf, "%d", g_AT_dhcpd_time);
	nvram_set_(SER2NET_NVRAM, "AT_dhcpd_time", buf);
	TIM_DEBUG("in:%s(%d)(%d)\r\n", __FUNCTION__, __LINE__, g_AT_dhcpd_time);
	sprintf(buf, "%d", g_AT_dhcpc);
	nvram_set_(SER2NET_NVRAM, "AT_dhcpc", buf);
	nvram_set_(SER2NET_NVRAM, "AT_net_ip", g_AT_net_ip);
	nvram_set_(SER2NET_NVRAM, "AT_net_dns", g_AT_net_dns);

	sprintf(buf, "%d", g_AT_pm);
	nvram_set_(SER2NET_NVRAM, "AT_pm", buf);

	nvram_set_(SER2NET_NVRAM, TTT_AT_DisName, g_AT_discovery_udp_name);

	nvram_set_(SER2NET_NVRAM, TTT_ser2net_SRemoteIpPort, g_SRemoteIpPort);

	sprintf(buf, "%d", g_SMaxConn);
	nvram_set_(SER2NET_NVRAM, TTT_ser2net_SMaxConn, buf);

	sprintf(buf, "%d", g_tcp_realtime);
	nvram_set_(SER2NET_NVRAM, TTT_ser2net_TCPRT, buf);

	do
	{ //wifi conf
		uchar ssid[32];
		uchar keytype[32];
		uchar key[32];
		uchar *p1, *p2;

		p1 = g_AT_wifi_conf;

		p2 = strchr(p1, ',');
		if (0 == p2)
		{
			break;
		}
		snprintf(ssid, MIN_(p2 - p1 + 1, sizeof(ssid)), p1);

		p1 = p2 + 1;
		p2 = strchr(p1, ',');
		if (0 == p2)
		{
			break;
		}
		snprintf(keytype, MIN_(p2 - p1 + 1, sizeof(keytype)), p1);

		p1 = p2 + 1;
		snprintf(key, sizeof(key) - 1, p1);

		TIM_DEBUG("in:%s(%d) (%s)  (%s) (%s) (%s)\r\n", __FUNCTION__, __LINE__, g_AT_wifi_conf, ssid, keytype, key);

		if (3 == g_AT_netmode)
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
		else if (2 == g_AT_netmode)
		{
			nvram_set_(SER2NET_NVRAM, "staSSID", ssid);

			if (0 == strcmp(keytype, "none"))
			{

				nvram_set_(SER2NET_NVRAM, "staEncrypt", "1");
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

			}
			else if ((0 == strcmp(keytype, "wep_open")))
			{

				nvram_set_(SER2NET_NVRAM, "staAuth", "0");
				nvram_set_(SER2NET_NVRAM, "staEncrypt", "0");
				nvram_set_(SER2NET_NVRAM, "staWpaPsk", "0");
				nvram_set_(SER2NET_NVRAM, "staKey1Type", "1");
				nvram_set_(SER2NET_NVRAM, "staKey1", key);

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

		if ((3 == g_AT_netmode) || (1 == g_AT_netmode) || (0 == g_AT_netmode))
		{

			do
			{
				const char *p;
				char *p2;
				char *p3;
				char ssid[20];
				char i;
				unsigned char x, y;

				if (0 == strcmp(nvram_bufget(SER2NET_NVRAM, "SSID1"), "Hi-Link_"))
				{
					p = nvram_bufget(SER2NET_NVRAM, "WAN_MAC_ADDR");
					if (p)
					{
						p2 = p;
						for (i = 0; i < 4; i++)
						{
							p3 = strchr(p2, ':');
							if (0 == p3)
								break;
							p2 = p3 + 1;
						}
						if (p3)
						{
							x = B16ToB10(p2);
							p3 = strchr(p2, ':');
							if (0 == p3)
								break;
							p2 = p3 + 1;
							y = B16ToB10(p2);
							sprintf(ssid, "HI-LINK_%02X%02X", x, y);
							nvram_bufset(SER2NET_NVRAM, "SSID1", ssid);
							sprintf(g_AT_wifi_conf, "%s,%s,%s", ssid, keytype, key);

							nvram_bufset(SER2NET_NVRAM, "AT_wifi", g_AT_wifi_conf);

						}
					}
				}

			} while (0);
		}

	} while (0);

	if ((0 != g_AT_netmode) && (2 != g_AT_netmode))
	{

		if (1 == g_AT_dhcpd)
		{
			nvram_set_(SER2NET_NVRAM, "dhcpEnabled", "1");
		}
		else
		{
			nvram_set_(SER2NET_NVRAM, "dhcpEnabled", "0");
		}

		do
		{ //dhcpd ip
			uchar ip[32];
			uchar ip2[32];
			uchar mask[32];
			uchar gw[32];
			uchar *p1, *p2;

			p1 = g_AT_dhcpd_ip;

			p2 = strchr(p1, ',');
			if (0 == p2)
			{
				break;
			}
			snprintf(ip, MIN_(p2 - p1 + 1, sizeof(ip) - 1), p1);

			p1 = p2 + 1;
			p2 = strchr(p1, ',');
			if (0 == p2)
			{
				break;
			}
			snprintf(ip2, MIN_(p2 - p1 + 1, sizeof(ip2) - 1), p1);

			p1 = p2 + 1;
			p2 = strchr(p1, ',');
			if (0 == p2)
			{
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

		} while (0);

		do
		{ //dhcpd dns
			uchar dns1[32];
			uchar dns2[32];
			uchar *p1, *p2;

			p1 = g_AT_dhcpd_dns;

			p2 = strchr(p1, ',');
			if (0 == p2)
			{
				break;
			}
			snprintf(dns1, MIN_(p2 - p1 + 1, sizeof(dns1) - 1), p1);

			p1 = p2 + 1;
			snprintf(dns2, sizeof(dns2) - 1, p1);

			TIM_DEBUG("in:%s(%d) (%s) (%s)\r\n", __FUNCTION__, __LINE__, dns1, dns2);

			nvram_set_(SER2NET_NVRAM, "dhcpPriDns", dns1);
			nvram_set_(SER2NET_NVRAM, "dhcpSecDns", dns2);

		} while (0);

		if (0 == g_AT_dhcpd_time)
		{
			g_AT_dhcpd_time = 86400;
		}
		sprintf(buf, "%d", g_AT_dhcpd_time);
		nvram_set_(SER2NET_NVRAM, "dhcpLease", buf);
	}

	if (0 != g_AT_netmode)
	{
		if (1 == g_AT_dhcpc)
		{
			nvram_set_(SER2NET_NVRAM, "wanConnectionMode", "DHCP");
		}
		else
		{
			nvram_set_(SER2NET_NVRAM, "wanConnectionMode", "STATIC");
		}
	}
	do
	{ //net ip
		uchar ip[32];
		uchar mask[32];
		uchar gw[32];
		uchar *p1, *p2;

		p1 = g_AT_net_ip;

		p2 = strchr(p1, ',');
		if (0 == p2)
		{
			break;
		}
		snprintf(ip, MIN_(p2 - p1 + 1, sizeof(ip) - 1), p1);

		p1 = p2 + 1;
		p2 = strchr(p1, ',');
		if (0 == p2)
		{
			break;
		}
		snprintf(mask, MIN_(p2 - p1 + 1, sizeof(mask) - 1), p1);

		p1 = p2 + 1;
		snprintf(gw, sizeof(gw) - 1, p1);

		TIM_DEBUG("in:%s(%d) (%s) (%s) (%s)\r\n", __FUNCTION__, __LINE__, ip, mask, gw);

		if (3 == g_AT_netmode)
		{
			nvram_set_(SER2NET_NVRAM, "lan_ipaddr", ip);
			nvram_set_(SER2NET_NVRAM, "lan_netmask", mask);
			nvram_set_(SER2NET_NVRAM, "wan_gateway", gw);
		}
		else if (1 == g_AT_netmode)
		{
			nvram_set_(SER2NET_NVRAM, "wan_ipaddr", ip);
			nvram_set_(SER2NET_NVRAM, "wan_netmask", mask);
			nvram_set_(SER2NET_NVRAM, "wan_gateway", gw);
		}
		else if (2 == g_AT_netmode)
		{
			nvram_set_(SER2NET_NVRAM, "wan_ipaddr", ip);
			nvram_set_(SER2NET_NVRAM, "wan_netmask", mask);
			nvram_set_(SER2NET_NVRAM, "wan_gateway", gw);
		}

	} while (0);

	if (0 != g_AT_netmode)
		do
		{ //net dns
			uchar dns1[32];
			uchar dns2[32];
			uchar *p1, *p2;


			p1 = g_AT_net_dns;

			p2 = strchr(p1, ',');
			if (0 == p2)
			{
				break;
			}
			snprintf(dns1, MIN_(p2 - p1 + 1, sizeof(dns1) - 1), p1);

			p1 = p2 + 1;
			snprintf(dns2, sizeof(dns2) - 1, p1);

			TIM_DEBUG("in:%s(%d) (%s) (%s)\r\n", __FUNCTION__, __LINE__, dns1, dns2);

			nvram_set_(SER2NET_NVRAM, "wan_primary_dns", dns1);
			nvram_set_(SER2NET_NVRAM, "wan_secondary_dns", dns2);

		} while (0);

#endif
	nvram_commit(SER2NET_NVRAM);

	hlk_check1();
}

uchar config_all_save_nvram_3(uchar m)
{
	uchar buf[32];

	if (m)
	{
		Language = at_set_value_(Language, nvram_get(SER2NET_NVRAM, "Language"));

#if 1
		{
			static uchar last_mode = 0;

			TIM_DEBUG("in:%s(%d)  %d\r\n", __FUNCTION__, __LINE__, g_AT_netmode);

			if (last_mode != g_AT_netmode)
			{
				switch (g_AT_netmode)
				{
				case 0:
					break;
				case 1:
					system("ralink_init clear 2860;ralink_init renew 2860 /etc_ro/Wireless/RT2860AP/RT2860_uart_eth");
					break;
				case 2:
					system("ralink_init clear 2860;ralink_init renew 2860 /etc_ro/Wireless/RT2860AP/RT2860_uart_client");
					break;
				case 3:
					system("ralink_init clear 2860;ralink_init renew 2860 /etc_ro/Wireless/RT2860AP/RT2860_uart_ap");
					break;
				case 4:
					system("ralink_init clear 2860;ralink_init renew 2860 /etc_ro/Wireless/RT2860AP/RT2860_uart_3g");
					break;
				case 5:
					break;
				default:
					break;
				}
				//	sleep(1);
				last_mode = g_AT_netmode;
			}
		}
#endif

		TIM_DEBUG("in:%s(%d)  %d\r\n", __FUNCTION__, __LINE__, g_AT_netmode);

		nvram_close(SER2NET_NVRAM);

		nvram_init(SER2NET_NVRAM);

		if (Language)
			nvram_set_(SER2NET_NVRAM, "Language", Language);
	}

	TIM_DEBUG("in:%s(%d)  %d\r\n", __FUNCTION__, __LINE__, g_AT_netmode);

	nvram_set_(SER2NET_NVRAM, TTT_ser2net_ser, g_serial_config);
	sprintf(buf, "%d", g_network_mode);
	nvram_set_(SER2NET_NVRAM, TTT_ser2net_mode, buf);

	nvram_set_(SER2NET_NVRAM, TTT_ser2net_ipdomain, g_remoteip_domain);
	sprintf(buf, "%d", g_locale_remote_port);
	nvram_set_(SER2NET_NVRAM, TTT_ser2net_port, buf);
	sprintf(buf, "%d", g_client_locale_port);
	nvram_set_(SER2NET_NVRAM, TTT_ser2net_localeport, buf);
	sprintf(buf, "%d", g_network_protocol);
	nvram_set_(SER2NET_NVRAM, TTT_ser2net_proto, buf);
	sprintf(buf, "%d", g_network_timeout);
	nvram_set_(SER2NET_NVRAM, TTT_ser2net_time_out, buf);
	sprintf(buf, "%d", g_tcp_auto_connec);
	nvram_set_(SER2NET_NVRAM, TTT_ser2net_tcp_autoc, buf);
	sprintf(buf, "%d", g_serial_package_len);
	nvram_set_(SER2NET_NVRAM, TTT_ser2net_ser_pack_len, buf);
	sprintf(buf, "%d", g_serial_package_time);
	nvram_set_(SER2NET_NVRAM, TTT_ser2net_ser_pack_time, buf);
	sprintf(buf, "%d", g_serial_package_interval_time);
	nvram_set_(SER2NET_NVRAM, TTT_ser2net_ser_pack_inter_time, buf);
	sprintf(buf, "%d", g_transparent_escape_en);
	nvram_set_(SER2NET_NVRAM, TTT_ser2net_escape_en, buf);
	sprintf(buf, "%d", g_transparent_escape2_en);
	nvram_set_(SER2NET_NVRAM, TTT_ser2net_escape2_en, buf);
	sprintf(buf, "%d", g_transparent_escape2_time);
	nvram_set_(SER2NET_NVRAM, TTT_ser2net_escape2_time, buf);
	sprintf(buf, "%d", FLOW_CONTROL_);
	nvram_set_(SER2NET_NVRAM, TTT_ser2net_RTS, buf);
	sprintf(buf, "%d", XON_XOFF);
	nvram_set_(SER2NET_NVRAM, TTT_ser2net_XON, buf);

	sprintf(buf, "%d", g_tcp_client_stat_check);
	nvram_set_(SER2NET_NVRAM, TTT_ser2net_tcp_c_check, buf);

#if 1

	sprintf(buf, "%d", g_AT_netmode);
	nvram_set_(SER2NET_NVRAM, "AT_netmode", buf);

	TIM_DEBUG("in:%s(%d)  %d\r\n", __FUNCTION__, __LINE__, g_AT_netmode);
	TIM_DEBUG("in:%s(%d)  %s\r\n", __FUNCTION__, __LINE__, buf);

	nvram_set_(SER2NET_NVRAM, "AT_wifi", g_AT_wifi_conf);
	sprintf(buf, "%d", g_AT_dhcpd);
	nvram_set_(SER2NET_NVRAM, "AT_dhcpd", buf);
	nvram_set_(SER2NET_NVRAM, "AT_dhcpd_ip", g_AT_dhcpd_ip);
	nvram_set_(SER2NET_NVRAM, "AT_dhcpd_dns", g_AT_dhcpd_dns);
	sprintf(buf, "%d", g_AT_dhcpd_time);
	nvram_set_(SER2NET_NVRAM, "AT_dhcpd_time", buf);
	TIM_DEBUG("in:%s(%d)(%d)\r\n", __FUNCTION__, __LINE__, g_AT_dhcpd_time);
	sprintf(buf, "%d", g_AT_dhcpc);
	nvram_set_(SER2NET_NVRAM, "AT_dhcpc", buf);
	nvram_set_(SER2NET_NVRAM, "AT_net_ip", g_AT_net_ip);
	nvram_set_(SER2NET_NVRAM, "AT_net_dns", g_AT_net_dns);

	sprintf(buf, "%d", g_AT_pm);
	nvram_set_(SER2NET_NVRAM, "AT_pm", buf);

	nvram_set_(SER2NET_NVRAM, TTT_AT_DisName, g_AT_discovery_udp_name);

	nvram_set_(SER2NET_NVRAM, TTT_ser2net_SRemoteIpPort, g_SRemoteIpPort);

	sprintf(buf, "%d", g_SMaxConn);
	nvram_set_(SER2NET_NVRAM, TTT_ser2net_SMaxConn, buf);

	sprintf(buf, "%d", g_tcp_realtime);
	nvram_set_(SER2NET_NVRAM, TTT_ser2net_TCPRT, buf);

	do
	{ //wifi conf
		uchar ssid[32];
		uchar keytype[32];
		uchar key[32];
		uchar *p1, *p2;

		p1 = g_AT_wifi_conf;

		p2 = strchr(p1, ',');
		if (0 == p2)
		{
			break;
		}
		snprintf(ssid, MIN_(p2 - p1 + 1, sizeof(ssid)), p1);

		p1 = p2 + 1;
		p2 = strchr(p1, ',');
		if (0 == p2)
		{
			break;
		}
		snprintf(keytype, MIN_(p2 - p1 + 1, sizeof(keytype)), p1);

		p1 = p2 + 1;
		snprintf(key, sizeof(key) - 1, p1);

		TIM_DEBUG("in:%s(%d) (%s)  (%s) (%s) (%s)\r\n", __FUNCTION__, __LINE__, g_AT_wifi_conf, ssid, keytype, key);

		if (3 == g_AT_netmode)
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
		else if (2 == g_AT_netmode)
		{
			nvram_set_(SER2NET_NVRAM, "staSSID", ssid);

			if (0 == strcmp(keytype, "none"))
			{

				nvram_set_(SER2NET_NVRAM, "staEncrypt", "1");
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

			}
			else if ((0 == strcmp(keytype, "wep_open")))
			{

				nvram_set_(SER2NET_NVRAM, "staAuth", "0");
				nvram_set_(SER2NET_NVRAM, "staEncrypt", "0");
				nvram_set_(SER2NET_NVRAM, "staWpaPsk", "0");
				nvram_set_(SER2NET_NVRAM, "staKey1Type", "1");
				nvram_set_(SER2NET_NVRAM, "staKey1", key);

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

		{

			do
			{
				const char *p;
				char *p2;
				char *p3;
				char ssid[20];
				char i;
				unsigned char x, y;

				if (0 == strcmp(nvram_bufget(SER2NET_NVRAM, "SSID1"), "Hi-Link_"))
				{
					p = nvram_bufget(SER2NET_NVRAM, "WAN_MAC_ADDR");

					if (p)
					{
						p2 = p;
						for (i = 0; i < 4; i++)
						{
							p3 = strchr(p2, ':');
							if (0 == p3)
								break;
							p2 = p3 + 1;
						}
						if (p3)
						{
							x = B16ToB10(p2);
							p3 = strchr(p2, ':');
							if (0 == p3)
								break;
							p2 = p3 + 1;
							y = B16ToB10(p2);
							sprintf(ssid, "HI-LINK_%02X%02X", x, y);
							nvram_bufset(SER2NET_NVRAM, "SSID1", ssid);
							sprintf(g_AT_wifi_conf, "%s,%s,%s", ssid, keytype, key);

							nvram_bufset(SER2NET_NVRAM, "AT_wifi", g_AT_wifi_conf);

							TIM_WARN("ser2net set ssid to %s\r\n", ssid);
						}
					}
				}
				else
				{

					TIM_WARN("WARN:%s(%d)(%s)\r\n", __FUNCTION__, __LINE__, (nvram_bufget(SER2NET_NVRAM, "SSID1")));
				}

			} while (0);
		}

	} while (0);

	if (0 != g_AT_netmode)
	{

		if (1 == g_AT_dhcpd)
		{
			nvram_set_(SER2NET_NVRAM, "dhcpEnabled", "1");
		}
		else
		{
			nvram_set_(SER2NET_NVRAM, "dhcpEnabled", "0");
		}

		do
		{ //dhcpd ip
			uchar ip[32];
			uchar ip2[32];
			uchar mask[32];
			uchar gw[32];
			uchar *p1, *p2;

			p1 = g_AT_dhcpd_ip;

			p2 = strchr(p1, ',');
			if (0 == p2)
			{
				break;
			}
			snprintf(ip, MIN_(p2 - p1 + 1, sizeof(ip) - 1), p1);

			p1 = p2 + 1;
			p2 = strchr(p1, ',');
			if (0 == p2)
			{
				break;
			}
			snprintf(ip2, MIN_(p2 - p1 + 1, sizeof(ip2) - 1), p1);

			p1 = p2 + 1;
			p2 = strchr(p1, ',');
			if (0 == p2)
			{
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

		} while (0);

		do
		{ //dhcpd dns
			uchar dns1[32];
			uchar dns2[32];
			uchar *p1, *p2;

			p1 = g_AT_dhcpd_dns;

			p2 = strchr(p1, ',');
			if (0 == p2)
			{
				break;
			}
			snprintf(dns1, MIN_(p2 - p1 + 1, sizeof(dns1) - 1), p1);

			p1 = p2 + 1;
			snprintf(dns2, sizeof(dns2) - 1, p1);

			TIM_DEBUG("in:%s(%d) (%s) (%s)\r\n", __FUNCTION__, __LINE__, dns1, dns2);

			nvram_set_(SER2NET_NVRAM, "dhcpPriDns", dns1);
			nvram_set_(SER2NET_NVRAM, "dhcpSecDns", dns2);

		} while (0);

		if (0 == g_AT_dhcpd_time)
		{
			g_AT_dhcpd_time = 86400;
		}
		sprintf(buf, "%d", g_AT_dhcpd_time);
		nvram_set_(SER2NET_NVRAM, "dhcpLease", buf);
	}

	if (0 != g_AT_netmode)
	{
		if (1 == g_AT_dhcpc)
		{
			nvram_set_(SER2NET_NVRAM, "wanConnectionMode", "DHCP");
		}
		else
		{
			nvram_set_(SER2NET_NVRAM, "wanConnectionMode", "STATIC");
		}
	}
	do
	{ //net ip
		uchar ip[32];
		uchar mask[32];
		uchar gw[32];
		uchar *p1, *p2;

		p1 = g_AT_net_ip;

		p2 = strchr(p1, ',');
		if (0 == p2)
		{
			break;
		}
		snprintf(ip, MIN_(p2 - p1 + 1, sizeof(ip) - 1), p1);

		p1 = p2 + 1;
		p2 = strchr(p1, ',');
		if (0 == p2)
		{
			break;
		}
		snprintf(mask, MIN_(p2 - p1 + 1, sizeof(mask) - 1), p1);

		p1 = p2 + 1;
		snprintf(gw, sizeof(gw) - 1, p1);

		TIM_DEBUG("in:%s(%d) (%s) (%s) (%s)\r\n", __FUNCTION__, __LINE__, ip, mask, gw);

		if (3 == g_AT_netmode)
		{
			nvram_set_(SER2NET_NVRAM, "lan_ipaddr", ip);
			nvram_set_(SER2NET_NVRAM, "lan_netmask", mask);
			nvram_set_(SER2NET_NVRAM, "wan_gateway", gw);
		}
		else if (1 == g_AT_netmode)
		{
			nvram_set_(SER2NET_NVRAM, "wan_ipaddr", ip);
			nvram_set_(SER2NET_NVRAM, "wan_netmask", mask);
			nvram_set_(SER2NET_NVRAM, "wan_gateway", gw);
		}
		else if (2 == g_AT_netmode)
		{
			nvram_set_(SER2NET_NVRAM, "wan_ipaddr", ip);
			nvram_set_(SER2NET_NVRAM, "wan_netmask", mask);
			nvram_set_(SER2NET_NVRAM, "wan_gateway", gw);
		}

	} while (0);

	if (0 != g_AT_netmode)
		do
		{ //net dns
			uchar dns1[32];
			uchar dns2[32];
			uchar *p1, *p2;

			p1 = g_AT_net_dns;

			p2 = strchr(p1, ',');
			if (0 == p2)
			{
				break;
			}
			snprintf(dns1, MIN_(p2 - p1 + 1, sizeof(dns1) - 1), p1);

			p1 = p2 + 1;
			snprintf(dns2, sizeof(dns2) - 1, p1);

			TIM_DEBUG("in:%s(%d) (%s) (%s)\r\n", __FUNCTION__, __LINE__, dns1, dns2);

			nvram_set_(SER2NET_NVRAM, "wan_primary_dns", dns1);
			nvram_set_(SER2NET_NVRAM, "wan_secondary_dns", dns2);

		} while (0);

#endif
	nvram_commit(SER2NET_NVRAM);

	hlk_check1();
}

uchar config_all_save_nvram()
{
	pthread_t id;

	hlk_check3();

	pthread_create(&id, NULL, (void *)config_all_save_nvram_, NULL);
}

void ser2network_notify_serial()
{
	int g_select_quit_fd;

	g_select_quit_fd = open(NOTIFY_SERIAL_PIPE_DEV, O_RDWR | O_NONBLOCK);
	{
		uchar buf[8];
		read(g_select_quit_fd, buf, sizeof(buf));
	}
	write(g_select_quit_fd, "xxxx", 1);

	close(g_select_quit_fd);
}

void ser2network_notify_socket()
{
	int g_select_quit_fd;

	g_select_quit_fd = open(NOTIFY_SOCKET_PIPE_DEV, O_RDWR | O_NONBLOCK);
	{
		uchar buf[8];
		read(g_select_quit_fd, buf, sizeof(buf));
	}
	write(g_select_quit_fd, "xxxx", 1);

	close(g_select_quit_fd);
}

void ser2network_notify()
{
	int g_select_quit_fd;

	g_select_quit_fd = open(NOTIFY_PIPE_DEV, O_RDWR | O_NONBLOCK);
	{
		uchar buf[8];
		read(g_select_quit_fd, buf, sizeof(buf));
	}
	write(g_select_quit_fd, "xxxx", 1);

	close(g_select_quit_fd);
}

void ser2network_server_thread_quit(uchar mode)
{
	int g_select_quit_fd;
	uchar buf[64];

	g_select_quit_fd = open(SERVER_QUIT_PIPE_DEV, O_RDWR | O_NONBLOCK);

	read(g_select_quit_fd, buf, sizeof(buf));

	if (0 == mode)
	{
		write(g_select_quit_fd, "x", 1);
	}
	else
	{
	}

	close(g_select_quit_fd);
}

void ser2network_client_thread_quit(uchar mode)
{
	int g_select_quit_fd;
	uchar buf[64];

	g_select_quit_fd = open(CLIENT_QUIT_PIPE_DEV, O_RDWR | O_NONBLOCK);

	read(g_select_quit_fd, buf, sizeof(buf));

	if (0 == mode)
	{
		write(g_select_quit_fd, "x", 1);
	}
	else
	{
	}

	hlk_check3();

	close(g_select_quit_fd);
}

void ser2network_serial_thread_quit(uchar mode)
{
	int g_select_quit_fd;
	uchar buf[64];

	g_select_quit_fd = open(SERIAL_QUIT_PIPE_DEV, O_RDWR | O_NONBLOCK);

	read(g_select_quit_fd, buf, sizeof(buf));

	if (0 == mode)
	{
		write(g_select_quit_fd, "x", 1);
	}
	else
	{
	}

	close(g_select_quit_fd);
}

void ser2network_udp_thread_quit(uchar mode)
{
	int g_select_quit_fd;
	uchar buf[64];

	g_select_quit_fd = open(UDP_QUIT_PIPE_DEV, O_RDWR | O_NONBLOCK);

	read(g_select_quit_fd, buf, sizeof(buf));

	if (0 == mode)
	{
		write(g_select_quit_fd, "x", 1);
	}
	else
	{
	}

	close(g_select_quit_fd);
}


uchar pthread_is_alive(pthread_t tid)
{
	int pthread_kill_err;
	unsigned long int xxx = (unsigned long int)(tid);

	if (xxx == 0)
	{
		return 0;
	}
	pthread_kill_err = pthread_kill(tid, 0);

	if (pthread_kill_err == ESRCH) {
		return 0;
	} else if (pthread_kill_err == EINVAL) {
		
		return 1;
	} else {

		return 1;
	}
}

void *udp_discovery_server(void *arg);

uchar config_do()
{
	pthread_t id;
	int i, ret;
	static uchar start_flags = 0;

	start_flags = 0;

	ret = pthread_create(&serial_thread_id, NULL, (void *)serial_thread, NULL);

	ret = pthread_create(&udp_thread_id, NULL, (void *)udp_discovery_server, NULL);

	if (G_serial_mode_get())
	{
		G_serial_mode_set(1);
	}

	if ((0 == g_network_mode) || (0 == g_network_protocol))
	{
		debug_log("No NetMode Config");
		G_serial_mode_set(1);
	}
	else if (1 == g_network_mode)
	{
		G_serial_mode_set(0);
		debug_log("Server Mode Start Thread");
		ret = pthread_create(&server_thread_id, NULL, (void *)ser2network_server_thread, NULL);
	}
	else if (2 == g_network_mode)
	{
		G_serial_mode_set(0);
		debug_log("Client Mode Start Thread");
		ret = pthread_create(&client_thread_id, NULL, (void *)ser2network_client_thread, NULL);
	}
	else
	{
		debug_log("Fail Mode ....................");
	}

	hlk_check1();

	return 1;
}

void InitSignal(void)
{
	struct sigaction sa;
	sa.sa_handler = SIG_IGN;
	sigaction(SIGPIPE, &sa, 0);
}

static char *gopid = "/var/run/ser2net.pid"; /* pid file */

/******************************************************************************/
/*
 *	Write pid to the pid file
 */
int writeSer2netPid(void)
{
	FILE *fp;

	fp = fopen(gopid, "w+");
	if (NULL == fp)
	{
		//		error(E_L, E_LOG, T("Ser2net.c: cannot open pid file"));
		TIM_ERROR("ERROR:%s(%d)cannot open pid file\r\n", "", __LINE__);
		return (-1);
	}
	fprintf(fp, "%d", getpid());
	fclose(fp);
	return 0;
}

void out_transparent_do(uchar value)
{
	if (value)
		G_serial_mode_set(2);
	else
		G_serial_mode_set(0);
}

static void ser2netSigHandler(int signum)
{
	if (signum != SIGUSR1)
		return;

	TIM_DEBUG("");

	if (G_serial_mode > 0) {
		TIM_DEBUG("Into At Mode\r\n");
		out_transparent_do(0);
	} else {
		TIM_DEBUG("out At Mode\r\n");
		out_transparent_do(1);
	}
}

void ser2netSigHandler2(int signum);

void check_test();

void power_save(void);

int init_()
{
	int res;

	memset(&serial_thread_id, 0, sizeof(serial_thread_id));
	memset(&server_thread_id, 0, sizeof(server_thread_id));
	memset(&client_thread_id, 0, sizeof(client_thread_id));
	memset(&udp_thread_id, 0, sizeof(udp_thread_id));

	res = pthread_mutex_init(&serial_buf_mutex, NULL);
	if (res != 0)
	{
		return -1;
	}

	res = pthread_mutex_init(&socket_buf_mutex, NULL); 
	if (res != 0)
	{
		return -1;
	}

	res = pthread_mutex_init(&G_serial_mode_mutex, NULL);
	if (res != 0)
	{
		return -1;
	}

	if (pthread_mutex_init(&g_mutex, NULL) != 0)
	{
		return -1;
	}
}

void sigalrm_fn(int sig)
{
	ser2network_notify();

	if (!pthread_is_alive(udp_thread_id)) {
		pthread_create(&udp_thread_id, NULL, (void *)udp_discovery_server, NULL);
	}
	alarm(3);
	return;
}

uchar RTS_state = 1;

void ser2net_RTS_1(int signum)
{
	if (signum != (SIGRTMIN + 7))
		return;

	TIM_DEBUG("DEBUG:%s(%d)\r\n", __FUNCTION__, __LINE__);
	RTS_state = 1;
}

void ser2net_RTS_0(int signum)
{
	if (signum != (SIGRTMIN + 8))
		return;

	TIM_DEBUG("DEBUG:%s(%d)\r\n", __FUNCTION__, __LINE__);

	RTS_state = 0;
}

uchar check_rts_state()
{
	return 1;
}

void cts_set(uchar value)
{
}

#define RALINK_GPIO_LED_INFINITY 4000
typedef struct
{
	int gpio;			 //gpio number (0 ~ 23)
	unsigned int on;	 //interval of led on
	unsigned int off;	 //interval of led off
	unsigned int blinks; //number of blinking cycles
	unsigned int rests;	 //number of break cycles
	unsigned int times;	 //blinking times
} ralink_gpio_led_info;

#define GPIO_DEV "/dev/gpio"

#define RALINK_GPIO_LED_SET 0x41

#define RALINK_GPIO_SET_DIR_OUT 0x12

#define RALINK_GPIO_SET 0x21
#define RALINK_GPIO_CLEAR 0x31

#define RALINK_GPIO_READ_BIT 0x04

int led_fd;
ralink_gpio_led_info led;

void led_init()
{

	return;

	led.gpio = 1;
	led.on = 0;
	led.off = 1;
	led.blinks = 0;
	led.rests = 0;
	led.times = 0;

	led_fd = open(GPIO_DEV, O_RDONLY);
	if (led_fd < 0)
	{
		perror(GPIO_DEV);
		return;
	}
	led_set(0);
}

void led_blink()
{
	return;

	if (0 == g_s2n_stat)
		return;

	if (led_fd < 0)
	{
		led_fd = open(GPIO_DEV, O_RDONLY);
		if (led_fd < 0)
		{
			perror(GPIO_DEV);
			return;
		}
	}
#if 1
	if (ioctl(led_fd, RALINK_GPIO_LED_SET, &led) < 0)
	{
		perror("ioctl");
		led_fd = -1;
		return;
	}

#endif
}

void led_set(uchar value)
{
	return;
	if (value)
	{
		g_s2n_stat = 1;
	}
	else
	{
		g_s2n_stat = 0;
	}

	if (led_fd < 0)
	{
		led_fd = open(GPIO_DEV, O_RDONLY);
		if (led_fd < 0)
		{
			perror(GPIO_DEV);
			return;
		}
	}

	if (ioctl(led_fd, RALINK_GPIO_SET_DIR_OUT, (1 << 1)) < 0)
	{
		led_fd = -1;
		return;
	}

	if (value)
	{
		if (ioctl(led_fd, RALINK_GPIO_CLEAR, (1 << 1)) < 0)
		{
			led_fd = -1;
			return;
		}
	}
	else
	{
		if (ioctl(led_fd, RALINK_GPIO_SET, (1 << 1)) < 0)
		{
			led_fd = -1;
			return;
		}
	}
}

uchar gpio2_get()
{
	int out = 0;

	if (led_fd < 0)
	{
		led_fd = open(GPIO_DEV, O_RDONLY);
		if (led_fd < 0)
		{
			perror(GPIO_DEV);
			return 0xff;
		}
	}

	if (ioctl(led_fd, (2 << (24)) + RALINK_GPIO_READ_BIT, &out) < 0)
	{
		led_fd = -1;
		return 0xff;
	}

	if (out)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

void gpio2_set(uchar value)
{
	if (led_fd < 0)
	{
		led_fd = open(GPIO_DEV, O_RDONLY);
		if (led_fd < 0)
		{
			perror(GPIO_DEV);
			return;
		}
	}

	if (ioctl(led_fd, RALINK_GPIO_SET_DIR_OUT, (1 << 2)) < 0)
	{
		led_fd = -1;
		return;
	}

	if (value)
	{
		if (ioctl(led_fd, RALINK_GPIO_CLEAR, (1 << 2)) < 0)
		{
			led_fd = -1;
			return;
		}
	}
	else
	{
		if (ioctl(led_fd, RALINK_GPIO_SET, (1 << 2)) < 0)
		{
			led_fd = -1;
			return;
		}
	}
}

//0 - closed 1 - net up, tcp connecting 2 - tcp connected 3 - blink
void ser_led_state(uchar value)
{
	if (3 == value)
	{
	}
	else if (2 == value)
	{
	}
	else if (1 == value)
	{
	}
	else if (0 == value)
	{
	}
}

int main(int argc, char *argv[])
{
	debug_log("hlk ser2net start");

	//	system("echo 5 >/proc/sys/net/ipv4/tcp_retries2");	//about 199s.
	system("echo 3 >/proc/sys/net/ipv4/tcp_retries2"); //about 45s.
													   //	/proc/sys/net/ipv4/tcp_syn_retries
	InitSignal();

	signal(SIGUSR1, ser2netSigHandler);
	signal(SIGUSR2, ser2netSigHandler2);

	signal(SIGALRM, sigalrm_fn);
	alarm(3);

	led_init();

	if (writeSer2netPid() < 0)
	{
		restart_app();
	}

	if (init_info() < 0)
	{
		restart_app();
	}
	hlk_check2();

	if (init_() < 0)
	{
		restart_app();
	}

	config_all_get_nvram();
	config_do();

	//power_save();

	while (1)
	{
		config_listen_handle();
		sleep(1);
	}

	return 0;
}
