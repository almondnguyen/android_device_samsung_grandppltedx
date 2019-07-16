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
#include <string.h>
#include <memory.h>
#include "fmr.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "FMLIB_COM"

static int g_stopscan = 0;

int COM_open_dev(const char *pname, int *fd)
{
    int ret = 0;
    int tmp = -1;

    FMR_ASSERT(pname);
    FMR_ASSERT(fd);

    LOGI("COM_open_dev start\n");
    tmp = open(pname, O_RDWR);
    if (tmp < 0) {
        LOGE("Open %s failed, %s\n", pname, strerror(errno));
        ret = -ERR_INVALID_FD;
    }
    *fd = tmp;
    LOGI("%s, [fd=%d] [ret=%d]\n", __func__, *fd, ret);
    return ret;
}

int COM_close_dev(int fd)
{
    int ret = 0;

    LOGI("COM_close_dev start\n");
    ret = close(fd);
    if (ret) {
        LOGE("%s, failed\n", __func__);
    }
    LOGD("%s, [fd=%d] [ret=%d]\n", __func__, fd, ret);
    return ret;
}

int COM_pwr_up(int fd, int band, int freq)
{
    int ret = 0;
    struct fm_tune_parm parm;

    LOGI("%s, [freq=%d]\n", __func__, freq);
    bzero(&parm, sizeof(struct fm_tune_parm));

    parm.band = band;
    parm.freq = freq;
    parm.hilo = FM_AUTO_HILO_OFF;
#ifdef MTK_FM_50KHZ_SUPPORT
    parm.space = 5;  // FM radio seek space, 5:50KHZ; 1:100KHZ; 2:200KHZ
#else
    parm.space = 1;
#endif

    ret = ioctl(fd, FM_IOCTL_POWERUP, &parm);
    if (ret) {
        LOGE("%s, failed\n", __func__);
    }
    LOGD("%s, [fd=%d] [ret=%d]\n", __func__, fd, ret);
    return ret;
}

int COM_pwr_down(int fd, int type)
{
    int ret = 0;
    LOGI("%s, [type=%d]\n", __func__, type);
    ret = ioctl(fd, FM_IOCTL_POWERDOWN, &type);
    if (ret) {
        LOGE("%s, failed\n", __func__);
    }
    LOGD("%s, [fd=%d] [ret=%d]\n", __func__, fd, ret);
    return ret;
}

int COM_get_chip_id(int fd, int *chipid)
{
    int ret = 0;
    uint16_t tmp = 0;

    FMR_ASSERT(chipid);

    ret = ioctl(fd, FM_IOCTL_GETCHIPID, &tmp);
    *chipid = (int)tmp;
    if (ret) {
        LOGE("%s, failed\n", __func__);
    }
    LOGD("%s, [fd=%d] [chipid=%x] [ret=%d]\n", __func__, fd, *chipid, ret);
    return ret;
}

int COM_get_rssi(int fd, int *rssi)
{
    int ret = 0;

    FMR_ASSERT(rssi);

    ret = ioctl(fd, FM_IOCTL_GETRSSI, rssi);
    if (ret) {
        LOGE("%s, failed, [ret=%d]\n", __func__, ret);
    }
    LOGI("%s, [rssi=%d] [ret=%d]\n", __func__, *rssi, ret);
    return ret;
}
/*0x20: space, 0x7E:~*/
#define ISVALID(c)((c) >= 0x20 && (c) <= 0x7E)
/*change any char which out of [0x20, 0x7E]to space(0x20)*/
void COM_change_string(uint8_t *str, int len)
{
    int i = 0;
    for (i = 0; i < len; i++) {
        if (false == ISVALID(str[i])) {
            str[i]= 0x20;
        }
    }
}
int COM_get_ps(RDSData_Struct *rds, uint8_t **ps, int *ps_len)
{
    int ret = 0;
    char tmp_ps[9] = {0};

    FMR_ASSERT(rds);
    FMR_ASSERT(ps);
    FMR_ASSERT(ps_len);

    if (rds->event_status&RDS_EVENT_PROGRAMNAME)
    {
        LOGD("%s, Success, [event_status=%d]\n", __func__, rds->event_status);
        *ps = &rds->PS_Data.PS[3][0];
        *ps_len = sizeof(rds->PS_Data.PS[3]);

        COM_change_string(*ps, *ps_len);
        memcpy(tmp_ps, *ps, 8);
        LOGI("PS=%s\n", tmp_ps);
    }
    else
    {
        LOGE("%s, Failed, [event_status=%d]\n", __func__, rds->event_status);
        *ps = NULL;
        *ps_len = 0;
        ret = -ERR_RDS_NO_DATA;
    }

    return ret;
}

int COM_get_rt(RDSData_Struct *rds, uint8_t **rt, int *rt_len)
{
    int ret = 0;
    char tmp_rt[65] = { 0 };

    FMR_ASSERT(rds);
    FMR_ASSERT(rt);
    FMR_ASSERT(rt_len);

    if (rds->event_status&RDS_EVENT_LAST_RADIOTEXT)
    {
        LOGD("%s, Success, [event_status=%d]\n", __func__, rds->event_status);
        *rt = &rds->RT_Data.TextData[3][0];
        *rt_len = rds->RT_Data.TextLength;

        COM_change_string(*rt, *rt_len);
        memcpy(tmp_rt, *rt, 64);
        LOGI("RT=%s\n", tmp_rt);
    }
    else
    {
        LOGE("%s, Failed, [event_status=%d]\n", __func__, rds->event_status);
        *rt = NULL;
        *rt_len = 0;
        ret = -ERR_RDS_NO_DATA;
    }
    return ret;
}

int COM_get_pi(RDSData_Struct *rds, uint16_t *pi)
{
    int ret = 0;

    FMR_ASSERT(rds);
    FMR_ASSERT(pi);

    if (rds->event_status&RDS_EVENT_PI_CODE) {
        LOGD("%s, Success, [event_status=%d] [PI=%d]\n", __func__, rds->event_status, rds->PI);
        *pi = rds->PI;
    } else {
        LOGI("%s, Failed, there's no pi, [event_status=%d]\n", __func__, rds->event_status);
        *pi = -1;
        ret = -ERR_RDS_NO_DATA;
    }

    return ret;
}

int COM_get_pty(RDSData_Struct *rds, uint8_t *pty)
{
    int ret = 0;

    FMR_ASSERT(rds);
    FMR_ASSERT(pty);

    if (rds->event_status&RDS_EVENT_PTY_CODE) {
        LOGD("%s, Success, [event_status=%d] [PTY=%d]\n", __func__, rds->event_status, rds->PTY);
        *pty = rds->PTY;
    } else {
        LOGI("%s, Success, there's no pty, [event_status=%d]\n", __func__, rds->event_status);
        *pty = -1;
        ret = -ERR_RDS_NO_DATA;
    }

    return ret;
}

int COM_tune(int fd, int freq)
{
    int ret = 0;

    struct fm_tune_parm parm;

    bzero(&parm, sizeof(struct fm_tune_parm));
    parm.freq = freq;
    parm.hilo = FM_AUTO_HILO_OFF;

    ret = ioctl(fd, FM_IOCTL_TUNE, &parm);
    if (ret) {
        LOGE("%s, failed\n", __func__);
    }
    LOGD("%s, [fd=%d] [freq=%d] [ret=%d]\n", __func__, fd, freq, ret);
    return ret;
}

int COM_seek(int fd, int *freq, int band, int dir, int lev)
{
    int ret = 0;
    struct fm_seek_parm parm;

    bzero(&parm, sizeof(struct fm_seek_parm));

    parm.band = band;
    parm.freq = *freq;
    parm.hilo = FM_AUTO_HILO_OFF;
#ifdef MTK_FM_50KHZ_SUPPORT
    parm.space = 5;  // FM radio seek space, 5:50KHZ; 1:100KHZ; 2:200KHZ
#else
    parm.space = 1;
#endif
    if (dir == 1) {
        parm.seekdir = FM_SEEK_UP;
    } else if (dir == 0) {
        parm.seekdir = FM_SEEK_DOWN;
    }
    parm.seekth = lev;

    ret = ioctl(fd, FM_IOCTL_SEEK, &parm);
    if (ret == 0) {
        *freq = parm.freq;
    }
    LOGD("%s, [fd=%d] [ret=%d]\n", __func__, fd, ret);
    return ret;
}

int COM_set_mute(int fd, int mute)
{
    int ret = 0;
    int tmp = mute;

    LOGD("%s, start \n", __func__);
    ret = ioctl(fd, FM_IOCTL_MUTE, &tmp);
    if (ret) {
        LOGE("%s, failed\n", __func__);
    }
    LOGD("%s, [fd=%d] [ret=%d]\n", __func__, fd, ret);
    return ret;
}

int COM_is_fm_pwrup(int fd, int *pwrup)
{
    int ret = 0;

    ret = ioctl(fd, FM_IOCTL_IS_FM_POWERED_UP, pwrup);
    if (ret) {
        LOGE("%s, failed\n", __func__);
    }
    LOGD("%s, [fd=%d] [ret=%d]\n", __func__, fd, ret);
    return ret;
}

int COM_fm_set_status(int fd, int which, bool stat)
{
    int ret = 0;
    fm_status_t stat_parm;

    bzero(&stat_parm, sizeof(fm_status_t));
    stat_parm.which = which;
    stat_parm.stat = stat;

    ret = ioctl(fd, FM_IOCTL_FM_SET_STATUS, &stat_parm);
    if (ret) {
        LOGE("%s, failed\n", __func__);
    }
    LOGD("%s, [fd=%d] [ret=%d]\n", __func__, fd, ret);
    return ret;
}

int COM_fm_get_status(int fd, int which, bool *stat)
{
    int ret = 0;
    fm_status_t stat_parm;

    bzero(&stat_parm, sizeof(fm_status_t));
    stat_parm.which = which;

    ret = ioctl(fd, FM_IOCTL_FM_GET_STATUS, &stat_parm);
    if (ret) {
        LOGE("%s, failed\n", __func__);
    }
    LOGD("%s, [fd=%d] [ret=%d]\n", __func__, fd, ret);

    *stat = stat_parm.stat;

    return ret;
}

static struct fm_scan_t scan_req;
static int scan_req_init_flag = 0;

int COM_pre_search(int fd)
{
    fm_s32 ret = 0;
    ret = ioctl(fd, FM_IOCTL_PRE_SEARCH, 0);
    LOGD("COM_pre_search:%d\n", ret);
    return ret;
}
int COM_restore_search(int fd)
{
    fm_s32 ret = 0;
    ret = ioctl(fd, FM_IOCTL_RESTORE_SEARCH, 0);
    LOGD("COM_restore_search:%d\n", ret);
    return ret;
}

/*soft mute tune function, usually for sw scan implement or CQI log tool*/
int COM_Soft_Mute_Tune(int fd, struct fm_softmute_tune_t *para)
{
    fm_s32 ret = 0;
// fm_s32 RSSI=0, PAMD=0, MR=0, ATDC=0;
// fm_u32 PRX=0;
// fm_u16 softmuteGainLvl=0;
    struct fm_softmute_tune_t value;

    value.freq = para->freq / 10; /* smt accept 10KHz unit */
    ret = ioctl(fd, FM_IOCTL_SOFT_MUTE_TUNE, &value);
    if (ret)
    {
        LOGE("FM soft mute tune faild:%d\n", ret);
        return ret;
    }
    #if 0
    LOGD("Raw data of soft mute tune[%d]: RSSI:[%x]PAMD:[%x]MR:[%x]ATDC:[%x]PRX:[%x]SMG:[%x]",
        para->freq, value.RSSI, value.PAMD, value.MR, value.ATDC, value.PRX, value.SMG);
    RSSI = ((value.RSSI & 0x03FF) >= 512) ? ((value.RSSI & 0x03FF) - 1024) : (value.RSSI & 0x03FF);
    PAMD = ((value.PAMD & 0xFF) >= 128) ? ((value.PAMD & 0x00FF) - 256) : (value.PAMD & 0x00FF);
    MR = ((value.MR & 0x01FF) >= 256) ? ((value.MR & 0x01FF) - 512) : (value.MR & 0x01FF);
    ATDC =((value.ATDC & 0x0FFF) >= 2048) ? ((value.ATDC & 0x0FFF) - 4096) : (value.ATDC & 0x0FFF);
    if (ATDC < 0)
    {
        ATDC = (~(ATDC)) - 1;  // Get abs value of ATDC
    }
    PRX = (value.PRX & 0x00FF);
    softmuteGainLvl = value.SMG;
    // check if the channel is valid according to each CQIs
    if ((RSSI >= RSSI_TH)
     && (PAMD <= PAMD_TH)
     && (ATDC <= ATDC_TH)
     && (MR >= MR_TH)
     && (PRX >= PRX_TH)
     && (softmuteGainLvl <= softMuteGainTH))
    {
        para->valid = fm_true;
    }
    else
    {
        para->valid = fm_false;
    }
    #endif
    para->valid = value.valid;
    para->rssi = value.rssi;
    // LOGI("soft mute tune[%d] valid[%d]: RSSI:[%d]PAMD:[%d]MR:[%d]ATDC:[%d]PRX:[%d]SMG:[%d]",para->freq,para->valid,RSSI,PAMD,MR,ATDC,PRX,softmuteGainLvl);
    return 0;
}

int COM_fastget_rssi(int fd, struct fm_rssi_req *rssi_req)
{
    int ret = 0;

    FMR_ASSERT(rssi_req);
    // get rssi per channel, mute --> fast tune --> get rssi
    if (rssi_req->read_cnt <= 0) {
        rssi_req->read_cnt = 1;  // RSSI read times, we can read more than one time to ensure rssi valid
    }
    ret = ioctl(fd, FM_IOCTL_SCAN_GETRSSI, rssi_req);
    if (ret) {
        LOGE("Fast get rssi faild\n");
    }

    return ret;
}

int COM_get_cqi(int fd, int num, char *buf, int buf_len)
{
    int ret;
    struct fm_cqi_req cqi_req;

    // check buf
    num = (num > CQI_CH_NUM_MAX) ? CQI_CH_NUM_MAX : num;
    num = (num < CQI_CH_NUM_MIN) ? CQI_CH_NUM_MIN : num;
    cqi_req.ch_num = (uint16_t)num;
    cqi_req.buf_size = cqi_req.ch_num * sizeof(struct fm_cqi);
    if (!buf || (buf_len < cqi_req.buf_size)) {
        LOGE("get cqi, invalid buf\n");
        return -1;
    }
    cqi_req.cqi_buf = buf;

    // get cqi from driver
    ret = ioctl(fd, FM_IOCTL_CQI_GET, &cqi_req);
    if (ret < 0) {
        LOGE("get cqi, failed %d\n", ret);
        return -1;
    }

    return 0;
}


int COM_sw_scan(int fd, uint16_t *scan_tbl, int *max_num, int band)
{
    int ret;
    int chl_cnt = 0;
    uint16_t start_freq = FM_FREQ_MIN;
    struct fm_seek_parm parm;

    g_stopscan = 0;

    do {
        bzero(&parm, sizeof(struct fm_seek_parm));
        parm.band = band;
        parm.freq = start_freq;
        parm.hilo = FM_AUTO_HILO_OFF;
#ifdef MTK_FM_50KHZ_SUPPORT
        parm.space = 5;  // FM radio seek space, 5:50KHZ; 1:100KHZ; 2:200KHZ
#else
        parm.space = 1;
#endif
        parm.seekdir = FM_SEEK_UP;
        parm.seekth = FM_SEEKTH_LEVEL_DEFAULT;

        ret = ioctl(fd, FM_IOCTL_SEEK, &parm);
        if (ret != 0) {
            LOGE("FM scan faild, %s, %d\n", strerror(errno), parm.err);
            break;
        }

        if ((parm.err == FM_SUCCESS) && (chl_cnt < *max_num) && (parm.freq > start_freq)) {
            scan_tbl[chl_cnt] = parm.freq;
            chl_cnt++;
        } else {
            break;
        }

        start_freq = parm.freq;
    } while (g_stopscan == 0);

    LOGI("FM sw scan %d station found\n", chl_cnt);
    return ret;
}

int COM_stop_scan(void)
{
    int ret = 0;

    g_stopscan = 1;

    return ret;
}

int COM_turn_on_off_rds(int fd, int onoff)
{
    int ret = 0;
    uint16_t rds_on = -1;

    LOGD("Rdsset start\n");
    if (onoff == FMR_RDS_ON) {
        rds_on = 1;
        ret = ioctl(fd, FM_IOCTL_RDS_ONOFF, &rds_on);
        if (ret) {
            LOGE("FM_IOCTL_RDS_ON failed\n");
            return ret;
        }
        LOGD("Rdsset Success, [rds_on=%d]\n", rds_on);
    } else {
        rds_on = 0;
        ret = ioctl(fd, FM_IOCTL_RDS_ONOFF, &rds_on);
        if (ret) {
            LOGE("FM_IOCTL_RDS_OFF failed\n");
            return ret;
        }
        LOGD("Rdsset Success, [rds_on=%d]\n", rds_on);
    }
    return ret;
}

int COM_is_rdsrx_support(int fd, int *supt)
{
    int ret = 0;
    int support = -1;

    if (fd < 0) {
        LOGE("FM isRDSsupport fail, g_fm_fd = %d\n", fd);
        *supt = -1;
        ret = -ERR_INVALID_FD;
        return ret;
    }

    ret = ioctl(fd, FM_IOCTL_RDS_SUPPORT, &support);
    if (ret) {
        LOGE("FM FM_IOCTL_RDS_SUPPORT fail, errno = %d\n", errno);
        // don't support
        *supt = 0;
        return ret;
    }
    LOGI("isRDSsupport Success, [support=%d]\n", support);
    *supt = support;
    return ret;
}

int COM_read_rds_data(int fd, RDSData_Struct *rds, uint16_t *rds_status)
{
    int ret = 0;
    uint16_t event_status;
    // char tmp_ps[9] = {0};
    // char tmp_rt[65] = { 0 };

    FMR_ASSERT(rds);
    FMR_ASSERT(rds_status);

    if (read(fd, rds, sizeof(RDSData_Struct)) == sizeof(RDSData_Struct)) {
        event_status = rds->event_status;
    // memcpy(tmp_ps, &rds->PS_Data.PS[3][0], 8);
    // memcpy(tmp_rt, &rds->RT_Data.TextData[3][0], 64);
    LOGI("event_status = 0x%x\n", event_status);
    // memset(tmp_ps, 0, 9);
    // memset(tmp_rt, 0, 65);
    *rds_status = event_status;

        return ret;
    } else {
        // LOGE("readrds get no event\n");
        ret = -ERR_RDS_NO_DATA;
    }
    return ret;
}

int COM_active_af(int fd, RDSData_Struct *rds, int band, uint16_t cur_freq, uint16_t *ret_freq)
{
    int ret = 0;
    int i = 0;
    struct fm_tune_parm parm;
    uint16_t rds_on = 0;
    uint16_t set_freq, sw_freq, org_freq, PAMD_Value, AF_PAMD_LBound, AF_PAMD_HBound;
    uint16_t PAMD_Level[25];
    uint16_t PAMD_DB_TBL[5] = {// 5dB, 10dB, 15dB, 20dB, 25dB,
                               // 13, 17, 21, 25, 29};
                                8, 12, 15, 18, 20};
    FMR_ASSERT(rds);
    FMR_ASSERT(ret_freq);

    sw_freq = cur_freq;  // current freq
    org_freq = cur_freq;
    parm.band = band;
    parm.freq = sw_freq;
    parm.hilo = FM_AUTO_HILO_OFF;
#ifdef MTK_FM_50KHZ_SUPPORT
    parm.space = FM_SPACE_50K;
#else
    parm.space = FM_SPACE_100K;
#endif


    if (!(rds->event_status&RDS_EVENT_AF)) {
        LOGE("activeAF failed\n");
        *ret_freq = 0;
        ret = -ERR_RDS_NO_DATA;
        return ret;
    }

    AF_PAMD_LBound = PAMD_DB_TBL[0];  // 5dB
    AF_PAMD_HBound = PAMD_DB_TBL[1];  // 15dB
    ioctl(fd, FM_IOCTL_GETCURPAMD, &PAMD_Value);
    LOGI("current_freq=%d, PAMD_Value=%d\n", cur_freq, PAMD_Value);

    if (PAMD_Value < AF_PAMD_LBound) {
        rds_on = 0;
        ioctl(fd, FM_IOCTL_RDS_ONOFF, &rds_on);
        // make sure rds->AF_Data.AF_Num is valid
        rds->AF_Data.AF_Num = (rds->AF_Data.AF_Num > 25)? 25 : rds->AF_Data.AF_Num;
        for (i = 0; i < rds->AF_Data.AF_Num; i++) {
            set_freq = rds->AF_Data.AF[1][i];  // method A or B
            if (set_freq != org_freq) {
                parm.freq = set_freq;
                ioctl(fd, FM_IOCTL_TUNE, &parm);
                usleep(250*1000);
                ioctl(fd, FM_IOCTL_GETCURPAMD, &PAMD_Level[i]);
                LOGI("next_freq=%d, PAMD_Level=%d\n", parm.freq, PAMD_Level[i]);
                if (PAMD_Level[i] > PAMD_Value) {
                     PAMD_Value = PAMD_Level[i];
                     sw_freq = set_freq;
                }
            }
        }
        LOGI("PAMD_Value=%d, sw_freq=%d\n", PAMD_Value, sw_freq);
        if ((PAMD_Value > AF_PAMD_HBound) && (sw_freq != 0)) {
            parm.freq = sw_freq;
            ioctl(fd, FM_IOCTL_TUNE, &parm);
            cur_freq = parm.freq;
        } else {
            parm.freq = org_freq;
            ioctl(fd, FM_IOCTL_TUNE, &parm);
            cur_freq = parm.freq;
        }
        rds_on = 1;
        ioctl(fd, FM_IOCTL_RDS_ONOFF, &rds_on);
    } else {
        LOGD("RDS_EVENT_AF old freq:%d\n", org_freq);
    }
    *ret_freq = cur_freq;

    return ret;
}

int COM_active_ta(int fd, RDSData_Struct *rds, int band, uint16_t cur_freq, uint16_t *backup_freq, uint16_t *ret_freq)
{
    int ret = 0;

    FMR_ASSERT(rds);
    FMR_ASSERT(backup_freq);
    FMR_ASSERT(ret_freq);

    if (rds->event_status&RDS_EVENT_TAON) {
        uint16_t rds_on = 0;
        struct fm_tune_parm parm;
        uint16_t PAMD_Level[25];
        uint16_t PAMD_DB_TBL[5] = {13, 17, 21, 25, 29};
        uint16_t set_freq, sw_freq, org_freq, PAMD_Value, TA_PAMD_Threshold;
        int i = 0;

        rds_on = 0;
        ioctl(fd, FM_IOCTL_RDS_ONOFF, &rds_on);
        TA_PAMD_Threshold = PAMD_DB_TBL[2];  // 15dB
        sw_freq = cur_freq;
        org_freq = cur_freq;
        *backup_freq = org_freq;
        parm.band = band;
        parm.freq = sw_freq;
        parm.hilo = FM_AUTO_HILO_OFF;
#ifdef MTK_FM_50KHZ_SUPPORT
        parm.space = FM_SPACE_50K;
#else
        parm.space = FM_SPACE_100K;
#endif

        ioctl(fd, FM_IOCTL_GETCURPAMD, &PAMD_Value);
        // make sure rds->AF_Data.AF_Num is valid
        rds->AFON_Data.AF_Num = (rds->AFON_Data.AF_Num > 25)? 25 : rds->AFON_Data.AF_Num;
        for (i = 0; i < rds->AFON_Data.AF_Num; i++) {
           set_freq = rds->AFON_Data.AF[1][i];
            LOGI("set_freq=0x%02x, org_freq=0x%02x\n", set_freq, org_freq);
           if (set_freq != org_freq) {
               parm.freq = sw_freq;
               ioctl(fd, FM_IOCTL_TUNE, &parm);
               ioctl(fd, FM_IOCTL_GETCURPAMD, &PAMD_Level[i]);
               if (PAMD_Level[i] > PAMD_Value) {
                   PAMD_Value = PAMD_Level[i];
                   sw_freq = set_freq;
               }
           }
        }

        if ((PAMD_Value > TA_PAMD_Threshold) && (sw_freq != 0)) {
           rds->Switch_TP = 1;
           parm.freq = sw_freq;
           ioctl(fd, FM_IOCTL_TUNE, &parm);
           cur_freq = parm.freq;
        } else {
            parm.freq = org_freq;
            ioctl(fd, FM_IOCTL_TUNE, &parm);
            cur_freq = parm.freq;
        }
        rds_on = 1;
        ioctl(fd, FM_IOCTL_RDS_ONOFF, &rds_on);
    }

    *ret_freq = cur_freq;
    return ret;
}

int COM_deactive_ta(int fd, RDSData_Struct *rds, uint16_t cur_freq, uint16_t *backup_freq, uint16_t *ret_freq)
{
    int ret = 0;

    FMR_ASSERT(rds);
    FMR_ASSERT(backup_freq);
    FMR_ASSERT(ret_freq);

    if (rds->event_status&RDS_EVENT_TAON_OFF) {
        uint16_t rds_on = 0;
        struct fm_tune_parm parm;
        parm.band = FM_RAIDO_BAND;
        parm.freq = *backup_freq;
        parm.hilo = FM_AUTO_HILO_OFF;
#ifdef MTK_FM_50KHZ_SUPPORT
        parm.space = FM_SPACE_50K;
#else
        parm.space = FM_SPACE_100K;
#endif
        ioctl(fd, FM_IOCTL_RDS_ONOFF, &rds_on);

        ioctl(fd, FM_IOCTL_TUNE, &parm);
        cur_freq = parm.freq;
        rds_on = 1;
        ioctl(fd, FM_IOCTL_RDS_ONOFF, &rds_on);
    }

    *ret_freq = cur_freq;
    return ret;
}

int COM_ana_switch(int fd, int antenna)
{
    int ret = 0;

    ret = ioctl(fd, FM_IOCTL_ANA_SWITCH, &antenna);
    if (ret < 0) {
        LOGE("%s: fail, ret = %d\n", __func__, ret);
    }

    LOGD("%s: [ret = %d]\n", __func__, ret);
    return ret;
}

int COM_get_badratio(int fd, int *badratio)
{
    int ret = 0;
    uint16_t tmp = 0;

    ret = ioctl(fd, FM_IOCTL_GETBLERRATIO, &tmp);
    *badratio = (int)tmp;
    if (ret) {
        LOGE("%s, failed\n", __func__);
    }
    LOGD("%s, [fd=%d] [ret=%d]\n", __func__, fd, ret);
    return ret;
}


int COM_get_stereomono(int fd, int *stemono)
{
    int ret = 0;
    uint16_t tmp = 0;

    ret = ioctl(fd, FM_IOCTL_GETMONOSTERO, &tmp);
    *stemono = (int)tmp;
    if (ret) {
        LOGE("%s, failed\n", __func__);
    }
    LOGD("%s, [fd=%d] [ret=%d]\n", __func__, fd, ret);
    return ret;
}

int COM_set_stereomono(int fd, int stemono)
{
    int ret = 0;

    ret = ioctl(fd, FM_IOCTL_SETMONOSTERO, stemono);
    if (ret) {
        LOGE("%s, failed\n", __func__);
    }
    LOGD("%s, [fd=%d] [ret=%d]\n", __func__, fd, ret);
    return ret;
}

int COM_get_caparray(int fd, int *caparray)
{
    int ret = 0;

    ret = ioctl(fd, FM_IOCTL_GETCAPARRAY, caparray);
    if (ret) {
        LOGE("%s, failed\n", __func__);
    }
    LOGD("%s, [fd=%d] [ret=%d]\n", __func__, fd, ret);
    return ret;
}

int COM_get_hw_info(int fd, struct fm_hw_info *info)
{
    int ret = 0;

    ret = ioctl(fd, FM_IOCTL_GET_HW_INFO, info);
    if (ret) {
        LOGE("%s, failed\n", __func__);
    }
    LOGD("%s, [fd=%d] [ret=%d]\n", __func__, fd, ret);
    return ret;
}


/*  COM_is_dese_chan -- check if gived channel is a de-sense channel or not
  *  @fd - fd of "dev/fm"
  *  @freq - gived channel
  *  return value: 0, not a dese chan; 1, a dese chan; else error NO.
  */
int COM_is_dese_chan(int fd, int freq)
{
    int ret = 0;
    int tmp = freq;

    ret = ioctl(fd, FM_IOCTL_IS_DESE_CHAN, &freq);
    if (ret < 0) {
        LOGE("%s, failed, ret=%d\n", __func__, ret);
        return ret;
    } else {
        LOGD("[fd=%d] %d --> dese=%d\n", fd, tmp, freq);
        return freq;
    }
}



/*  COM_desense_check -- check if gived channel is a de-sense channel or not
  *  @fd - fd of "dev/fm"
  *  @freq - gived channel
  *  @rssi-freq's rssi
  *  return value: 0, is desense channel and rssi is less than threshold; 1, not desense channel or it is but rssi is more than threshold.
  */
int COM_desense_check(int fd, int freq, int rssi)
{
    int ret = 0;
    fm_desense_check_t parm;

    parm.freq = freq / 100; /* driver accept 10KHz or 100KHz unit */
    parm.rssi = rssi;
    ret = ioctl(fd, FM_IOCTL_DESENSE_CHECK, &parm);
    if (ret < 0) {
        LOGE("%s, failed, ret=%d\n", __func__, ret);
        return ret;
    } else {
        LOGD("[fd=%d] %d --> dese=%d\n", fd, freq, ret);
        return ret;
    }
}
/*
th_idx:
	threshold type: 0, RSSI. 1, desense RSSI. 2, SMG.
th_val: threshold value*/
int COM_set_search_threshold(int fd, int th_idx, int th_val)
{
    int ret = 0;
    struct fm_search_threshold_t th_parm;
    th_parm.th_type = th_idx;
    th_parm.th_val = th_val;
    th_parm.reserve = 0;

    ret = ioctl(fd, FM_IOCTL_SET_SEARCH_THRESHOLD, th_parm);
    if (ret < 0)
        LOGE("%s, failed, ret=%d\n", __func__, ret);

    return ret;
}
int COM_full_cqi_logger(int fd, fm_full_cqi_log_t *log_parm)
{
    int ret = 0;

    ret = ioctl(fd, FM_IOCTL_FULL_CQI_LOG, log_parm);
    if (ret < 0)
    {
        LOGE("%s, failed, ret=%d\n", __func__, ret);
    }
    return ret;
}
