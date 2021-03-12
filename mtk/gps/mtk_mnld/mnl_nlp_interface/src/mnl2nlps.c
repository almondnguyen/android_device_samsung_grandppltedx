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

#define MTK_LOG_ENABLE 1
#include "data_coder.h"
#include "mnl2nlps.h"

#ifdef LOGD
#undef LOGD
#endif
#ifdef LOGW
#undef LOGW
#endif
#ifdef LOGE
#undef LOGE
#endif

#if 1  // defined(ANDROID)
#define LOG_TAG "MNLD"
#include <cutils/sockets.h>
#include <cutils/log.h>     /*logging in logcat*/
#define LOGD(fmt, arg ...) ALOGD("%s: " fmt, __FUNCTION__ , ##arg)
#define LOGW(fmt, arg ...) ALOGW("%s: " fmt, __FUNCTION__ , ##arg)
#define LOGE(fmt, arg ...) ALOGE("%s: " fmt, __FUNCTION__ , ##arg)
#endif

//================

#define MNL_TO_NLPS_SOCKET_NAME "com.mediatek.nlpservice.NlpService"
#define MNL2NLP_BUFF_SIZE 32

typedef enum {
    MNL_NLPS_CMD_QUIT          = 100,  // Say good bye to server
    MNL_NLPS_CMD_GPS_NIJ_REQ   = 101  // Request a NLP data
} mnl_nlps_cmd_enum_t;

//================

static int mnl2nlp_connect() {
    return socket_local_client(MNL_TO_NLPS_SOCKET_NAME, ANDROID_SOCKET_NAMESPACE_ABSTRACT,
        SOCK_STREAM | SOCK_CLOEXEC | SOCK_NONBLOCK);
}

static int close_fd(int fd) {
    int ret = close(fd);
    if (ret != 0) {
        LOGE("[nlp] close fd %d error:%s\n", fd, strerror(errno));
    } else {
        LOGD("[nlp] close fd %d ok\n", fd);
    }
    return ret;
}

static int fillCmd3(char *buff, mnl_nlps_cmd_enum_t cmd, int data1, int data2, int data3) {
    int offset = 0;

    put_int(buff, &offset, cmd);
    put_int(buff, &offset, data1);
    put_int(buff, &offset, data2);
    put_int(buff, &offset, data3);
    return offset;
}

static int write_cmd3(int socket_fd, mnl_nlps_cmd_enum_t cmd, int data1, int data2, int data3) {
    char buff[MNL2NLP_BUFF_SIZE];
    int len = fillCmd3(buff, cmd, data1, data2, data3);
    int wlen = write(socket_fd, buff, len);
    int ret = 0;  // ok

    if (len != wlen) {
        LOGE("[nlp] cmd(%d) error:%s\n", cmd, strerror(errno));
        ret = -1;  // error
    } else {
        LOGD("[nlp] cmd(%d) ok\n", cmd);
    }
    return ret;
}

static int write_cmd0(int socket_fd, mnl_nlps_cmd_enum_t cmd) {
    return write_cmd3(socket_fd, cmd, 0x12345678, 0x13572468, 0x97651562);
}

static int mnl2nlp_send_cmd3_and_quit(mnl_nlps_cmd_enum_t cmd, int data1, int data2, int data3) {
    int ret;
    int socket_fd = mnl2nlp_connect();
    LOGD("[nlp] socket_fd: %d", socket_fd);
    if (socket_fd < 0) {
        LOGE("[nlp] connect error:%s\n", strerror(errno));
        return -1;
    }
    ret = write_cmd3(socket_fd, cmd, data1, data2, data3);
    if (ret == 0) {
        ret = write_cmd0(socket_fd, MNL_NLPS_CMD_QUIT);
    }

    close_fd(socket_fd);

    return ret;
}

static int mnl2nlp_send_cmd0_and_quit(mnl_nlps_cmd_enum_t cmd) {
    return mnl2nlp_send_cmd3_and_quit(cmd, 0, 0, 0);
}

// ret val: 0 (OK), -1 (error)
int mnl2nlp_request_nlp() {
    LOGD("[nlp] request_nlp");
    return mnl2nlp_send_cmd0_and_quit(MNL_NLPS_CMD_GPS_NIJ_REQ);
}
