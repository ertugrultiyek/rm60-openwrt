#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <uci/uci.h>


static struct uci_context * ctx = NULL; //定义一个UCI上下文的静态变量.


#ifndef debug_log
#include <syslog.h>

#define debug_log(fmt, arg...) \
do {\
	fprintf(stdout, "%s %d "fmt "\r\n", __FUNCTION__,__LINE__, ##arg); \
    syslog(LOG_DEBUG, "%s %d "fmt "\r\n", __FUNCTION__,__LINE__,##arg);\
} while (0)

#endif

#ifndef bool
#define bool
#endif
/*********************************************
* 载入配置文件,并遍历Section.
*/

char *uci_get_option_val(char *uci_config_file, char *section_name, char *option_name)
{
    char *res = NULL;
    struct uci_package * pkg = NULL;
    struct uci_element *e;
    char *value = NULL;
    
    ctx = uci_alloc_context(); // 申请一个UCI上下文.
    if (NULL == ctx ) {
        debug_log("");
        goto cleanup;
    }

    if (UCI_OK != uci_load(ctx, uci_config_file, &pkg)) {
        debug_log("");
        goto cleanup; //如果打开UCI文件失败,则跳到末尾 清理 UCI 上下文.
    }
     
    /*遍历UCI的每一个节*/
    uci_foreach_element(&pkg->sections, e) {
        struct uci_section *s = uci_to_section(e);
        if (!strcmp(s->e.name, section_name)) {
            debug_log("section:%s option val :%s", s->e.name, uci_lookup_option_string(ctx, s, option_name));
            if (NULL != (value = uci_lookup_option_string(ctx, s, option_name))) {
                res = strdup(value); //如果您想持有该变量值，一定要拷贝一份。当 pkg销毁后value的内存会被释放。
            }
        }
        // 将一个 element 转换为 section类型, 如果节点有名字,则 s->anonymous 为 false.
        // 此时通过 s->e->name 来获取.
        // 此时 您可以通过 uci_lookup_option()来获取 当前节下的一个值.
        // 如果您不确定是 string类型 可以先使用 uci_lookup_option() 函数得到Option 然后再判断.
        // Option 的类型有 UCI_TYPE_STRING 和 UCI_TYPE_LIST 两种.
    }

cleanup:

    debug_log("");

    if (pkg != NULL) {
        uci_unload(ctx, pkg); // 释放 pkg
    }

    uci_free_context(ctx);
    ctx = NULL;

    return res;
}