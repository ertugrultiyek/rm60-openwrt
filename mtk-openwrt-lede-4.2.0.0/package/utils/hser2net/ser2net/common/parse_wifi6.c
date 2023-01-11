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
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

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
#include <net/if.h>
#include <linux/if_ether.h>
#include <sys/types.h>
#include <unistd.h>

#include "pthread.h"

#include <sys/socket.h>
#include <sys/un.h>

#include <unistd.h>

#include <sys/msg.h>

#include <signal.h>

#include <net/route.h>

#include "ser2net.h"

#define CONF_NUM 512

int wifi6_keyval_cnt = 0;
struct wifi6_keyval *wifi6_conf = NULL;


int wifi6_set_param(struct wifi6_keyval *wifi_conf, char *key, char *val)
{
    int i = 0;
    debug_log("wifi6_keyval_cnt :%d", wifi6_keyval_cnt);

    for (i = 0; i < wifi6_keyval_cnt; i++) {
        if (!strcmp(wifi_conf[i].key, key)) {
            if (wifi_conf[i].val != NULL) {
                free(wifi_conf[i].val); wifi_conf[i].val = NULL;
            }
            wifi_conf[i].val = strdup(val);
            return 0;
        }
    }

    //没找到
    //添加新的参数
    if (i < CONF_NUM) {
        wifi_conf[i].key = strdup(key);
        wifi_conf[i].val = strdup(val);
        wifi6_keyval_cnt = i+1;
        debug_log("wifi6_keyval_cnt :%d", wifi6_keyval_cnt);
    } else {
        return -1;
    }
    
    return 0;
}

int wifi6_param_store(char *path)
{
    //file header
    char buff[512] = {0};

    if (NULL == wifi6_conf ) {
        return -1;
    }

    int i;
    char *file_header = "#The word of \"Default\" must not be removed\nDefault";

    int fd = open(path, O_RDWR|O_TRUNC);

    write(fd, file_header, strlen(file_header));write(fd, "\n", 1);
    
    for (i = 0; i < wifi6_keyval_cnt; i++) {
        if (NULL == wifi6_conf[i].val) {
            sprintf(buff, "%s=", wifi6_conf[i].key);
        } else {
            sprintf(buff, "%s=%s", wifi6_conf[i].key, wifi6_conf[i].val);
        }
        
        write(fd, buff, strlen(buff));write(fd, "\n", 1);
    }

    close(fd);

    for (i = 0; i < wifi6_keyval_cnt; i++) {
        if (wifi6_conf[i].key != NULL) {
            free(wifi6_conf[i].key);
        } 
        if (wifi6_conf[i].val != NULL) {
            free(wifi6_conf[i].val);
        }
    }
    free(wifi6_conf);
    wifi6_conf = NULL;

    return 0;
}

//加载WIFI6配置文件到内存
struct wifi6_keyval *load_wifi6_profile(char *path)
{
    int ret = -1;
    int fd = -1;
    struct stat buf;
    char *filecontent = NULL;
    char *pdef = "Default\n";
    struct wifi6_keyval *wifi6_cfg = NULL;
    int index = 0;
    off_t offset = 0;
    char *p = NULL;
    char *p1 = NULL;
    char *p2 = NULL;

    lstat(path, &buf);

    fd = open(path, O_RDONLY);
    if (fd < 0) {
        goto end;
    }

    offset = lseek(fd, 0, SEEK_END);
    if (offset < 0) {
        goto end;
    }

    if (0 == buf.st_size ) {
        debug_log("file size is 0");
        goto end;
    }
    debug_log("offset seek set :%d", lseek(fd, 0, SEEK_SET));

    filecontent = malloc(offset);

    /*格式 xxx=xx
    #The word of "Default" must not be removed
    Default
    AccessControlList0=
    AccessControlList1=
    AccessControlList10=
    AccessControlList11=
    AccessControlList12=
    */
    ret = read(fd, filecontent, offset);
    if (ret < 0) {
        goto end;
    }

    p = strstr(filecontent, pdef);
    if (NULL == p) {
        goto end;
    }

    wifi6_cfg = malloc(sizeof(struct wifi6_keyval)*CONF_NUM);
    p += (strlen("Default\n")+1);

    do {
        p1 = strchr(p, '\n');
        if (NULL == p1) {
            if (p != filecontent+offset) {
                p += 1;
                continue;
            }
            break;
        }
        *p1 = '\0';

        p2 = strchr(p, '=');
        if (NULL == p2) {
            if (p != filecontent+offset) {
                p += 1;
                continue;
            }
            break;
        }
        *p2 = '\0';
        wifi6_cfg[index].key = strdup(p);

        if (*(p2+1) != '\0') {
            wifi6_cfg[index].val = strdup(p2+1);
        } else {
            wifi6_cfg[index].val = NULL;
        }
        //debug_log("index:%d key:%s val:%s", index, wifi6_cfg[index].key, wifi6_cfg[index].val);
        p = p1+1;
        index++;
    } while (1);

    wifi6_keyval_cnt = index;
   
end:
    if (filecontent != NULL) {
        free(filecontent);filecontent = NULL;
    }

    if (fd > 0) {
        close(fd);
    }

    return wifi6_cfg;
}