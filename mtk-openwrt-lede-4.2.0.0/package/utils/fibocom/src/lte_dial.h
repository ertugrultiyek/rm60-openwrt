#ifndef _LTE_DIAL_H_
#define _LTE_DIAL_H_

typedef struct {
    int pdp;
    char *pin;
    char *user;
    char *pwd;
    char *apn;
    const char *logfile;
    int enable_ipv4;
    int enable_ipv6;

    char enable_5g[10];
} profile_t;

extern profile_t f_profile_5g;

void * lte_dial_task(void* arg);
#endif // !_LTE_DIAL_H_