/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein is
 * confidential and proprietary to MediaTek Inc. and/or its licensors. Without
 * the prior written permission of MediaTek inc. and/or its licensors, any
 * reproduction, modification, use or disclosure of MediaTek Software, and
 * information contained herein, in whole or in part, shall be strictly
 * prohibited.
 *
 * MediaTek Inc. (C) 2015. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER
 * ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR
 * NONINFRINGEMENT. NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH
 * RESPECT TO THE SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY,
 * INCORPORATED IN, OR SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES
 * TO LOOK ONLY TO SUCH THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO.
 * RECEIVER EXPRESSLY ACKNOWLEDGES THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO
 * OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES CONTAINED IN MEDIATEK
 * SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE
 * RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S
 * ENTIRE AND CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE
 * RELEASED HEREUNDER WILL BE, AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE
 * MEDIATEK SOFTWARE AT ISSUE, OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE
 * CHARGE PAID BY RECEIVER TO MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek
 * Software") have been modified by MediaTek Inc. All revisions are subject to
 * any receiver's applicable license agreements with MediaTek Inc.
 */

#define LOG_TAG "fmradio_hw_hal"
/*#define LOG_NDEBUG 0*/

#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
#include <sys/prctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cutils/log.h>
#include <cutils/list.h>

#include "fmr.h"
#include <system/radio.h>
#include <system/radio_metadata.h>
#include <hardware/hardware.h>
#include <hardware/radio.h>
#include "fm.h"

static int g_idx = -1;
static int g_rds = 0;

struct fmradio_tuner {
    struct radio_tuner interface;
    struct fmradio_device *dev;
    radio_callback_t callback;
    void *cookie;
    radio_hal_band_config_t config;
    radio_program_info_t program;
    bool audio;
    pthread_t callback_thread;
    pthread_t rds_thread;
    pthread_mutex_t lock;
    pthread_mutex_t rds_lock;
    pthread_cond_t cond;
    pthread_cond_t rds_cond;
    struct listnode command_list;
};

struct fmradio_device {
    struct radio_hw_device device;
    struct fmradio_tuner *tuner;
    pthread_mutex_t lock;
};

static const radio_hal_properties_t fmradio_hw_properties = {
    .class_id = RADIO_CLASS_AM_FM,
    .implementor = "MTK",
    .product = "MTK FMRadio",
    .version = "0.1",
    .serial = "0123456789",
    .num_tuners = 1,
    .num_audio_sources = 1,
    .supports_capture = false,
    .num_bands = 2,
    .bands = {
        {
            .type = RADIO_BAND_FM,
            .antenna_connected = false,
            .lower_limit = 87500,
            .upper_limit = 108000,
            .num_spacings = 2,
            .spacings = { 100, 200 },
            /* default space: 100KHz
	           US: 200KHz */
            .fm = {
                .deemphasis = RADIO_DEEMPHASIS_50,
                .stereo = true,
                .rds = RADIO_RDS_WORLD,
                .ta = false,
                .af = false,
            }
    },
        {
            .type = RADIO_BAND_FM,
            .antenna_connected = true,
            .lower_limit = 87900,
            .upper_limit = 107900,
            .num_spacings = 2,
            .spacings = { 100, 200 },
            .fm = {
                .deemphasis = RADIO_DEEMPHASIS_75,
                .stereo = true,
                .rds = RADIO_RDS_US,
                .ta = false,
                .af = false,
            }
        }
    }
};

typedef enum {
    CMD_EXIT,
    CMD_CONFIG,
    CMD_STEP,
    CMD_SEEK,
    CMD_TUNE,
    CMD_CANCEL,
    CMD_CHECK_RDS,
    CMD_SHOW_RDS,
} thread_cmd_type_t;

struct thread_command {
    struct listnode node;
    thread_cmd_type_t type;
    struct timespec ts;
    union {
        unsigned int param;
        radio_hal_band_config_t config;
    };
};

/* must be called with out->lock locked */
static int send_command_l(struct fmradio_tuner *tuner,
                          thread_cmd_type_t type,
                          unsigned int delay_ms,
                          void *param)
{
    struct thread_command *cmd = (struct thread_command *)calloc(1, sizeof(struct thread_command));
    struct timespec ts;

    if (cmd == NULL)
        return -ENOMEM;

    ALOGV("%s %d delay_ms %d", __func__, type, delay_ms);

    cmd->type = type;
    if (param != NULL) {
        if (cmd->type == CMD_CONFIG) {
            cmd->config = *(radio_hal_band_config_t *)param;
            ALOGV("%s CMD_CONFIG type %d", __func__, cmd->config.type);
        } else
            cmd->param = *(unsigned int *)param;
    }

    clock_gettime(CLOCK_REALTIME, &ts);

    ts.tv_sec  += delay_ms/1000;
    ts.tv_nsec += (delay_ms%1000) * 1000000;
    if (ts.tv_nsec >= 1000000000) {
        ts.tv_nsec -= 1000000000;
        ts.tv_sec += 1;
    }
    cmd->ts = ts;
    list_add_tail(&tuner->command_list, &cmd->node);
    pthread_cond_signal(&tuner->cond);
    return 0;
}

static int prepare_metadata(struct fmradio_tuner *tuner,
                            radio_metadata_t **metadata)
{
    int ret = 0;
    uint16_t status = 0;
    uint8_t pty;
    uint8_t *tmp_ps = NULL;
    uint8_t *tmp_rt = NULL;
    int rt_len;
    int ps_len;

    if (metadata == NULL)
        return -EINVAL;

    if (*metadata != NULL)
        radio_metadata_deallocate(*metadata);

    *metadata = NULL;

    ret = radio_metadata_allocate(metadata, tuner->program.channel, 0);
    if (ret != 0)
        return ret;

    ret = FMR_read_rds_data(g_idx, &status);
    if (ret != 0) {
        goto exit;
    }

    if (status & RDS_EVENT_PTY_CODE) {
        ret = FMR_get_pty(&pty);
        if (ret != 0) {
            ALOGW("%s get pty fail", __func__);
            goto exit;
        }
        ALOGV("pty = %d", pty);
        ret = radio_metadata_add_int(metadata, RADIO_METADATA_KEY_RBDS_PTY, pty);
        if (ret != 0)
            goto exit;
    }

    if (status & RDS_EVENT_PROGRAMNAME) {
        ret = FMR_get_ps(&tmp_ps, &ps_len);
        if (ret != 0) {
            ALOGW("%s get ps fail", __func__);
            goto exit;
        }
        ALOGV("ps = %s", tmp_ps);
        ret = radio_metadata_add_text(metadata, RADIO_METADATA_KEY_RDS_PS, (const char *)tmp_ps);
        if (ret != 0)
            goto exit;
    }

    if (status & RDS_EVENT_LAST_RADIOTEXT) {
        ret = FMR_get_rt(&tmp_rt, &rt_len);
        if (ret != 0) {
            ALOGW("%s get rt fail", __func__);
            goto exit;
        }
        ALOGI("rt = %s", tmp_rt);
        ret = radio_metadata_add_text(metadata, RADIO_METADATA_KEY_TITLE, (const char *)tmp_rt);
        if (ret != 0)
            goto exit;
    }

    return 0;

exit:
    radio_metadata_deallocate(*metadata);
    *metadata = NULL;
    return ret;
}

static void *rds_thread_loop(void *context)
{
    struct fmradio_tuner *tuner = (struct fmradio_tuner *)context;
    struct timespec cur_ts = {0, 0};
    unsigned short int status = 0;
    int ret;

    prctl(PR_SET_NAME, (unsigned long)"fmradio rds thread", 0, 0, 0);

    pthread_mutex_lock(&tuner->rds_lock);

    clock_gettime(CLOCK_REALTIME, &cur_ts);
    cur_ts.tv_sec += 1; /* check RDS event every 1s */

    while (true) {
        if ((cur_ts.tv_sec != 0) || (cur_ts.tv_nsec != 0)) {
            pthread_cond_timedwait(&tuner->rds_cond, &tuner->rds_lock, &cur_ts);
        }

        if (g_rds)
            goto exit;

        pthread_mutex_lock(&tuner->lock);
        ret = prepare_metadata(tuner, &tuner->program.metadata);
        if (ret == 0)
            send_command_l(tuner, CMD_SHOW_RDS, 0, NULL);
        pthread_mutex_unlock(&tuner->lock);
        clock_gettime(CLOCK_REALTIME, &cur_ts);
        cur_ts.tv_sec += 1; /* check RDS event every 1s */
    }

exit:
    pthread_mutex_unlock(&tuner->rds_lock);

    ALOGV("%s Exiting", __func__);
    return NULL;
}

static void *callback_thread_loop(void *context)
{
    struct fmradio_tuner *tuner = (struct fmradio_tuner *)context;
    prctl(PR_SET_NAME, (unsigned long)"fmradio callback thread", 0, 0, 0);
    pthread_mutex_lock(&tuner->lock);

    while (true) {
        struct thread_command *cmd = NULL;
        struct listnode *item;
        struct listnode *tmp;
        struct timespec cur_ts;
        bool got_cancel = false;

        if (list_empty(&tuner->command_list)) {
            pthread_cond_wait(&tuner->cond, &tuner->lock);
        }

        clock_gettime(CLOCK_REALTIME, &cur_ts);

        list_for_each_safe(item, tmp, &tuner->command_list) {
            cmd = node_to_item(item, struct thread_command, node);

            if (got_cancel && (cmd->type == CMD_STEP || cmd->type == CMD_SEEK ||
                    cmd->type == CMD_TUNE || cmd->type == CMD_CHECK_RDS || cmd->type == CMD_SHOW_RDS)) {
                 list_remove(item);
                 free(cmd);
                 ALOGI("Cancel pending commands",__func__);
                 continue;
            }

            radio_hal_event_t event;
            radio_metadata_t *metadata = NULL;

            event.type = RADIO_EVENT_HW_FAILURE;
            list_remove(item);
            ALOGI("%s processing command %d time %ld.%ld", __func__, cmd->type, cur_ts.tv_sec,
                  cur_ts.tv_nsec);

            switch (cmd->type) {
            default:
            case CMD_EXIT:
                free(cmd);
                goto exit;

            case CMD_CONFIG: {
                tuner->config = cmd->config;
                event.type = RADIO_EVENT_CONFIG;
                event.config = tuner->config;
                ALOGV("%s CMD_CONFIG type %d low %d up %d",
                  __func__, tuner->config.type,
                  tuner->config.lower_limit, tuner->config.upper_limit);
                if (tuner->config.type == RADIO_BAND_FM) {
                    ALOGV(" FM - stereo %d\n   - rds %d\n   - ta %d\n   - af %d",
                      tuner->config.fm.stereo, tuner->config.fm.rds,
                      tuner->config.fm.ta, tuner->config.fm.af);
                } else {
                    ALOGV(" AM - stereo %d", tuner->config.am.stereo);
                }
            } break;

            case CMD_STEP: {
                unsigned int freq;
                int ret;
                int sm;

                freq = tuner->program.channel; /* KHz */
                if (cmd->param == RADIO_DIRECTION_UP)
                    freq += tuner->config.spacings[0];
                else
                    freq -= tuner->config.spacings[0];

                if (freq > tuner->config.upper_limit)
                    freq = tuner->config.lower_limit;

                if (freq < tuner->config.lower_limit)
                    freq = tuner->config.upper_limit;

                ret = FMR_turn_on_off_rds(g_idx, FMR_RDS_OFF);
                if (ret)
                    ALOGE("%s turn off rds fail", __func__);

                ret = FMR_tune(g_idx, freq);
                if (ret)
                    tuner->program.tuned = false;
                else {
                    tuner->program.channel = freq;
                    tuner->program.tuned = true;
                }

                ret = FMR_turn_on_off_rds(g_idx, FMR_RDS_ON);
                if (ret)
                    ALOGE("%s turn on rds fail", __func__);

                if (tuner->program.metadata != NULL)
                    radio_metadata_deallocate(tuner->program.metadata);
                tuner->program.metadata = NULL;
                if (tuner->program.tuned) {
                    send_command_l(tuner, CMD_CHECK_RDS, 0, NULL);
                }

                ret = FMR_get_stereomono(g_idx, &sm);
                if (ret)
                    ALOGV(" FM - get stereomono fail,ret=%d", ret);

                if (tuner->config.type == RADIO_BAND_FM)
                    tuner->program.stereo = sm;
                else
                    tuner->program.stereo = false;

                event.type = RADIO_EVENT_TUNED;
                event.info = tuner->program;
            } break;

            case CMD_SEEK: {
                unsigned int freq;
                int ret;
                int sm;

                ret = FMR_turn_on_off_rds(g_idx, FMR_RDS_OFF);
                if (ret)
                    ALOGE("%s turn off rds fail", __func__);

                freq = tuner->program.channel; /* KHz */
                ret = FMR_seek(g_idx, &tuner->config, freq, cmd->param, &tuner->program);
                if (ret)
                    ALOGE("%s seek fail", __func__);

                ret = FMR_tune(g_idx, tuner->program.channel);
                if (ret) {
                    ALOGE("%s tune fail", __func__);
                    tuner->program.tuned = false;
                }
                else
                    tuner->program.tuned = true;

                ret = FMR_turn_on_off_rds(g_idx, FMR_RDS_ON);
                if (ret)
                    ALOGE("%s turn on rds fail", __func__);

                if (tuner->program.metadata != NULL)
                    radio_metadata_deallocate(tuner->program.metadata);
                tuner->program.metadata = NULL;
                if (tuner->program.tuned) {
                    send_command_l(tuner, CMD_CHECK_RDS, 0, NULL);
                }

                ret = FMR_get_stereomono(g_idx, &sm);
                if (ret)
                    ALOGV(" FM - get stereomono fail,ret=%d", ret);

                if (tuner->config.type == RADIO_BAND_FM)
                    tuner->program.stereo = sm;
                else
                    tuner->program.stereo = false;

                event.type = RADIO_EVENT_TUNED;
                event.info = tuner->program;
            } break;

            case CMD_TUNE: {
                int ret;
                unsigned int freq;
                int sm;
                int rssi;

                tuner->program.channel = cmd->param;
                freq = cmd->param;

                ret = FMR_turn_on_off_rds(g_idx, FMR_RDS_OFF);
                if (ret)
                    ALOGE("%s turn off rds fail", __func__);

                ret = FMR_tune(g_idx, freq);
                if (ret)
                    tuner->program.tuned = false;
                else
                    tuner->program.tuned = true;

                ret = FMR_turn_on_off_rds(g_idx, FMR_RDS_ON);
                if (ret)
                    ALOGE("%s turn off rds fail", __func__);

                if (tuner->program.metadata != NULL)
                    radio_metadata_deallocate(tuner->program.metadata);
                tuner->program.metadata = NULL;
                if (tuner->program.tuned) {
                    send_command_l(tuner, CMD_CHECK_RDS, 0, NULL);
                }

                ret = FMR_get_rssi(g_idx, &rssi);
                if (ret)
                    ALOGV(" FM - get rssi fail,ret=%d", ret);
                else
                    tuner->program.signal_strength = (unsigned int)rssi;

                ret = FMR_get_stereomono(g_idx, &sm);
                if (ret)
                    ALOGV(" FM - get stereomono fail,ret=%d", ret);

                if (tuner->config.type == RADIO_BAND_FM)
                    tuner->program.stereo = (bool)sm;
                else
                    tuner->program.stereo = false;

                event.type = RADIO_EVENT_TUNED;
                event.info = tuner->program;
            } break;

            case CMD_CHECK_RDS: {
                ALOGV("%s processing command CMD_CHECK_RDS, &metadata = %p, metadata = %p",
                    __func__, &metadata, metadata);
                int ret = prepare_metadata(tuner, &metadata);
                if (ret == 0) {
                    event.type = RADIO_EVENT_METADATA;
                    event.metadata = metadata;
                }
                ALOGV("%s processing command CMD_CHECK_RDS done", __func__);
            } break;

            case CMD_SHOW_RDS: {
                ALOGV("%s processing command CMD_SHOW_RDS, &metadata = %p, metadata = %p",
                    __func__, &metadata, metadata);
                int ret = radio_metadata_add_metadata(&metadata, tuner->program.metadata);
                if (ret == 0) {
                    event.type = RADIO_EVENT_METADATA;
                    event.metadata = metadata;
                }
                ALOGV("%s processing command CMD_SHOW_RDS done", __func__);
            } break;

            case CMD_CANCEL: {
                got_cancel = true;
            } break;
            }

            if (event.type != RADIO_EVENT_HW_FAILURE && tuner->callback != NULL) {
                pthread_mutex_unlock(&tuner->lock);
                tuner->callback(&event, tuner->cookie);
                pthread_mutex_lock(&tuner->lock);
                if (event.type == RADIO_EVENT_METADATA && metadata != NULL) {
                    radio_metadata_deallocate(metadata);
                    metadata = NULL;
                }
            }

            ALOGV("%s processed command %d", __func__, cmd->type);
            free(cmd);
        }
    }

exit:
    pthread_mutex_unlock(&tuner->lock);

    ALOGV("%s Exiting", __func__);

    return NULL;
}

static int tuner_set_configuration(const struct radio_tuner *tuner,
                         const radio_hal_band_config_t *config)
{
    struct fmradio_tuner *fmr_tuner = (struct fmradio_tuner *)tuner;
    int status = 0;

    ALOGI("%s fmr_tuner %p", __func__, fmr_tuner);
    pthread_mutex_lock(&fmr_tuner->lock);
    if (config == NULL) {
        status = -EINVAL;
        goto exit;
    }
    send_command_l(fmr_tuner, CMD_CANCEL, 0, NULL);
    send_command_l(fmr_tuner, CMD_CONFIG, 0, (void *)config);

exit:
    pthread_mutex_unlock(&fmr_tuner->lock);
    return status;
}

static int tuner_get_configuration(const struct radio_tuner *tuner,
                         radio_hal_band_config_t *config)
{
    struct fmradio_tuner *fmr_tuner = (struct fmradio_tuner *)tuner;
    int status = 0;
    struct listnode *item;
    radio_hal_band_config_t *src_config;

    ALOGI("%s fmr_tuner %p", __func__, fmr_tuner);
    pthread_mutex_lock(&fmr_tuner->lock);
    src_config = &fmr_tuner->config;

    if (config == NULL) {
        status = -EINVAL;
        goto exit;
    }

    list_for_each(item, &fmr_tuner->command_list) {
        struct thread_command *cmd = node_to_item(item, struct thread_command, node);
        if (cmd->type == CMD_CONFIG) {
            src_config = &cmd->config;
        }
    }
    *config = *src_config;

exit:
    pthread_mutex_unlock(&fmr_tuner->lock);
    return status;
}

static int tuner_step(const struct radio_tuner *tuner,
                     radio_direction_t direction, bool skip_sub_channel)
{
    struct fmradio_tuner *fmr_tuner = (struct fmradio_tuner *)tuner;

    ALOGI("%s fmr_tuner %p direction %d, skip_sub_channel %d",
          __func__, fmr_tuner, direction, skip_sub_channel);

    pthread_mutex_lock(&fmr_tuner->lock);
    send_command_l(fmr_tuner, CMD_STEP, 0, &direction);
    pthread_mutex_unlock(&fmr_tuner->lock);
    return 0;
}

static int tuner_seek(const struct radio_tuner *tuner,
                     radio_direction_t direction, bool skip_sub_channel)
{
    struct fmradio_tuner *fmr_tuner = (struct fmradio_tuner *)tuner;

    ALOGI("%s fmr_tuner %p direction %d, skip_sub_channel %d",
          __func__, fmr_tuner, direction, skip_sub_channel);

    pthread_mutex_lock(&fmr_tuner->lock);
    send_command_l(fmr_tuner, CMD_SEEK, 0, &direction);
    pthread_mutex_unlock(&fmr_tuner->lock);
    return 0;
}

static int tuner_tune(const struct radio_tuner *tuner,
                     unsigned int channel, unsigned int sub_channel)
{
    struct fmradio_tuner *fmr_tuner = (struct fmradio_tuner *)tuner;

    ALOGI("%s fmr_tuner %p channel %d, sub_channel %d",
          __func__, fmr_tuner, channel, sub_channel);

    pthread_mutex_lock(&fmr_tuner->lock);
    if (channel < fmr_tuner->config.lower_limit || channel > fmr_tuner->config.upper_limit) {
        pthread_mutex_unlock(&fmr_tuner->lock);
        ALOGI("%s channel out of range", __func__);
        return -EINVAL;
    }
    send_command_l(fmr_tuner, CMD_TUNE, 0, &channel);
    pthread_mutex_unlock(&fmr_tuner->lock);
    return 0;
}

static int tuner_cancel(const struct radio_tuner *tuner)
{
    struct fmradio_tuner *fmr_tuner = (struct fmradio_tuner *)tuner;

    ALOGI("%s fmr_tuner %p", __func__, fmr_tuner);

    pthread_mutex_lock(&fmr_tuner->lock);
    send_command_l(fmr_tuner, CMD_CANCEL, 0, NULL);
    pthread_mutex_unlock(&fmr_tuner->lock);
    return 0;
}

static int tuner_get_program_information(const struct radio_tuner *tuner,
                 radio_program_info_t *info)
{
    struct fmradio_tuner *fmr_tuner = (struct fmradio_tuner *)tuner;
    int status = 0;
    radio_metadata_t *metadata;

    ALOGI("%s fmr_tuner %p", __func__, fmr_tuner);
    pthread_mutex_lock(&fmr_tuner->lock);
    if (info == NULL) {
         status = -EINVAL;
         goto exit;
    }
    metadata = info->metadata;
    *info = fmr_tuner->program;
    info->metadata = metadata;
    if (metadata != NULL)
        radio_metadata_add_metadata(&info->metadata, fmr_tuner->program.metadata);

    exit:
    pthread_mutex_unlock(&fmr_tuner->lock);
    return status;
}

static int fmr_dev_get_properties(const struct radio_hw_device *dev,
             radio_hal_properties_t *properties)
{
    struct fmradio_device *rdev = (struct fmradio_device *)dev;

    ALOGI("%s", __func__);
    if (properties == NULL)
        return -EINVAL;

    memcpy(properties, &fmradio_hw_properties, sizeof(radio_hal_properties_t));

    return 0;
}

static int fmr_dev_open_tuner(const struct radio_hw_device *dev,
               const radio_hal_band_config_t *config,
               bool audio,
               radio_callback_t callback,
               void *cookie,
               const struct radio_tuner **tuner)
{
    struct fmradio_device *rdev = (struct fmradio_device *)dev;
    int status = 0;

    ALOGI("%s rdev %p", __func__, rdev);
    pthread_mutex_lock(&rdev->lock);

    if (rdev->tuner != NULL) {
        status = -ENOSYS;
        goto exit;
    }

    if (config == NULL || callback == NULL || tuner == NULL) {
        status = -EINVAL;
        goto exit;
    }

    rdev->tuner = (struct fmradio_tuner *)calloc(1, sizeof(struct fmradio_tuner));
    if (rdev->tuner == NULL) {
        status = -ENOMEM;
        goto exit;
    }

    rdev->tuner->interface.set_configuration = tuner_set_configuration;
    rdev->tuner->interface.get_configuration = tuner_get_configuration;
    rdev->tuner->interface.scan = tuner_seek;
    rdev->tuner->interface.step = tuner_step;
    rdev->tuner->interface.tune = tuner_tune;
    rdev->tuner->interface.cancel = tuner_cancel;
    rdev->tuner->interface.get_program_information = tuner_get_program_information;

    rdev->tuner->audio = audio;
    rdev->tuner->callback = callback;
    rdev->tuner->cookie = cookie;

    rdev->tuner->dev = rdev;

    status = FMR_open_dev(&g_idx);
    if (status)
        ALOGI("%s open device fail", __func__);

    status = FMR_pwr_up(g_idx, config);
    if (status)
        ALOGI("%s power up fail", __func__);

    g_rds = 0;
    pthread_mutex_init(&rdev->tuner->lock, (const pthread_mutexattr_t *) NULL);
    pthread_mutex_init(&rdev->tuner->rds_lock, (const pthread_mutexattr_t *) NULL);
    pthread_cond_init(&rdev->tuner->cond, (const pthread_condattr_t *) NULL);
    pthread_cond_init(&rdev->tuner->rds_cond, (const pthread_condattr_t *) NULL);
    list_init(&rdev->tuner->command_list);
    pthread_create(&rdev->tuner->callback_thread, (const pthread_attr_t *) NULL,
                    callback_thread_loop, rdev->tuner);
    pthread_create(&rdev->tuner->rds_thread, (const pthread_attr_t *) NULL,
                    rds_thread_loop, rdev->tuner);

    pthread_mutex_lock(&rdev->tuner->lock);
    send_command_l(rdev->tuner, CMD_CONFIG, 0, (void *)config);
    pthread_mutex_unlock(&rdev->tuner->lock);

    *tuner = &rdev->tuner->interface;

exit:
    pthread_mutex_unlock(&rdev->lock);
    ALOGI("%s DONE", __func__);
    return status;
}

static int fmr_dev_close_tuner(const struct radio_hw_device *dev,
                  const struct radio_tuner *tuner)
{
    struct fmradio_device *rdev = (struct fmradio_device *)dev;
    struct fmradio_tuner *fmr_tuner = (struct fmradio_tuner *)tuner;
    int status = 0;

    ALOGI("%s tuner %p", __func__, tuner);
    pthread_mutex_lock(&rdev->lock);

    if (tuner == NULL) {
        status = -EINVAL;
        goto exit;
    }

    status = FMR_pwr_down(g_idx, 0); /* 0: close RX */
    if (status)
        ALOGI("%s power down fail", __func__);

    status = FMR_close_dev(g_idx);
    if (status)
        ALOGI("%s close device fail", __func__);

    pthread_mutex_lock(&fmr_tuner->lock);
    fmr_tuner->callback = NULL;
    g_rds = 1; /* exit RDS thread */
    send_command_l(fmr_tuner, CMD_EXIT, 0, NULL);
    pthread_mutex_unlock(&fmr_tuner->lock);
    pthread_join(fmr_tuner->callback_thread, (void **) NULL);
    pthread_join(fmr_tuner->rds_thread, (void **) NULL);

    if (fmr_tuner->program.metadata != NULL)
        radio_metadata_deallocate(fmr_tuner->program.metadata);

    free(fmr_tuner);
    rdev->tuner = NULL;

exit:
    pthread_mutex_unlock(&rdev->lock);
    return status;
}

static int fmr_dev_close(hw_device_t *device)
{
    struct fmradio_device *rdev = (struct fmradio_device *)device;
    if (rdev != NULL) {
        free(rdev->tuner);
    }
    free(rdev);
    return 0;
}

static int fmr_dev_open(const hw_module_t* module, const char* name,
              hw_device_t** device)
{
    struct fmradio_device *rdev;
    int ret;

    ALOGI("%s", __func__);
    if (strcmp(name, RADIO_HARDWARE_DEVICE) != 0) {
        ALOGI("FMRadio device name error, name = %s, expect = %s", name, RADIO_HARDWARE_DEVICE);
        return -EINVAL;
    }
    else
        ALOGI("device name = %s", name);

    rdev = calloc(1, sizeof(struct fmradio_device));
    if (!rdev)
        return -ENOMEM;
    else
        ALOGI("FMRadio allocate fmradio_device success");

    rdev->device.common.tag = HARDWARE_DEVICE_TAG;
    rdev->device.common.version = RADIO_DEVICE_API_VERSION_1_0;
    rdev->device.common.module = (struct hw_module_t *) module;
    rdev->device.common.close = fmr_dev_close;
    rdev->device.get_properties = fmr_dev_get_properties;
    rdev->device.open_tuner = fmr_dev_open_tuner;
    rdev->device.close_tuner = fmr_dev_close_tuner;

    pthread_mutex_init(&rdev->lock, (const pthread_mutexattr_t *) NULL);

    *device = &rdev->device.common;

    /* customize */
    ret = FMR_init();
    if (ret)
        ALOGI("%s customize fail", __func__);
    else
        ALOGI("FMR_init success");

    return 0;
}

static struct hw_module_methods_t radio_hal_module_methods = {
    .open = fmr_dev_open,
};

struct radio_module HAL_MODULE_INFO_SYM = {
    .common = {
        .tag = HARDWARE_MODULE_TAG,
        .module_api_version = RADIO_MODULE_API_VERSION_1_0,
        .hal_api_version = HARDWARE_HAL_API_VERSION,
        .id = RADIO_HARDWARE_MODULE_ID,
        .name = "MTK fmradio HAL",
        .author = "MTK",
        .methods = &radio_hal_module_methods,
    },
};

