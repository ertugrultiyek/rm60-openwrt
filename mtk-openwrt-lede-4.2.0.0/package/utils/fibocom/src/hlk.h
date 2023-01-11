#ifndef _H_HLK_H_
#define _H_HLK_H_

#include <syslog.h>

#define MOBILE_DIR  "/tmp/mobile/"

#define MODULE_NAME_FILE MOBILE_DIR"module_name"
#define SIM_STATUS_FILE MOBILE_DIR"sim_status"
#define IMEI_FILE MOBILE_DIR"imei"
#define SIGNAL_FILE MOBILE_DIR"rssi"
#define OP_FILE MOBILE_DIR"operator"
#define NETTYPE_FILE MOBILE_DIR"nettype"

#define IP_FILE MOBILE_DIR"ipaddr"

int read_str_from_file (char *file_path, char *rstr, int max_len);

int write_str_to_file(char *file_path, char *wstr);
void do_system(char *fmt, ...);


#endif //_H_HLK_H_