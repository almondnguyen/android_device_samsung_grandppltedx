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
 * MediaTek Inc. (C) 2010. All rights reserved.
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

/*******************************************************************
 * FM JNI core
 * return -1 if error occured. else return needed value.
 * if return type is char *, return NULL if error occured.
 * Do NOT return value access paramater.
 *
 * FM JNI core should be independent from lower layer, that means
 * there should be no low layer dependent data struct in FM JNI core
 *
 * Naming rule: FMR_n(paramter Micro), FMR_v(functions), fmr_n(global param)
 * pfmr_n(global paramter pointer)
 *
 *******************************************************************/
#include <memory.h>
#include <cutils/log.h>
#include "fmr.h"
#include <system/radio.h>

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "FMLIB_CORE"

struct fmr_ds fmr_data;

int FMR_get_cfgs(void)
{
    int ret = -1;

    fmr_data.custom_handler = dlopen(CUST_LIB_NAME, RTLD_NOW);
    if (fmr_data.custom_handler == NULL) {
        ALOGE("%s failed, %s\n", __FUNCTION__, dlerror());
    } else {
        *(void **) (&(fmr_data.get_cfg)) = dlsym(fmr_data.custom_handler, "CUST_get_cfg");
        if (fmr_data.get_cfg == NULL) {
            ALOGE("%s failed, %s\n", __FUNCTION__, dlerror());
        } else {
            ALOGI("Go to run cust function\n");
            (*(fmr_data.get_cfg))(&(fmr_data.cfg_data));
            ALOGI("OK\n");
            ret = 0;
        }

        fmr_data.custom_handler = NULL;
        fmr_data.get_cfg = NULL;
    }
    ALOGI("%s succefully. chip: 0x%x, lband: %d, hband: %d, seek_space: %d, max_scan_num: %d\n", __FUNCTION__,
    fmr_data.cfg_data.chip, fmr_data.cfg_data.low_band, fmr_data.cfg_data.high_band, fmr_data.cfg_data.seek_space,
    fmr_data.cfg_data.max_scan_num);

    return ret;
}

int FMR_init()
{
    int ret = 0;

    memset(&fmr_data, 0, sizeof(struct fmr_ds));

    if (FMR_get_cfgs() < 0) {
        LOGI("FMR_get_cfgs failed\n");
        return -1;
    }

    return 0;
}

int FMR_open_dev(int *idx)
{
    int ret = 0;
    int real_chip;

    ret = COM_open_dev(FM_DEV_NAME, idx);
    if (ret || (*idx < 0)) {
        LOGE("%s failed, [fd=%d]\n", __func__, *idx);
        return ret;
    }

    /* Check if customer's cfg matchs driver. */
    ret = FMR_get_chip_id(*idx, &real_chip);
    if (fmr_data.cfg_data.chip != real_chip) {
        LOGE("%s, Chip config error. 0x%x\n", __func__, real_chip);
        ret = COM_close_dev(*idx);
        return ret;
    }

    LOGD("%s, [fd=%d] [chipid=0x%x] [ret=%d]\n", __func__, *idx, real_chip, ret);
    return ret;
}

int FMR_close_dev(int idx)
{
    int ret = 0;

    ret = COM_close_dev(idx);
    LOGD("%s, [fd=%d] [ret=%d]\n", __func__, idx, ret);
    return ret;
}

int FMR_pwr_up(int idx, const radio_hal_band_config_t *config)
{
    int ret = 0;

    LOGI("%s \n", __func__);
    fmr_data.cfg_data.low_band = (int)config->lower_limit / 100;
    fmr_data.cfg_data.high_band = (int)config->upper_limit / 100;
    fmr_data.cfg_data.seek_space = config->spacings[0];

    ret = COM_pwr_up(idx, fmr_data.cfg_data.band, 980); /* power up default 98M*/
    if (ret) {
        LOGE("%s failed, [ret=%d]\n", __func__, ret);
    }

    fmr_data.cur_freq = 980;
    LOGD("%s, [ret=%d]\n", __func__, ret);
    return ret;
}

int FMR_pwr_down(int idx, int type)
{
    int ret = 0;

    ret = COM_pwr_down(idx, type);
    LOGD("%s, [ret=%d]\n", __func__, ret);
    return ret;
}

int FMR_get_chip_id(int idx, int *chipid)
{
    int ret = 0;

    FMR_ASSERT(chipid);

    ret = COM_get_chip_id(idx, chipid);
    if (ret) {
        LOGE("%s failed, %s\n", __func__, FMR_strerr());
        *chipid = -1;
    }
    LOGD("%s, [ret=%d]\n", __func__, ret);
    return ret;
}

int FMR_get_rssi(int idx, int *rssi)
{
    int ret = 0;

    FMR_ASSERT(rssi);

    ret = COM_get_rssi(idx, rssi);
    if (ret) {
        LOGE("%s failed, [ret=%d]\n", __func__, ret);
        *rssi = -1;
    }
    LOGD("%s, [rssi=%d] [ret=%d]\n", __func__, *rssi, ret);
    return ret;
}

int FMR_get_ps(uint8_t **ps, int *ps_len)
{
    int ret = 0;

    FMR_ASSERT(ps);
    FMR_ASSERT(ps_len);
    ret = COM_get_ps(&fmr_data.rds, ps, ps_len);
    LOGD("%s, [ret=%d]\n", __func__, ret);
    return ret;
}

int FMR_get_rt(uint8_t **rt, int *rt_len)
{
    int ret = 0;

    FMR_ASSERT(rt);
    FMR_ASSERT(rt_len);

    ret = COM_get_rt(&fmr_data.rds, rt, rt_len);
    LOGD("%s, [ret=%d]\n", __func__, ret);
    return ret;
}

int FMR_get_pi(uint16_t *pi)
{
    int ret = 0;

    ret = COM_get_pi(&fmr_data.rds, pi);
    if (ret) {
        LOGE("%s failed, %s\n", __func__, FMR_strerr());
        *pi = -1;
    }
    LOGD("%s, [ret=%d]\n", __func__, ret);
    return ret;
}

int FMR_get_pty(uint8_t *pty)
{
    int ret = 0;

    ret = COM_get_pty(&fmr_data.rds, pty);
    if (ret) {
        *pty = -1;
        LOGI("%s failed, %s\n", __func__, FMR_strerr());
    }
    LOGD("%s, [ret=%d]\n", __func__, ret);
    return ret;
}

int FMR_tune(int idx, unsigned int freq) /* freq unit: KHz */
{
    int ret = 0;

    ret = COM_tune(idx, freq / 100); /* driver accept 10KHz or 100KHz unit */
    if (ret) {
        LOGE("%s failed, [ret=%d]\n", __func__, ret);
    }

    fmr_data.cur_freq = (uint16_t)freq;
    LOGD("%s, [freq=%d] [ret=%d]\n", __func__, freq, ret);
    return ret;
}

/*return: fm_true: desense, fm_false: not desene channel*/
fm_bool FMR_DensenseDetect(fm_s32 idx, fm_u32 ChannelNo, fm_s32 RSSI)
{
    fm_u8 bDesenseCh = 0;

    bDesenseCh = COM_desense_check(idx, ChannelNo, RSSI);
    if (bDesenseCh == 1)
    {
        return fm_true;
    }
    return fm_false;
}

fm_bool FMR_SevereDensense(fm_u32 ChannelNo, fm_s32 RSSI)
{
    fm_s32 i, j;
    struct fm_fake_channel_t *chan_info = fmr_data.cfg_data.fake_chan;

    for (i = 0; i < chan_info->size; i++)
    {
        if ((ChannelNo / 100) == chan_info->chan[i].freq)
        {
            if (RSSI < chan_info->chan[i].rssi_th)
            {
                LOGI(" SevereDensense[%d] RSSI[%d]\n", ChannelNo, RSSI);
                return fm_true;
            }
            else
            {
                break;
            }
        }
    }
    return fm_false;
}
#if (FMR_NOISE_FLOORT_DETECT == 1)
/*return TRUE:get noise floor freq*/
fm_bool FMR_NoiseFloorDetect(fm_bool *rF, fm_s32 rssi, fm_s32 *F_rssi)
{
    if (rF[0] == fm_true)
    {
        if (rF[1] ==  fm_true)
        {
            F_rssi[2] = rssi;
            rF[2] = fm_true;
            return fm_true;
        }
        else
        {
            F_rssi[1] = rssi;
            rF[1] = fm_true;
        }
    }
    else
    {
        F_rssi[0] = rssi;
        rF[0] = fm_true;
    }
    return fm_false;
}
#endif

/*check the cur_freq->freq is valid or not
return fm_true : need check cur_freq->valid
         fm_false: check faild, should stop seek
*/
static fm_bool FMR_Seek_TuneCheck(int idx, struct fm_softmute_tune_t *cur_freq)
{
    int ret;

    if (fmr_data.scan_stop == fm_true)
    {
        ret = FMR_tune(idx, fmr_data.cur_freq);
        LOGI("seek stop!!! tune ret=%d", ret);
        return fm_false;
    }

    ret = COM_Soft_Mute_Tune(idx, cur_freq);
    if (ret)
    {
        LOGE("soft mute tune, failed:[%d]\n", ret);
        cur_freq->valid = fm_false;
        return fm_true;
    }
    if (cur_freq->valid == fm_true) /*get valid channel*/
    {
        if (FMR_DensenseDetect(idx, cur_freq->freq, cur_freq->rssi) == fm_true)
        {
            LOGI("desense channel detected:[%d] \n", cur_freq->freq);
            cur_freq->valid = fm_false;
            return fm_true;
        }
        if (FMR_SevereDensense(cur_freq->freq, cur_freq->rssi) == fm_true)
        {
            LOGI("sever desense channel detected:[%d] \n", cur_freq->freq);
            cur_freq->valid = fm_false;
            return fm_true;
        }
        LOGI("seek result freq:[%d] \n", cur_freq->freq);
    }
    return fm_true;
}
/*
check more 2 freq, curfreq: current freq, seek_dir: 1, forward. 0, backword
*/
static int FMR_Seek_More(int idx, struct fm_softmute_tune_t *validfreq, fm_u8 seek_dir, fm_u8 step,
                                fm_u32 min_freq, fm_u32 max_freq)
{
    fm_s32 i;
    struct fm_softmute_tune_t cur_freq;
    cur_freq.freq = validfreq->freq;

    for (i = 0; i < 2; i++)
    {
        if (seek_dir == 0)  // forward
        {
            if (cur_freq.freq + step > max_freq)
            {
                return 0;
            }
            cur_freq.freq += step;
        }
        else  // backward
        {
            if (cur_freq.freq - step < min_freq)
            {
                return 0;
            }
            cur_freq.freq -= step;
        }
        if (FMR_Seek_TuneCheck(idx, &cur_freq) == fm_true)
        {
            if (cur_freq.valid == fm_true)
            {
                if (cur_freq.rssi > validfreq->rssi)
                {
                    validfreq->freq = cur_freq.freq;
                    validfreq->rssi = cur_freq.rssi;
                    LOGI("seek cover last by freq=%d", cur_freq.freq);
                }
            }
        }
        else
        {
            return -1;
        }
    }
    return 0;
}

/*check the a valid channel
return -1 : seek fail
         0: seek success
*/
int FMR_seek(int idx, radio_hal_band_config_t *config, unsigned int start_freq, unsigned int dir, radio_program_info_t *ret_info)
{
    int ret, i, j;
    struct fm_softmute_tune_t cur_freq;
    unsigned int band_channel_no;
    unsigned int min_freq, max_freq, seek_space;
    int rssi;

    if ((start_freq < config->lower_limit) || (start_freq > config->upper_limit)) {
        ALOGD("%s start_freq = %d is out of range low %d : up %d", __func__, start_freq,
              config->lower_limit, config->upper_limit);
        start_freq = config->lower_limit;
    }

    ret_info->channel = start_freq;
    min_freq = config->lower_limit;
    max_freq = config->upper_limit;
    seek_space = config->spacings[0]; /* KHz */
    band_channel_no = (unsigned int)((max_freq - min_freq) / seek_space + 1);
    if (band_channel_no <= 0) {
        ALOGD("%s band range error, upper_limit=%d, lower_limit=%d", __func__, max_freq, min_freq);
        return -1;
    }
    fmr_data.scan_stop = fm_false;
    LOGD("seek start freq=%d band_channel_no=[%d] seek_space=%d band[%d - %d] dir=%d\n", start_freq,
         band_channel_no, seek_space, min_freq, max_freq, dir);

    if (dir == RADIO_DIRECTION_UP) /* forward */
    {
        for (i = ((start_freq - min_freq) / seek_space + 1); i < (int)band_channel_no; i++)
        {
            cur_freq.freq = min_freq + seek_space * i;
            LOGI("i=%d, freq=%d-----1", i, cur_freq.freq);
            ret = FMR_Seek_TuneCheck(idx, &cur_freq);
            if (ret == fm_false)
            {
                return -1;
            }
            else
            {
                if (cur_freq.valid == fm_false)
                {
                    continue;
                }
                else
                {
                    if (FMR_Seek_More(idx, &cur_freq, dir, seek_space, min_freq, max_freq) == 0)
                    {
                        ret_info->channel = cur_freq.freq;
                        ret_info->signal_strength = cur_freq.rssi; /* ?? rssi should be in 0~100 */
                        return 0;
                    }
                    else
                    {
                        return -1;
                    }
                }
            }
        }
        for (i = 0; i < (int)((start_freq - min_freq) / seek_space); i++)
        {
            cur_freq.freq = min_freq + seek_space * i;
            LOGI("i=%d, freq=%d-----2", i, cur_freq.freq);
            ret = FMR_Seek_TuneCheck(idx, &cur_freq);
            if (ret == fm_false)
            {
                return -1;
            }
            else
            {
                if (cur_freq.valid == fm_false)
                {
                    continue;
                }
                else
                {
                    if (FMR_Seek_More(idx, &cur_freq, dir, seek_space, min_freq, max_freq) == 0)
                    {
                        ret_info->channel = cur_freq.freq;
                        ret_info->signal_strength = cur_freq.rssi; /* ?? rssi should be in 0~100 */
                        return 0;
                    }
                    else
                    {
                        return -1;
                    }
                }
            }
        }
    }
    else/*backward*/
    {
        for (i = (int)((start_freq - min_freq) / seek_space - 1); i >= 0; i--)
        {
            cur_freq.freq = min_freq + seek_space * i;
            LOGI("i=%d, freq=%d-----3", i, cur_freq.freq);
            ret = FMR_Seek_TuneCheck(idx, &cur_freq);
            if (ret == fm_false)
            {
                return -1;
            }
            else
            {
                if (cur_freq.valid == fm_false)
                {
                    continue;
                }
                else
                {
                    if (FMR_Seek_More(idx, &cur_freq, dir, seek_space, min_freq, max_freq) == 0)
                    {
                        ret_info->channel = cur_freq.freq;
                        ret_info->signal_strength = cur_freq.rssi; /* ?? rssi should be in 0~100 */
                        return 0;
                    }
                    else
                    {
                        return -1;
                    }
                }
            }
        }
        for (i = (int)(band_channel_no - 1); i > (int)((start_freq - min_freq) / seek_space); i--)
        {
            cur_freq.freq = min_freq + seek_space * i;
            LOGI("i=%d, freq=%d-----4", i, cur_freq.freq);
            ret = FMR_Seek_TuneCheck(idx, &cur_freq);
            if (ret == fm_false)
            {
                return -1;
            }
            else
            {
                if (cur_freq.valid == fm_false)
                {
                    continue;
                }
                else
                {
                    if (FMR_Seek_More(idx, &cur_freq, dir, seek_space, min_freq, max_freq) == 0)
                    {
                        ret_info->channel = cur_freq.freq;
                        ret_info->signal_strength = cur_freq.rssi; /* ?? rssi should be in 0~100 */
                        return 0;
                    }
                    else
                    {
                        return -1;
                    }
                }
            }
        }
    }

    return -1; /* no valid channel found */
}


int FMR_set_mute(int idx, int mute)
{
    int ret = 0;

    if ((mute < 0) || (mute > 1)) {
        LOGE("%s error param mute:  %d\n", __func__, mute);
    }

    ret = COM_set_mute(idx, mute);
    if (ret) {
        LOGE("%s failed, %s\n", __func__, FMR_strerr());
    }
    LOGD("%s, [mute=%d] [ret=%d]\n", __func__, mute, ret);
    return ret;
}

int FMR_is_fm_pwrup(int idx, int *pwrup)
{
    int ret = 0;

    FMR_ASSERT(pwrup);

    ret = COM_is_fm_pwrup(idx, pwrup);
    if (ret) {
        *pwrup = 0;
        LOGE("%s failed, %s\n", __func__, FMR_strerr());
    }

    LOGD("%s, [pwrup=%d] [ret=%d]\n", __func__, *pwrup, ret);
    return ret;
}

int FM_set_status(int idx, int which, bool stat)
{
    int ret = 0;

    ret = COM_fm_set_status(idx, which, stat);

    if (ret) {
        LOGE("%s failed, %s\n", __func__, FMR_strerr());
    }

    LOGD("%s, [which=%d] [stat=%d] [ret=%d]\n", __func__, which, stat, ret);
    return ret;
}

int FM_get_status(int idx, int which, bool *stat)
{
    int ret = 0;

    ret = COM_fm_get_status(idx, which, stat);

    if (ret) {
        LOGE("%s failed, %s\n", __func__, FMR_strerr());
    }

    LOGD("%s, [which=%d] [stat=%d] [ret=%d]\n", __func__, which, *stat, ret);
    return ret;
}

int FMR_is_rdsrx_support(int idx, int *supt)
{
    int ret = 0;

    FMR_ASSERT(supt);

    ret = COM_is_rdsrx_support(idx, supt);
    if (ret) {
        *supt = 0;
        LOGE("%s, failed\n", __func__);
    }
    LOGD("%s, [supt=%d] [ret=%d]\n", __func__, *supt, ret);
    return ret;
}

int FMR_Pre_Search(int idx)
{
    /* avoid scan stop flag clear if stop cmd send before pre-search finish */
    fmr_data.scan_stop = fm_false;
    COM_pre_search(idx);
    return 0;
}

int FMR_Restore_Search(int idx)
{
    COM_restore_search(idx);
    return 0;
}

int FMR_stop_scan(void)
{
    fmr_data.scan_stop = fm_true;
    return 0;
}

int FMR_turn_on_off_rds(int idx, int onoff)
{
    int ret = 0;

    ret = COM_turn_on_off_rds(idx, onoff);
    if (ret) {
        LOGE("%s, failed\n", __func__);
    }
    LOGD("%s, [onoff=%d] [ret=%d]\n", __func__, onoff, ret);
    return ret;
}

int FMR_read_rds_data(int idx, uint16_t *rds_status)
{
    int ret = 0;

    FMR_ASSERT(rds_status);

    ret = COM_read_rds_data(idx, &fmr_data.rds, rds_status);
	/*if (ret) {
		LOGE("%s, get no event\n", __func__);
	}*/
    return ret;
}

int FMR_active_af(int idx, uint16_t *ret_freq)
{
    int ret = 0;

    FMR_ASSERT(ret_freq);
    ret = COM_active_af(idx, &fmr_data.rds, fmr_data.cfg_data.band, fmr_data.cur_freq, ret_freq);
    if ((ret == 0) && (*ret_freq != fmr_data.cur_freq)) {
        fmr_data.cur_freq = *ret_freq;
        LOGI("active AF OK, new channel[freq=%d]\n", fmr_data.cur_freq);
    }
    LOGD("%s, [ret=%d]\n", __func__, ret);
    return ret;
}

int FMR_active_ta(int idx, uint16_t *ret_freq)
{
    int ret = 0;

    FMR_ASSERT(ret_freq);
    ret = COM_active_ta(idx, &fmr_data.rds, fmr_data.cfg_data.band, fmr_data.cur_freq,
                        &fmr_data.backup_freq, ret_freq);
    if ((ret == 0) && (*ret_freq != fmr_data.cur_freq)) {
        fmr_data.cur_freq = *ret_freq;
        LOGI("active TA OK, new channel[freq=%d]\n", fmr_data.cur_freq);
    }
    LOGD("%s, [ret=%d]\n", __func__, ret);
    return ret;
}

int FMR_deactive_ta(int idx, uint16_t *ret_freq)
{
    int ret = 0;

    FMR_ASSERT(ret_freq);
    ret = COM_deactive_ta(idx, &fmr_data.rds, fmr_data.cur_freq, &(fmr_data.backup_freq), ret_freq);
    if ((ret == 0) && (*ret_freq != fmr_data.cur_freq)) {
        fmr_data.cur_freq = *ret_freq;
        LOGI("deactive TA OK, new channel[freq=%d]\n", fmr_data.cur_freq);
    }
    LOGD("%s, [ret=%d]\n", __func__, ret);
    return ret;
}

int FMR_ana_switch(int idx, int antenna)
{
    int ret = 0;

    if (fmr_data.cfg_data.short_ana_sup == true) {
        ret = COM_ana_switch(idx, antenna);
        if (ret) {
            LOGE("%s failed, [ret=%d]\n", __func__, ret);
        }
    } else {
        LOGW("FM antenna switch not support!\n");
        ret = -ERR_UNSUPT_SHORTANA;
    }

    LOGD("%s, [ret=%d]\n", __func__, ret);
    return ret;
}

int FMR_get_badratio(int idx, int *badratio)
{
    int ret = 0;

    FMR_ASSERT(badratio);

    ret = COM_get_badratio(idx, badratio);
    if (ret) {
        *badratio = 0;
        LOGE("%s failed, %s\n", __func__, FMR_strerr());
    }

    LOGD("%s, [badratio=%d] [ret=%d]\n", __func__, *badratio, ret);
    return ret;
}

int FMR_get_stereomono(int idx, int *stemono)
{
    int ret = 0;

    FMR_ASSERT(stemono);

    ret = COM_get_stereomono(idx, stemono);
    if (ret) {
        *stemono = 0;
        LOGE("%s failed, %s\n", __func__, FMR_strerr());
    }

    LOGD("%s, [stemono=%d] [ret=%d]\n", __func__, *stemono, ret);
    return ret;
}

int FMR_set_stereomono(int idx, int stemono)
{
    int ret = 0;

    ret = COM_set_stereomono(idx, stemono);
    if (ret) {
        LOGE("%s failed, %s\n", __func__, FMR_strerr());
    }

    LOGD("%s, [stemono=%d] [ret=%d]\n", __func__, stemono, ret);
    return ret;
}

int FMR_get_caparray(int idx, int *caparray)
{
    int ret = 0;

    FMR_ASSERT(caparray);

    ret = COM_get_caparray(idx, caparray);
    if (ret) {
        *caparray = 0;
        LOGE("%s failed, %s\n", __func__, FMR_strerr());
    }

    LOGD("%s, [caparray=%d] [ret=%d]\n", __func__, *caparray, ret);
    return ret;
}

int FMR_get_hw_info(int idx, int **info, int *info_len)
{
    int ret = 0;
    static int inited = 0;
    static int info_array[10] = {0};

    FMR_ASSERT(info);
    FMR_ASSERT(info_len);

    if (!inited) {
        ret = COM_get_hw_info(idx, &fmr_data.hw_info);
        if (ret >= 0)
            inited = 1;
    }

    info_array[0] = fmr_data.hw_info.chip_id;
    info_array[1] = fmr_data.hw_info.eco_ver;
    info_array[2] = fmr_data.hw_info.rom_ver;
    info_array[3] = fmr_data.hw_info.patch_ver;

    *info = info_array;
    *info_len = sizeof(struct fm_hw_info) / sizeof(int);

    LOGD("chip:0x%08x, eco:0x%08x, rom:0x%08x, patch: 0x%08x\n", info_array[0], info_array[1], info_array[2], info_array[3]);
    LOGD("%s, [ret=%d]\n", __func__, ret);
    return ret;
}
/*
th_idx:
	threshold type: 0, RSSI. 1, desense RSSI. 2, SMG.
th_val: threshold value*/
int FMR_EMSetTH(int idx, int th_idx, int th_val)
{
    int ret = -1;
    ret = COM_set_search_threshold(idx, th_idx, th_val);

    return ret;
}

int FMR_EM_CQI_logger(int idx, uint16_t cycle)
{
    int ret = -1;
    fm_full_cqi_log_t log_setting;
    uint i = 0;

#ifdef MTK_FM_50KHZ_SUPPORT
    log_setting.lower = FM_FREQ_MIN * 10;
    log_setting.upper = FM_FREQ_MAX * 10;
#else
    log_setting.lower = FM_FREQ_MIN;
    log_setting.upper = FM_FREQ_MAX;
#endif
    log_setting.space = 0x2;

    for (i = 0; i < cycle; i++)
    {
        log_setting.cycle = i;
        ret = COM_full_cqi_logger(idx, &log_setting);

        LOGD("%s, [%d]\n", __func__, i);
    }

    return ret;
}

