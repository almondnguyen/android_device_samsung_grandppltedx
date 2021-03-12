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

#define _MNL_COMMON_C_
/*******************************************************************************
* Dependency
*******************************************************************************/
#include <stdio.h>   /* Standard input/output definitions */
#include <string.h>  /* String function definitions */
#include <stdlib.h>
#include <sys/time.h>
#include <cutils/properties.h>
#include "mnl_common.h"
#include "mtk_lbs_utility.h"

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
#define LOGD(...) tag_log(1, "[mnl_linux]", __VA_ARGS__);
#define LOGW(...) tag_log(1, "[mnl_linux] WARNING: ", __VA_ARGS__);
#define LOGE(...) tag_log(1, "[mnl_linux] ERR: ", __VA_ARGS__);
#else
#define LOG_TAG "mnl_linux"
#include <cutils/sockets.h>
#include <cutils/log.h>     /*logging in logcat*/
#define LOGD(fmt, arg ...) ALOGD("%s: " fmt, __FUNCTION__ , ##arg)
#define LOGW(fmt, arg ...) ALOGW("%s: " fmt, __FUNCTION__ , ##arg)
#define LOGE(fmt, arg ...) ALOGE("%s: " fmt, __FUNCTION__ , ##arg)
#endif
#if ANDROID_MNLD_PROP_SUPPORT
#define MNL_CONFIG_STATUS "persist.radio.mnl.prop"
#endif
/*******************************************************************************
* structure & enumeration
*******************************************************************************/

/******************************************************************************
 * Functions
******************************************************************************/
#if defined(READ_PROPERTY_FROM_FILE)
/******************************************************************************
Sample for mnl.prop: (for emulator)
-------------------------------------------------------------------------------
init.speed=38400
link.speed=38400
dev.dsp=/dev/ttyS3
dev.gps=/dev/gps
bee.path=/bee
pmtk.conn=socket
pmtk.socket.port=7000
#pmtk.conn=serial
#pmtk.serial.dev=/dev/ttygserial
debug.nema=1 (0:none; 1:normal; 2:full)
debug.mnl=1
******************************************************************************/
const char *mnl_prop_path[] = {
    "/data/misc/gps/mnl.prop",   /*mainly for target*/
    "/sbin/mnl.prop",   /*mainly for emulator*/
};
#define PROPBUF_SIZE 512
static char propbuf[512];
extern int epo_setconfig;
#define IS_SPACE(ch) ((ch == ' ') || (ch == '\t') || (ch == '\n'))
/******************************************************************************
* Read property from file and overwritting the existing property
******************************************************************************/
int get_prop(char *pStr, char** ppKey, char** ppVal) {
    int len = (int)strlen(pStr);
    char *end = pStr + len;
    char *key = NULL, *val = NULL;
    int stage = 0;

    if (!len) {
        return -1;    // no data
    }
    else if (pStr[0] == '#') {   /*ignore comment*/
        *ppKey = *ppVal = NULL;
        return 0;
    } else if (pStr[len-1] != '\n') {
        if (len >= PROPBUF_SIZE-1) {
            LOGE("%s: buffer is not enough!!\n", __FUNCTION__);
            return -1;
        } else {
            pStr[len] = '\n';
        }
    }
    key = pStr;
    while ((*pStr != '=') && (pStr < end)) pStr++;
    if (pStr >= end) {
        LOGE("%s: '=' is not found!!\n", __FUNCTION__);
        return -1;    // format error
    }

    *pStr++ = '\0';
    while (IS_SPACE(*pStr) && (pStr < end)) pStr++;    // skip space chars
    val = pStr;
    while (!IS_SPACE(*pStr) && (pStr < end)) pStr++;
    *pStr = '\0';
    *ppKey = key;
    *ppVal = val;
    return 0;
}
/*****************************************************************************/
int set_prop(MNL_CONFIG_T* prConfig, char* key, char* val) {
    if (!strcmp(key, "init.speed")) {
        prConfig->init_speed = atoi(val);
    } else if (!strcmp(key, "link.speed")) {
        prConfig->link_speed = atoi(val);
    } else if (!strcmp(key, "dev.dsp")) {
    if (strlen(val) < sizeof(prConfig->dev_dsp))
            strcpy(prConfig->dev_dsp, val);
    } else if (!strcmp(key, "dev.gps")) {
    if (strlen(val) < sizeof(prConfig->dev_gps))
        strcpy(prConfig->dev_gps, val);
    } else if (!strcmp(key, "bee.path")) {
    if (strlen(val) < sizeof(prConfig->bee_path))
            strcpy(prConfig->bee_path, val);
    } else if (!strcmp(key, "pmtk.serial.dev")) {
    if (strlen(val) < sizeof(prConfig->dev_dbg))
            strcpy(prConfig->dev_dbg, val);
    } else if (!strcmp(key, "pmtk.conn")) {
        if (!strcmp(val, "serial"))
            prConfig->pmtk_conn = PMTK_CONNECTION_SERIAL;
        else if (!strcmp(val, "socket"))
            prConfig->pmtk_conn = PMTK_CONNECTION_SOCKET;
    }
    else if (!strcmp(key, "pmtk.serial.port")) {
        prConfig->socket_port = atoi(val);
    }
    else if (!strcmp(key, "debug.debug_nmea")) {
        /* it will be set to enable_dbg_log */
        prConfig->debug_nmea = (atoi(val) > 0) ? (1) : (0);
    } else if (!strcmp(key, "debug.mnl")) {
        prConfig->debug_mnl  = strtol(val, NULL, 16);
    } else if (!strcmp(key, "debug.dbg2file")) {
        prConfig->dbg2file  = atoi(val);
    } else if (!strcmp(key, "timeout.monitor")) {
        prConfig->timeout_monitor = atoi(val);
    } else if (!strcmp(key, "timeout.init")) {
        prConfig->timeout_init = atoi(val);
    } else if (!strcmp(key, "timeout.sleep")) {
        prConfig->timeout_sleep = atoi(val);
    } else if (!strcmp(key, "timeout.pwroff")) {
        prConfig->timeout_pwroff = atoi(val);
    } else if (!strcmp(key, "timeout.wakeup")) {
        prConfig->timeout_wakeup = atoi(val);
    } else if (!strcmp(key, "timeout.ttff")) {
        prConfig->timeout_ttff = atoi(val);
    } else if (!strcmp(key, "delay.reset_dsp")) {
        prConfig->delay_reset_dsp = atoi(val);
    } else if (!strcmp(key, "EPO_enabled")) {
        prConfig->EPO_enabled = atoi(val);
        epo_setconfig = 1;
    } else if (!strcmp(key, "BEE_enabled")) {
        prConfig->BEE_enabled = atoi(val);
    } else if (!strcmp(key, "SUPL_enabled")) {
        prConfig->SUPL_enabled = atoi(val);
    } else if (!strcmp(key, "SUPLSI_enabled")) {
        prConfig->SUPLSI_enabled = atoi(val);
    } else if (!strcmp(key, "EPO_priority")) {
        prConfig->EPO_priority = atoi(val);
    } else if (!strcmp(key, "BEE_priority")) {
        prConfig->BEE_priority = atoi(val);
    } else if (!strcmp(key, "SUPL_priority")) {
        prConfig->SUPL_priority = atoi(val);
    } else if (!strcmp(key, "AVAILIABLE_AGE")) {
        prConfig->AVAILIABLE_AGE = atoi(val);
    } else if (!strcmp(key, "RTC_DRIFT")) {
        prConfig->RTC_DRIFT = atoi(val);
    } else if (!strcmp(key, "TIME_INTERVAL")) {
        prConfig->TIME_INTERVAL = atoi(val);
    } else if (!strcmp(key, "TEST_MACHINE")) {
        prConfig->u1AgpsMachine = atoi(val);
    } else if (!strcmp(key, "ACC_SNR")) {
        prConfig->ACCURACY_SNR = atoi(val);
    } else if (!strcmp(key, "GNSS_MODE")) {
        prConfig->GNSSOPMode = atoi(val);
    } else if (!strcmp(key, "DBGLOG_FILE_MAX")) {
        prConfig->dbglog_file_max_size = atoi(val);
    } else if (!strcmp(key, "DBGLOG_FOLDER_MAX")) {
        prConfig->dbglog_folder_max_size = atoi(val);
    } else if ((!strcmp(key, "OFFLOAD_enabled"))) {
        prConfig->OFFLOAD_enabled = atoi(val);
    } else if ((!strcmp(key, "OFFLOAD_testMode"))) {
        prConfig->OFFLOAD_testMode = atoi(val);
    }
    return 0;
}
/*****************************************************************************/
int read_prop(MNL_CONFIG_T* prConfig, const char* name) {
    FILE *fp = fopen(name, "rb");
    char *key, *val;
    if (!fp) {
        LOGD("%s: open %s fail!\n", __FUNCTION__, name);
        return -1;
    }
    while (fgets(propbuf, sizeof(propbuf), fp)) {
        if (get_prop(propbuf, &key, &val)) {
            LOGD("%s: Get Property fails!!\n", __FUNCTION__);
            fclose(fp);
            return -1;
        }
        if (!key || !val)
            continue;
        // LOGD("%s: Get Property: '%s' => '%s'\n", __FUNCTION__, key, val);
        if (set_prop(prConfig, key, val)) {
            LOGE("%s: Set Property fails!!\n", __FUNCTION__);
            fclose(fp);
            return -1;
        }
    }
    fclose(fp);
    return 0;
}
/*****************************************************************************/
int mnl_utl_load_property(MNL_CONFIG_T* prConfig) {
    int idx;
    int cnt = sizeof(mnl_prop_path)/sizeof(mnl_prop_path[0]);
    int res = 0;
    epo_setconfig = 0;
    for (idx = 0; idx < cnt; idx++) {
        if (!read_prop(prConfig, mnl_prop_path[idx]))
            break;
    }
#if ANDROID_MNLD_PROP_SUPPORT
    // Add for reading property set by YGPS
    char result[PROPERTY_VALUE_MAX] = {0};
    if (property_get(MNL_CONFIG_STATUS, result, NULL)) {
        prConfig->dbg2file = result[2] - '0';
        LOGD("dbg2file: %d\n", prConfig->dbg2file);
        prConfig->debug_nmea = result[3] - '0';
        LOGD("debug_nmea:%d \n", prConfig->debug_nmea);
        prConfig->BEE_enabled = result[4] - '0';
        LOGD("BEE_enabled: %d", prConfig->BEE_enabled);
        // prConfig->test_mode = result[5] - '0';
        // LOGD("test_mode: %d", prConfig->test_mode);
    } else {
        LOGE("Config is not set yet, ignore");
    }
#endif
    LOGD("========================================\n");
    if (idx < cnt)  /* successfully read property from file */  {
        LOGD("[setting] reading from %s\n", mnl_prop_path[idx]);
        res = 0;
    } else {
        LOGD("[setting] load default value\n");
        res = -1;
    }
    LOGD("dev_dsp/dev_gps : %s %s\n", prConfig->dev_dsp, prConfig->dev_gps);
    LOGD("init_speed/link_speed: %d %d\n", prConfig->init_speed, prConfig->link_speed);
    LOGD("pmtk_conn/socket_port/dev_dbg : %d %d %s\n",
        prConfig->pmtk_conn, prConfig->socket_port, prConfig->dev_dbg);
    LOGD("debug_nmea/debug_mnl: %d 0x%04X\n", prConfig->debug_nmea, prConfig->debug_mnl);
    LOGD("nmea2file/dbg2file: %d/%d\n", prConfig->nmea2file, prConfig->dbg2file);
    LOGD("time-out: %d %d %d %d %d %d\n", prConfig->timeout_init,
        prConfig->timeout_monitor, prConfig->timeout_wakeup, prConfig->timeout_ttff,
        prConfig->timeout_sleep, prConfig->timeout_pwroff);
    LOGD("EPO_Enabled: %d\n", prConfig->EPO_enabled);
    LOGD("BEE_Enabled: %d\n", prConfig->BEE_enabled);
    LOGD("SUPL_Enabled: %d\n", prConfig->SUPL_enabled);
    LOGD("SUPLSI_Enabled: %d\n", prConfig->SUPLSI_enabled);
    LOGD("EPO_priority: %d\n", prConfig->EPO_priority);
    LOGD("BEE_priority: %d\n", prConfig->BEE_priority);
    LOGD("SUPL_priority: %d\n", prConfig->SUPL_priority);
    LOGD("AVAILIABLE_AGE: %d", prConfig->AVAILIABLE_AGE);
    LOGD("RTC_DRIFT: %d", prConfig->RTC_DRIFT);
    LOGD("TIME_INTERVAL: %d", prConfig->TIME_INTERVAL);
    LOGD("========================================\n");
    return res;
}
#endif

/*****************************************************************************/
int str2int(const char*  p, const char*  end) {
    int   result = 0;
    int   len    = end - p;
    int   sign = 1;

    if (*p == '-') {
        sign = -1;
        p++;
        len = end - p;
    }

    for (; len > 0; len--, p++) {
        int  c;

        if (p >= end)
            return -1;

        c = *p - '0';
        if ((unsigned)c >= 10)
            return -1;

        result = result*10 + c;
    }
    return  sign*result;
}

