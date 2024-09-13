#include <stdio.h>
#include <stdarg.h>

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <syslog.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <signal.h>
/* According to earlier standards */
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <sys/stat.h>
#define MAX_(A,B) ((A>B)?A:B)		
#define MIN_(A,B) ((A<B)?A:B)		

#define ARRAY_SIZE(arr) \
  (sizeof(arr) / sizeof((arr)[0]) \
  + sizeof(typeof(int[1 - \
                      2*!!__builtin_types_compatible_p(typeof(arr), \
                                                       typeof(&arr[0]))]))*0)

#define AT_RES_DEF_STR " "

#if 0
#define QWAN_TTYUSB_DEV "/dev/ttyUSB2"
#define AT_CPIN "AT+CPIN?\r"	//获取SIM的状态
#define AT_COSPS "AT+COPS?\r"	//获取当前SIM卡的相关信息 运营商名称
#define AT_CSQ "AT+CSQ\r"
#define AT_GSN "AT+GSN\r"       //查询IMEI
#define AT_CIMI "AT+CIMI\r"     //查询IMSI   International Mobile Subscriber Identity
#define AT_QENG "AT+QENG=\"servingcell\"\r" //获取RSSI 和 网络类型
#define AT_CREG "AT+CREG?\r"      //Network Registration Status

#define OPERATOR_MOBILE "MOBILE"
#define OPERATOR_TELECOM "Telecom"
#define OPERATOR_UNICOM "Unicom"
#else
#define QWAN_TTYUSB_DEV "/dev/ttyUSB2"

#if 1
#define AT_CPIN "AT+CPIN?\\r\\n"	//获取SIM的状态
#define AT_COSPS "AT+COPS?\\r\\n"	//获取当前SIM卡的相关信息 运营商名称
#define AT_CSQ "AT+CSQ\\r\\n"
#define AT_GSN "AT+GSN\\r\\n"       //查询IMEI
#define AT_CIMI "AT+CIMI\\r\\n"     //查询IMSI   International Mobile Subscriber Identity
#define AT_QENG "AT+QENG=\\\"servingcell\\\"\\r\\n" //获取RSSI 和 网络类型
#define AT_CREG "AT+CREG?\\r\\n"      //Network Registration Status
#else
#define AT_CPIN "AT+CPIN?\r"	//获取SIM的状态
#define AT_COSPS "AT+COPS?\r"	//获取当前SIM卡的相关信息 运营商名称
#define AT_CSQ "AT+CSQ\r"
#define AT_GSN "AT+GSN\r"       //查询IMEI
#define AT_CIMI "AT+CIMI\r"     //查询IMSI   International Mobile Subscriber Identity
#define AT_QENG "AT+QENG=\"servingcell\"\r" //获取RSSI 和 网络类型
#define AT_CREG "AT+CREG?\r"      //Network Registration Status

#endif

#define OPERATOR_MOBILE "MOBILE"
#define OPERATOR_TELECOM "CHN-CT"
#define OPERATOR_UNICOM "CHN-UNICOM"

#define OPERATOR_MOBILE_STR "MOBILE"
#define OPERATOR_TELECOM_STR "TELECOM"
#define OPERATOR_UNICOM_STR "UNICOM"

#endif

#define CSQ_HEADER "+CSQ: "
#define QENG_HEADER "+QENG: "
#define CREG_HEADER "+CREG: "


#define MOBILE_DIR  "/tmp/mobile/"

#define MODULE_NAME_FILE MOBILE_DIR"module_name"
#define SIM_STATUS_FILE MOBILE_DIR"sim_status"
#define IMEI_FILE MOBILE_DIR"imei"
#define SIGNAL_FILE MOBILE_DIR"rssi"
#define OP_FILE MOBILE_DIR"operator"
#define NETTYPE_FILE MOBILE_DIR"nettype"

#define IP_FILE MOBILE_DIR"ipaddr"


char *at_list[] = {
	AT_CPIN,
	AT_COSPS,
	AT_CSQ,
    AT_GSN,
    AT_QENG,
    AT_CREG
};

extern int uart_init( char *dev, int speed);
extern int uart_write(int fd, unsigned char *buffer, int bufferLength);
extern int uart_close(int sfd);


int rssi2csq(int rssi)
{
    int csq;

    if (rssi < -113) {
        csq = 0;
    } else if(rssi == -111) {
        csq = 1;
    } else if((rssi>=-109)&&(rssi<=-53)) {
        csq = (rssi + 113)/2;
    } else if((rssi>= -51)&&(rssi<0)) {
        csq = 31;
    } else {
        csq = 99;
    }

    return csq;
}

int csq2rssi(char* csqstr)
{
	int rssi;
    int csq = atoi(csqstr);

	if (0 == csq) {
		rssi = -113;
	} else if (1 == csq ) {
		rssi == -111;
	} else if (csq >= 2 && csq <= 30) {
		rssi = csq*2 -113;
	} else if (31 == csq) {
		rssi = -51;
	} else {
		rssi = -115;
	}

    //printf("CSQ:%d RSSI:%d\n", csq, rssi);

    return rssi;
}

//10分钟获取一次状态 


#define AT_OK_STR "OK"
#define AT_ERROR_STR "ERROR"


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


int gw_signal_init(void)
{
	int i;
	struct sigaction sa;

	/* Setup fatal signals (block fatal handler from all own signals) */
    signal(SIGCHLD, NULL);

	return 0;
}

void close_ate_debug(int sfd)
{
    char buffer[128] = {0};

    do_system("echo -e \"ATE0\" > %s", QWAN_TTYUSB_DEV);
    usleep(100);
    read(sfd, buffer, sizeof(buffer));
    usleep(100);
    read(sfd, buffer, sizeof(buffer));
    usleep(100);
    read(sfd, buffer, sizeof(buffer));
    usleep(100);

}

#include <linux/serial.h>
#include <sys/ioctl.h>
#include "ringbuffer.h"
#include "./platform/include/hlk_platform.h"

#define USB_VID_FIBCOM   "2cb7"
#define USB_VID_QUECTEL    "2c7c"
#define USB_PID_QUECTEL_RM500Q   "0800"
#define USB_PID_QUECTEL_RM500U   "0900"
#define USB_PID_FIBCOM_FM650U_CN "0a05"

#define MODULE_NAME_FM650_CN    "FM650-CN"
#define MODULE_NAME_RM500U_CN    "RM500U-CN"
#define MODULE_NAME_RM500Q    "RM500Q"
#define MODULE_NAME_RM520N    "RM520N"
#define MODULE_NAME_EC200T    "EC200T"

#define AT_TTY_USB0 "/dev/ttyUSB0"
#define AT_TTY_USB2 "/dev/ttyUSB2"

struct usb_match_table {
    char *module_name;
    char *at_tty;
    char *vid_pid;
};

struct usb_match_table module_vid_pid_match_array[] = {
    {MODULE_NAME_FM650_CN, AT_TTY_USB0, "2cb7:0a05"},
    {MODULE_NAME_RM500U_CN, AT_TTY_USB2, "2c7c:0900"},
    {MODULE_NAME_RM500Q, AT_TTY_USB2, "2c7c:0800"},
    {MODULE_NAME_RM520N, AT_TTY_USB2, "2c7c:0801"},
    {MODULE_NAME_EC200T, AT_TTY_USB2, "2c7c:6026"},    

};

ring_buffer ring_buffer_handle;
char ring_buff[2048] = {0};
AC_STRUCT *ACTree = NULL;

int hlk_get_atcmd_res(char *dev, int baud, char *cmd)
{
    int sfd;
	int rc;
    int pid;
    int res = -1;
	struct timeval tv;
    struct timeval *s_timeout = NULL;
	fd_set read_fds;
	fd_set write_fds;
	int maxfd = 0;
    struct timeval cur;
    struct timeval start_time;
    int rbuff_length = 0;

    char tmp_buff[2048] = {0};

    sfd = uart_init(dev, baud);
    if (sfd < 0) {
        syslog(LOG_DEBUG, "uart init failed");
        return -1;
    }

    memset(ring_buff, 0, sizeof(ring_buff));
    Ring_Buffer_Init(&ring_buffer_handle, ring_buff, sizeof(ring_buff));

    gettimeofday(&start_time, NULL);

    gettimeofday(&cur, NULL);

    struct timeval _timeout = {.tv_sec=0, .tv_usec=500*1000};

    rc = uart_write(sfd, cmd, strlen(cmd));

    while (hlk_tool_timerout(&start_time, &cur, &_timeout)) {
        gettimeofday(&cur, NULL);

        FD_ZERO(&read_fds);
        FD_ZERO(&write_fds);

        FD_SET(sfd, &read_fds);
        maxfd = MAX_(maxfd, sfd);
        tv.tv_sec = 0;
        tv.tv_usec = 100*1000; //500ms
        s_timeout = &tv;
        
        rc = select(maxfd + 1, &read_fds, NULL, NULL, s_timeout);
        if (rc < 0) {
            uart_close(sfd);
            hlk_tool_sleep(100);
            sfd = uart_init(dev, baud);
            if (sfd < 0) {
                syslog(LOG_DEBUG, "uart init failed");
                break;
            }

            continue;
        } else if (0 == rc) {
            continue;
        }

        if (FD_ISSET(sfd, &read_fds)) {
            int fl = Ring_Buffer_Get_FreeSize(&ring_buffer_handle);
            int rl = read(sfd, tmp_buff, fl);
            if (rl < 0) {
                break;
            }

            Ring_Buffer_Write_String(&ring_buffer_handle, tmp_buff, rl);
        }
    }

    rbuff_length = Ring_Buffer_Get_Lenght(&ring_buffer_handle);
    if (rbuff_length > 0) {
        res = rbuff_length;
    }

end:
    return res;
}

#define IMEI_FILE_PATH "/tmp/imei"

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

    debug_log(DEBUG_DEBUG, "open imei file failed\n");
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

    debug_log(DEBUG_DEBUG, "open imei file failed\n");
    return -1;
}


#include "pcre.h"
#define OVECCOUNT 30 /* should be a multiple of 3 */
void hlk_get_module_imei(char *dev, int baud, char *cmd, char *module_name)
{
    int res = -1;
    pcre * pPcre = NULL;
    const char *error;
    int erroffset;
    int ringbuff_len = 0;
    int ovector[OVECCOUNT];
    int pcre_res;
    char imei[16] = {0};

    //只读取一次 IMEI号 存储到文件中
    if (access(IMEI_FILE, R_OK) == 0) {
        int imei_fd = open(IMEI_FILE, O_RDONLY);
        if (imei_fd > 0) {
            res = read(imei_fd, imei, 16);
            printf("%s", imei);
            close(imei_fd);
            return;
        }
    }

    res = hlk_get_atcmd_res(dev, baud, cmd);
    if (res < 0) {
        return;
    }
    ringbuff_len = res;

    //hlk_tool_print_hexdump(ring_buff, ringbuff_len, 1);
    if (!strcmp(module_name, MODULE_NAME_RM500Q) || !strcmp(module_name, MODULE_NAME_RM500U_CN) || !strcmp(module_name, MODULE_NAME_RM520N) ) {
        //查询数字 863305040646886 15位数字组成
        char * pPattern = "\\d{15}";
        pPcre = pcre_compile(pPattern, 0, &error, &erroffset, NULL);

        pcre_res = pcre_exec(pPcre, NULL, ring_buff, ringbuff_len, 0, 0, ovector, OVECCOUNT);
        //printf("\npcre_res: %d ovector:%d\n", pcre_res, ovector[0]);
        if (pcre_res > 0) {
            memcpy(imei, &ring_buff[ovector[0]], 15);
            printf("%s", imei);
            //写入到文件中
            write_str_to_file(IMEI_FILE, imei);
            
        }
        free(pPcre);
    }

    return;
}

#define SIM_STATUS_PATH "/tmp/sim_status"
#define CONN_STATUS_PATH "/tmp/conn_status"
#define NETTYPE_PATH "/tmp/nettype"
void hlk_get_module_sim_status(char *dev, int baud, char *cmd, char *module_name)
{
    int res = -1;
    pcre * pPcre = NULL;
    const char *error;
    int erroffset;
    int ringbuff_len = 0;
    int ovector[OVECCOUNT];
    int pcre_res;
    char imei[16] = {0};

    //如果短时间内再次获取 从文件中获取状态
    //只读取一次 IMEI号 存储到文件中
    if (access(SIM_STATUS_PATH, R_OK) == 0) {
        //判断 文件创建时间 和当前时间的差值
        struct stat file_stat;
        res = stat(SIM_STATUS_PATH, &file_stat);
        time_t cur_time = time(NULL);

        if (cur_time - file_stat.st_ctime < 120) {   //1分钟之内 获取状态 从 文件中获取
            char sim_status[16] = {0};
            //printf("ctime :%d cur time:%d", file_stat.st_ctime, cur_time);

            read_str_from_file(SIM_STATUS_PATH, sim_status, 16);
            printf("%s", sim_status);
            return;
        }
        do_system("rm -f %s", SIM_STATUS_PATH);
    }

    res = hlk_get_atcmd_res(dev, baud, cmd);
    if (res < 0) {
        return;
    }
    ringbuff_len = res;

    hlk_tool_print_hexdump(ring_buff, ringbuff_len, 1);

    //RM500Q 未插入SIM卡时 使用at+cpin? 会返回+CME ERROR: 13  插入sim卡后 会返回 +CPIN: READY 主动拔掉SIM 模块过一段时间会主动上报 +CPIN: NOT READY 这个状态 主动查询是拿不到的
    //RM500U-CN 未插入SIM 卡 或启动后插入SIM卡 CME ERROR: 10 
    
    if (!strcmp(module_name, MODULE_NAME_RM500Q) || !strcmp(module_name, MODULE_NAME_RM500U_CN)  || !strcmp(module_name, MODULE_NAME_RM520N) ) {
        char * pPattern = "READY";
        pPcre = pcre_compile(pPattern, 0, &error, &erroffset, NULL);

        pcre_res = pcre_exec(pPcre, NULL, ring_buff, ringbuff_len, 0, 0, ovector, OVECCOUNT);
        //printf("\npcre_res: %d ovector:%d\n", pcre_res, ovector[0]);
        if (pcre_res > 0) {
            memcpy(imei, &ring_buff[ovector[0]], 15);
            printf("READY");
            write_str_to_file(SIM_STATUS_PATH, "READY");
        } else {
            printf("NOT READY");
            write_str_to_file(SIM_STATUS_PATH, "NOT READY");
        }
        free(pPcre);
    }

    return;
}

char *hlk_get_simcard_num(char *dev, int baud, char *module_name)
{
    int res = -1;
    pcre * pPcre = NULL;
    const char *error;
    int erroffset;
    int ringbuff_len = 0;
    int ovector[OVECCOUNT];
    int pcre_res;

    char *at_cmd = NULL;

    if (!strcmp(module_name, MODULE_NAME_RM500Q) || !strcmp(module_name, MODULE_NAME_RM520N)) {
        at_cmd = "AT+CCID\r\n";
    }

    res = hlk_get_atcmd_res(dev, baud, at_cmd);
    if (res < 0) {
        return NULL;
    }
    ringbuff_len = res;

    hlk_tool_print_hexdump(ring_buff, ringbuff_len, 1);
    
    if (!strcmp(module_name, MODULE_NAME_RM500Q) || !strcmp(module_name, MODULE_NAME_RM500U_CN)  || !strcmp(module_name, MODULE_NAME_RM520N) ) {

        //查询哪个运行商
        char * pPattern = "(?i)[+]CCID:[\\s\\S]*OK";
        pPcre = pcre_compile(pPattern, 0, &error, &erroffset, NULL);

        pcre_res = pcre_exec(pPcre, NULL, ring_buff, ringbuff_len, 0, 0, ovector, OVECCOUNT);
        if (pcre_res > 0) {
            ring_buff[ovector[0]];
            char connect_status[16] = {0};
            char nettype[16] = {0};
            char *token, *entry;
            char *save = NULL;
            char *ptr = strdup(&ring_buff[ovector[0]]+strlen("+CCID: "));
            int comma = 0;
            char *p;

            printf("ptr :%s", ptr);

            /*
            AT+CCID 

            */
            entry = ptr;

            for (;; entry = NULL ) {
                token = strtok_r(entry, ",", &save);
                if (NULL == token) {
                    break;
                }
                comma++;
                switch (comma) {
                    case 1:
                    //连接状态 需要去除 ""
                    p = strchr(token, '\r');
                    if (p != NULL) {
                        *p = '\0';
                    }
                    printf("CCID: %s", token);
                    break;
                    case 2:
                    break;
                    case 3:
                    break;
                    case 4:
                    break;
                    case 5:
                    break;
                    case 6:
                    break;
                    case 7:
                    break;
                    default:
                    break;
                }
            }
            free(ptr);
        } else {
            debug_log(DEBUG_DEBUG, "at rssi command exec failed");
        }
        free(pPcre);
    }

    return NULL;
}

//获取网络注册状态
char *hlk_get_reg_status(char *dev, int baud, char *module_name)
{
    int res = -1;
    pcre * pPcre = NULL;
    const char *error;
    int erroffset;
    int ringbuff_len = 0;
    int ovector[OVECCOUNT];
    int pcre_res;

    char *at_cmd = NULL;

    if (!strcmp(module_name, MODULE_NAME_RM500Q) || !strcmp(module_name, MODULE_NAME_RM520N)) {
        at_cmd = "AT+CREG?\r\n";
    }

    res = hlk_get_atcmd_res(dev, baud, at_cmd);
    if (res < 0) {
        return NULL;
    }
    ringbuff_len = res;

    hlk_tool_print_hexdump(ring_buff, ringbuff_len, 1);

    //RM500Q 未插入SIM卡时 使用at+cpin? 会返回+CME ERROR: 13  插入sim卡后 会返回 +CPIN: READY 主动拔掉SIM 模块过一段时间会主动上报 +CPIN: NOT READY 这个状态 主动查询是拿不到的
    //RM500U-CN 未插入SIM 卡 或启动后插入SIM卡 CME ERROR: 10 
    
    if (!strcmp(module_name, MODULE_NAME_RM500Q) || !strcmp(module_name, MODULE_NAME_RM500U_CN)  || !strcmp(module_name, MODULE_NAME_RM520N) ) {

        //查询哪个运行商
        char * pPattern = "(?i)[+]CREG:[\\s\\S]*OK";
        pPcre = pcre_compile(pPattern, 0, &error, &erroffset, NULL);

        pcre_res = pcre_exec(pPcre, NULL, ring_buff, ringbuff_len, 0, 0, ovector, OVECCOUNT);
        //printf("\npcre_res: %d ovector:%d\n", pcre_res, ovector[0]);
        if (pcre_res > 0) {
            //命令执行成功 1. 获取 连接状态 2. 获取连接的网络类型
            ring_buff[ovector[0]];
            char connect_status[16] = {0};
            char nettype[16] = {0};
            char *token, *entry;
            char *save = NULL;
            char *ptr = strdup(&ring_buff[ovector[0]]+strlen("+CREG: "));
            int comma = 0;

            printf("ptr :%s", ptr);

            /*
            AT+CREG Network Registration Status
            +CREG: <n>,<stat>[,<lac>,<ci>[,<Act>]]
            <n> Integer type 
            0 Disable network registration unsolicited result code
            1 Enable network registration unsolicited result code: +CREG: <stat>
            2 Enable network registration unsolicited result code with location information:
            +CREG: <stat>[,<lac>,<ci>[,<Act>]]
            <stat> Integer type. Indicate the circuit mode registration status.
            0 Not registered. MT is not currently searching a new operator to register to
            1 Registered, home network
            2 Not registered, but MT is currently searching a new operator to register to
            3 Registration denied
            4 Unknown
            5 Registered, roaming
            <lac> Two bytes location area code in hexadecimal format.
            <ci> 28-bit (UMTS/LTE) cell ID in hexadecimal format.
            <Act> Integer type. Access technology selected.
            2 UTRAN
            4 UTRAN W/HSDPA
            5 UTRAN W/HSUPA
            6 UTRAN W/HSDPA and HSUPA
            7 E-UTRAN 
            10 E-UTRAN connected to a 5GCN
            11 NR connected to 5GCN
            12 NG-RAN
            13 E-UTRAN-NR dual connectivity
            <err> Error codes. For more details, please refer to Table 11.


            */
            entry = ptr;

            for (;; entry = NULL ) {
                token = strtok_r(entry, ",", &save);
                if (NULL == token) {
                    break;
                }
                comma++;
                switch (comma) {
                    case 1:
                    //连接状态 需要去除 ""
                    printf("RSSI: %s", token);
                    break;
                    case 2:
                    //网络类型
                    printf("ber: %s", token);
                    break;
                    case 3:
                    break;
                    case 4:
                    break;
                    case 5:
                    break;
                    case 6:
                    break;
                    case 7:
                    break;
                    default:
                    break;
                }
            }
            free(ptr);
        } else {
            debug_log(DEBUG_DEBUG, "at rssi command exec failed");
        }
        free(pPcre);
    }

    return NULL;
}

char *hlk_get_rssi(char *dev, int baud, char *module_name)
{
    int res = -1;
    pcre * pPcre = NULL;
    const char *error;
    int erroffset;
    int ringbuff_len = 0;
    int ovector[OVECCOUNT];
    int pcre_res;

    char *at_cmd = NULL;

    if (!strcmp(module_name, MODULE_NAME_RM500Q) || !strcmp(module_name, MODULE_NAME_RM520N)) {
        at_cmd = "AT+CSQ\r\n";
    }

    res = hlk_get_atcmd_res(dev, baud, at_cmd);
    if (res < 0) {
        return NULL;
    }
    ringbuff_len = res;

    hlk_tool_print_hexdump(ring_buff, ringbuff_len, 1);

    //RM500Q 未插入SIM卡时 使用at+cpin? 会返回+CME ERROR: 13  插入sim卡后 会返回 +CPIN: READY 主动拔掉SIM 模块过一段时间会主动上报 +CPIN: NOT READY 这个状态 主动查询是拿不到的
    //RM500U-CN 未插入SIM 卡 或启动后插入SIM卡 CME ERROR: 10 
    
    if (!strcmp(module_name, MODULE_NAME_RM500Q) || !strcmp(module_name, MODULE_NAME_RM500U_CN)  || !strcmp(module_name, MODULE_NAME_RM520N) ) {

        //查询哪个运行商
        char * pPattern = "(?i)[+]csq:[\\s\\S]*OK";
        pPcre = pcre_compile(pPattern, 0, &error, &erroffset, NULL);

        pcre_res = pcre_exec(pPcre, NULL, ring_buff, ringbuff_len, 0, 0, ovector, OVECCOUNT);
        //printf("\npcre_res: %d ovector:%d\n", pcre_res, ovector[0]);
        if (pcre_res > 0) {
            //命令执行成功 1. 获取 连接状态 2. 获取连接的网络类型
            ring_buff[ovector[0]];
            char connect_status[16] = {0};
            char nettype[16] = {0};
            char *token, *entry;
            char *save = NULL;
            char *ptr = strdup(&ring_buff[ovector[0]]+strlen("+csq: "));
            int comma = 0;

            //printf("res ptr :%s", ptr);

            /*
            +CSQ: <RSSI>,<ber>
            <RSSI> Integer type. 0 -113 dBm or less
            1 -111 dBm
            2...30 -109 dBm... -53 dBm
            31 -51 dBm or greater
            99 Not known or not detectable
            <ber> Integer type. Channel bit error rate (in percent).
            0...7 As RxQual values in the table in 3GPP TS 45.008 subclause 8.2.4
            99 Not known or not detectable
            <err> Error codes. For more details, please refer to Table 11.
            */
            entry = ptr;

            for (;; entry = NULL ) {
                token = strtok_r(entry, ",", &save);
                if (NULL == token) {
                    break;
                }
                comma++;
                switch (comma) {
                    case 1:
                    //连接状态 需要去除 ""
                    //RSSI
                    //printf("RSSI: %s", token);
                    {
                        int rssi = csq2rssi(token);
                        do_system("echo %d > %s", rssi, SIGNAL_FILE);
                    }
                    break;
                    case 2:
                    //网络类型
                    //printf("ber: %s", token);
                    break;
                    case 3:
                    break;
                    case 4:
                    break;
                    case 5:
                    break;
                    case 6:
                    break;
                    case 7:
                    break;
                    default:
                    break;
                }
            }
            free(ptr);
        } else {
            debug_log(DEBUG_DEBUG, "at rssi command exec failed");
        }
        free(pPcre);
    }

    return NULL;
}

//获取运营商信息
char *hlk_get_operator(char *dev, int baud, char *module_name)
{
    int res = -1;
    pcre * pPcre = NULL;
    const char *error;
    int erroffset;
    int ringbuff_len = 0;
    int ovector[OVECCOUNT];
    int pcre_res;

    char *at_cmd = NULL;

    /*
    if (access(OP_FILE, R_OK) == 0) {
        return NULL;
    }
    */

    if (!strcmp(module_name, MODULE_NAME_RM500Q) || !strcmp(module_name, MODULE_NAME_RM520N)) {
        at_cmd = "AT+COPS?\r\n";
    }

    res = hlk_get_atcmd_res(dev, baud, at_cmd);
    if (res < 0) {
        return NULL;
    }
    ringbuff_len = res;

    hlk_tool_print_hexdump(ring_buff, ringbuff_len, 1);

    //RM500Q 未插入SIM卡时 使用at+cpin? 会返回+CME ERROR: 13  插入sim卡后 会返回 +CPIN: READY 主动拔掉SIM 模块过一段时间会主动上报 +CPIN: NOT READY 这个状态 主动查询是拿不到的
    //RM500U-CN 未插入SIM 卡 或启动后插入SIM卡 CME ERROR: 10 
    
    if (!strcmp(module_name, MODULE_NAME_RM500Q) || !strcmp(module_name, MODULE_NAME_RM500U_CN)  || !strcmp(module_name, MODULE_NAME_RM520N) ) {

        //查询哪个运行商
        char * pPattern = "[+]COPS:[\\s\\S]*OK";
        pPcre = pcre_compile(pPattern, 0, &error, &erroffset, NULL);

        pcre_res = pcre_exec(pPcre, NULL, ring_buff, ringbuff_len, 0, 0, ovector, OVECCOUNT);
        //printf("\npcre_res: %d ovector:%d\n", pcre_res, ovector[0]);
        if (pcre_res > 0) {
            //命令执行成功 1. 获取 连接状态 2. 获取连接的网络类型
            ring_buff[ovector[0]];
            char connect_status[16] = {0};
            char nettype[16] = {0};
            char *token, *entry;
            char *save = NULL;
            char *ptr = strdup(&ring_buff[ovector[0]]+strlen("+COPS: "));
            int comma = 0;

            printf("res ptr :%s", ptr);

            //+COPS: <mode>[,<format>[,<oper>][,<Act>]]
            /*
            <stat> Integer type. 0 Unknown
            1 Operator available
            2 Current operator
            3 Operator forbidden
            <oper> String type. Operator in format as per <mode>.
            <mode> Integer type. 0 Automatic mode. <oper> field is ignored
            1 Manual operator selection. <oper> field shall be present and <Act> optionally
            2 Manually deregister from network
            3 Set only <format> (for AT+COPS? Read Command), and do not attempt 
            registration/deregistration (<oper> and <Act> fields are ignored). This value is 
            invalid in the response of Read Command.
            4 Manual/automatic selection. <oper> field shall be presented. If manual selection 
            fails, automatic mode (<mode> = 0) will be entered
            <format> Integer type. 0 Long format alphanumeric <oper> which can be up to 16 characters long
            1 Short format alphanumeric <oper>
            2 Numeric <oper>. GSM location area identification number
            <Act> Integer type.
            Access technology selected. Values 4, 5, 6 occur only in the response of Read Command 
            while MS is in data service state and is not intended for the AT+COPS Write Command.
            2 UTRAN
            4 UTRAN W/HSDPA
            5 UTRAN W/HSUPA
            6 UTRAN W/HSDPA and HSUPA
            7 E-UTRAN
            10 E-UTRAN connected to a 5GCN
            11 NR connected to 5GCN
            12 NG-RAN
            */
            entry = ptr;

            for (;; entry = NULL ) {
                token = strtok_r(entry, ",", &save);
                if (NULL == token) {
                    break;
                }
                comma++;
                switch (comma) {
                    case 1:
                    //连接状态 需要去除 ""
                    printf("mode: %s", token);
                    break;
                    case 2:
                    //网络类型
                    printf("format: %s", token);
                    break;
                    case 3:
                    //show operator
                    printf("operaton: %s", token);
                    {
                        //把 引号去掉
                        if (*token == '"') {
                            token += 1;
                        }
                        char *p0 = strchr(token, '"');
                        if (p0 != NULL) {
                            *p0 = '\0';
                        }
                    }
                    printf("\n OP_FILE:%s token:%s\n", OP_FILE, token);
                    //记录运营商类型
                    do_system("echo %s > %s", token, OP_FILE);
                    break;
                    case 4:
                    //printf("act:%s", token);
                    break;
                    case 5:
                    break;
                    case 6:
                    break;
                    case 7:
                    break;
                    default:
                    break;
                }
            }
            free(ptr);
        } else {
            debug_log(DEBUG_DEBUG, "at_qeng command exec failed");
        }
        free(pPcre);
    }

    return NULL;
}

char *hlk_get_module_nettype(char *dev, int baud, char *module_name)
{
    char *at_cmd = NULL;
    int res = -1;
    pcre * pPcre = NULL;
    const char *error;
    int erroffset;
    int ringbuff_len = 0;
    int ovector[OVECCOUNT];
    int pcre_res;

    //如果短时间内再次获取 从文件中获取状态
    //只读取一次 IMEI号 存储到文件中
    if (access(NETTYPE_PATH, R_OK) == 0) {
        //判断 文件创建时间 和当前时间的差值
        struct stat file_stat;
        res = stat(NETTYPE_PATH, &file_stat);
        time_t cur_time = time(NULL);

        if (cur_time - file_stat.st_ctime < 60) {   //1分钟之内 获取状态 从 文件中获取
            char sim_status[16] = {0};
            //printf("ctime :%d cur time:%d", file_stat.st_ctime, cur_time);

            read_str_from_file(NETTYPE_PATH, sim_status, 16);
            printf("%s", sim_status);
            return NULL;
        }
        do_system("rm -f %s", NETTYPE_PATH);
    }


    if (!strcmp(module_name, MODULE_NAME_RM500Q)  || !strcmp(module_name, MODULE_NAME_RM520N) ) {
        at_cmd = "AT+QENG=\"servingcell\"\r\n";
    } else if (!strcmp(module_name, MODULE_NAME_RM500U_CN)) {

    } else if (!strcmp(module_name, MODULE_NAME_FM650_CN)) {

    }
    //printf("atcmd :%s", at_cmd);

    res = hlk_get_atcmd_res(dev, baud, at_cmd);
    if (res < 0) {
        return NULL;
    }
    ringbuff_len = res;

    hlk_tool_print_hexdump(ring_buff, ringbuff_len, 1);
    
    if (!strcmp(module_name, MODULE_NAME_RM500Q)  || !strcmp(module_name, MODULE_NAME_RM520N) ) {

        //+QENG: "servingcell",<state>,"LTE",<is_tdd>,<MCC>,<MNC>,<cellID>,<PCID>,<earfcn>,<freq_band_ind>,<UL_bandwidth>,<DL_bandwidth>,<TAC>,<RSRP>,<RSRQ>,<RSSI>,<SINR>,<CQI>,<tx_power>,<srxlev>
        //+QENG: "servingcell","NOCONN","NR5G-SA","TDD", 460,11,1A383013,351,81A060,627264,78,3,-94,-12,15,1,2943
        //查询哪个运行商
        char * pPattern = "QENG: \"servingcell\",";
        pPcre = pcre_compile(pPattern, 0, &error, &erroffset, NULL);

        pcre_res = pcre_exec(pPcre, NULL, ring_buff, ringbuff_len, 0, 0, ovector, OVECCOUNT);
        //printf("\npcre_res: %d ovector:%d\n", pcre_res, ovector[0]);
        if (pcre_res > 0) {
            //命令执行成功 1. 获取 连接状态 2. 获取连接的网络类型
            ring_buff[ovector[0]];
            char connect_status[16] = {0};
            char nettype[16] = {0};
            char *token, *entry;
            char *save = NULL;
            char *ptr = strdup(&ring_buff[ovector[0]]+strlen("QENG: \"servingcell\","));
            int comma = 0;

            //printf("ptr :%s", ptr);

            entry = ptr;

            for (;; entry = NULL ) {
                token = strtok_r(entry, ",", &save);
                if (NULL == token) {
                    break;
                }
                comma++;
                switch (comma) {
                    case 1:
                    //连接状态 需要去除 ""
                    //printf("conn status: %s", token);
                    write_str_to_file(CONN_STATUS_PATH, token);
                    break;
                    case 2:
                    //网络类型
                    printf("net type: %s", token);
                    write_str_to_file(NETTYPE_PATH, token);
                    break;
                    case 3:
                    break;
                    case 4:
                    break;
                    case 5:
                    break;
                    case 6:
                    break;
                    case 7:
                    break;
                    default:
                    break;
                }
            }
            free(ptr);
        } else {
            debug_log(DEBUG_DEBUG, "at_qeng command exec failed");
        }

        free(pPcre);
    }

    return NULL;
}

char *hlk_get_module_name(char *dev, int baud, char *cmd)
{
    int sfd;
	int rc;
    int pid;
	struct timeval tv;
    struct timeval *s_timeout = NULL;
	fd_set read_fds;
	fd_set write_fds;
	int maxfd = 0;
    struct timeval cur;
    struct timeval start_time;
    char *res = NULL;
    int rbuff_length = 0;

    char tmp_buff[2048] = {0};

    sfd = uart_init(dev, baud);
    if (sfd < 0) {
        syslog(LOG_DEBUG, "uart init failed");
        return NULL;
    }

    memset(ring_buff, 0, sizeof(ring_buff));
    Ring_Buffer_Init(&ring_buffer_handle, ring_buff, sizeof(ring_buff));

    gettimeofday(&start_time, NULL);

    gettimeofday(&cur, NULL);

    struct timeval _timeout = {.tv_sec=0, .tv_usec=200*1000};

    rc = uart_write(sfd, cmd, strlen(cmd));

    while (hlk_tool_timerout(&start_time, &cur, &_timeout)) {
        gettimeofday(&cur, NULL);

        FD_ZERO(&read_fds);
        FD_ZERO(&write_fds);

        FD_SET(sfd, &read_fds);
        maxfd = MAX_(maxfd, sfd);
        tv.tv_sec = 0;
        tv.tv_usec = 100*1000; //500ms
        s_timeout = &tv;
        
        rc = select(maxfd + 1, &read_fds, NULL, NULL, s_timeout);
        if (rc < 0) {
            break;
        } else if (0 == rc) {
            continue;
        }

        if (FD_ISSET(sfd, &read_fds)) {
            int fl = Ring_Buffer_Get_FreeSize(&ring_buffer_handle);
            int rl = read(sfd, tmp_buff, fl);
            if (rl < 0) {
                break;
            }

            Ring_Buffer_Write_String(&ring_buffer_handle, tmp_buff, rl);
        }
    }

    rbuff_length = Ring_Buffer_Get_Lenght(&ring_buffer_handle);
    if (rbuff_length > 0) {

    }

end:
    return res;
}

int main(int argc, char **argv)
{
    char *cmd;
    int i,j;
	int rc;
	char buffer[512] = {0};
    char cmd_buff[512] = {0};
    fd_set select_fds;
    struct timeval tv;
    int exec_cnt = 0;
    int pid;
    char *res;
    
    int fd[2];
	int *write_fd = &fd[1];
	int *read_fd = &fd[0];
    int lenp;
    char *usb_tty;
    int baud = 115200;
    char *find_str;
	char *tmp_buf = NULL;
    char *module_name = NULL;
    int lsusb_fd;

    gw_signal_init();

    //根据VID PID判断 设备类型
    lenp = sizeof(buffer);
    if (access("/tmp/lsusb", R_OK)) {
        rc = hlk_tool_get_cmd_ret("lsusb", buffer, lenp);
        if (rc < 0) {
            printf(AT_RES_DEF_STR);
            return -1;
        }
        #if 1
        lsusb_fd = open("/tmp/lsusb", O_WRONLY|O_CREAT);
        lenp = write(lsusb_fd, buffer, rc);
        close(lsusb_fd);
        #endif
    } else {
        struct stat  stat_buf;
        lsusb_fd= open("/tmp/lsusb",  O_RDONLY);
        rc = read(lsusb_fd, buffer, lenp);

        close(lsusb_fd);
    }    
    lenp = rc;

	ACTree = ac_alloc();
    ACTree->pNum = 0; //当前模式串个数为0

	for (i = 0; i < ARRAY_SIZE(module_vid_pid_match_array); i++) {
		ac_add_string(ACTree, module_vid_pid_match_array[i].vid_pid, strlen(module_vid_pid_match_array[i].vid_pid), i+1);
	}

    rc = ac_implement(ACTree);

    search_init(ACTree, lenp, buffer);
    ac_search(ACTree);

    //检查搜索结果
    for (i = 0; i < ARRAY_SIZE(module_vid_pid_match_array); i++) {
        for (j = 0; j < POS_RECORD_NUM; j++) {
            if (ACTree->match_table[i].pos[j] != -1) {
                //printf("match usb device table dev name:%s pos:%d\n", module_vid_pid_match_array[i].module_name, ACTree->match_table[i].pos[j]);
                usb_tty = module_vid_pid_match_array[i].at_tty;
                module_name = module_vid_pid_match_array[i].module_name;
                break;
            }
        }
    }
    if (NULL == usb_tty) {
        //printf("No devices found\n");
        return -1;;
    }

    if (!strcmp(argv[1], "show_module_name")) {
        //获取模组的型号名称
        printf("%s", module_name);
    } else if (!strcmp(argv[1], "show_imei")) {
        hlk_get_module_imei(usb_tty, baud, "AT+GSN\r\n", module_name);
    } else if (!strcmp(argv[1], "show_sim_status")) {
        hlk_get_module_sim_status(usb_tty, baud, "AT+CPIN?\r\n", module_name);
    } else if (!strcmp(argv[1], "show_nettype")) {
        //网络类型 3G/4G/5G
        hlk_get_module_nettype(usb_tty, baud, module_name);
    } else if (!strcmp(argv[1], "show_operator")) {
        //获取运行商名称
        hlk_get_operator(usb_tty, baud, module_name);
    } else if (!strcmp(argv[1], "show_rssi")) {
        hlk_get_rssi(usb_tty, baud, module_name);
    } else if (!strcmp(argv[1], "show_register_status")) {
        hlk_get_reg_status(usb_tty, baud, module_name);
    } else if (!strcmp(argv[1], "show_simcard_num")) {
        hlk_get_simcard_num(usb_tty, baud, module_name);
    } else {
        return -1;
    }

    return -1;
}
