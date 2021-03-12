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

#include "mtk_gps_agps.h"
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
#define LOG_TAG "qepo"
#include <cutils/sockets.h>
#include <cutils/log.h>     /*logging in logcat*/
#define LOGD(fmt, arg ...) ALOGD("%s: " fmt, __FUNCTION__ , ##arg)
#define LOGW(fmt, arg ...) ALOGW("%s: " fmt, __FUNCTION__ , ##arg)
#define LOGE(fmt, arg ...) ALOGE("%s: " fmt, __FUNCTION__ , ##arg)
#endif

#define QUARTER_FILE_HAL "/data/misc/gps/QEPOHAL.DAT"
#define GPS_EPO_FILE_LEN  20

static char quarter_epo_file_name[GPS_EPO_FILE_LEN] = {0};
static char quarter_epo_md_file_name[GPS_EPO_FILE_LEN] = {0};

static int qepo_file_update_impl();
typedef enum {
    MAIN2QEPO_EVENT_START            = 0,
} main2qepo_event;

typedef struct qepo_gps_time {
    int wn;
    int tow;
    int sys_time;
}QEPO_GPS_TIME_T;

static QEPO_GPS_TIME_T gps_time;
static int g_fd_qepo;
static int qepo_download_finished = 1;
extern int gps_epo_type;
/////////////////////////////////////////////////////////////////////////////////
// static functions

static int pre_day = 0;
static int server_not_updated = 0;
static int Qepo_res = 0;
static int counter = 1;

static CURLcode curl_easy_download_quarter_epo(void) {
    int res_val;
    CURLcode res;
    char gps_epo_md_file_temp[40] = {0};
    char gps_epo_md_key[80] = {0};
    char* url = NULL;
    char* key = NULL;
    char* md_url = NULL;
    char count_str[15]={0};

    strcat(gps_epo_md_file_temp, "/data/misc/gps/");
    strcat(gps_epo_md_file_temp, quarter_epo_md_file_name);

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
    md_url = getEpoUrl(quarter_epo_md_file_name, gps_epo_md_key);
    if (md_url == NULL) {
        LOGD("getEpoUrl failed!\n");
        return CURLE_FAILED_INIT;
    }
    res = curl_easy_download(md_url, gps_epo_md_file_temp);
    //LOGD("md_url = %s, md file curl_easy_download res = %d\n", md_url, res);
    free(md_url);
    memset(gps_epo_md_key, 0 , sizeof(gps_epo_md_key));
    if (res == 0) {
        FILE *fp = NULL;
        char* key_temp = NULL;
        int len = 0;
        fp = fopen(gps_epo_md_file_temp, "r");
        if (NULL == fp) {
            strcpy(gps_epo_md_key, "0000000000000000");
        } else {
            len = fread(gps_epo_md_key, sizeof(char), sizeof(char)*48, fp);
            key_temp = gps_epo_md_key;
            //LOGD("gps_epo_md_key before cpy= %s, len=%d\n", gps_epo_md_key, len);
            memcpy(gps_epo_md_key, key_temp+32, 16);  // comment by rayjf li
            gps_epo_md_key[16] = '\0';
            fclose(fp);
            unlink(gps_epo_md_file_temp);
            //LOGD("gps_epo_md_key = %s\n", gps_epo_md_key);
        }
        counter = 1;
    } else {
        counter++;
        strcpy(gps_epo_md_key, "0000000000000000");
    }
    memset(count_str, 0, sizeof(count_str));
    sprintf(count_str, "%d", counter);
    strcat(gps_epo_md_key, "&counter=");
    strcat(gps_epo_md_key, count_str);
    if (counter <= 1) {
        strcat(gps_epo_md_key, "&url=1");
    } else {
        strcat(gps_epo_md_key, "&url=0");
    }
    url = getEpoUrl(quarter_epo_file_name, gps_epo_md_key);

    //LOGD("url = %s\n", url);
    if (url == NULL) {
        LOGD("getEpoUrl failed!\n");
        return CURLE_FAILED_INIT;
    }
    res = curl_easy_download(url, QUARTER_FILE_HAL);
    LOGD("qepo curl_easy_download res = %d\n", res);
    free(url);
    Qepo_res = res;
    if (res == 0) {
        counter = 1;
        res_val = chmod(QUARTER_FILE_HAL, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IROTH);
        //LOGD("chmod res_val = %d, %s\n", res_val, strerror(errno));
    } else {
        counter++;
    }
    return res;
}
/*****************************************************************************/
static int is_quarter_epo_valid(void) {
    unsigned int u4GpsSecs_start;  // GPS seconds
    time_t uSecond_start;   // UTC seconds
    time_t mnl_time;
    time_t *mnl_gps_time = NULL;
    int fd = 0;
    int ret = EPO_DOWNLOAD_RESULT_FAIL;

    fd = open(QUARTER_FILE_HAL, O_RDONLY);
    if (fd < 0) {
        LOGE("Open QEPO fail, return\n");
        return ret;
    } else {
        if (mtk_gps_sys_epo_period_start(fd, &u4GpsSecs_start, &uSecond_start)) {
            LOGE("Read QEPO file failed\n");
            close(fd);
            return ret;
        } else {
            mnl_gps_time = &mnl_time;
            //LOGD("gps_time.wn, tow %d, %d\n", gps_time.wn, gps_time.tow);
            GpsToUtcTime(gps_time.wn, gps_time.tow, mnl_gps_time);
            LOGD("The Start time of QEPO file is %lld\n", (long long)uSecond_start);
            LOGD("The start time of QEPO file is %s\n", ctime(&uSecond_start));
            LOGD("GPS time: %s\n", ctime(mnl_gps_time));

            if ((mnl_time >= uSecond_start) && (mnl_time < ((6*60*60) + uSecond_start))) {
                ret = EPO_DOWNLOAD_RESULT_SUCCESS;
            } else {
                ret = EPO_DOWNLOAD_RESULT_FAIL;
                if (uSecond_start >= ((18*60*60) + mnl_time)) {
                    // download time 23:55, server has updated
                    pre_day = 1;
                } else {
                    pre_day = 0;
                }
                if (mnl_time >= ((24*60*60) + uSecond_start)) {
                   // download time 00:04,server has not updated
                   server_not_updated = 1;
                } else {
                    server_not_updated = 0;
                }
            }
        }
        close(fd);
    }
    return ret;
}

static void
gps_download_quarter_epo_file_name(int count) {
    if (gps_epo_type == 1) {
        if (count == 1) {
            strcpy(quarter_epo_file_name, "QG_R_1.DAT");
            strcpy(quarter_epo_md_file_name, "QG_R_1.MD5");
        } else if (count == 2) {
            strcpy(quarter_epo_file_name, "QG_R_2.DAT");
            strcpy(quarter_epo_md_file_name, "QG_R_2.MD5");
        } else if (count == 3) {
            strcpy(quarter_epo_file_name, "QG_R_3.DAT");
            strcpy(quarter_epo_md_file_name, "QG_R_3.MD5");
        } else if (count == 4) {
            strcpy(quarter_epo_file_name, "QG_R_4.DAT");
            strcpy(quarter_epo_md_file_name, "QG_R_4.MD5");
        } else if (count == 5) {
            strcpy(quarter_epo_file_name, "QG_R_5.DAT");
            strcpy(quarter_epo_md_file_name, "QG_R_5.MD5");
        }
        //LOGD("quarter_epo_file_name=%s, quarter_epo_md_file_name=%s\n",
        //quarter_epo_file_name, quarter_epo_md_file_name);
    } else if (gps_epo_type == 0) {
        if (count == 1) {
            strcpy(quarter_epo_file_name, "QG_R_1.DAT");
            strcpy(quarter_epo_md_file_name, "QG_R_1.MD5");
        } else if (count == 2)  {
            strcpy(quarter_epo_file_name, "QG_R_2.DAT");
            strcpy(quarter_epo_md_file_name, "QG_R_2.MD5");
        } else if (count == 3) {
            strcpy(quarter_epo_file_name, "QG_R_3.DAT");
            strcpy(quarter_epo_md_file_name, "QG_R_3.MD5");
        } else if (count == 4) {
            strcpy(quarter_epo_file_name, "QG_R_4.DAT");
            strcpy(quarter_epo_md_file_name, "QG_R_4.MD5");
        } else if (count == 5) {
            strcpy(quarter_epo_file_name, "QG_R_5.DAT");
            strcpy(quarter_epo_md_file_name, "QG_R_5.MD5");
        }
        //LOGD("quarter_epo_file_name=%s, quarter_epo_md_file_name=%s\n",
        //quarter_epo_file_name, quarter_epo_md_file_name);
    }
}

void gps_mnl_set_gps_time(int wn, int tow, int sys_time) {
    gps_time.wn = wn;
    gps_time.tow = tow;
    gps_time.sys_time = sys_time;
}
static void quarter_epo_download_process(void) {
    // LOGD("quarter_epo_download_process begin");
    int index = 1;
    INT32 SecofDay = gps_time.tow % 86400;

    if ((SecofDay > 300) && (SecofDay <= 21900)) {
        index = 1;
    } else if ((SecofDay > 21900) && (SecofDay <= 43500)) {
        index = 2;
    } else if ((SecofDay > 43500) && (SecofDay <= 65100)) {
        index = 3;
    } else if ((SecofDay > 65100) && (SecofDay <= 85500)) {
        index = 4;
    } else if ((SecofDay <= 300) || (SecofDay > 85500)) {
        if (server_not_updated) {
            index = 4;
        } else {
            index = 5;
        }
    }

    if (pre_day) {
        index = 5;
    }
    //LOGD("SecofDay = %d , index = %d\n", SecofDay, index);
    gps_download_quarter_epo_file_name(index);
    curl_easy_download_quarter_epo();
}

static int qepo_file_update_impl() {
    int try_time = 10;  // for network issue download failed retry.
    int qepo_valid = EPO_DOWNLOAD_RESULT_FAIL;

    //LOGD("qepo_download_finished = 0\n");
    quarter_epo_download_process();
    while (((qepo_valid = is_quarter_epo_valid()) == EPO_DOWNLOAD_RESULT_FAIL)
           && (try_time > 0) && is_network_connected()) {
        try_time--;

        LOGD("qepo retry again, try_time = %d\n", try_time);
        quarter_epo_download_process();
    }
    LOGD("try time is %d, qepo_valid is %d\n", try_time, qepo_valid);
    if (try_time < 10) {
        try_time = 10;
    }
    if (server_not_updated) {
        Qepo_res = CURLE_RECV_ERROR;  // server has not updated
    }
    return qepo_valid;  // success
}

void qepo_update_quarter_epo_file(int qepo_valid) {
    if (qepo_valid == EPO_DOWNLOAD_RESULT_SUCCESS) {
        int qdownload_status;
        int ret = mtk_agps_agent_qepo_file_update();
        if (ret == MTK_GPS_ERROR) {
            qdownload_status = MTK_QEPO_RSP_UPDATE_FAIL;
        } else {
            qdownload_status = MTK_QEPO_RSP_UPDATE_SUCCESS;
            unlink(QEPO_UPDATE_HAL);
        }
        LOGD("qdownload_status = %d\n", qdownload_status);
        if ((mtk_agps_set_param (MTK_PARAM_QEPO_DOWNLOAD_RESPONSE,
            &qdownload_status, MTK_MOD_DISPATCHER, MTK_MOD_AGENT))) {
            LOGE("GPS QEPO update fail\n");
        }
    } else {
        int qdownload_status = Qepo_res;
        int ret;
        if (qdownload_status > 0) {
            qdownload_status = MTK_QEPO_RSP_DOWNLOAD_FAIL;
        }
        if ((ret = mtk_agps_set_param (MTK_PARAM_QEPO_DOWNLOAD_RESPONSE,
            &qdownload_status, MTK_MOD_DISPATCHER, MTK_MOD_AGENT))) {
            LOGE("GPS QEPO update status fail\n");
        }
    }
}

//////////////////////////////////////////////////////////////////////////////////
// MAIN -> EPO Download (handlers)
static int mnld_qepo_download() {
    //LOGD("mnld_qepo_download");

    qepo_download_finished = 0;
    int ret = qepo_file_update_impl();
    qepo_download_finished = 1;
    mnld_qepo_download_done(ret);
    return ret;
}

int is_qepo_download_finished() {
    return qepo_download_finished;
}
static int qepo_event_hdlr(int fd) {
    char buff[MNLD_INTERNAL_BUFF_SIZE] = {0};
    int offset = 0;
    main2qepo_event cmd;
    int read_len;

    read_len = safe_recvfrom(fd, buff, sizeof(buff));
    if (read_len <= 0) {
        LOGE("qepo_event_hdlr() safe_recvfrom() failed read_len=%d", read_len);
        return -1;
    }

    cmd = get_int(buff, &offset);
    switch (cmd) {
    case MAIN2QEPO_EVENT_START: {
        LOGW("mnld_qepo_download() before");
        // need to call mnld_qepo_download_done() when QEPO download is done
        mnld_qepo_download();
        LOGW("mnld_qepo_download() after");
        break;
    }
    default: {
        LOGE("qepo_event_hdlr() unknown cmd=%d", cmd);
        return -1;
    }
    }
    return 0;
}

static void qepo_downloader_thread_timeout() {
    LOGE("qepo_downloader_thread_timeout() crash here for debugging");
    CRASH_TO_DEBUG();
}

static void* qepo_downloader_thread(void *arg) {
    #define MAX_EPOLL_EVENT 50
    //timer_t hdlr_timer = init_timer(qepo_downloader_thread_timeout);
    struct epoll_event events[MAX_EPOLL_EVENT];
    UNUSED(arg);

    int epfd = epoll_create(MAX_EPOLL_EVENT);
    if (epfd == -1) {
        LOGE("qepo_downloader_thread() epoll_create failure reason=[%s]%d",
            strerror(errno), errno);
        return 0;
    }

    if (epoll_add_fd(epfd, g_fd_qepo) == -1) {
        LOGE("qepo_downloader_thread() epoll_add_fd() failed for g_fd_qepo failed");
        return 0;
    }

    while (1) {
        int i;
        int n;
        //LOGD("qepo_downloader_thread wait");
        n = epoll_wait(epfd, events, MAX_EPOLL_EVENT , -1);
        if (n == -1) {
            if (errno == EINTR) {
				LOGW("qepo_downloader_thread EINTR");
                continue;
            } else {
                LOGE("qepo_downloader_thread() epoll_wait failure reason=[%s]%d",
                    strerror(errno), errno);
                return 0;
            }
        }
        //start_timer(hdlr_timer, MNLD_QEPO_HANDLER_TIMEOUT);
        for (i = 0; i < n; i++) {
            if (events[i].data.fd == g_fd_qepo) {
                if (events[i].events & EPOLLIN) {
                    qepo_event_hdlr(g_fd_qepo);
                }
            } else {
                LOGE("qepo_downloader_thread() unknown fd=%d",
                    events[i].data.fd);
            }
        }
        //stop_timer(hdlr_timer);
    }

    LOGE("qepo_downloader_thread() exit");
    return 0;
}

int qepo_downloader_start() {
    char buff[MNLD_INTERNAL_BUFF_SIZE] = {0};
    int offset = 0;
    put_int(buff, &offset, MAIN2QEPO_EVENT_START);
    return safe_sendto(MNLD_QEPO_DOWNLOAD_SOCKET, buff, offset);
}

int qepo_downloader_init() {
    pthread_t pthread_qepo;

    g_fd_qepo = socket_bind_udp(MNLD_QEPO_DOWNLOAD_SOCKET);
    if (g_fd_qepo < 0) {
        LOGE("socket_bind_udp(MNLD_QEPO_DOWNLOAD_SOCKET) failed");
        return -1;
    }

    pthread_create(&pthread_qepo, NULL, qepo_downloader_thread, NULL);
    return 0;
}

