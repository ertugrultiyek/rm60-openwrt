#ifndef __SER2NET_H
#define __SER2NET_H

#include <syslog.h>

#ifndef   uchar
#define   uchar   unsigned char
#endif
#ifndef   uint
#define   uint    unsigned int
#endif
#ifndef   ushort  
#define   ushort  unsigned short
#endif
#ifndef   ulong
#define   ulong   unsigned long
#endif


#define SERVER_QUIT_PIPE_DEV "/tmp/.ser2net_1"
#define CLIENT_QUIT_PIPE_DEV "/tmp/.ser2net_2"
#define AT_QUIT_PIPE_DEV "/tmp/.ser2net_quit_3"

#define SERIAL_QUIT_PIPE_DEV "/tmp/.ser2net_4"
#define UDP_QUIT_PIPE_DEV "/tmp/.ser2net_7"

#define NOTIFY_SERIAL_PIPE_DEV "/tmp/.ser2net_5"
#define NOTIFY_SOCKET_PIPE_DEV "/tmp/.ser2net_6"

#define NOTIFY_PIPE_DEV "/tmp/.ser2net_8"

#define SOCKET_QUIT_PIPE_DEV "/tmp/.ser2net_S0"
#define SOCKET_NOTIFY_SOCKET_PIPE_DEV "/tmp/.ser2net_S1"

#define MAX_(A,B) ((A>B)?A:B)		
#define MIN_(A,B) ((A<B)?A:B)		

#define AT_VERSION 200

#define SER2NET_NVRAM    0

#define TIM_DEBUG(fmt,...) \
do {\
	printf("%s %d " fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__);\
	syslog(LOG_DEBUG, "%s %d " fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__);\
} while (0)

#define TIM_WARN printf

#define TIM_ERROR printf

#define debug_log(fmt, arg...) \
do {\
	fprintf(stdout, "%s %d "fmt "\r\n", __FUNCTION__,__LINE__, ##arg); \
    syslog(LOG_DEBUG, "%s %d "fmt "\r\n", __FUNCTION__,__LINE__,##arg);\
} while (0)

uchar config_command(uchar *in,uchar *out);

uchar at(uchar *comm, uchar *out);

#define COMM_BUF_LEN (1024*4)

#define MAX_CONNECTS (100)
#define socket_buf_LEN (1024*4)
#define SERIAL_BUF_LEN (1024*4)

#define HLK_FIREWALL_FILE "/etc/hlk_firewall"

//#define SER2NET_COM2

typedef struct {
	uchar *nvram_key;
	uchar type;
	void *p;
	void *default_;
} nvram_t;

struct wifi6_keyval {
    char *key;
    char *val;
};
extern int wifi6_keyval_cnt;
extern struct wifi6_keyval *wifi6_conf;

uchar *nvram_get(uchar *a,uchar *b);
void nvram_set(uchar *a,uchar *b,uchar *c);

void nvram_bufset(uchar *a,uchar *b,uchar *c);

uchar *nvram_bufget(uchar *a,uchar *b);

#define RT2860_NVRAM SER2NET_NVRAM


#define keyCHECKdisable 1

extern int is_udp_at_cmd;
extern int delay_network_restart;

extern uchar *split(uchar *src, uchar *sp, uchar index);
extern uchar strcmpT(uchar *a, uchar *b);
extern void strcpyT(uchar *a, uchar *b, uint n);
extern void strcpyT2(uchar *a, uchar *b, uint n, char cc);

extern char *uci_get_option_val(char *uci_config_file, char *section_name, char *option_name);
extern int getipinfo(char *ip, char *bip, char *netmask, char *ifname);
extern int GetGatewayForInterface(const char* interface, char *gateway);
extern int GetDNSForInterface(const char* interface, char *dns);

extern struct wifi6_keyval *load_wifi6_profile(char *path);
extern int wifi6_set_param(struct wifi6_keyval *wifi_conf, char *key, char *val);
extern int hlk_strtol(char *str, int base, long *val);
extern int get_cmd_ret_str(char *rstr, char *cmd);
extern void do_system(char *fmt, ...);

#endif

