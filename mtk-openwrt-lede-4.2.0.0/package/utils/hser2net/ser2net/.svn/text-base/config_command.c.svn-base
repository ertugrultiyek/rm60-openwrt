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


#include "ser2net.h"



#define SER2NET_CFG_DOMAIN "/tmp/SER2NET_CFG_HLK"



#define TIM_TRACE() TIM_DEBUG("In:%s(%d)\r\n",__FUNCTION__,__LINE__)


uchar config_command(uchar *in,uchar *out)
{
	static struct sockaddr_un srv_addr;
	int connect_fd;
	
	int ret;
	char snd_buf[1024];
	int i;
	
//creat unix socket
	connect_fd=socket(PF_UNIX,SOCK_STREAM,0);
	if(connect_fd<0)
	{
		perror("cannot create communication socket");
		return 1;
	}	
	bzero(&srv_addr, sizeof( srv_addr ) );
	srv_addr.sun_family=AF_UNIX;
	strcpy(srv_addr.sun_path,SER2NET_CFG_DOMAIN);
//connect server
	ret=connect(connect_fd,(struct sockaddr*)&srv_addr,sizeof(srv_addr));
	if(ret==-1)
	{
		perror("cannot connect to the server");
		close(connect_fd);
		return 1;
	}
	
		TIM_DEBUG("in:%s(%d) (%s)\r\n",__FUNCTION__,__LINE__,in);
		write(connect_fd,in,strlen(in));
	
//		read(connect_fd,snd_buf,sizeof(snd_buf));
{
	struct timeval tv; 
	fd_set select_fds;
	int recived=0;
	uchar x1=0;
	uchar x2=0;
recv_again:		
	FD_ZERO(&select_fds); 
	FD_SET(connect_fd, &select_fds); 
	
	tv.tv_sec = 2; 
	tv.tv_usec = (800)*1000; 
	ret = select(connect_fd+1,&select_fds,NULL,NULL,&tv);
	
	if(ret>0){	
		int n=read(connect_fd,out+recived,COMM_BUF_LEN-recived);
		if(n>0){
			recived+=n;
			if((out[recived-1]!=0)){
				if(x1++<20)
					goto recv_again;
			}
			out[recived-1]=0;
		}
		else
			out[0]=0;
		TIM_DEBUG("in:%s(%d) (%s)\r\n",__FUNCTION__,__LINE__,out);
	}
	else if(ret == 0){
		TIM_DEBUG("in:%s(%d)\r\n",__FUNCTION__,__LINE__);
		{
			if(x2++<8)
				goto recv_again;
			else
				out[0]=0;
		}
	}
	else{
		TIM_ERROR("ERROR:%s(%d)\r\n",__FUNCTION__,__LINE__);
	}


}

	shutdown(connect_fd,SHUT_RDWR);
	close(connect_fd);
	return 0;
}


