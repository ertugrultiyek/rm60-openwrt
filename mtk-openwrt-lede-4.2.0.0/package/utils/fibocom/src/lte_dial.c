#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include "lte_dial.h"
#include "uart.h"
#include <stdio.h>
#include <unistd.h>
#include "string.h"
#include "process_fifo_msg.h"
#include "uci_file.h"
#include "at_tok.h"
#include "hlk.h"

int lte_dial_index = 0;
int fd = 0;                            //文件描述符
int fibocom_module_type = 0;//0:FM150  //1:FM650
char *UART_PATH_FM150 = "/dev/ttyUSB2";
char *UART_PATH_FM650 = "/dev/ttyUSB0";
static int search_cnt = 0;
char rcv_buf[255] = {0};


#define NET_SEARCH_CNT          10

enum
{
    LTE_CHECK_MODULE,
    LTE_CHECK_SIM,
    LTE_CHECK_CSQ,
    LTE_COPS_INFO,
    LTE_CHECK_CEREG,
    LTE_GOBINET_DIAL,
    LTE_CHECK_DIAL,
};

typedef enum{
    NET_CONNECT_SUCCESS =  0,
    NET_NO_SIM          = -1,
    NET_NO_IMSI         = -2,
    NET_NO_CCID         = -3,
    NET_NO_APN          = -4,
    NET_NO_SIGNAL       = -5,
    NET_NO_REGISTER     = -6,
    NET_NO_ACTIVE       = -7,
    NET_DIAL_FAIL       = -8,
}netInitInfo;


static char *CSTT_CHINA_MOBILE  = "AT+CGDCONT=1,\"IPV4V6\",\"CMNET\"\r\n";
static char *CSTT_CHINA_UNICOM  = "AT+CGDCONT=1,\"IPV4V6\",\"3GNET\"\r\n";
static char *CSTT_CHINA_TELECOM = "AT+CGDCONT=1,\"IPV4V6\",\"CTNET\"\r\n";
// static char *CMD_MSG_AT         = "AT\r\n";
// static char *CMD_MSG_ATE0       = "ATE0\r\n";
// static char *CMD_MSG_CPIN       = "AT+CPIN?\r\n";
// static char *CMD_MSG_GTCCINFO   = "AT+GTCCINFO?\r\n";
// static char *CMD_MSG_COPS       = "AT+COPS?\r\n";
int get_dial_sta(void)
{
    return lte_dial_index;
}

void set_dial_sta(int index)
{
    lte_dial_index=index;
    search_cnt=0;
}

int csq2rssi(int csq)
{
	int rssi;

	if (0 == csq) {
		rssi = -113;
	} else if (1 == csq ) {
		rssi = -111;
	} else if (csq >= 2 && csq <= 30) {
		rssi = csq*2 -113;
	} else if (31 == csq) {
		rssi = -51;
	} else {
		rssi = -115;
	}

    return rssi;
}


int lte_net_dial_init(void)
{
    int dial_index=0;
    int ret =0;

    while(1)
    {
        search_cnt++;
        dial_index=get_dial_sta();
        ret =uart_recv(fd,rcv_buf,sizeof(rcv_buf),500);
        memset(rcv_buf,0,sizeof(rcv_buf));
        switch(dial_index)
        {
            case LTE_CHECK_MODULE:
                //ret = uart_send_cmd(fd,"ATE0\r\n",rcv_buf,sizeof(rcv_buf),1000*2);
                //if(strstr(rcv_buf,"OK"))
                //{
                //     set_dial_sta(LTE_CHECK_SIM);
                //     break;
                // }
                // else
                // {
                //     if(search_cnt>NET_SEARCH_CNT)
                //     {
                //         return NET_DIAL_FAIL;
                //     }
                // }
                // sleep(5);
                ret = uart_send_cmd(fd,"AT+CFUN?\r\n",rcv_buf,sizeof(rcv_buf),1000*2);
                if(strstr(rcv_buf,"+CFUN: 1"))
                {
                    // get imei  +CGSN: "862138050245880"
                    memset(rcv_buf,0,sizeof(rcv_buf));
                    ret = uart_send_cmd(fd,"AT+CGSN?\r\n",rcv_buf,sizeof(rcv_buf),1000*2);
                    if(strstr(rcv_buf,"+CGSN: "))
                    {
                        #if 0
                        char *imei_tmp=NULL;
                        imei_tmp=strstr(rcv_buf,"+CGSN: \"");
                        imei_tmp[strlen("+CGSN: \"")+15]=0;
                        printf("moudle imei[%s]\n",imei_tmp);
                        memset(imei_msg,0,sizeof(imei_msg));
                        strcpy(imei_msg,&imei_tmp[strlen("+CGSN: \"")]);
                        #endif
                        char *imei = NULL;
                        int err = 0;

                        err = at_tok_scanf(rcv_buf, "%s", &imei);
                        if (1 == err) {
                        // set_imei_info(imei_msg);
                            if (access(IMEI_FILE, R_OK) != 0) {
                                write_str_to_file(IMEI_FILE, imei);
                            }
                        }
                        
                    }
                    set_dial_sta(LTE_CHECK_SIM);
                    break;
                }
                else
                {
                    ret = uart_send_cmd(fd,"AT+CFUN=1\r\n",rcv_buf,sizeof(rcv_buf),1000*5);
                    if(strstr(rcv_buf,"OK"))
                    {
                        
                    }
                    else
                    {
                        if(search_cnt>NET_SEARCH_CNT)
                        {
                            return NET_DIAL_FAIL;
                        }
                    }
                    
                    sleep(1);
                }
                
            break;

            case LTE_CHECK_SIM:
                printf("\n[%s] [%s] [%d] \n",__FILE__,__FUNCTION__,__LINE__);
                ret = uart_send_cmd(fd,"AT+CPIN?\r\n",rcv_buf,sizeof(rcv_buf),1000*5);
                if(strstr(rcv_buf,"+CPIN: READY"))
                {
                    set_dial_sta(LTE_CHECK_CSQ);
                    strcpy(sim_msg,"SIM READY");
                    // set_sim_info("SIM READY");
                    write_str_to_file(SIM_STATUS_FILE, "SIM READY");

                    break;
                }
                else
                {
                    if(search_cnt>NET_SEARCH_CNT)
                    {
                        write_str_to_file(SIM_STATUS_FILE, "NO READY");
                        // set_sim_info("NO READY");
                        return NET_NO_SIM;
                    }
                }
                sleep(2);
            break;

            case LTE_CHECK_CSQ:
            {
                int rssi=0;
                int rrssi = 0;
                int ber=0;
                int ss_sinr=0;
                int ss_rsrp=0;
                int ss_rsrq=0;
                char str_rssi[10];

                ret = uart_send_cmd(fd,"AT+CSQ?\r\n",rcv_buf,sizeof(rcv_buf),1000*2);
                sscanf(rcv_buf,"\n+CSQ: %d,%d",&rssi,&ber);
                memset(str_rssi,0,sizeof(str_rssi));

                sprintf(str_rssi,"%d",rssi);

                rrssi = csq2rssi(rssi);
                {
                    do_system("echo %d > %s", rrssi, SIGNAL_FILE);
                }
                // set_rssi_info(str_rssi);
                if((rssi==0||rssi==99))
                {
                    if(search_cnt>NET_SEARCH_CNT)
                    {
                        return NET_NO_SIGNAL;
                    }
                }
                else
                {
                    set_dial_sta(LTE_COPS_INFO);
                }

                //AT+CESQ// +CESQ: 99,99,255,255,26,64,255,255,255
                memset(rcv_buf,0,sizeof(rcv_buf));
                ret = uart_send_cmd(fd,"AT+CESQ\r\n",rcv_buf,sizeof(rcv_buf),1000*2);
                sscanf(rcv_buf,"\n+CESQ: %*[^,],%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],%d,%d,%d,\r\n",&ss_rsrq,&ss_rsrp,&ss_sinr);
                printf("lte signal rssi [%d] ss_rsrp [%d]",rssi,ss_rsrp);
                if((ss_rsrp==255)||(ss_rsrp==0))
                {
                    if(search_cnt>NET_SEARCH_CNT)
                    {
                        return NET_NO_SIGNAL;
                    }
                }
                else
                {
                    set_dial_sta(LTE_COPS_INFO);
                }
            }
            break;
            case LTE_COPS_INFO:
            {
                int mode=0;
                int format=0;
                char oper_type[30]={0};
                int act=0;
                char at_cmd_temp[512]={0};
                char mgauth_cmd_temp[512]={0};
                memset(at_cmd_temp,0,sizeof(at_cmd_temp));
                memset(mgauth_cmd_temp,0,sizeof(mgauth_cmd_temp));

                if (f_profile_5g.apn != NULL ) {

                    if ( (f_profile_5g.user != NULL) && (f_profile_5g.pwd != NULL) ) {

                        sprintf(mgauth_cmd_temp, "AT+MGAUTH=1,0,\"%s\",\"%s\"\r\n", f_profile_5g.user, f_profile_5g.pwd);
                        memset(rcv_buf, 0, sizeof(rcv_buf));
                        ret = uart_send_cmd(fd,mgauth_cmd_temp, rcv_buf, sizeof(rcv_buf), 1000*2);
                        if (strstr(rcv_buf, "OK")) {
                            printf("lte_dial set user[%s] pwd[%s] success ",f_profile_5g.user, f_profile_5g.pwd);
                        }
                    }

                    sprintf(at_cmd_temp,"AT+CGDCONT=1,\"IPV4V6\",\"%s\"\r\n", f_profile_5g.apn);
                } else {
                    ret = uart_send_cmd(fd,"AT+COPS?\r\n",rcv_buf,sizeof(rcv_buf),1000*2);
                    memset(oper_type, 0, sizeof(oper_type));
                    sscanf(rcv_buf, "\n+COPS: %d,%d,\"%[^\"]\",%d", &mode, &format, oper_type, &act);
                    // printf("uart mode[%d]format[%d]oper_type[%s]act[%d]\n",mode,format,oper_type,act);

                    memset(rcv_buf, 0, sizeof(rcv_buf));

                    if (strstr(oper_type,"CHINA MOBILE")) {
                        memcpy(at_cmd_temp,CSTT_CHINA_MOBILE,strlen(CSTT_CHINA_MOBILE));
                    } else if (strstr(oper_type,"CHN-UNICOM")) {
                        memcpy(at_cmd_temp,CSTT_CHINA_UNICOM,strlen(CSTT_CHINA_UNICOM));
                    } else if (strstr(oper_type,"CHN-CT")) {
                        memcpy(at_cmd_temp,CSTT_CHINA_TELECOM,strlen(CSTT_CHINA_TELECOM));
                    }

                    if (access(OP_FILE, R_OK) != 0) {
                        write_str_to_file(OP_FILE, oper_type);
                    }
                }
                
                memset(rcv_buf, 0, sizeof(rcv_buf));
                ret = uart_send_cmd(fd, at_cmd_temp, rcv_buf, sizeof(rcv_buf), 1000*2);
                if (strstr(rcv_buf,"OK")) {
                    set_dial_sta(LTE_CHECK_CEREG);
                } else {
                    if (search_cnt > NET_SEARCH_CNT)
                    {
                        return NET_NO_REGISTER;
                    }
                    break;
                }
            }
            break;


            case LTE_CHECK_CEREG:
            {
                int n=0;
                int state=0;
                int n_5g=0;
                int state_5g=0;
                sleep(2);

                ret = uart_send_cmd(fd,"AT+C5GREG?\r\n",rcv_buf,sizeof(rcv_buf),1000*2);
                sscanf(rcv_buf,"\n+C5GREG: %d,%d", &n_5g, &state_5g);
                if(state_5g  == 1 || state_5g == 5) {
                    strcpy(nettype_msg,"5G");
                    set_dial_sta(LTE_CHECK_DIAL);
                    write_str_to_file(NETTYPE_FILE, "5G");
                    break;
                } else {
                    if (search_cnt > NET_SEARCH_CNT)
                    {
                        return NET_NO_REGISTER;
                    }
                }

                ret = uart_send_cmd(fd,"AT+CEREG?\r\n",rcv_buf,sizeof(rcv_buf),1000*2);
                sscanf(rcv_buf,"\n+CEREG: %d,%d",&n,&state);
                //printf("LTE_CHECK_CEREG n[%d] stat[%d]",n,state);
                if (state == 1 || state == 5)
                {
                    strcpy(nettype_msg,"4G");
                    write_str_to_file(NETTYPE_FILE, "4G");
                    set_dial_sta(LTE_CHECK_DIAL);
                }
                else
                {
                    if(search_cnt>NET_SEARCH_CNT)
                    {
                        return NET_NO_REGISTER;
                    }
                }
            }
            break;

            case LTE_GOBINET_DIAL:

            if (fibocom_module_type==0) {
                 ret = uart_send_cmd(fd,"AT$QCRMCALL=1,1,3\r\n",rcv_buf,sizeof(rcv_buf),1000*10);
                printf("LTE_GOBINET_DIAL recv[%s]\n",rcv_buf);
                if(strstr(rcv_buf,"$QCRMCALL:"))
                {
                    set_dial_sta(LTE_CHECK_DIAL);
                    break;
                }
                else
                {
                    if(search_cnt>NET_SEARCH_CNT)
                    {
                        return NET_DIAL_FAIL;
                    }
                }

             }
             else
             {
                 ret = uart_send_cmd(fd,"AT+GTRNDIS=1,1\r\n",rcv_buf,sizeof(rcv_buf),1000*10);
                //printf("LTE_GOBINET_DIAL recv[%s]\n",rcv_buf);
                if(strstr(rcv_buf,"OK"))
                {
                    set_dial_sta(LTE_CHECK_DIAL);
                    break;
                }
                else
                {
                    if(search_cnt>NET_SEARCH_CNT)
                    {
                        return NET_DIAL_FAIL;
                    }
                }

             }
             sleep(5);
            break;

            case LTE_CHECK_DIAL:

            if(fibocom_module_type==0)
            {
                ret = uart_send_cmd(fd,"AT$QCRMCALL?\r\n",rcv_buf,sizeof(rcv_buf),1000*2);
                //printf("LTE_CHECK_DIAL rcv_buf[%s]\n",rcv_buf);
                if(strstr(rcv_buf,"$QCRMCALL"))
                {
                    system("ifconfig usb0 down");
                    sleep(1);
                    system("ifconfig usb0 up");
                    sleep(4);
                     {
                        char udhcpc_cmd[128];
                        char cmd_res_buf[256] = {0};
                        FILE * udhcpc_fp;
                        int rc = -1;

                        //snprintf(udhcpc_cmd, sizeof(udhcpc_cmd), "busybox udhcpc -f -n -q -t 5 -i usb0 -s /lib/netifd/dhcp.script");
                        snprintf(udhcpc_cmd, sizeof(udhcpc_cmd), "busybox udhcpc -f -n -q -t 5 -i usb0");
                        printf("exec: %s \n", udhcpc_cmd);
                        //system(udhcpc_cmd);
                        do {
                            rc = 0;
                            udhcpc_fp = popen(udhcpc_cmd, "r");
                            if (udhcpc_fp) {
                                char buf[0xff];

                                buf[sizeof(buf)-1] = '\0';
                                while ( (fgets(buf, sizeof(buf)-1, udhcpc_fp)) != NULL) {
                                    if (strstr(buf, "Command failed: Not found")) {
                                        //udhcpc 执行失败 重新执行 
                                        printf("Command failed: Not found re exec udhcpc\n");
                                        sleep(1);
                                        rc = 1;
                                        break;
                                    }
                                }

                                pclose(udhcpc_fp);
                            }
                        } while (rc != 0);
                    } 

                    return NET_CONNECT_SUCCESS;
                }
                else
                {
                    set_dial_sta(LTE_GOBINET_DIAL);
                }
            }
            else
            {
                char *p = NULL;
                ret = uart_send_cmd(fd,"AT+GTRNDIS?\r\n",rcv_buf,sizeof(rcv_buf),1000*2);
                //printf("LTE_CHECK_DIAL rcv_buf[%s]\n",rcv_buf);
                p = strstr(rcv_buf,"+GTRNDIS: 1");
                if (p != NULL) {
                    system("ifconfig usb0 down");
                    sleep(1);
                    system("ifconfig usb0 up");
                    sleep(3);
                    {
                        char udhcpc_cmd[128];
                        char cmd_res_buf[256] = {0};
                        FILE * udhcpc_fp;
                        int rc = -1;

                        //snprintf(udhcpc_cmd, sizeof(udhcpc_cmd), "busybox udhcpc -f -n -q -t 5 -i usb0 -s /lib/netifd/dhcp.script");
                        snprintf(udhcpc_cmd, sizeof(udhcpc_cmd), "busybox udhcpc -f -n -q -t 5 -i usb0");
                        printf("exec: %s \n", udhcpc_cmd);
                        do {
                            rc = 0;
                            udhcpc_fp = popen(udhcpc_cmd, "r");
                            if (udhcpc_fp) {
                                char buf[0xff];

                                buf[sizeof(buf)-1] = '\0';
                                while ( (fgets(buf, sizeof(buf)-1, udhcpc_fp)) != NULL) {
                                    if (strstr(buf, "Command failed: Not found")) {
                                        //udhcpc 执行失败 重新执行 
                                        printf("Command failed: Not found re exec udhcpc\n");
                                        sleep(1);
                                        rc = 1;
                                        break;
                                    }
                                }

                                pclose(udhcpc_fp);
                            }
                        } while (rc != 0);
                    } 
                    {
                        int comma = 0;
                        char *p1, *p2, *p3;
                        //解析DNS
                        p1 = p;
                        //p1 += strlen("+GTRNDIS: ");
                        //printf("GTRNDIS str: %s\n", p1);
                        int state;
                        int cid;
                        char *IP = NULL;
                        char *DNS1 = NULL;
                        char *DNS2 = NULL;
                        int err;
                        p2 = strchr(p1, '\n');
                        if (p2 != NULL) *p2 = '\0';
                        p2 = strchr(p1, '\r');
                        if (p2 != NULL) *p2 = '\0';
                        //printf("After GTRNDIS str: %s\n", p1);

                        err = at_tok_scanf(p1, "%d%d%s%s%s", &state, &cid, &IP, &DNS1, &DNS2);
                        if (err < 5 ) {
                            printf("at_tok_scanf failed err:%d\n", err);
                        } else {
                            printf("state:%d cid:%d IP:%s DNS1:%s DNS2:%s\n", state, cid, IP, DNS1, DNS2);
                            char *pdns_ipv4, *sdns_ipv4;
                            if ( (p3 = strchr(DNS1, ',')) != NULL) *p3 = '\0';
                            if ( (p3 = strchr(DNS2, ',')) != NULL) *p3 = '\0';
                            printf("ipv4 pdns :%s snds:%s\n", DNS1, DNS2);
                            //设置DNS 到/tmp/
                            do_system("echo \"nameserver %s\" > /tmp/resolv.conf.auto", DNS1);
                            do_system("echo \"nameserver %s\" >> /tmp/resolv.conf.auto", DNS1);
                        }
                    }

                    printf("####################net connect success\n");
                    return NET_CONNECT_SUCCESS;
                }
                else
                {
                    set_dial_sta(LTE_GOBINET_DIAL);
                }
            }
                
            break;
        }
        usleep(1000*300);
    }
    return 0;
}
  

void * lte_dial_task(void* arg)
{
    //  uart_init
    char at_rcv[255]={0};
    int ret =0;

lte_dial_enter:
    memset(imei_msg,0,sizeof(imei_msg));
    strcpy(imei_msg,"UNKNOWN");
    if(fibocom_module_type==0) //fm150
    {
        fd = uart_open(UART_PATH_FM150); //打开串口，返回文件描述符
        if(fd<0)
        {
            printf("open %s fail [%d]\n",UART_PATH_FM150,fd);
            sleep(1);
            goto lte_dial_enter;
        }
    }
    else    //fm650
    {
        fd = uart_open(UART_PATH_FM650); //打开串口，返回文件描述符
        if(fd<0)
        {
            printf("open %s fail [%d]\n",UART_PATH_FM650,fd);
            sleep(1);
            goto lte_dial_enter;
        }
    }
    
        ret = uart_init(fd,115200,0,8,1,'N');
        if(ret<0)
        {
            uart_close(fd);
            sleep(2);
            goto lte_dial_enter;
        }

    sleep(1);
    //close echo
    ret = uart_send_cmd(fd,"ATE0\r\n",rcv_buf,sizeof(rcv_buf),1000*2);
    //ATI
    memset(rcv_buf,0,sizeof(rcv_buf));
    ret = uart_send_cmd(fd,"ATI\r\n",rcv_buf,sizeof(rcv_buf),1000*2);
    switch(fibocom_module_type)
    {
        case 0:  //FM150
            if(strstr(rcv_buf,"FM150"))
            {

            }
            else
            {
                fibocom_module_type=1;
                uart_close(fd);
                sleep(1);
                goto lte_dial_enter;
            }
        break;
        case 1: //FM650
            if(strstr(rcv_buf,"FM650"))
            {
                memset(rcv_buf,0,sizeof(rcv_buf));
                ret = uart_send_cmd(fd,"AT+GTUSBMODE=37\r\n",rcv_buf,sizeof(rcv_buf),1000*2);  
            }
            else
            {
                fibocom_module_type=0;
                uart_close(fd);
                sleep(1);
                goto lte_dial_enter;
            }
        break;

        default:
        break;
    }
    
    memset(rcv_buf,0,sizeof(rcv_buf));
    ret = uart_send_cmd(fd,"AT+GTAUTOCONNECT=0\r\n",rcv_buf,sizeof(rcv_buf),1000*2);
    // if(strstr(rcv_buf,"OK"))
    // {
    //    
    // }
//hilink default SIM2
    memset(rcv_buf,0,sizeof(rcv_buf));
    ret = uart_send_cmd(fd,"AT+GTDUALSIM=1\r\n",rcv_buf,sizeof(rcv_buf),1000*2);
    if(strstr(rcv_buf,"OK"))
    {
       printf("set sim2 success\n"); 
    }

    while(1)
    {

//value:    CONNECT   DISCONECT
        //uci_load_file();
        // set_netstatus_info("DISCONNECT");
        memset(netstatus_msg,0,sizeof(netstatus_msg));
        strcpy(netstatus_msg,"DISCONNECT");
        memset(sim_msg,0,sizeof(sim_msg));
        strcpy(sim_msg,"NO SIM");
        set_dial_sta(LTE_CHECK_MODULE);


        ret = lte_net_dial_init();
        printf("dial result ret [%d]\n",ret);
        switch(ret)
        {
            case NET_CONNECT_SUCCESS:
			// set_netstatus_info("CONNECT");
            memset(netstatus_msg,0,sizeof(netstatus_msg));
            strcpy(netstatus_msg,"CONNECT");
                while(1)
                {
                    //check dial status
                    {
                        int n=0;
                        int stat=0;
                        int n_5g=0;
                        int state_5g=0;
                        memset(at_rcv,0,sizeof(at_rcv));
                        ret = uart_send_cmd(fd,"AT+CEREG?\r\n",at_rcv,sizeof(at_rcv),1000*2);
                        sscanf(at_rcv,"\n+CEREG: %d,%d",&n,&stat);
                        printf("auto check n[%d] stat[%d]\n",n,stat);
                        sleep(1);
                        memset(at_rcv,0,sizeof(at_rcv));
                        ret = uart_send_cmd(fd,"AT+C5GREG?\r\n",at_rcv,sizeof(at_rcv),1000*2);
                        sscanf(at_rcv,"\n+C5GREG: %d,%d",&n_5g,&state_5g);
                        printf("auto check n_5g[%d] state_5g[%d]\n",n_5g,state_5g);

                        memset(nettype_msg,0,sizeof(nettype_msg));
                        if(((stat==1)||(stat==5))&&((state_5g==1)||(state_5g==5)))
                        {
                            //5G
                            strcpy(nettype_msg,"5G");
                        
                        }
                        else if(((stat==1||stat==5)==0)&&((state_5g==1||state_5g==5)))
                        {
                            //5G
                            strcpy(nettype_msg,"5G");
                        }
                        else if((stat==1||stat==5)&&((state_5g==1||state_5g==5)==0))
                        {
                            strcpy(nettype_msg,"4G");
                            //4G
                        }
                        else
                        {
                            strcpy(nettype_msg,"UNKNOWN");
                            //unknown
                        }
                    }
                    sleep(1);
                    //CSQ
                    memset(rssi_msg,0,sizeof(rssi_msg));
                    if(strstr(nettype_msg,"5G"))
                    {
                        int ss_sinr=0;
                        int ss_rsrp=0;
                        int ss_rsrq=0;
                        char str_ss_rsrp[10];
                        
                        // ss_rsrp   //AT+CESQ// +CESQ: 99,99,255,255,26,64,255,255,255
                        memset(at_rcv,0,sizeof(at_rcv));
                        ret = uart_send_cmd(fd,"AT+CESQ\r\n",at_rcv,sizeof(at_rcv),1000*2);
                        sscanf(at_rcv,"\n+CESQ: %*[^,],%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],%d,%d,%d,\r\n",&ss_rsrq,&ss_rsrp,&ss_sinr);
                        printf("lte signal ss_rsrp [%d]",ss_rsrp);

                        memset(str_ss_rsrp,0,sizeof(str_ss_rsrp));
                        sprintf(str_ss_rsrp,"%d",ss_rsrp);
                        strcpy(rssi_msg,str_ss_rsrp);

                    }
                    else if(strstr(nettype_msg,"4G"))
                    {
                        int rssi=0;
                        int ber=0;
                        char str_rssi[10];
                        //RSSI
                        memset(at_rcv,0,sizeof(at_rcv));
                        ret = uart_send_cmd(fd,"AT+CSQ?\r\n",at_rcv,sizeof(at_rcv),1000*2);
                        sscanf(at_rcv,"\n+CSQ: %d,%d",&rssi,&ber);
                        memset(str_rssi,0,sizeof(str_rssi));
                        sprintf(str_rssi,"%d",rssi);
                        strcpy(rssi_msg,str_rssi);
                    }
                    else
                    {
                        strcpy(rssi_msg,"UNKNOWN");
                    }
                    sleep(5);
                    //check dial
                     memset(at_rcv,0,sizeof(at_rcv));
                    if(fibocom_module_type==0) //fm150
                    {
                        ret = uart_send_cmd(fd,"AT$QCRMCALL?\r\n",at_rcv,sizeof(at_rcv),1000*3);
                            if(strstr(at_rcv,"$QCRMCALL: 1,V4")==NULL)
                            {
                                break;
                            }
                    }
                    else
                    {
                        ret = uart_send_cmd(fd,"AT+GTRNDIS?\r\n",at_rcv,sizeof(at_rcv),1000*2);
                        //printf("LTE_CHECK_DIAL at_rcv[%s]\n",at_rcv);
                        if(strstr(at_rcv,"+GTRNDIS: 1")==NULL)
                        {
                            break;
                        }
                    }
                    sleep(2);
                }

            break;
            case NET_NO_REGISTER:
            case NET_NO_ACTIVE:
            case NET_DIAL_FAIL:
                memset(at_rcv,0,sizeof(at_rcv));
                ret = uart_send_cmd(fd,"AT+CFUN=0\r\n",at_rcv,sizeof(at_rcv),1000*5);
                if(strstr(at_rcv,"OK"))
                {

                }
                 sleep(2);
                 int cfun_cnt=0;
                while(1)
                {  
                    memset(at_rcv,0,sizeof(at_rcv));
                    ret = uart_send_cmd(fd,"AT+CFUN=1\r\n",at_rcv,sizeof(at_rcv),1000*5);
                    if(strstr(at_rcv,"OK"))
                    {
                        
                    }
                    sleep(1);
                    memset(at_rcv,0,sizeof(at_rcv));
                    ret = uart_send_cmd(fd,"AT+CFUN?\r\n",at_rcv,sizeof(at_rcv),1000*5);
                    if(strstr(at_rcv,"+CFUN: 1"))
                    {
                        break;
                    }

                    cfun_cnt++;
                    if(cfun_cnt>10)
                    {
                        cfun_cnt=0;
                        ret = uart_send_cmd(fd,"AT+CFUN=15\r\n",at_rcv,sizeof(at_rcv),1000*5);
                        uart_close(fd);
                        sleep(2);
                        goto lte_dial_enter;

                    }
                }
            break;

        }
    }
}