#include <cutils/log.h>

struct xlog_record {
    const char *tag_str;
    const char *fmt_str;
    int prio;
};

void __attribute__((weak)) __xlog_buf_printf(__unused int bufid, const struct xlog_record *xlog_record, ...) {
    va_list args;
    va_start(args, xlog_record);
#if    HAVE_LIBC_SYSTEM_PROPERTIES
    int len = 0;
    int do_xlog = 0;
    char results[PROP_VALUE_MAX];


    // MobileLog
    len = __system_property_get ("debug.MB.running", results);
    if (len && atoi(results))
        do_xlog = 1;

    // ModemLog
    len = __system_property_get ("debug.mdlogger.Running", results);
    if (len && atoi(results))
        do_xlog = 1;

    // Manual
    len = __system_property_get ("persist.debug.xlog.enable", results);
    if (len && atoi(results))
        do_xlog = 1;

    if (do_xlog > 0)
#endif
        __android_log_vprint(xlog_record->prio, xlog_record->tag_str, xlog_record->fmt_str, args);

    return;
}
