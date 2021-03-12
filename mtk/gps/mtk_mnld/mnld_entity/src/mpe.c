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
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <pthread.h>
#include <errno.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <private/android_filesystem_config.h>


#include "mtk_lbs_utility.h"
#include "mnld.h"
#include "mtk_gps.h"
#include "mpe.h"
#include "gps_dbg_log.h"

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
#define LOGD(...) tag_log(1, "[mpe]", __VA_ARGS__);
#define LOGW(...) tag_log(1, "[mpe] WARNING: ", __VA_ARGS__);
#define LOGE(...) tag_log(1, "[mpe] ERR: ", __VA_ARGS__);
#else
#define LOG_TAG "mpe"
#include <cutils/sockets.h>
#include <cutils/log.h>     /*logging in logcat*/
#define LOGD(fmt, arg ...) ALOGD("%s: " fmt, __FUNCTION__ , ##arg)
#define LOGW(fmt, arg ...) ALOGW("%s: " fmt, __FUNCTION__ , ##arg)
#define LOGE(fmt, arg ...) ALOGE("%s: " fmt, __FUNCTION__ , ##arg)
#endif

#define MPEMNL_UDP_CLI_PATH "/data/mpe_mnl/mnl2mpe"
#define MPEMNL_UDP_SRV_PATH "/data/mpe_mnl/mpe2mnl"

#define CMD_SEND_FROM_MNLD  0x40   // with payload, mtklogger recording setting & path
#define CMD_MPED_REBOOT_DONE  0x41

typedef struct {
  UINT32    type;           /* message ID */
  UINT32    length;         /* length of 'data' */
} TOMPE_MSG;

static int g_fd_mpe = -1;
MPECallBack gMpeCallBackFunc = NULL;
static int create_mpe2mnl_fd() {
    int sockfd;
    struct sockaddr_un soc_addr;
    socklen_t addr_len;

    sockfd = socket(AF_LOCAL, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        LOGE("socket failed reason=[%s]\n", strerror(errno));
        return -1;
    }

    strcpy(soc_addr.sun_path, MPEMNL_UDP_SRV_PATH);
    soc_addr.sun_family = AF_LOCAL;
    addr_len = (offsetof(struct sockaddr_un, sun_path) + strlen(soc_addr.sun_path) + 1);

    unlink(soc_addr.sun_path);
    if (bind(sockfd, (struct sockaddr *)&soc_addr, addr_len) < 0) {
        LOGE("bind failed path=[%s] reason=[%s]\n", MPEMNL_UDP_SRV_PATH, strerror(errno));
        close(sockfd);
        return -1;
    }
    if (chmod(MPEMNL_UDP_SRV_PATH, 0660) < 0)
        LOGE("chmod error: %s", strerror(errno));
    if (chown(MPEMNL_UDP_SRV_PATH, -1, AID_INET))
        LOGE("chown error: %s", strerror(errno));
    return sockfd;
}

static int mnl_sendto_mpe(void* dest, char* buf, int size) {
    int ret = 0;
    struct sockaddr_un soc_addr;
    socklen_t addr_len;
    int retry = 10;
    int sockfd = socket(AF_LOCAL, SOCK_DGRAM, 0);

    if (sockfd < 0) {
        LOGE("safe_sendto() socket() failed reason=[%s]%d",
            strerror(errno), errno);
        return -1;
    }
    LOGD("mnld2mpe fd: %d\n", sockfd);
    strcpy(soc_addr.sun_path, dest);
    soc_addr.sun_family = AF_UNIX;
    addr_len = (offsetof(struct sockaddr_un, sun_path) + strlen(soc_addr.sun_path) + 1);
    while ((ret = sendto(sockfd, buf, size, 0,
        (const struct sockaddr *)&soc_addr, (socklen_t)addr_len)) == -1) {
        if (errno == EINTR) continue;
        if (errno == EAGAIN) {
            if (retry-- > 0) {
                usleep(100 * 1000);
                continue;
            }
        }
        LOGE("[mnld2mpe] ERR: sendto dest=[%s] len=%d reason =[%s]\n",
            (char *)dest, size, strerror(errno));
        break;
    }
    close(sockfd);
    return ret;
}

int mnl2mpe_set_log_path(char* path, int status_flag, int mode_flag) {
    char mpemsg[256] = {0};
    char path_tmp[GPS_DEBUG_LOG_FILE_NAME_MAX_LEN] = {0};
    int log_rec, rec_loc;
    int mpe_len;
    TOMPE_MSG msg_head;

    msg_head.type = (UINT32)CMD_SEND_FROM_MNLD;
    msg_head.length = GPS_DEBUG_LOG_FILE_NAME_MAX_LEN + 2*sizeof(UINT32);
    log_rec = status_flag;
    rec_loc = mode_flag;
    strcpy(path_tmp, path);
    mpe_len = msg_head.length + sizeof(TOMPE_MSG);
    memcpy(mpemsg, &msg_head, sizeof(TOMPE_MSG));
    memcpy((mpemsg + sizeof(TOMPE_MSG)), &log_rec, sizeof(UINT32));
    memcpy((mpemsg + sizeof(TOMPE_MSG)+ sizeof(UINT32)), &rec_loc, sizeof(UINT32));
    memcpy((mpemsg + sizeof(TOMPE_MSG)+ 2*sizeof(UINT32)) , path_tmp, GPS_DEBUG_LOG_FILE_NAME_MAX_LEN);
    if (-1 == mnl_sendto_mpe(MPEMNL_UDP_CLI_PATH, mpemsg, mpe_len)) {
        LOGE("[MNL2MPE]Send to MPE failed, %s\n", strerror(errno));
        return MTK_GPS_ERROR;
    } else {
        LOGD("[MNL2MPE]Send to MPE successfully\n");
    }
    return 0;
}

static int mpe2mnl_hdlr(int sock) {  /*sent from mpe*/
    char buff[128] = {0};
    int len = 0;
    TOMPE_MSG msg_head;

    len = safe_recvfrom(sock, buff, sizeof(buff));
    memcpy(&msg_head, buff, sizeof(TOMPE_MSG));
    LOGD("len=%d buff:%d\n", len, msg_head.type);

    switch (msg_head.type) {
        case CMD_MPED_REBOOT_DONE: {
            mtklogger_mped_reboot_message_update();
            break;
        }
        case CMD_START_MPE_RES:
        case CMD_STOP_MPE_RES:
        case CMD_SEND_SENSOR_RAW_RES:
        case CMD_SEND_SENSOR_CALIBRATION_RES:
        case CMD_SEND_SENSOR_FUSION_RES:
        case CMD_SEND_PDR_STATUS_RES:
        case CMD_SEND_ADR_STATUS_RES:
        case CMD_SEND_GPS_TIME_REQ: {
            if (mnld_is_gps_started_done()) {
                if (len > 0) {
                    mtk_gps_mnl_get_sensor_info((UINT8 *)buff, len);
                }
            }
            break;
        }
        default: {
           LOGE("unknown cmd=%d\n", msg_head.type);
           break;
       }
    }
    LOGD("end\n");
    return 0;
}

void mtk_gps_mnl_set_sensor_info(UINT8 *msg, int len) {
    char buff[128] = {0};

    LOGD("len=%d msg:%s\n", len, msg);
    if ((len > 0) && (msg != NULL)) {
        memcpy(&buff[0], msg, len);
        if (-1 == mnl_sendto_mpe(MPEMNL_UDP_CLI_PATH, buff, len)) {
            LOGE("[MNL2MPE]Send to MPE failed, %s\n", strerror(errno));
        } else {
            LOGD("[MNL2MPE]Send to MPE successfully\n");
        }
    }
}

int mtk_gps_mnl_trigger_mpe(void) {
    UINT16 mpe_len;
    char buff[128] = {0};
    int ret = MTK_GPS_ERROR;

    LOGD("mtk_gps_mnl_trigger_mpe\n");
    mpe_len = mtk_gps_set_mpe_info((UINT8 *)buff);
    LOGD("mpemsg len=%d\n", mpe_len);
    if ((mpe_len > 0)) {
        if (-1 == mnl_sendto_mpe(MPEMNL_UDP_CLI_PATH, buff, mpe_len)) {
            LOGE("[MNL2MPE]Send to MPE failed, %s\n", strerror(errno));
        } else {
            ret = MTK_GPS_SUCCESS;
            LOGD("[MNL2MPE]Send to MPE successfully\n");
        }
    }
    return ret;
}

static void mpe_thread_timeout() {
    LOGE("mpe_thread_timeout() crash here for debugging");
    CRASH_TO_DEBUG();
}
void *mnld_mpe_thread(void * arg) {
    #define MAX_EPOLL_EVENT 50
    timer_t hdlr_timer = init_timer(mpe_thread_timeout);
    struct epoll_event events[MAX_EPOLL_EVENT];
    UNUSED(arg);

    int mpefd = epoll_create(MAX_EPOLL_EVENT);
    if (mpefd == -1) {
        LOGE("epoll_create failure reason=[%s]%d\n",
          strerror(errno), errno);
        return 0;
    }

    if (epoll_add_fd(mpefd, g_fd_mpe) == -1) {
        LOGE("epoll_add_fd() failed for g_fd_epo failed");
        return 0;
    }
    while (1) {
        int i;
        int n;
        LOGD("mnld_mpe_thread wait");
        n = epoll_wait(mpefd, events, MAX_EPOLL_EVENT , -1);
        if (n == -1) {
            if (errno == EINTR) {
                continue;
            } else {
                LOGE("epoll_wait failure reason=[%s]%d",
                  strerror(errno), errno);
                return 0;
            }
        }
        start_timer(hdlr_timer, MNLD_MPE_HANDLER_TIMEOUT);
        for (i = 0; i < n; i++) {
            if (events[i].data.fd == g_fd_mpe) {
                if (events[i].events & EPOLLIN) {
                    mpe2mnl_hdlr(g_fd_mpe);
                }
            } else {
                LOGE("unknown fd=%d",
                events[i].data.fd);
            }
        }
        stop_timer(hdlr_timer);
    }

    LOGE("exit");
    return 0;
}

int mnl_mpe_thread_init() {
    int ret;
    LOGD("mpe enabled");

    gMpeCallBackFunc = mtk_gps_mnl_trigger_mpe;
    ret = mtk_gps_mnl_mpe_callback_reg((MPECallBack *)gMpeCallBackFunc);
    LOGD("register mpe cb %d,gMpeCallBackFunc= %p,mtk_gps_mnl_trigger_mpe=%p\n",
        ret, gMpeCallBackFunc, mtk_gps_mnl_trigger_mpe);
    return 0;
}

int mpe_function_init(void) {
    pthread_t pthread_mpe;

    g_fd_mpe = create_mpe2mnl_fd();
    if (g_fd_mpe < 0) {
        LOGE("socket_bind_udp(mpe2mnl) failed");
        return -1;
    }
    if (pthread_create(&pthread_mpe, NULL, mnld_mpe_thread, NULL)) {
        LOGE("mnlmpe thread init failed");
    } else {
        LOGD("mnlmpe_thread create ok!! \n");
    }
    return 0;
}
