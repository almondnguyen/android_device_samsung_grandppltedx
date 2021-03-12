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

#ifndef __NMEA_PARSER_H__
#define __NMEA_PARSER_H__

#include <hardware/gps.h>
#include "hal_mnl_interface_common.h"
#include "mnl_common.h"

// for different SV parse
typedef enum {
    UNKNOWN_SV = 0,
    GPS_SV,
    SBAS_SV,
    GLONASS_SV,
    QZSS_SV,
    BDS_SV,
    GALILEO_SV,
} SV_TYPE;

/*****************************************************************/
/*****************************************************************/
/*****                                                       *****/
/*****       N M E A   P A R S E R                           *****/
/*****                                                       *****/
/*****************************************************************/
/*****************************************************************/
#define  NMEA_MAX_SIZE  255
/*maximum number of SV information in GPGSV*/
#define  NMEA_MAX_SV_INFO 4
#define  LOC_FIXED(pNmeaReader) ((pNmeaReader->fix_mode == 2) || (pNmeaReader->fix_mode ==3))
typedef struct {
    int     pos;
    int     overflow;
    int     utc_year;
    int     utc_mon;
    int     utc_day;
    int     utc_diff;
    gps_location  fix;

    /*
     * The fix flag extracted from GPGSA setence: 1: No fix; 2 = 2D; 3 = 3D
     * if the fix mode is 0, no location will be reported via callback
     * otherwise, the location will be reported via callback
     */
    int     fix_mode;
    /*
     * Indicate that the status of callback handling.
     * The flag is used to report GPS_STATUS_SESSION_BEGIN or GPS_STATUS_SESSION_END:
     * (0) The flag will be set as true when callback setting is changed via nmea_reader_set_callback
     * (1) GPS_STATUS_SESSION_BEGIN: receive location fix + flag set + callback is set
     * (2) GPS_STATUS_SESSION_END:   receive location fix + flag set + callback is null
     */
    int     cb_status_changed;
    int     sv_count;           /*used to count the number of received SV information*/
    gnss_sv_info  sv_status;
    // GpsCallbacks callbacks;
    char    in[ NMEA_MAX_SIZE+1 ];
} NmeaReader;

typedef struct {
    const char*  p;
    const char*  end;
} Token;

#define  MAX_NMEA_TOKENS  24

typedef struct {
    int     count;
    Token   tokens[ MAX_NMEA_TOKENS ];
} NmeaTokenizer;

static int nmea_reader_update_altitude(NmeaReader* const r,
                             Token        altitude,
                             Token        units);
static int nmea_reader_update_bearing(NmeaReader* const r,
                            Token        bearing);
static int nmea_reader_update_speed(NmeaReader* const r,
                            Token        speed);
static int nmea_reader_update_accuracy(NmeaReader* const r,
                             Token accuracy);
static int nmea_reader_update_sv_status(NmeaReader* r, int sv_index,
                                  int id, Token elevation,
                                  Token azimuth, Token snr);
static void nmea_reader_parse(NmeaReader* const r);
static void nmea_reader_addc(NmeaReader* const r, int   c);
void mtk_mnl_nmea_parser_process(const char * buffer, UINT32 length);
extern int op01_log_write_internal(const char* log);
int get_gps_nmea_parser_end_status();

#endif
