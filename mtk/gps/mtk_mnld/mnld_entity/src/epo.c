#include <errno.h>   /* Error number definitions */
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "agps_agent.h"
#include "data_coder.h"
#include "mnl_common.h"
#include "mnld.h"
#include "mtk_lbs_utility.h"
#include "epo.h"
#include "qepo.h"
#include "curl.h"
#include "easy.h"

#ifdef LOGD
#undef LOGD
#endif
#ifdef LOGW
#undef LOGW
#endif
#ifdef LOGE
#undef LOGE
#endif
#if 0
#define LOGD(...) tag_log(1, "[epo]", __VA_ARGS__);
#define LOGW(...) tag_log(1, "[epo] WARNING: ", __VA_ARGS__);
#define LOGE(...) tag_log(1, "[epo] ERR: ", __VA_ARGS__);
#else
#define LOG_TAG "epo"
#include <cutils/sockets.h>
#include <cutils/log.h>     /*logging in logcat*/
#define LOGD(fmt, arg ...) ALOGD("%s: " fmt, __FUNCTION__ , ##arg)
#define LOGW(fmt, arg ...) ALOGW("%s: " fmt, __FUNCTION__ , ##arg)
#define LOGE(fmt, arg ...) ALOGE("%s: " fmt, __FUNCTION__ , ##arg)
#endif
#define GPS_EPO_FILE_LEN  20
#define MTK_EPO_MAX_DAY      30
#define MTK_EPO_ONE_SV_SIZE  72
#define GPS_CONF_FILE_SIZE 100
#define EPO_CONTROL_FILE_PATH "/data/misc/gps/gps.conf"
#define IS_SPACE(ch) ((ch == ' ') || (ch == '\t') || (ch == '\n'))

static int epo_data_updated = 0;
static int gps_epo_period = 3;
static int wifi_epo_period = 3;
static int gps_epo_download_days = 9;
static int gps_epo_enable = 1;
static int gps_epo_wifi_trigger = 0;
static int gps_epo_file_count = 0;
static char gps_epo_file_name[GPS_EPO_FILE_LEN] = {0};
static char gps_epo_md_file_name[GPS_EPO_FILE_LEN] = {0};
int gps_epo_type = 0;    // 0 for G+G;1 for GPS only, default is G+G
static int epo_download_failed = 0;
static int epo_download_retry = 1;
static timer_t retry_download_timer;
//static timer_t hdlr_timer;

static int epo_file_update_impl();
static int mtk_epo_is_expired(int wifi_tragger);
static void gps_download_epo_file_name(int count);
static int mtk_gps_epo_file_update();
static int mtk_gps_epo_file_time_hal(time_t uTime[]);

typedef enum {
    MAIN2EPO_EVENT_START            = 0,
} main2epo_event;

static int g_fd_epo;

/////////////////////////////////////////////////////////////////////////////////
// static functions
void epo_update_epo_file() {
    if (mtk_agps_agent_epo_file_update() == MTK_GPS_ERROR) {
        LOGE("EPO file updates fail\n");
    } else {
        unlink(EPO_UPDATE_HAL);
    }
}
static int epo_file_update_retry(void) {
    static int delay_time = 0;

    if (mnld_is_gps_started() || (is_wifi_network_connected())) {
        if (is_network_connected() && epo_download_retry) {
            // if download has failed last time, we should complete downloading.
            time_t uTime[3] ={0};
            // time_t time_st;
            time_t         now = time(NULL);
            struct tm      tm_utc;
            time_t       time_utc;
            int file_count_temp = 0;
            static int file_retrying = 50;   // 50 for first restore
            static int retry_time = 0;
            int ret = 0;
            if (file_retrying == 50) {
                file_retrying = gps_epo_file_count;
            }
            file_count_temp = gps_epo_file_count;
            //LOGD("EPO data download resume...file_count_temp=%d\n", file_count_temp);
            // time(&time_st);
            gmtime_r(&now, &tm_utc);
            time_utc = mktime(&tm_utc);
            ret = mtk_gps_epo_file_time_hal(uTime);

            if ((ret >= 0) && (time_utc >= (uTime[0] + 24*60*60))) {
                // if epo date is expired > 1 day, we should begin with first file.
                epo_download_failed = 0;
                gps_epo_file_count = 0;
                epo_data_updated = 1;
                unlink(EPO_UPDATE_HAL);
                delay_time = 0;
            } else {   // if epo data is expired < 1 day, we can begin with failed file last time.
                gps_download_epo_file_name(gps_epo_file_count);
                mtk_gps_epo_file_update();
            }
            LOGD("gps_epo_file_count=%d\n", gps_epo_file_count);
            if (file_count_temp == gps_epo_file_count) {
                int time_out = 15;  // delay 15 s

                if (file_retrying == gps_epo_file_count) {
                    retry_time++;
                } else {
                    file_retrying = gps_epo_file_count;
                    retry_time = 0;
                }
                LOGD("retry_time=%d, time_out=%d\n", retry_time, time_out);
                if (retry_time < 10) {
                    epo_download_retry = 0;
                    start_timer(retry_download_timer, MNLD_EPO_RETRY_HANDLER_TIMEOUT);
                } else {
                    epo_download_failed = 0;
                    retry_time = 0;
                    file_retrying = 50;
                    delay_time = 0;
                }
            }
        } else if (epo_download_retry == 1) {
            if (delay_time >= 10) {
                epo_download_failed = 0;
                delay_time = 0;
            } else {
                delay_time ++;
                usleep(100*1000);
            }
        }
    } else {
        epo_download_failed = 0;
        delay_time = 0;
    }
    return 0;
}

static int epo_file_update_impl() {
    int schedule_delay = 100;
    epo_data_updated = 1;
    epo_download_failed = 0;
    gps_epo_file_count = 0;
    unlink(EPO_UPDATE_HAL);
    unlink(EPO_FILE);
    while (1) {
        if (is_qepo_download_finished()) {
            if (epo_data_updated == 1) {
                //LOGD("EPO data download begin...");
                epo_download_failed = 0;
                gps_download_epo_file_name(gps_epo_file_count);
                mtk_gps_epo_file_update();
                if (epo_download_failed == 1) {
                    epo_data_updated = 0;
                }
            } else if (epo_download_failed == 1) {
                int ret = 0;
                ret = epo_file_update_retry();
            } else {
                // end of downalod epo
                LOGD("end of download epo\n");
                break;
            }
        } else {
            //start_timer(hdlr_timer, MNLD_EPO_HANDLER_TIMEOUT);
        }
        usleep(schedule_delay*1000);
    }
    LOGD("download epo ret\n");
    if (gps_epo_file_count != 0) {
        return EPO_DOWNLOAD_RESULT_FAIL;
    }

    return EPO_DOWNLOAD_RESULT_SUCCESS;  // success
}

/*****************************************************************************/
static int get_val(char *pStr, char** ppKey, char** ppVal) {
    int len = (int)strlen(pStr);
    char *end = pStr + len;
    char *key = NULL, *val = NULL;

    LOGD("pStr = %s, len=%d!!\n", pStr, len);

    if (!len) {
        return -1;       // no data
    } else if (pStr[0] == '#') {   /*ignore comment*/
        *ppKey = *ppVal = NULL;
        return 0;
    } else if (pStr[len-1] != '\n') {
        if (len >= GPS_CONF_FILE_SIZE-1) {
            LOGD("buffer is not enough!!\n");
            return -1;
        } else {
            pStr[len] = '\n';
        }
    }
    key = pStr;

    LOGD("key = %s!!\n", key);
    while ((*pStr != '=') && (pStr < end)) pStr++;
    if (pStr >= end) {
        LOGD("'=' is not found!!\n");
        return -1;       // format error
    }

    *pStr++ = '\0';
    while (IS_SPACE(*pStr) && (pStr < end)) pStr++;       // skip space chars
    val = pStr;
    while (!IS_SPACE(*pStr) && (pStr < end)) pStr++;
    *pStr = '\0';
    *ppKey = key;
    *ppVal = val;

    LOGD("val = %s!!\n", val);
    return 0;
}

/*****************************************************************************/
int epo_read_cust_config(void) {
    char result[GPS_CONF_FILE_SIZE] = {0};

    FILE *fp = fopen(EPO_CONTROL_FILE_PATH, "r");
    char *key, *val;
    if (!fp) {
           // LOGD("%s: open %s fail!\n", __FUNCTION__, EPO_CONTROL_FILE_PATH);
        return 1;
    }

    while (fgets(result, sizeof(result), fp)) {
        if (get_val(result, &key, &val)) {
            LOGD("%s: Get data fails!!\n", __FUNCTION__);
            fclose(fp);
            return 1;
        }
        if (!key || !val)
            continue;
        if (!strcmp(key, "EPO_ENABLE")) {
            int len = strlen(val);

            LOGD("gps_epo_enablebg = %d, len =%d\n", gps_epo_enable, len);
            gps_epo_enable = str2int(val, val+len);   // *val-'0';
            if ((gps_epo_enable != 1) && (gps_epo_enable != 0)) {
                gps_epo_enable = 1;
            }
            LOGD("gps_epo_enableend = %d\n", gps_epo_enable);
        }
        if (!strcmp(key, "DW_DAYS")) {
            int len = strlen(val);
            gps_epo_download_days = str2int(val, val+len);         // *val-'0';
            if (gps_epo_download_days > MTK_EPO_MAX_DAY || gps_epo_download_days < 0) {
                gps_epo_download_days = 9;
            }
        }
        if (!strcmp(key, "EPO_WIFI_TRIGGER")) {
            int len = strlen(val);
            LOGD("gps_epo_wifi_triggerbg = %d, len =%d\n", gps_epo_wifi_trigger, len);
            gps_epo_wifi_trigger = str2int(val, val+len);   // *val-'0';
            if ((gps_epo_wifi_trigger != 1) && (gps_epo_wifi_trigger != 0)) {
                gps_epo_wifi_trigger = 0;
            }
            LOGD("gps_epo_wifi_triggerend = %d\n", gps_epo_wifi_trigger);
        }
        LOGD("gps_epo_enable = %d, gps_epo_period = %d, \
            wifi_epo_period = %d, gps_epo_wifi_trigger = %d\n", gps_epo_enable, gps_epo_period,
            wifi_epo_period, gps_epo_wifi_trigger);
    }

    fclose(fp);
    return 1;
}

/*****************************************************************************/
static void gps_download_epo_file_name(int count) {
    //  LOGD("count is %d\n", count);
    if (gps_epo_type == 1) {
        if (count == 0) {
            strcpy(gps_epo_file_name, "EPO_GPS_3_1.DAT");
            strcpy(gps_epo_md_file_name, "EPO_GPS_3_1.MD5");
        } else if (count == 1) {
            strcpy(gps_epo_file_name, "EPO_GPS_3_2.DAT");
            strcpy(gps_epo_md_file_name, "EPO_GPS_3_2.MD5");
        } else if (count == 2) {
            strcpy(gps_epo_file_name, "EPO_GPS_3_3.DAT");
            strcpy(gps_epo_md_file_name, "EPO_GPS_3_3.MD5");
        } else if (count == 3) {
            strcpy(gps_epo_file_name, "EPO_GPS_3_4.DAT");
            strcpy(gps_epo_md_file_name, "EPO_GPS_3_4.MD5");
        } else if (count == 4) {
            strcpy(gps_epo_file_name, "EPO_GPS_3_5.DAT");
            strcpy(gps_epo_md_file_name, "EPO_GPS_3_5.MD5");
        } else if (count == 5) {
            strcpy(gps_epo_file_name, "EPO_GPS_3_6.DAT");
            strcpy(gps_epo_md_file_name, "EPO_GPS_3_6.MD5");
        } else if (count == 6) {
            strcpy(gps_epo_file_name, "EPO_GPS_3_7.DAT");
            strcpy(gps_epo_md_file_name, "EPO_GPS_3_7.MD5");
        } else if (count == 7) {
            strcpy(gps_epo_file_name, "EPO_GPS_3_8.DAT");
            strcpy(gps_epo_md_file_name, "EPO_GPS_3_8.MD5");
        } else if (count == 8) {
            strcpy(gps_epo_file_name, "EPO_GPS_3_9.DAT");
            strcpy(gps_epo_md_file_name, "EPO_GPS_3_9.MD5"); }
        else if (count == 9) {
            strcpy(gps_epo_file_name, "EPO_GPS_3_10.DAT");
            strcpy(gps_epo_md_file_name, "EPO_GPS_3_10.MD5");
        }
        //LOGD("download request for file %d, gps_epo_file_name=%s\n", gps_epo_file_count, gps_epo_file_name);
    }
    else if (gps_epo_type == 0) {
        if (count == 0) {
            strcpy(gps_epo_file_name, "EPO_GR_3_1.DAT");
            strcpy(gps_epo_md_file_name, "EPO_GR_3_1.MD5");
        } else if (count == 1) {
            strcpy(gps_epo_file_name, "EPO_GR_3_2.DAT");
            strcpy(gps_epo_md_file_name, "EPO_GR_3_2.MD5");
        } else if (count == 2) {
            strcpy(gps_epo_file_name, "EPO_GR_3_3.DAT");
            strcpy(gps_epo_md_file_name, "EPO_GR_3_3.MD5");
        } else if (count == 3) {
            strcpy(gps_epo_file_name, "EPO_GR_3_4.DAT");
            strcpy(gps_epo_md_file_name, "EPO_GR_3_4.MD5");
        } else if (count == 4) {
            strcpy(gps_epo_file_name, "EPO_GR_3_5.DAT");
            strcpy(gps_epo_md_file_name, "EPO_GR_3_5.MD5");
        } else if (count == 5) {
            strcpy(gps_epo_file_name, "EPO_GR_3_6.DAT");
            strcpy(gps_epo_md_file_name, "EPO_GR_3_6.MD5");
        } else if (count == 6) {
            strcpy(gps_epo_file_name, "EPO_GR_3_7.DAT");
            strcpy(gps_epo_md_file_name, "EPO_GR_3_7.MD5");
        } else if (count == 7) {
            strcpy(gps_epo_file_name, "EPO_GR_3_8.DAT");
            strcpy(gps_epo_md_file_name, "EPO_GR_3_8.MD5");
        } else if (count == 8) {
            strcpy(gps_epo_file_name, "EPO_GR_3_9.DAT");
            strcpy(gps_epo_md_file_name, "EPO_GR_3_9.MD5");
        } else if (count == 9) {
            strcpy(gps_epo_file_name, "EPO_GR_3_10.DAT");
            strcpy(gps_epo_md_file_name, "EPO_GR_3_10.MD5");
        }
        //LOGD("download request for file %d, gps_epo_file_name=%s, gps_epo_md_file_name=%s\n",
        //    gps_epo_file_count, gps_epo_file_name, gps_epo_md_file_name);
    }
}

/*****************************************************************************/
int mtk_gps_sys_read_lock(int fd, off_t offset, int whence, off_t len) {
    struct flock lock;

    lock.l_type = F_RDLCK;
    lock.l_start = offset;
    lock.l_whence = whence;
    lock.l_len = len;

    if (fcntl(fd, F_SETLK, &lock) < 0) {
        return -1;
    }

    return 0;
}

/*****************************************************************************/
static unsigned int mtk_gps_sys_get_file_size() {
    unsigned int fileSize;
    int res_epo, res_epo_hal;
    struct stat st;
    char *epo_file = EPO_FILE;
    char *epo_file_hal = EPO_UPDATE_HAL;
    char epofile[32] = {0};
    res_epo = access(EPO_FILE, F_OK);
    res_epo_hal = access(EPO_UPDATE_HAL, F_OK);
    if (res_epo < 0 && res_epo_hal < 0) {
        LOGD("no EPO data yet\n");
        return -1;
    }
    if (res_epo_hal == 0) {  /*EPOHAL.DAT is here*/
        // LOGD("find EPOHAL.DAT here\n");
        strcpy(epofile, epo_file_hal);
    } else if (res_epo == 0) {  /*EPO.DAT is here*/
        // LOGD("find EPO.DAT here\n");
        strcpy(epofile, epo_file);
    } else
        LOGE("unknown error happened\n");

    if (stat(epofile, &st) < 0) {
        LOGE("Get file size error, return\n");
        return 0;
    }

    fileSize = st.st_size;
       // LOGD("EPO file size: %d\n", fileSize);
    return fileSize;
}

/*****************************************************************************/
void GpsToUtcTime(int i2Wn, double dfTow, time_t* uSecond) {
    struct tm target_time;
    int iYearsElapsed;        //  Years since 1980.
    unsigned int iDaysElapsed;         //  Days elapsed since Jan 1, 1980.
    double dfSecElapsed;
    unsigned int fgLeapYear;
    int pi2Yr = 0;
    int pi2Mo = 0;
    int pi2Day = 0;
    int pi2Hr = 0;
    int pi2Min = 0;
    double pdfSec = 0;
    int i = 0;


    //  Number of days into the year at the start of each month (ignoring leap
    //  years).
    unsigned int doy[12] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};

    //  Convert time to GPS weeks and seconds
    iDaysElapsed = i2Wn * 7 + ((int)dfTow / 86400) + 5;
    dfSecElapsed = dfTow - ((int)dfTow / 86400) * 86400;


    //  decide year
    iYearsElapsed = 0;       //  from 1980
    while (iDaysElapsed >= 365) {
        if ((iYearsElapsed % 100) == 20) {   //  if year % 100 == 0
            if ((iYearsElapsed % 400) == 20) {   //  if year % 400 == 0
                if (iDaysElapsed >= 366) {
                    iDaysElapsed -= 366;
                } else {
                    break;
                }
            } else {
                iDaysElapsed -= 365;
            }
        } else if ((iYearsElapsed % 4) == 0) {   //  if year % 4 == 0
            if (iDaysElapsed >= 366) {
                iDaysElapsed -= 366;
            } else {
                break;
            }
        } else {
            iDaysElapsed -= 365;
        }
        iYearsElapsed++;
    }
    pi2Yr = 1980 + iYearsElapsed;


    // decide month, day
    fgLeapYear = 0;
    if ((iYearsElapsed % 100) == 20) {    // if year % 100 == 0
        if ((iYearsElapsed % 400) == 20) {    // if year % 400 == 0
           fgLeapYear = 1;
        }
    }
    else if ((iYearsElapsed % 4) == 0) {   // if year % 4 == 0
        fgLeapYear = 1;
    }

    if (fgLeapYear) {
        for (i = 2; i < 12; i++) {
            doy[i] += 1;
        }
    }
    for (i = 0; i < 12; i++) {
        if (iDaysElapsed < doy[i]) {
            break;
        }
    }
    pi2Mo = i;
    if (i > 0) {
        pi2Day = iDaysElapsed - doy[i-1] + 1;
    }

    // decide hour, min, sec
    pi2Hr = dfSecElapsed / 3600;
    pi2Min = ((int)dfSecElapsed % 3600) / 60;
    pdfSec = dfSecElapsed - ((int)dfSecElapsed / 60) * 60;

    // change the UTC time to seconds
    memset(&target_time, 0, sizeof(target_time));
    target_time.tm_year = pi2Yr - 1900;
    target_time.tm_mon = pi2Mo - 1;
    target_time.tm_mday = pi2Day;
    target_time.tm_hour = pi2Hr;
    target_time.tm_min = pi2Min;
    target_time.tm_sec = pdfSec;
    target_time.tm_isdst = -1;
    *uSecond = mktime(&target_time);
    if (*uSecond < 0) {
        LOGE("Convert UTC time to seconds fail, return\n");
    }
}


/*****************************************************************************/
int mtk_gps_sys_epo_period_start(int fd, unsigned int* u4GpsSecs, time_t* uSecond) {         // no file lock
    char szBuf[MTK_EPO_ONE_SV_SIZE];
    int pi2WeekNo;
    unsigned int pu4Tow;

    // if (fread(szBuf, 1, MTK_EPO_ONE_SV_SIZE, pFile) != MTK_EPO_ONE_SV_SIZE) {
    if (read(fd, szBuf, MTK_EPO_ONE_SV_SIZE) != MTK_EPO_ONE_SV_SIZE) {
        return -1;
    }

    *u4GpsSecs = (((*(unsigned int*)(&szBuf[0])) & 0x00FFFFFF) *3600);
    pi2WeekNo = (*u4GpsSecs) / 604800;
    pu4Tow = (*u4GpsSecs) % 604800;

    // TRC();
    // LOGD("pi2WeekNo = %d, pu4Tow = %d\n", pi2WeekNo, pu4Tow);
    GpsToUtcTime(pi2WeekNo, pu4Tow, uSecond);   // to get UTC second
    return 0;
}

/*****************************************************************************/
static int mtk_gps_sys_epo_period_end(int fd, unsigned int *u4GpsSecs, time_t* uSecond) {           // no file lock
    int fileSize;
    char szBuf[MTK_EPO_ONE_SV_SIZE];
    int pi2WeekNo;
    unsigned int pu4Tow;

    fileSize = mtk_gps_sys_get_file_size();
    if (fileSize < MTK_EPO_ONE_SV_SIZE) {
        return -1;
    }

    if (-1 == lseek(fd, (fileSize - MTK_EPO_ONE_SV_SIZE), SEEK_SET)) {
        LOGE("lseek error\n");
        return -1;
    }

    if (read(fd, szBuf, MTK_EPO_ONE_SV_SIZE) != MTK_EPO_ONE_SV_SIZE) {
        return -1;
    }

    *u4GpsSecs = (((*(unsigned int*)(&szBuf[0])) & 0x00FFFFFF) *3600);
    (*u4GpsSecs) += 21600;

    pi2WeekNo = (*u4GpsSecs) / 604800;
    pu4Tow = (*u4GpsSecs) % 604800;

    // TRC();
    // LOGD("pi2WeekNo = %d, pu4Tow = %d\n", pi2WeekNo, pu4Tow);
    GpsToUtcTime(pi2WeekNo, pu4Tow, uSecond);

    return 0;
}

/*****************************************************************************/
static int mtk_gps_epo_file_time_hal(time_t uTime[]) {
    LOGD("mtk_gps_epo_file_time_hal");
    struct stat filestat;
    int fd = 0;
    int res_epo, res_epo_hal;
    unsigned int u4GpsSecs_start;    // GPS seconds
    unsigned int u4GpsSecs_expire;
    char *epo_file = EPO_FILE;
    char *epo_file_hal = EPO_UPDATE_HAL;
    char epofile[32] = {0};
    time_t uSecond_start;      // UTC seconds
    time_t uSecond_expire;
// int ret = 0;
    // pthread_mutex_t mutx = PTHREAD_MUTEX_INITIALIZER;

    res_epo = access(EPO_FILE, F_OK);
    res_epo_hal = access(EPO_UPDATE_HAL, F_OK);
    if (res_epo < 0 && res_epo_hal < 0) {
        LOGD("no EPO data yet\n");
//        ret = pthread_mutex_unlock(&mutx);
        return -1;
    }
    if (res_epo_hal== 0) {  /*EPOHAL.DAT is here*/
        // LOGD("find EPOHAL.DAT here\n");
        strcpy(epofile, epo_file_hal);
    } else if (res_epo == 0) {  /*EPO.DAT is here*/
           // LOGD("find EPO.DAT here\n");
        strcpy(epofile, epo_file);
    } else
        LOGE("unknown error happened\n");

    // open file
    fd = open(epofile, O_RDONLY);
    if (fd < 0) {
        LOGE("Open EPO fail, return\n");
//        ret = pthread_mutex_unlock(&mutx);
        return -1;
    }

    // Add file lock
    if (mtk_gps_sys_read_lock(fd, 0, SEEK_SET, 0) < 0) {
        LOGE("Add read lock failed, return\n");
        close(fd);
//      ret = pthread_mutex_unlock(&mutx);
        return -1;
    }

    // EPO start time
    if (mtk_gps_sys_epo_period_start(fd, &u4GpsSecs_start, &uSecond_start)) {
        LOGE("Get EPO file start time error, return\n");
        close(fd);
//        ret = pthread_mutex_unlock(&mutx);
        return -1;
    } else {
        uTime[0] = uSecond_start;
        //  LOGD("The Start time of EPO file is %lld", uTime[0]);
        //  LOGD("The start time of EPO file is %s", ctime(&uTime[0]));
    }

    // download time
    stat(epofile, &filestat);
    uTime[1] = filestat.st_mtime;
    // uTime[1] = uTime[1] - 8 * 3600;
    // LOGD("Download time of EPO file is %lld", uTime[1]);
    // LOGD("Download time of EPO file is %s\n", ctime(&uTime[1]));

    // EPO file expire time
    if (mtk_gps_sys_epo_period_end(fd, &u4GpsSecs_expire, &uSecond_expire)) {
        LOGE("Get EPO file expire time error, return\n");
        close(fd);
//        ret = pthread_mutex_unlock(&mutx);
        return -1;
    } else {
        uTime[2] = uSecond_expire;
        // LOGD("The expire time of EPO file is %lld", uTime[2]);
        //  LOGD("The expire time of EPO file is %s", ctime(&uTime[2]));
    }

    close(fd);
//    ret = pthread_mutex_unlock(&mutx);
    return 0;
}

size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    size_t written;
    written = fwrite(ptr, size, nmemb, stream);
    return written;
}
CURLcode curl_easy_download(char* url, char* filename) {
    CURL *curl = NULL;
    FILE *fp = NULL;
    CURLcode res;

    //LOGD("curl_easy_download url: %s to %s", url, filename);
    if ((res = curl_global_init(CURL_GLOBAL_DEFAULT)) != 0) {
        LOGE("curl_global_init fail, res = %d", res);
    } else {
        LOGD("curl_global_init success");
    }
    curl = curl_easy_init();
    //LOGD("curl_easy_init done");
    if (curl) {
        fp = fopen(filename, "w+");
        if (fp == NULL) {
            curl_easy_cleanup(curl);
            return CURLE_FAILED_INIT;
        }

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60);
        //   curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        fclose(fp);
        return res;
    } else {
        return CURLE_FAILED_INIT;
    }
}
static int counter = 1;
CURLcode curl_easy_download_epo(void) {
    int res_val;
    CURLcode res;
    char gps_epo_md_file_temp[60] = {0};
    char gps_epo_md_key[80] = {0};
    char gps_epo_data_file_name[60] = {0};
    char* url = NULL;
    char* key = NULL;
    char* md_url = NULL;
    char count_str[15]={0};

    //LOGD("curl_easy_download_epo");
    strcat(gps_epo_md_file_temp, "/data/misc/gps/");
    strcat(gps_epo_md_file_temp, gps_epo_md_file_name);
    // LOGD("gps_epo_md_file_name = %s\n", gps_epo_md_file_name);
    strcpy(gps_epo_md_key, "0000000000000000");
    memset(count_str, 0, sizeof(count_str));
    sprintf(count_str, "%d", counter);
    strcat(gps_epo_md_key, "&counter=");
    strcat(gps_epo_md_key, count_str);
    if (counter <= 1) {
        strcat(gps_epo_md_key, "&url=1");
    } else {
        strcat(gps_epo_md_key, "&url=0");
    }
    md_url = getEpoUrl(gps_epo_md_file_name, gps_epo_md_key);
    // LOGD("md_url = %s\n", md_url);
    if (md_url == NULL) {
        LOGD("getEpoUrl failed!\n");
        return CURLE_FAILED_INIT;
    }
    res = curl_easy_download(md_url, gps_epo_md_file_temp);
    // LOGD("md file curl_easy_download res = %d\n", res);
    free(md_url);
    memset(gps_epo_md_key, 0 , sizeof(gps_epo_md_key));
    if (res == 0) {
        FILE *fp = NULL;
        char* key_temp = NULL;
        int len = 0;
        fp = fopen(gps_epo_md_file_temp, "r");
        if (fp != NULL) {
            len = fread(gps_epo_md_key, sizeof(char), sizeof(char)*48, fp);
            key_temp = gps_epo_md_key;
            fclose(fp);
            unlink(gps_epo_md_file_temp);
            LOGD("gps_epo_md_key before cpy= %s, len=%d\n", gps_epo_md_key, len);
            memcpy(gps_epo_md_key, key_temp+32, 16);    // comment by rayjf li
            gps_epo_md_key[16] = '\0';
        }
        else {
            strcpy(gps_epo_md_key, "0000000000000000");
        }
        counter = 1;
    }
    else {
        strcpy(gps_epo_md_key, "0000000000000000");
        counter++;
    }
    // LOGD("gps_epo_md_key = %s\n", gps_epo_md_key);
    memset(count_str, 0, sizeof(count_str));
    sprintf(count_str, "%d", counter);
    strcat(gps_epo_md_key, "&counter=");
    strcat(gps_epo_md_key, count_str);
    if (counter <= 1) {
        strcat(gps_epo_md_key, "&url=1");
    } else {
        strcat(gps_epo_md_key, "&url=0");
    }
    url = getEpoUrl(gps_epo_file_name, gps_epo_md_key);

    //LOGD("url = %s\n", url);
    if (url == NULL) {
        LOGE("getEpoUrl failed!\n");
        return CURLE_FAILED_INIT;
    }
    strcat(gps_epo_data_file_name, "/data/misc/gps/");
    strcat(gps_epo_data_file_name, gps_epo_file_name);
    res = curl_easy_download(url, gps_epo_data_file_name);
    LOGD("curl_easy_download res = %d\n", res);
    free(url);
    if (res == CURLE_OK) {
        FILE *fp_temp = NULL;
        FILE *fp = NULL;

        counter = 1;
        if (gps_epo_file_count == 0) {
            unlink(EPO_UPDATE_HAL);
        }
        res_val = chmod(gps_epo_data_file_name, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IROTH);
        //LOGD("chmod res_val = %d, %s\n", res_val, strerror(errno));
        fp_temp = fopen(EPO_UPDATE_HAL, "at");
        if (fp_temp != NULL) {
            fp = fopen(gps_epo_data_file_name, "r");
            if (fp != NULL) {
            #define buf_size  256
                char data[buf_size] = {0};
                int bytes_in = 0, bytes_out = 0;
                int len = 0;

                while ((bytes_in = fread(data, 1, sizeof(data), fp)) > 0
                        && (bytes_in <= (int)(buf_size* sizeof(char)))) {
                    bytes_out = fwrite(data, 1, bytes_in, fp_temp);
                    if (bytes_in != bytes_out) {
                        //LOGD("bytes_in = %d,bytes_out = %d\n", bytes_in, bytes_out);
                    }
                    len += bytes_out;
                    // LOGD("copying file...%d bytes copied\n",len);
                }
                fclose(fp);
            } else {
                LOGE("Open merged file fp=NULL\n");
            }
            fclose(fp_temp);
        }
        else {
            LOGE("Open merged file failed\n");
        }
        res_val = chmod(EPO_UPDATE_HAL, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IROTH);
        //LOGD("chmod res_val = %d, %s\n", res_val, strerror(errno));
    } else {
        unlink(gps_epo_data_file_name);
        counter++;
    }
    return res;
}

/*****************************************************************************/
static int mtk_gps_epo_file_update_hal() {
    CURLcode res;

    res = curl_easy_download_epo();
    // LOGD("curl_easy_download_epo res = %d\n", res);
    if (res != CURLE_OK) {
        epo_download_failed = 1;
        return -1;
    } else {
        //LOGD("download piece file%d done\n", (gps_epo_file_count+1));
        return 0;
    }
}
static unsigned int mtk_gps_epo_get_piece_file_size() {
    struct stat st;
    unsigned int fileSize;
    char gps_epo_data_file_name[60] = {0};

    strcat(gps_epo_data_file_name, "/data/misc/gps/");
    strcat(gps_epo_data_file_name, gps_epo_file_name);

    if (stat(gps_epo_data_file_name, &st) < 0) {
        LOGE("Get file size error, return\n");
        return 0;
    }
    fileSize = st.st_size;
    LOGD("EPO piece file size: %d\n", fileSize);
    return fileSize;
}

/*****************************************************************************/
static int mtk_gps_epo_piece_data_start(int fd, unsigned int* u4GpsSecs, time_t* uSecond) {
    char szBuf[MTK_EPO_ONE_SV_SIZE];
    int pi2WeekNo;
    unsigned int pu4Tow;


    // if (fread(szBuf, 1, MTK_EPO_ONE_SV_SIZE, pFile) != MTK_EPO_ONE_SV_SIZE) {
    if (read(fd, szBuf, MTK_EPO_ONE_SV_SIZE) != MTK_EPO_ONE_SV_SIZE) {
        return -1;
    }

    *u4GpsSecs = (((*(unsigned int*)(&szBuf[0])) & 0x00FFFFFF) *3600);
    pi2WeekNo = (*u4GpsSecs) / 604800;
    pu4Tow = (*u4GpsSecs) % 604800;

    //LOGD("mtk_gps_epo_piece_data_start");
    //LOGD("pi2WeekNo = %d, pu4Tow = %d\n", pi2WeekNo, pu4Tow);
    GpsToUtcTime(pi2WeekNo, pu4Tow, uSecond);   // to get UTC second
    return 0;
}

/*****************************************************************************/
static int mtk_gps_epo_piece_data_end(int fd, unsigned int *u4GpsSecs, time_t* uSecond) {
    int fileSize = 0;
    char szBuf[MTK_EPO_ONE_SV_SIZE] = {0};
    int pi2WeekNo;
    unsigned int pu4Tow;

    if (-1 != fd) {
        fileSize = mtk_gps_epo_get_piece_file_size();
        if (fileSize < MTK_EPO_ONE_SV_SIZE) {
            LOGE("Get file size is error\n");
            return -1;
        }
        if (-1 == lseek(fd, (fileSize - MTK_EPO_ONE_SV_SIZE), SEEK_SET)) {
            LOGE("lseek error\n");
            return -1;
        }

        if (read(fd, szBuf, MTK_EPO_ONE_SV_SIZE) != MTK_EPO_ONE_SV_SIZE) {
            LOGE("read epo file end data faied\n");
            return -1;
        }

        *u4GpsSecs = (((*(unsigned int*)(&szBuf[0])) & 0x00FFFFFF) *3600);
        (*u4GpsSecs) += 21600;

        pi2WeekNo = (*u4GpsSecs) / 604800;
        pu4Tow = (*u4GpsSecs) % 604800;

        LOGD("mtk_gps_epo_piece_data_end");
        LOGD("pi2WeekNo = %d, pu4Tow = %d\n", pi2WeekNo, pu4Tow);
        GpsToUtcTime(pi2WeekNo, pu4Tow, uSecond);
    }
    return 0;
}

/*****************************************************************************/
static int mtk_gps_epo_server_data_is_changed() {
    time_t uTime_end = 0;
    time_t uTime_start = 0;
    int fd_end = -1;
    int fd_start = -1;
    char gps_epo_data_file_name_end[60] = {0};
    char gps_epo_data_file_name_start[60] = {0};
    time_t uSecond_start;
    time_t uSecond_end;
    unsigned int u4GpsSecs_start;
    unsigned int u4GpsSecs_end;
    int ret = 0;

    strcat(gps_epo_data_file_name_start, "/data/misc/gps/");
    strcat(gps_epo_data_file_name_start, gps_epo_file_name);

    fd_start = open(gps_epo_data_file_name_start, O_RDONLY);
    if (fd_start >= 0) {
        int res = 0;
        res = mtk_gps_epo_piece_data_start(fd_start, &u4GpsSecs_start, &uSecond_start);
        if (res == 0) {
            uTime_start = uSecond_start;
        } else {
            epo_download_failed = 1;
            ret = 1;
            LOGE("Get start time failed\n");
        }
        close(fd_start);
    } else {
        LOGE("Open start file failed\n");
    }
    if (gps_epo_file_count > 0) {
        gps_download_epo_file_name(gps_epo_file_count - 1);
        strcat(gps_epo_data_file_name_end, "/data/misc/gps/");
        strcat(gps_epo_data_file_name_end, gps_epo_file_name);
        // open file
        fd_end = open(gps_epo_data_file_name_end, O_RDONLY);
        if (fd_end >= 0) {
            int res = 0;
            res = mtk_gps_epo_piece_data_end(fd_end, &u4GpsSecs_end, &uSecond_end);
            if (res == 0) {
                uTime_end = uSecond_end;
            } else {
                epo_download_failed = 1;
                LOGE("Get end time failed\n");
                ret = 1;
            }
            close(fd_end);
        } else {
            LOGE("Open end file failed\n");
        }
    } else if (gps_epo_file_count == 0) {
        uTime_end = uTime_start;
    }

    // LOGD("gps_epo_data_file_start =%s, end =%s\n", gps_epo_data_file_name_start, gps_epo_data_file_name_end);
    LOGD("The end time of EPO file is %s, The start time of EPO file is %s\n",
        ctime(&uTime_end), ctime(&uTime_start));
    if (uTime_start >= ((24*60*60) + uTime_end)) {
        int i;
        LOGD("The epo data is updated on the server!!!\n");
        for (i = gps_epo_file_count; 0 <= i; i--) {
            char gps_epo_piece_file_name[40] = {0};

            gps_download_epo_file_name(i);
            strcat(gps_epo_piece_file_name, "/data/misc/gps/");
            strcat(gps_epo_piece_file_name, gps_epo_file_name);
            unlink(gps_epo_piece_file_name);
        }
        unlink(EPO_UPDATE_HAL);
        gps_epo_file_count = 0;
        return 1;
    }
    return ret;
}

/*****************************************************************************/
static int mtk_gps_epo_file_update() {
    int ret;
    int is_changed = 0;
    int count_temp = 10;
    int count_mini = 0;

    // update framework downlaod data to EPOHAL.DAT
    ret = mtk_gps_epo_file_update_hal();
    if (ret < 0) {
        LOGE("Update EPOHAL.DAT error\n");
        return -1;
    }
    is_changed = mtk_gps_epo_server_data_is_changed();
    if (is_changed == 1) {
        return -1;
    }
    gps_epo_file_count++;
    count_temp = gps_epo_download_days/3;
    count_mini = gps_epo_download_days%3;
    if (count_mini > 0) {
        count_temp++;
    }
    if (gps_epo_file_count < 10 && gps_epo_file_count < count_temp) {
        //LOGD("Download next epo file continue...\n");
        return 0;
    } else {
        int i = 0;
        LOGD("download epo file completed!file count=%d, epo_download_failed=%d, epo_data_updated=%d\n",
            gps_epo_file_count, epo_download_failed, epo_data_updated);

        for (i = 0; i < count_temp; i++) {
            char gps_epo_data_file_name[60] = {0};

            gps_download_epo_file_name(i);
            strcat(gps_epo_data_file_name, "/data/misc/gps/");
            strcat(gps_epo_data_file_name, gps_epo_file_name);
            unlink(gps_epo_data_file_name);
        }
        epo_data_updated = 0;
        gps_epo_file_count = 0;
        if (epo_download_failed == 1) {
            epo_download_failed = 0;
        }
    }
    return ret;
}

static int mtk_epo_is_expired(int wifi_tragger) {
    time_t uTime[3];  // [0] epo start time, [1] download time, [2] expired time
    memset(uTime, 0, sizeof(uTime));
    time_t         now = time(NULL);
    struct tm      tm_utc;
    time_t  time_utc;
    long etime = gps_epo_period*24*60*60;
    long expired_set = 0;
    int download_day = 0;

    gmtime_r(&now, &tm_utc);
    time_utc = mktime(&tm_utc);
    mtk_gps_epo_file_time_hal(uTime);

    if (wifi_tragger) {
        expired_set = wifi_epo_period*24*60*60;    // for wifi tragger we change checking expired time to 1 day.
    } else {
        download_day = (uTime[2] - uTime[0])/(24*60*60);
           // LOGD("epo data downloaded dat: %d\n", download_day);
        if (download_day < 3) {
            expired_set = 0;
        } else if (download_day < 6) {
            expired_set = 2*24*60*60;
        } else if ((6 <= download_day) && (download_day < 9)) {
            expired_set = 5*24*60*60;
        } else if ((9 <= download_day) && (download_day < 12)) {
            expired_set = 7*24*60*60;
        } else if ((12 <= download_day) && (download_day < 15)) {
            expired_set = 7*24*60*60;
        } else if ((15 <= download_day) && (download_day < 18)) {
            expired_set = 7*24*60*60;
        } else if (download_day >= 18) {
            expired_set = 7*24*60*60;
        } else {
            expired_set = etime;
        }
    }

    //LOGD("current time: %ld, current time:%s", time_utc, ctime(&time_utc));
    //LOGD("EPO start time: %ld, EPO start time: %s", uTime[0], ctime(&uTime[0]));
      //  LOGD("EPO expired_set: %lld", expired_set);
    if ((time_utc - uTime[0]) >= expired_set) {
        LOGD("EPO file is expired");
        gps_epo_file_count = 0;
        return 1;
    } else if ((time_utc - uTime[0]) < 0) {
        LOGD("Current time is invalid");
        gps_epo_file_count = 0;
        return 1;
    } else {
        LOGD("EPO file is valid, no need update");
        return 0;
    }
}

//////////////////////////////////////////////////////////////////////////////////
// MAIN -> EPO Download (handlers)
static int mnld_epo_download() {
    //LOGD("begin\n");

    int ret = epo_file_update_impl();
    mnld_epo_download_done(ret);
    //LOGD("end\n");
    return ret;
}
static int epo_event_hdlr(int fd) {
    char buff[MNLD_INTERNAL_BUFF_SIZE] = {0};
    int offset = 0;
    main2epo_event cmd;
    int read_len;

    read_len = safe_recvfrom(fd, buff, sizeof(buff));
    if (read_len <= 0) {
        LOGE("epo_event_hdlr() safe_recvfrom() failed read_len=%d", read_len);
        return -1;
    }

    cmd = get_int(buff, &offset);
    switch (cmd) {
    case MAIN2EPO_EVENT_START: {
        LOGW("mnld_epo_download() before");
        // need to call mnld_epo_download_done() when EPO download is done
        mnld_epo_download();
        LOGW("mnld_epo_download() after");
        break;
    }
    default: {
        LOGE("epo_event_hdlr() unknown cmd=%d", cmd);
        return -1;
    }
    }
    return 0;
}

static void epo_downloader_thread_timeout() {
    LOGE("epo_downloader_thread_timeout() crash here for debugging");
    CRASH_TO_DEBUG();
}

static void retry_alarm_timeout_handler() {
    epo_download_retry = 1;
    LOGD("epo_download_retry is =%d\n", epo_download_retry);
}

static void* epo_downloader_thread(void *arg) {
    #define MAX_EPOLL_EVENT 50
    //hdlr_timer = init_timer(epo_downloader_thread_timeout);
    retry_download_timer = init_timer(retry_alarm_timeout_handler);
    struct epoll_event events[MAX_EPOLL_EVENT];
    UNUSED(arg);

    int epfd = epoll_create(MAX_EPOLL_EVENT);
    if (epfd == -1) {
        LOGE("epo_downloader_thread() epoll_create failure reason=[%s]%d\n",
            strerror(errno), errno);
        return 0;
    }

    if (epoll_add_fd(epfd, g_fd_epo) == -1) {
        LOGE("epo_downloader_thread() epoll_add_fd() failed for g_fd_epo failed");
        return 0;
    }
    while (1) {
        int i;
        int n;
        //LOGD("epo_downloader_thread wait");
        n = epoll_wait(epfd, events, MAX_EPOLL_EVENT , -1);
        if (n == -1) {
            if (errno == EINTR) {
				LOGW("epo_downloader_thread EINTR");
                continue;
            } else {
                LOGE("epo_downloader_thread() epoll_wait failure reason=[%s]%d",
                    strerror(errno), errno);
                return 0;
            }
        }
        //start_timer(hdlr_timer, MNLD_EPO_HANDLER_TIMEOUT);
        for (i = 0; i < n; i++) {
            if (events[i].data.fd == g_fd_epo) {
                if (events[i].events & EPOLLIN) {
                    epo_event_hdlr(g_fd_epo);
                }
            } else {
                LOGE("epo_downloader_thread() unknown fd=%d",
                    events[i].data.fd);
            }
        }
        //stop_timer(hdlr_timer);
    }

    LOGE("epo_downloader_thread() exit");
    return 0;
}

int epo_downloader_is_file_invalid() {
    return mtk_epo_is_expired(0);
}

int epo_is_wifi_trigger_enabled() {
    return gps_epo_wifi_trigger;
}

int epo_is_epo_download_enabled() {
    return gps_epo_enable;
}

int epo_downloader_start() {
    char buff[MNLD_INTERNAL_BUFF_SIZE] = {0};
    int offset = 0;
    put_int(buff, &offset, MAIN2EPO_EVENT_START);
    return safe_sendto(MNLD_EPO_DOWNLOAD_SOCKET, buff, offset);
}

int epo_downloader_init() {
    pthread_t pthread_epo;
    g_fd_epo = socket_bind_udp(MNLD_EPO_DOWNLOAD_SOCKET);
    if (g_fd_epo < 0) {
        LOGE("socket_bind_udp(MNLD_EPO_DOWNLOAD_SOCKET) failed");
        return -1;
    }

    pthread_create(&pthread_epo, NULL, epo_downloader_thread, NULL);
    return 0;
}

