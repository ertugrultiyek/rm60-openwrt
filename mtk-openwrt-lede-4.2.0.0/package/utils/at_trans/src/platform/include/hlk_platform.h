#ifndef _H_HLK_C_PLATFORM_H_
#define _H_HLK_C_PLATFORM_H_

#if defined(HLK_ALG_AC)
#include "../alg/ac/actrie.h"
#endif

#if defined(HLK_ALG_KMP)
#include "../alg/kmp/kmp.h"
#endif

#if defined(HLK_PRODUCT_76X8) || defined(HLK_PRODUCT_7621)

#include "debug.h"
extern int hlk_tool_get_cmd_ret(char *cmd, char *rstr, int len);   
extern int hlk_tool_timerout(struct timeval *last, struct timeval *current, struct timeval *timeout);
extern void hlk_tool_do_system(char *fmt, ...);
extern void hlk_tool_print_hexdump(char *msg, int msg_len, int type);
extern void hlk_tool_sleep(int ms);

#endif  //defined(CONFIG_HLK_PRODUCT_76X8) || definded(CONFIG_HLK_PRODUCT_7621)

#endif  //_H_HLK_C_PLATFORM_H_