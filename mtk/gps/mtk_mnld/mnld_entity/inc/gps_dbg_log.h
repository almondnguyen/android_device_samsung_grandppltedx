#ifndef __GPS_DBG_LOG_H__
#define __GPS_DBG_LOG_H__

#ifdef __cplusplus
extern "C" {
#endif

#define LOG_FILE            "/data/misc/gps/gpsdebug.log"
#define LOG_FILE_PATH       "/data/misc/gps/"
#define PATH_SUFFIX         "mtklog/gpsdbglog/"
#define GPS_DBG_LOG_FILE_NUM_LIMIT 1000
#define MAX_DBG_LOG_DIR_SIZE       (240*1024*1024)
#define MAX_DBG_LOG_FILE_SIZE      (20*1024*1024)

enum {
    MTK_GPS_DISABLE_DEBUG_MSG_WR_BY_MNL = 0x00,
    MTK_GPS_ENABLE_DEBUG_MSG_WR_BY_MNL = 0x01,
    MTK_GPS_DISABLE_DEBUG_MSG_WR_BY_MNLD = 0x10,
    MTK_GPS_ENABLE_DEBUG_MSG_WR_BY_MNLD = 0x11,
};

int gps_dbg_log_thread_init();

int create_mtklogger2mnl_fd();

int mtklogger2mnl_hdlr(int fd);

INT32 gps_log_dir_check(char * dirname);

void gps_stop_dbglog_release_condition(void);

void mtklogger_mped_reboot_message_update(void);

#ifdef __cplusplus
}
#endif

#endif



