/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 *
 * MediaTek Inc. (C) 2016. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
 * AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek Software")
 * have been modified by MediaTek Inc. All revisions are subject to any receiver's
 * applicable license agreements with MediaTek Inc.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/epoll.h>
#include <pthread.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>
#include <cutils/sockets.h>
#include <cutils/properties.h>

#include "mtk_lbs_utility.h"
#include "data_coder.h"
#include "mnld.h"
#include "mtk_gps.h"
#include "mtk_gps_type.h"

#include "gps_dbg_log.h"
#include "mpe.h"

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
#define LOGD(...) tag_log(1, "[gps_dbg_log]", __VA_ARGS__);
#define LOGW(...) tag_log(1, "[gps_dbg_log] WARNING: ", __VA_ARGS__);
#define LOGE(...) tag_log(1, "[gps_dbg_log] ERR: ", __VA_ARGS__);
#else
#define LOG_TAG "gps_dbg_log"
#include <cutils/sockets.h>
#include <cutils/log.h>     /*logging in logcat*/
#define LOGD(fmt, arg ...) ALOGD("%s: " fmt, __FUNCTION__ , ##arg)
#define LOGW(fmt, arg ...) ALOGW("%s: " fmt, __FUNCTION__ , ##arg)
#define LOGE(fmt, arg ...) ALOGE("%s: " fmt, __FUNCTION__ , ##arg)
#endif
#define FILE_FCLOSE(fd) do {\
    if (NULL != fd) {\
        fclose(fd);\
        fd = NULL;}\
    } while (0)

unsigned char gps_debuglog_state = MTK_GPS_DISABLE_DEBUG_MSG_WR_BY_MNLD;
char gps_debuglog_file_name[GPS_DEBUG_LOG_FILE_NAME_MAX_LEN] = "/mnt/sdcard/mtklog/gpsdbglog/gpsdebug.log";
char storagePath[GPS_DEBUG_LOG_FILE_NAME_MAX_LEN] = "/mnt/sdcard/mtklog/gpsdbglog/";
static char log_filename_suffix[GPS_DEBUG_LOG_FILE_NAME_MAX_LEN] = "gpsdebug.log";
static char gsGpsLogFileName[GPS_DEBUG_LOG_FILE_NAME_MAX_LEN] = {0};
static int total_file_count = 0;
static int Current_FileSize = 0;
static int Filenum = 0;
static int DirLogSize = 0;

FILE* g_hLogFile = NULL;
bool g_gpsdbglogThreadExit = true;
bool g_pingpang_init = false;
pthread_mutex_t FILE_WRITE_MUTEX;

#define PINGPANG_WRITE_LOCK 0
#define PINGPANG_FLUSH_LOCK 1
typedef struct sync_lock {
    pthread_mutex_t mutx;
    pthread_cond_t con;
    int condtion;
    int index;
}SYNC_LOCK_T;
static SYNC_LOCK_T lock_for_sync[] = {{PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER, 0, 0},
                                      {PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER, 0, 1}};

//  data
#define PINGPANG_BUFFER_SIZE         (20*1024)

// describle one single buffer
typedef enum {
    NOTINIT = 0,
    WRITABLE,  // the state which can swtich to writing state
    WRITING,   // means the buffer is writing
    READABLE,  // means mtklogger thread now can read this buffer and write data to flash
    READING    // means mtklogger thread is writing data to flash
} buffer_state;

typedef struct {
    char* next_write;
    char* start_address_buffer1;
    char* start_address_buffer2;
    char* end_address_buffer1;
    char* end_address_buffer2;
    int* p_buffer1_lenth_to_write;
    int* p_buffer2_lenth_to_write;
    buffer_state* p_buffer1_state;
    buffer_state* p_buffer2_state;
    int num_loose;
} ping_pang_buffer;
static buffer_state buffer1_state = NOTINIT;
static buffer_state buffer2_state = NOTINIT;
static int lenth_to_write_buffer1 = 0;
static int lenth_to_write_buffer2 = 0;
static ping_pang_buffer ping_pang_buffer_body;

// function related
// called when mtklogger thread init if debugtype set true
static INT32 create_debug_log_file();
static INT32 mtklog_gps_set_debug_file(char* file_name);
static INT32 gps_dbg_log_pingpang_init();
// called in function mnl_sys_alps_nmea_output
static INT32 gps_dbg_log_pingpang_copy(ping_pang_buffer* pingpang, const char* buffer, INT32 len);
// called in mtklogger thread when it is need to write
static INT32 gps_dbg_log_pingpang_write(ping_pang_buffer* pingpang, FILE* filp);
// called when debugtype set 1 to 0 or  mtklogger thread exit
static INT32 gps_dbg_log_pingpang_free();
// called when debugtype set 1 to 0 or  mtklogger thread exit
static INT32 gps_dbg_log_pingpang_flush(ping_pang_buffer* pingpang, FILE * filp);

static void init_condition(SYNC_LOCK_T *lock) {
    int ret = 0;

    ret = pthread_mutex_lock(&(lock->mutx));
    lock->condtion = 0;
    ret = pthread_mutex_unlock(&(lock->mutx));
    LOGD("ret mutex unlock = %d\n", ret);

    return;
}
static void get_condition(SYNC_LOCK_T *lock) {
    int ret = 0;

    ret = pthread_mutex_lock(&(lock->mutx));
    LOGD("ret mutex lock = %d\n", ret);

    while (!lock->condtion) {
        ret = pthread_cond_wait(&(lock->con), &(lock->mutx));
        LOGD("ret cond wait = %d\n" , ret);
    }

    lock->condtion = 0;
    ret = pthread_mutex_unlock(&(lock->mutx));
    LOGD("ret mutex unlock = %d\n", ret);

    return;
}

static void release_condition(SYNC_LOCK_T *lock) {
    int ret = 0;

    ret = pthread_mutex_lock(&(lock->mutx));
    LOGD("ret mutex lock = %d\n", ret);

    lock->condtion = 1;
    ret = pthread_cond_signal(&(lock->con));
    LOGD("ret cond_signal = %d\n", ret);

    ret = pthread_mutex_unlock(&(lock->mutx));
    LOGD("ret unlock= %d\n", ret);

    return;
}

void gps_stop_dbglog_release_condition(void) {
    g_gpsdbglogThreadExit = true;
    release_condition(&lock_for_sync[PINGPANG_WRITE_LOCK]);
    release_condition(&lock_for_sync[PINGPANG_FLUSH_LOCK]);
    LOGD("exit while, gps_dbg_log thread exit\n");
}

static int get_mtklog_path(char *logpath) {
    char mtklogpath[GPS_DEBUG_LOG_FILE_NAME_MAX_LEN] = {0};
    char temp[GPS_DEBUG_LOG_FILE_NAME_MAX_LEN] = {0};
    unsigned int len;

    char* ptr;
    ptr = strchr(logpath, ',');
    if (ptr) {
        strcpy(temp, ptr + 1);
        LOGD("logpath for mtklogger socket msg: %s", temp);
    } else {
        LOGD("logpath for mtklogger socket msg has not ',': %s", temp);
        strcpy(logpath, "/data/misc/gps/");
        return 0;
    }

    len = strlen(temp);
    if (len != 0  && temp[len-1] != '/') {
        temp[len++] = '/';
        if (len < GPS_DEBUG_LOG_FILE_NAME_MAX_LEN) {
            temp[len] = '\0';
        }
    }
    if (len <= GPS_DEBUG_LOG_FILE_NAME_MAX_LEN - strlen(PATH_SUFFIX)) {
        sprintf(logpath, "%smtklog/gpsdbglog/", temp);
        LOGD("get_mtklog_path:logpath is %s", logpath);
    }

    if (len <= GPS_DEBUG_LOG_FILE_NAME_MAX_LEN-7) {
        sprintf(mtklogpath, "%smtklog/", temp);
        if (0 != access(mtklogpath, F_OK)) {    // if mtklog dir is not exit, mkdir
             LOGD("access dir error(%s), Try to create dir", mtklogpath);
             if (mkdir(mtklogpath, 0775) == -1) {
                 strcpy(logpath, "/data/misc/gps/");  // if mkdir fail, set default path
                 LOGD("mkdir %s fail(%s), set default logpath(%s)", mtklogpath, strerror(errno), logpath);
             }
        }
    }
    return 1;
}
static int mode = 0;
int mtklogger2mnl_hdlr(int fd) {
    int ret = 0;
    int read_len;
    int offset = 0;
    char ans[255] = {0};
    char buff_msg[253] = {0};

    read_len = safe_recvfrom(fd, buff_msg, sizeof(buff_msg) - 1);
    if (read_len <= 0) {
        LOGE("mtklogger2mnl_hdlr() safe_recvfrom() failed read_len=%d, reason=[%s]%d",
            read_len, strerror(errno), errno);
        return -1;
    }

    // response "msg,1" to mtklogger
    sprintf(ans, "%s,1", buff_msg);
    LOGD("notify client, recv %s from mtklogger, ans: %s\n", buff_msg, ans);
    if (strstr(buff_msg, "set_storage_path")) {  // buff is "set_storage_path,storagePath"
        strcpy(storagePath, buff_msg);
        get_mtklog_path(storagePath);
        write(fd, ans, strlen(ans));
    } else if (!strncmp(buff_msg, "deep_start,1", 12)) {
        gps_debuglog_state = MTK_GPS_ENABLE_DEBUG_MSG_WR_BY_MNLD;
        LOGD("gps_debuglog_state:%d", gps_debuglog_state);

        #if ANDROID_MNLD_PROP_SUPPORT
        property_set("debug.gpsdbglog.enable", "1");
        #endif
        write(fd, ans, strlen(ans));

        mnl2mpe_set_log_path(storagePath, 1, 0);
        mode = 0;
        strcpy(gps_debuglog_file_name, storagePath);
        strcat(gps_debuglog_file_name, log_filename_suffix);

        if (mnld_is_gps_started_done()) {
            ret = mtk_gps_set_debug_type(gps_debuglog_state);
            if (MTK_GPS_ERROR == ret) {
                LOGE("deep_start,1,mtk_gps_set_debug_type fail");
            } else {
                LOGD("start gpsdbglog successfully\n");
            }
            ret = mtklog_gps_set_debug_file(gps_debuglog_file_name);
            if (MTK_GPS_ERROR == ret) {
                LOGE("deep_start,1,mtklog_gps_set_debug_file fail");
            }
        }
    } else if (!strncmp(buff_msg, "deep_start,2", 12)) {
        gps_debuglog_state = MTK_GPS_ENABLE_DEBUG_MSG_WR_BY_MNLD;
        LOGD("gps_debuglog_state:%d", gps_debuglog_state);

        #if ANDROID_MNLD_PROP_SUPPORT
        property_set("debug.gpsdbglog.enable", "1");
        #endif
        write(fd, ans, strlen(ans));

        strcpy(gps_debuglog_file_name, LOG_FILE);
        strcpy(storagePath, LOG_FILE_PATH);
        if (mnld_is_gps_started_done()) {
            ret = mtk_gps_set_debug_type(gps_debuglog_state);
            if (MTK_GPS_ERROR == ret) {
                LOGE("deep_start,2,mtk_gps_set_debug_type fail");
            } else {
                LOGD("start gpsdbglog successfully\n");
            }
            ret = mtklog_gps_set_debug_file(gps_debuglog_file_name);
            if (MTK_GPS_ERROR == ret) {
                LOGE("deep_start,2,mtklog_gps_set_debug_file fail");
            }
        }

        mnl2mpe_set_log_path(LOG_FILE_PATH, 1, 1);
        mode = 1;
    } else if (!strncmp(buff_msg, "deep_stop", 9)) {
        gps_debuglog_state = MTK_GPS_DISABLE_DEBUG_MSG_WR_BY_MNLD;
        LOGD("gps_debuglog_state:%d", gps_debuglog_state);

        #if ANDROID_MNLD_PROP_SUPPORT
        property_set("debug.gpsdbglog.enable", "0");
        #endif
        write(fd, ans, strlen(ans));

        if (mnld_is_gps_started_done()) {
            ret = mtk_gps_set_debug_type(gps_debuglog_state);
            if (MTK_GPS_ERROR== ret) {
                LOGE("deep_stop, mtk_gps_set_debug_type fail");
            } else {
                LOGD("stop gpsdbglog successfully\n");
            }
        }
        mnl2mpe_set_log_path(storagePath, 0, 0);
        mode = 0;
    } else {
        write(fd, ans, strlen(ans));
        LOGE("unknown message: %s\n", buff_msg);
    }
    return 0;
}

void mtklogger_mped_reboot_message_update(void) {
    if (gps_debuglog_state == MTK_GPS_DISABLE_DEBUG_MSG_WR_BY_MNLD) {
        mnl2mpe_set_log_path(storagePath, 0, mode);
    } else if (gps_debuglog_state == MTK_GPS_ENABLE_DEBUG_MSG_WR_BY_MNLD) {
        mnl2mpe_set_log_path(storagePath, 1, mode);
    }
}
void* gps_dbg_log_thread(void* arg) {
    INT32 count = 0;
    LOGD("create: %.8X, arg = %p\r\n", (unsigned int)pthread_self(), arg);
    pthread_detach(pthread_self());

    init_condition(&lock_for_sync[PINGPANG_WRITE_LOCK]);
    init_condition(&lock_for_sync[PINGPANG_FLUSH_LOCK]);
    FILE_FCLOSE(g_hLogFile);

    if (!g_pingpang_init && (gps_debuglog_state == MTK_GPS_ENABLE_DEBUG_MSG_WR_BY_MNLD)) {
        if (MTK_GPS_ERROR == gps_dbg_log_pingpang_init()) {
            g_gpsdbglogThreadExit = true;
            LOGE("gps dbg log pingpang init fail, thread exit");
        }
    }

    if (gps_debuglog_state == MTK_GPS_ENABLE_DEBUG_MSG_WR_BY_MNLD) {
        create_debug_log_file();
    }

    if ((access(storagePath, F_OK|R_OK|W_OK)) == 0) {
        DirLogSize = gps_log_dir_check(storagePath);
    }

    while (!g_gpsdbglogThreadExit) {
        get_condition(&lock_for_sync[PINGPANG_WRITE_LOCK]);
        LOGD("get_condition PINGPANG_WRITE_LOCK");

        if (gps_debuglog_state == MTK_GPS_ENABLE_DEBUG_MSG_WR_BY_MNLD) {
            if (0 != access(storagePath, F_OK|R_OK|W_OK)) {  // return value 0:success, -1 : fail
                LOGE("Access gps debug log dir fail(%s)!\r\n", strerror(errno));
                FILE_FCLOSE(g_hLogFile);  // close file before open,  if the file has been open.

                gps_debuglog_state = MTK_GPS_DISABLE_DEBUG_MSG_WR_BY_MNLD;
                if (mnld_is_gps_started_done()) {
                    if (MTK_GPS_ERROR == mtk_gps_set_debug_type(gps_debuglog_state)) {
                        LOGE("GPS_DEBUGLOG_DISABLE, mtk_gps_set_debug_type fail");
                    } else {
                        LOGD("GPS_DEBUGLOG_DISABLE, stop gpsdbglog successfully\n");
                    }
                }
            }
            if (g_hLogFile != NULL) {
                if (Current_FileSize + PINGPANG_BUFFER_SIZE > MAX_DBG_LOG_FILE_SIZE) {
                    char tmpfilename[GPS_DEBUG_LOG_FILE_NAME_MAX_LEN]={0};
                    Filenum++;
                    FILE_FCLOSE(g_hLogFile);  // close file before open, if the file has been open.
                    DirLogSize = DirLogSize + Current_FileSize;
                    snprintf(tmpfilename, sizeof(tmpfilename), "%s-%d", gsGpsLogFileName, Filenum);

                    g_hLogFile = fopen(tmpfilename, "w");
                    if (NULL == g_hLogFile) {
                        LOGE("open file fail, NULL == g_hLogFile\r\n");
                        break;
                    }
                    if (DirLogSize  > (int)(MAX_DBG_LOG_DIR_SIZE - MAX_DBG_LOG_FILE_SIZE)
                        || (total_file_count + Filenum) > GPS_DBG_LOG_FILE_NUM_LIMIT) {
                        DirLogSize = gps_log_dir_check(storagePath);
                    }

                    Current_FileSize = 0;
                }
            }
            if (g_hLogFile != NULL) {
                count = gps_dbg_log_pingpang_write(&ping_pang_buffer_body, g_hLogFile);
                Current_FileSize = count + Current_FileSize;
            }
        } else {
            if ((ping_pang_buffer_body.start_address_buffer1 != NULL)
                && (ping_pang_buffer_body.start_address_buffer2 != NULL)) {
                LOGD("debuglog switch closed, flush gpsdbglog to flash from pingpang\r\n");
                if (g_hLogFile != NULL) {
                    gps_dbg_log_pingpang_flush(&ping_pang_buffer_body, g_hLogFile);
                }
                gps_dbg_log_pingpang_free();
                FILE_FCLOSE(g_hLogFile);
            }
        }
    }

    if (g_hLogFile != NULL) {
        if ((ping_pang_buffer_body.start_address_buffer1 != NULL)
            && (ping_pang_buffer_body.start_address_buffer2 != NULL)) {
            // flush
            LOGD("thread will exit,flush gpsdbglog to flash from pingpang\r\n");
            gps_dbg_log_pingpang_flush(&ping_pang_buffer_body, g_hLogFile);

            // free pingpang
            LOGD("free pingpang buffer now\r\n");
            gps_dbg_log_pingpang_free();
        }

        FILE_FCLOSE(g_hLogFile);
        g_hLogFile = NULL;
        LOGD("close log file\r\n");
    }

    pthread_exit(NULL);
}

int gps_dbg_log_thread_init() {
    pthread_t pthread_dbg_log;

    g_gpsdbglogThreadExit = false;
    pthread_create(&pthread_dbg_log, NULL, gps_dbg_log_thread, NULL);
    return 0;
}

int create_mtklogger2mnl_fd() {
    static int socket_fd = 0;

    socket_fd = socket_local_server("gpslogd", ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM);
    if (socket_fd < 0) {
        LOGE("create server fail(%s)", strerror(errno));
        return -1;
    }
    LOGD("socket_fd = %d", socket_fd);

    if (listen(socket_fd, 5) < 0) {
        LOGE("listen socket fail(%s)", strerror(errno));
        close(socket_fd);
        return -1;
    }

    return socket_fd;
}

static int FindGPSlogFile(char Filename[]) {
    int i;
    int GPSlogNamelen = 0;

    GPSlogNamelen = strlen(log_filename_suffix);

    for (i = 0; i < GPSlogNamelen; i++) {
        if (Filename[i]!= log_filename_suffix[i]) {
            return MTK_GPS_ERROR;
        }
    }
    return MTK_GPS_SUCCESS;
}

static int GetFileSize(char *filename) {
    char dir_filename[GPS_DEBUG_LOG_FILE_NAME_MAX_LEN];
    struct stat statbuff;

    memset(dir_filename, 0x00, sizeof(dir_filename));

    if (NULL == filename) {
        LOGE("[GetFileSize][error]: File name is NULL!!\r\n");
        return 0;
    }

    // LOGD("[GetFileSize]File name:%s!\r\n", filename);
    snprintf(dir_filename, sizeof(dir_filename), "%s%s", storagePath,
            filename);

    if (stat(dir_filename, &statbuff) < 0) {
        LOGE("[GetFileSize][error]: open file(%s) state fail(%s)!\r\n", dir_filename, strerror(errno));
        return 0;
    } else {
        return statbuff.st_size;  // return the file size
    }
}

static int CmpStrFile(char a[], char b[]) {  // compare two log files
    char *pa = a, *pb = b;
    while (*pa == *pb) {
        pa++;
        pb++;
    }
    return (*pa - *pb);
}

static int CmpFileTime(char *filename1, char *filename2) {
    char dir_filename1[GPS_DEBUG_LOG_FILE_NAME_MAX_LEN];
    char dir_filename2[GPS_DEBUG_LOG_FILE_NAME_MAX_LEN];
    struct stat statbuff1;
    struct stat statbuff2;

    memset(dir_filename1, 0x00, sizeof(dir_filename1));
    memset(dir_filename2, 0x00, sizeof(dir_filename2));

    if (NULL == filename1 || NULL == filename2) {
        LOGE("[CmpFileTime][error]: File name is NULL!!\r\n");
        return 0;
    }
    // LOGD("[CmpFileTime]File name1:%s, filename2:%s!!\r\n", filename1, filename2);

    snprintf(dir_filename1, sizeof(dir_filename1), "%s%s", storagePath, filename1);
    snprintf(dir_filename2, sizeof(dir_filename2), "%s%s", storagePath, filename2);

    if (stat(dir_filename1, &statbuff1) < 0) {
        LOGD("[CmpFileTime][error]: open file(%s) state  fail(%s)!!\r\n", dir_filename1, strerror(errno));
        return 0;
    }

    if (stat(dir_filename2, &statbuff2) < 0) {
        LOGD("[CmpFileTime][error]: open file(%s) state  fail(%s)!!\r\n", dir_filename2, strerror(errno));
        return 0;
    }
    return (statbuff1.st_mtime-statbuff2.st_mtime);
}

static INT32 create_debug_log_file() {
    time_t tm;
    struct tm *p;
    time(&tm);
    INT32 res;

    p = localtime(&tm);
    if (strlen((const char*)gps_debuglog_file_name) && (p != NULL)) {  // if filename length > 0
        // initialize debug log (use OPEN_ALWAYS to append debug log)
        // GPS debug log dir is not exit, mkdir, return value 0:success, -1 : fail
        if (0 != access(storagePath, F_OK)) {
            LOGE("access dir error(%s), Try to create dir\r\n", strerror(errno));
            if (mkdir(storagePath, 0775) == -1) {  // mkdir ,if fail print the fail info to main log
                LOGE("mkdir %s fail(%s)", storagePath, strerror(errno));
                return MTK_GPS_ERROR;
             }
        }

        memset(gsGpsLogFileName, 0x00, sizeof(gsGpsLogFileName));
        snprintf(gsGpsLogFileName, sizeof(gsGpsLogFileName), "%s.%04d%02d%02d%02d%02d%02d", gps_debuglog_file_name,
        1900 + p->tm_year, 1 + p->tm_mon, p->tm_mday,
        p->tm_hour, p->tm_min, p->tm_sec);
        Filenum = 0;
        FILE_FCLOSE(g_hLogFile);  // close file before open, if the file has been open.

        int LogFile_fd = open(gsGpsLogFileName, O_RDWR|O_NONBLOCK|O_CREAT, 0660);
        if (LogFile_fd < 0) {
            LOGE("open file fail(%s)", strerror(errno));
            return MTK_GPS_ERROR;
        } else {
            int flags = fcntl(LogFile_fd, F_GETFL, 0);
            if (fcntl(LogFile_fd, F_SETFL, flags|O_NONBLOCK) < 0) {
                LOGD("fcntl logFile_fd fail");
            }
            g_hLogFile = fdopen(LogFile_fd, "w");
        }
        LOGD("file(%s) created successfully(0x%x)\r\n", gsGpsLogFileName, (unsigned int)g_hLogFile);

        Current_FileSize = 0;
        return MTK_GPS_SUCCESS;
    }
    LOGD("create_debug_log_file fail");
    return MTK_GPS_ERROR;
}

/*****************************************************************************
 * FUNCTION
 *  mtklog_gps_set_debug_file
 * DESCRIPTION
 *  Set the GPS debug file name(include the path name) in running time
 * PARAMETERS
 *  file_name         [IN]   the debug file name needs to be changed
 * RETURNS
 *  success(MTK_GPS_SUCCESS); failure (MTK_GPS_ERROR)
 *****************************************************************************/
static INT32 mtklog_gps_set_debug_file(char* file_name) {
    if (NULL == file_name) {  // Null pointer, return error
        LOGE("file_name is NULL pointer! \r\n");
        return MTK_GPS_ERROR;
    }
// The length of file_name is too long, return error
    if (GPS_DEBUG_LOG_FILE_NAME_MAX_LEN <= (strlen(file_name) + 1)) {
        LOGE("file_name is too long! \r\n");
        return MTK_GPS_ERROR;
    }

    if (!g_pingpang_init) {
        if (MTK_GPS_ERROR == gps_dbg_log_pingpang_init()) {
            g_gpsdbglogThreadExit = true;
            LOGE("gps dbg log pingpang init fail, thread exit");
        }
    }

    if (MTK_GPS_ERROR == create_debug_log_file()) {
        LOGD("create debug file(%s) error\r\n", file_name);
    } else {  // Create file success, check dir size
        DirLogSize = gps_log_dir_check(storagePath);
    }

    return MTK_GPS_SUCCESS;
}

INT32 gps_log_dir_check(char * dirname) {   // file size check
    char temp_filename[GPS_DEBUG_LOG_FILE_NAME_MAX_LEN];
    char OldGpsFile[GPS_DEBUG_LOG_FILE_NAME_MAX_LEN] = {0};
    int DirLogSize_temp;
    DIR *p_dir;
    char OldFile[GPS_DEBUG_LOG_FILE_NAME_MAX_LEN];

    struct dirent *p_dirent;
    do {
        if (0 != access(dirname, F_OK|R_OK)) {  // Check if the dir exist, can read,return value 0:success, -1 : fail
            LOGE("Access gps debug log dir fail(%s)!\r\n", strerror(errno));
            return MTK_GPS_ERROR;
        }

        if ((p_dir = opendir(dirname)) == NULL) {
            LOGE("open dir error(%s)\r\n", strerror(errno));
            return MTK_GPS_ERROR;
        } else {
            LOGD("open dir sucess\r\n");
        }

        total_file_count = 0;
        memset(OldGpsFile, 0x00, GPS_DEBUG_LOG_FILE_NAME_MAX_LEN);  // For compare file name,set a max char value
        DirLogSize_temp = 0;

        while ((p_dirent = readdir(p_dir)) && !g_gpsdbglogThreadExit) {
            if (NULL == p_dirent || (0 != access(dirname, F_OK))) {  // return value 0:success, -1 : fail
                LOGE("Access gps debug log dir fail(%s)!\r\n", dirname);
                break;
            }
            if (strcmp(p_dirent->d_name, ".") == 0 || strcmp(p_dirent->d_name, "..") == 0) {  // Ignore the "." & ".."
                continue;  // while((p_dirent=readdir(p_dir)) && !g_gpsdbglogThreadExit)
            }

            if (GPS_DEBUG_LOG_FILE_NAME_MAX_LEN > strlen(p_dirent->d_name)) {
                memset(temp_filename, 0 , GPS_DEBUG_LOG_FILE_NAME_MAX_LEN);
                strncpy(temp_filename, (void *)&p_dirent->d_name, GPS_DEBUG_LOG_FILE_NAME_MAX_LEN - 1);
            } else {  // The length of d_name is too long, ignore this file
                LOGD("d_name is too long!\r\n");
                continue;
            }

            if (FindGPSlogFile(temp_filename) == MTK_GPS_SUCCESS) {
                // LOGD("%s is a GPS debug log file!\r\n", temp_filename);

                DirLogSize_temp = GetFileSize(temp_filename) + DirLogSize_temp;
                total_file_count++;
                if (strncmp(OldGpsFile, temp_filename, strlen(log_filename_suffix)) != 0) {
                    strncpy(OldGpsFile, temp_filename, GPS_DEBUG_LOG_FILE_NAME_MAX_LEN - 1);
                    LOGD("copy file name to OldGpsFile: %s, and continue\r\n", OldGpsFile);
                    continue;  // while((p_dirent=readdir(p_dir)) && !g_gpsdbglogThreadExit)
                }

                if (CmpFileTime(temp_filename, OldGpsFile) < 0) {  // Find the latest old file
                    memset(OldGpsFile, '\0', GPS_DEBUG_LOG_FILE_NAME_MAX_LEN);
                    strncpy(OldGpsFile, temp_filename, GPS_DEBUG_LOG_FILE_NAME_MAX_LEN - 1);
                }
                LOGD("DirLogSize_temp:%d, the latest OldGpsFile:%s!\r\n", DirLogSize_temp, OldGpsFile);
            }
        }
        closedir(p_dir);
        if (DirLogSize_temp >= (MAX_DBG_LOG_DIR_SIZE - MAX_DBG_LOG_FILE_SIZE)
            || (total_file_count > GPS_DBG_LOG_FILE_NUM_LIMIT)) {
            // Over size or the number of GPS debug log file over the limitation
            // when OldGpsFile is small, it will cause many re-calculation in the second loop.  need to avoid it.
            INT32 ret = 0;

            ret = GetFileSize(OldGpsFile);
            memset(OldFile, 0x00, sizeof(OldFile));
            snprintf(OldFile, sizeof(OldFile), "%s%s", storagePath, OldGpsFile);

            LOGD("need delete OldFile:%s\r\n", OldFile);

            if (remove(OldFile) != 0) {  // Error handle
                LOGW("Remove file %s error(%s)!\r\n", OldFile, strerror(errno));
            }
            DirLogSize_temp = DirLogSize_temp - ret;
        }

        LOGD("After remove file gpsdebug log dir size:%d!\r\n", DirLogSize_temp);
    }while (((DirLogSize_temp > (MAX_DBG_LOG_DIR_SIZE - MAX_DBG_LOG_FILE_SIZE))
    || (total_file_count > GPS_DBG_LOG_FILE_NUM_LIMIT)) && !g_gpsdbglogThreadExit);

    if ((DirLogSize_temp <= (MAX_DBG_LOG_DIR_SIZE - MAX_DBG_LOG_FILE_SIZE)) && g_gpsdbglogThreadExit) {
        LOGD("gps_log_dir_check interrupted size=%d!!\r\n", DirLogSize_temp);
    }
    LOGD("dir size:%d\r\n", DirLogSize_temp);
    return DirLogSize_temp;
}

INT32 mnl_sys_alps_gps_dbg2file_mnld(const char* buffer, UINT32 length) {
    INT32 ret = MTK_GPS_SUCCESS;
    UINT32 count = 0;

    if ((gps_debuglog_state == MTK_GPS_ENABLE_DEBUG_MSG_WR_BY_MNLD) \
                 && (ping_pang_buffer_body.next_write != NULL)) {
        gps_dbg_log_pingpang_copy(&ping_pang_buffer_body, buffer, length);
    } else {
        LOGD("will not copy to pingpang, DebugType:%d, buffer1:%p, buffer2:%p, g_hLogFile:%p\r\n",
            gps_debuglog_state, ping_pang_buffer_body.start_address_buffer1,
            ping_pang_buffer_body.start_address_buffer2, g_hLogFile);
    }

    return MTK_GPS_SUCCESS;
}

// if return error , gpsdbglog will not be writen
static INT32 gps_dbg_log_pingpang_init() {
    LOGD("gps_dbg_log_pingpang_init");
    memset(&ping_pang_buffer_body, 0x00, sizeof(ping_pang_buffer_body));
    if (((ping_pang_buffer_body.start_address_buffer1 = calloc(1, PINGPANG_BUFFER_SIZE)) == NULL) || \
        ((ping_pang_buffer_body.start_address_buffer2 = calloc(1, PINGPANG_BUFFER_SIZE)) == NULL)) {
        if (ping_pang_buffer_body.start_address_buffer1 != NULL) {
            free(ping_pang_buffer_body.start_address_buffer1);
            ping_pang_buffer_body.start_address_buffer1 = NULL;
        }
        if (ping_pang_buffer_body.start_address_buffer2 != NULL) {
            free(ping_pang_buffer_body.start_address_buffer2);
           ping_pang_buffer_body.start_address_buffer2 = NULL;
        }
        return MTK_GPS_ERROR;
    }
    lenth_to_write_buffer1 = 0;
    lenth_to_write_buffer2 = 0;
    ping_pang_buffer_body.p_buffer1_state = &buffer1_state;
    ping_pang_buffer_body.p_buffer2_state = &buffer2_state;
    buffer1_state = WRITING;
    buffer2_state = WRITABLE;
    ping_pang_buffer_body.p_buffer1_lenth_to_write = &lenth_to_write_buffer1;
    ping_pang_buffer_body.p_buffer2_lenth_to_write = &lenth_to_write_buffer2;
    ping_pang_buffer_body.end_address_buffer1 = ping_pang_buffer_body.start_address_buffer1 + PINGPANG_BUFFER_SIZE-2;
    ping_pang_buffer_body.end_address_buffer2 = ping_pang_buffer_body.start_address_buffer2 + PINGPANG_BUFFER_SIZE-2;
    ping_pang_buffer_body.next_write = ping_pang_buffer_body.start_address_buffer1;

    g_pingpang_init = true;
    return MTK_GPS_SUCCESS;
}

static INT32 gps_dbg_log_pingpang_copy(ping_pang_buffer* pingpang, const char* buffer, INT32 len) {
    if ((*(pingpang->p_buffer1_state) == WRITING) && (*(pingpang->p_buffer2_state) != WRITING)) {
        if (pingpang->next_write+len > pingpang->end_address_buffer1) {
            if (*(pingpang->p_buffer2_state) == WRITABLE) {
                *(pingpang->p_buffer1_lenth_to_write) = pingpang->next_write - pingpang->start_address_buffer1;
                pingpang->next_write = pingpang->start_address_buffer2;
                *(pingpang->p_buffer2_state) = WRITING;
                *(pingpang->p_buffer1_state) = READABLE;
                release_condition(&lock_for_sync[PINGPANG_WRITE_LOCK]);
            } else {
                memset(pingpang->start_address_buffer1, 0x0, PINGPANG_BUFFER_SIZE);
                pingpang->next_write = pingpang->start_address_buffer1;
                pingpang->num_loose++;
                LOGD("loose log ,num is %d \r\n", pingpang->num_loose);
            }
        }
    } else if ((*(pingpang->p_buffer2_state) == WRITING) && (*(pingpang->p_buffer1_state) != WRITING)) {
        if (pingpang->next_write+len > pingpang->end_address_buffer2) {
            if (*(pingpang->p_buffer1_state) == WRITABLE) {
                *(pingpang->p_buffer2_lenth_to_write) = pingpang->next_write - pingpang->start_address_buffer2;
                pingpang->next_write = pingpang->start_address_buffer1;
                *(pingpang->p_buffer1_state) = WRITING;
                *(pingpang->p_buffer2_state) = READABLE;
                release_condition(&lock_for_sync[PINGPANG_WRITE_LOCK]);
            } else {
                memset(pingpang->start_address_buffer2, 0x0, PINGPANG_BUFFER_SIZE);
                pingpang->next_write = pingpang->start_address_buffer2;
                pingpang->num_loose++;
                LOGD("loose log ,num is %d \r\n", pingpang->num_loose);
            }
        }
    } else {
        LOGE("abnormal happens\r\n");
        return MTK_GPS_ERROR;
    }

    memcpy(pingpang->next_write, buffer, len);
    pingpang->next_write += len;
    return MTK_GPS_SUCCESS;
}

// the real write to flash
static INT32 gps_dbg_log_pingpang_write(ping_pang_buffer* pingpang, FILE* filp) {
    LOGD("gps_dbg_log_pingpang_write");
    int count = 0;
    if ((*(pingpang->p_buffer1_state) == READABLE)\
        && (*(pingpang->p_buffer2_state) != READABLE)\
        && ((*(pingpang->p_buffer1_lenth_to_write)) != 0)) {
        *(pingpang->p_buffer1_state) = READING;
        count = fwrite(pingpang->start_address_buffer1, 1, *(pingpang->p_buffer1_lenth_to_write), filp);
        memset(pingpang->start_address_buffer1, 0x0, PINGPANG_BUFFER_SIZE);
        *(pingpang->p_buffer1_lenth_to_write) = 0;
        *(pingpang->p_buffer1_state) = WRITABLE;
    } else if ((*(pingpang->p_buffer2_state) == READABLE)\
        && (*(pingpang->p_buffer1_state) != READABLE)\
        && ((*(pingpang->p_buffer2_lenth_to_write)) != 0)) {
        *(pingpang->p_buffer2_state) = READING;
        count = fwrite(pingpang->start_address_buffer2, 1, *(pingpang->p_buffer2_lenth_to_write), filp);
        memset(pingpang->start_address_buffer2, 0x0, PINGPANG_BUFFER_SIZE);
        *(pingpang->p_buffer2_lenth_to_write) = 0;
        *(pingpang->p_buffer2_state) = WRITABLE;
    } else {
        return count;
    }
    return count;
}

// when mnl exit or mtklogger set 1 to 0, there is a need free pingpang buffer
static INT32 gps_dbg_log_pingpang_free() {
    if (ping_pang_buffer_body.start_address_buffer1 != NULL) {
        free(ping_pang_buffer_body.start_address_buffer1);
    }
    if (ping_pang_buffer_body.start_address_buffer2 != NULL) {
        free(ping_pang_buffer_body.start_address_buffer2);
    }
    memset(&ping_pang_buffer_body, 0x00, sizeof(ping_pang_buffer_body));
    lenth_to_write_buffer1 = 0;
    lenth_to_write_buffer2 = 0;
    buffer1_state = NOTINIT;
    buffer2_state = NOTINIT;

    g_pingpang_init = false;
    LOGD("free pingpang buffer\r\n");
    return MTK_GPS_SUCCESS;
}

// when mnl exit or mtklogger set 1 to 0, there is a need to flush the data to flash from buffer
static INT32 gps_dbg_log_pingpang_flush(ping_pang_buffer * pingpang, FILE* filp) {
    char* tmp_next_write = NULL;
    get_condition(&lock_for_sync[PINGPANG_FLUSH_LOCK]);
    tmp_next_write = pingpang->next_write;
    pingpang->next_write = NULL;   // make sure will not copy to pingpang again
    release_condition(&lock_for_sync[PINGPANG_FLUSH_LOCK]);
    if ((*(pingpang->p_buffer1_state) == WRITING) && (*(pingpang->p_buffer2_state) != WRITING)) {
        if (*(pingpang->p_buffer2_state) == READABLE) {
            fwrite(pingpang->start_address_buffer2, 1, *(pingpang->p_buffer2_lenth_to_write), filp);
        }
        fwrite(pingpang->start_address_buffer1, 1, tmp_next_write - pingpang->start_address_buffer1, filp);
    } else if ((*(pingpang->p_buffer2_state) == WRITING) && (*(pingpang->p_buffer1_state) != WRITING)) {
        if (*(pingpang->p_buffer1_state) == READABLE) {
            fwrite(pingpang->start_address_buffer1, 1, *(pingpang->p_buffer1_lenth_to_write), filp);
        }
        fwrite(pingpang->start_address_buffer2, 1, tmp_next_write - pingpang->start_address_buffer2, filp);
    } else {
        LOGE("abnormal happens\r\n");
    }
    LOGD("flush gpsdbg to flash done!\r\n");
    return MTK_GPS_SUCCESS;
}

