#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include <unistd.h>

#include <fcntl.h>   /* File Control Definitions           */
#include <termios.h> /* POSIX Terminal Control Definitions */
#include <unistd.h>  /* UNIX Standard Definitions 	   */
#include <errno.h>   /* ERROR Number Definitions           */
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <libubox/uloop.h>

#include "uart.h"
#include "lte_dial.h"
#include "uci_file.h"
#include "process_fifo_msg.h"

#define CONFIG_DEFAULT_PDP 1
FILE *logfilefp = NULL;

#define dbg_time(fmt, args...) do { \
    fprintf(stdout, "%s:%d " fmt "\n", __FUNCTION__, __LINE__, ##args); \
    if (logfilefp) fprintf(logfilefp, "%s:%d " fmt "\n", __FUNCTION__, __LINE__, ##args); \
} while(0)


static int usage(const char *progname) {
    dbg_time("Usage: %s [options]", progname);
    dbg_time("-s [apn [user password auth]]          Set apn/user/password/auth get from your network provider. auth: 1~pap, 2~chap, 3~MsChapV2");
    dbg_time("-p pincode                             Verify sim card pin if sim card is locked");
    dbg_time("-f logfilename                         Save log message of this program to file");
    dbg_time("-4                                     Setup IPv4 data call (default)");
    dbg_time("-6                                     Setup IPv6 data call");
    return 0;
}


static int parse_user_input(int argc, char **argv, profile_t *profile) {
    int opt = 1;

    profile->pdp = CONFIG_DEFAULT_PDP;

    if (!strcmp(argv[argc-1], "&"))
        argc--;

#define has_more_argv() ((opt < argc) && (argv[opt][0] != '-'))
    while  (opt < argc) {
        if (argv[opt][0] != '-') {
            return usage(argv[0]);
        }

        switch (argv[opt++][1])
        {
            case 's':
                profile->apn = profile->user = profile->pwd = "";
                if (has_more_argv()) {
                    profile->apn = argv[opt++];
                }
                if (has_more_argv()) {
                    profile->user = argv[opt++];
                }
                if (has_more_argv()) {
                    profile->pwd = argv[opt++];
                }
            break;
            case 'p':
                if (has_more_argv()) {
                    const char *arg = argv[opt++];

                    if ((999 < atoi(arg)) && (atoi(arg) < 10000)) {
                        profile->pin = arg;
                    } else {
                        dbg_time("unknow -p '%s'", arg);
                        return usage(argv[0]);
                    }
                }
            break;

            case 'n':
                if (has_more_argv())
                    profile->pdp = argv[opt++][0] - '0';
            break;

            case 'f':
                if (has_more_argv()) {
                    profile->logfile = argv[opt++];
                }
            break;

            case '4':
                profile->enable_ipv4 = 1; 
            break;

            case '6':
                profile->enable_ipv6 = 1;
            break;
            
            default:
                return usage(argv[0]);
            break;
        }
    }

    if (profile->enable_ipv4 != 1 && profile->enable_ipv6 != 1) { // default enable IPv4
        profile->enable_ipv4 = 1;
    }

    return 1;
}

int main(int argc, char **argv)
{

  #if 0 
    pid_t pid=fork();
    if(pid<0)
    {
      //The child thread
      printf("\n app lte fork fail pid[%d]\n",pid);
      return -1;
    }
    else if(pid>0)
    {
      printf("\n app fork The parent thread exit pid [%d] \n",pid);
      exit(0); //退出父线程
    }
    printf("\n app fork The child thread init pid [%d] \n",pid);
    #endif

    int ret = 0;

    printf("*************************************\n");
    printf("user app versions:V100.20.06.22\n");
    printf("*************************************\n");

    printf("\n app lte pthread_create 123! \n");

    //apn user psswd pincode
    ret = parse_user_input(argc, argv, &f_profile_5g);
    if (!ret) {
        return ret;
    }

    lte_fifo_enter();
    lte_dial_task(NULL);

    sleep(1);

    printf("\n app lte pthread_end!\n");

    return 1;
}
