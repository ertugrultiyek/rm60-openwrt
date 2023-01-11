#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pty.h>
#include <termios.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <utmp.h>
#include <openssl/sha.h>
#include <time.h>
#include <sched.h>
#include <openssl/md5.h>
#include <dirent.h>
#include <linux/serial.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/un.h>
#include <sys/msg.h>
#include <sys/mman.h>
#include <syslog.h>
#include <netinet/in.h>	//ntoh

typedef unsigned char uchar;

#define TRUE 1
#define FALSE 0

#define COUNT_OF(arr)				(sizeof(arr) / sizeof(arr[0]))
#define MAX_BUFF_SIZE				512

#define MAX_(A,B) ((A>B)?A:B)		
#define MIN_(A,B) ((A<B)?A:B)		

int m_fd;
int m_isOpened = 0;
int uartrd_cnt = 0;						//uart read char number
int sfd;

char uartbuf[MAX_BUFF_SIZE];
int uart_open( char *uart_name );
int uart_close(int sfd);
int setSpeed(int fd, int speed);
int setParity(int fd, int databits, int parity, int stopbits);
int uart_write(int fd, unsigned char *buffer, int bufferLength);
int uart_read(char *buffer, int bufferLength);
int uart_write_wrap(int fd, unsigned char *buf, int len);
int uart_init(char *dev, int speed);

int uart_open( char *uart_name )
{	
	int flags;

	int fd = open( uart_name, O_RDWR);
	if (fd < 0) {
		syslog(LOG_DEBUG, "Open - Open device [%s] failed!\n", uart_name );
		return -1;
	}
	
	if(flags = fcntl(fd, F_GETFL, 0) < 0) {
		close(fd);
		return -1;
	}

	flags |= O_NONBLOCK;
	if (fcntl(fd, F_SETFL, flags) < 0) {
		close(fd);
		return -1;
	}

	return fd;
}


int uart_close(int sfd )
{
	close(sfd);
	return TRUE;
}

int setSpeed(int fd, int speed)
{
	const int speed_arr[] = {B2000000, B1000000, B921600, B460800, B115200, B57600, B38400, B19200, B9600, B4800, B2400, B1200, B300, };
	const int name_arr[]  = {2000000, 1000000, 921600,  460800, 115200,  57600,  38400,  19200,  9600,  4800,  2400,  1200,  300, };

	unsigned int i;
	int ret = 0;
	struct termios options;
	struct serial_struct ss, ss_set;

	for (i = 0; i < COUNT_OF( speed_arr ); ++ i) {
		if (speed == name_arr[i]) {
			ret = tcgetattr(fd, &options);
			if (ret != 0) {
				syslog(LOG_DEBUG, "%s %d\n", __FUNCTION__, __LINE__);
				return FALSE;
			}
			tcflush(fd, TCIOFLUSH);
			cfsetispeed(&options, speed_arr[i]);
			cfsetospeed(&options, speed_arr[i]);
			ret = tcsetattr(fd, TCSANOW, &options);

		   if ((ioctl(fd, TIOCGSERIAL, &ss)) < 0 ) {
			   syslog(LOG_DEBUG, "%s %d\n", __FUNCTION__, __LINE__);
		       return FALSE;
		   }
			
		   ss.flags = ASYNC_SPD_CUST;
		   ss.custom_divisor = ss.baud_base / speed;
		   
		   ss.custom_divisor = 22;
		
		   unsigned int x = (40000000*10/16/speed);
		   ss.custom_divisor = (x/10);
		   
		   if ( (x % 10) > 4 )
		       ss.custom_divisor++;
		
		   ioctl(fd,TIOCSSERIAL, &ss);
		
		   ioctl(fd, TIOCGSERIAL, &ss_set);

		   ioctl(fd, TIOCSERCONFIG, NULL);

			return ((ret == 0) ? TRUE : FALSE);
		}
	}

	return FALSE;
}

int setParity(int fd, int databits, int parity, int stopbits)
{
	int ret = 0;
	struct termios options;

	ret = tcgetattr(fd, &options);
	if (ret != 0) {
		return FALSE;
	}

	options.c_cflag &= ~CSIZE;

	switch (databits) {
	case 7:
		options.c_cflag |= CS7;
		break;
	case 8:
		options.c_cflag |= CS8;
		break;
	default:
		return FALSE;
	}

	switch (parity) {
	case 'n':
	case 'N':
		options.c_cflag &= ~PARENB; /* Clear parity enable */
		options.c_iflag &= ~INPCK;  /* Enable parity checking */
		break;
	case 'o':
	case 'O':
		options.c_cflag |= (PARODD | PARENB);
		options.c_iflag |= INPCK;   /* Disnable parity checking */
		break;
	case 'e':
	case 'E':
		options.c_cflag |= PARENB;  /* Enable parity */
		options.c_cflag &= ~PARODD;
		options.c_iflag |= INPCK;   /* Disnable parity checking */
		break;
	case 'S':
	case 's':
		/*as no parity*/
		options.c_cflag &= ~PARENB;
		options.c_cflag &= ~CSTOPB;
		options.c_iflag |= INPCK;   /* Disnable parity checking */
		break;
	default:
		return FALSE;
	}

	switch (stopbits) {
	case 1:
		options.c_cflag &= ~CSTOPB;
		break;
	case 2:
		options.c_cflag |= CSTOPB;
		break;
	default:
		return FALSE;
	}

	/* Set raw mode. */
	options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); /*Input*/
	options.c_oflag &= ~OPOST;                          /*Output*/
	options.c_iflag &= ~(IXON | IXOFF | IXANY);

	//关闭流控
	options.c_iflag &= ~CRTSCTS; // Disable RTS/CTS hardware flow control (most common)

	/* Don't treat <CR> as <LF>.*/
	options.c_iflag &= ~(INLCR | ICRNL | IGNCR);
	options.c_oflag &= ~(ONLCR | OCRNL);

	tcflush(fd, TCIFLUSH);
	//options.c_cc[VTIME] = 150; /* Set timeout 15 seconds*/
	options.c_cc[VTIME] = 40; /* Set timeout 15 seconds*/
	options.c_cc[VMIN] = 0;    /* Update the options and do it NOW */

	ret = tcsetattr(fd, TCSANOW, &options);
	return ((ret == 0) ? TRUE : FALSE);
}

int uart_write(int fd, unsigned char *buffer, int bufferLength)
{
	if (NULL == buffer || bufferLength <= 0) {
		syslog(LOG_DEBUG, "Write - Wrong data buffer to write!\n");
		return -1;
	}

	int ret = write(fd, buffer, bufferLength);
	return ret;
}

int uart_write_wrap(int fd, unsigned char *buf, int len )
{
	{
		int i;
		time_t timeStamp;
		time (&timeStamp);

		syslog(LOG_DEBUG, "%s  -> Tx %d bytes: ", asctime(gmtime(&timeStamp)), len);
		for (i = 0; i < len; i++) {
			syslog(LOG_DEBUG, "%02x ", buf[i]);
		}

		syslog(LOG_DEBUG, "\n");
	}

	return uart_write(fd, buf, len );
}

int uart_init( char *dev, int speed)
{
	int fd;

	fd = uart_open(dev);
	if (fd  < 0 ) {
		syslog(LOG_DEBUG,  " open \"%s\" failed\r\n", dev );
		return -1;
	} else {
		//syslog(LOG_DEBUG,  " open \"%s\" successful\r\n", dev );
	}
	
	setSpeed(fd, speed );
	
	setParity(fd, 8, 'N', 1);
	//关闭流控
	
	//syslog(LOG_DEBUG, "%s fd:%d uartname:%s\n", __FUNCTION__, fd, dev);

	return fd;
}

char _inbyte(int wait)
{	
	int ret = 0;
	struct timeval tv;
	char c;

	fd_set read_fds;
	FD_ZERO(&read_fds);
	FD_SET(sfd, &read_fds);

	tv.tv_sec = 0;
	tv.tv_usec = wait*1000;

	ret = select(sfd + 1, &read_fds, NULL, NULL, &tv);
	if (ret > 0) {
		read(sfd, &c, 1);
		return c;
	}

	return -1;
}

int _outbyte(char c)
{
	int ret = 0;
	ret = write(sfd, &c, 1);

	return ret;
}

