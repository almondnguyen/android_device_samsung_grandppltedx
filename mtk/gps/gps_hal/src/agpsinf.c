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
#include "gpshal.h"
#include "hal2mnl_interface.h"
#include "mtk_lbs_utility.h"
//=========================================================
// Agps Interface

static void agpsinf_init(AGpsCallbacks* callbacks) {
    g_gpshal_ctx.agps_cbs = callbacks;
}

static int agpsinf_set_server(AGpsType type, const char* hostname, int port) {
    hal2mnl_set_server(type, hostname, port);
    return 0;  // 0:ok,   non-zero: error; but GPS JNI will ignore it
}

static int agpsinf_data_conn_open_with_apn_ip_type(
        const char* apn,
        ApnIpType apnIpType) {
    hal2mnl_data_conn_open_with_apn_ip_type(apn, apnIpType);
    return 0;  // 0:ok,   non-zero: error; but GPS JNI will ignore it
}

const AGpsInterface mtk_agps_inf = {
    sizeof(AGpsInterface),
    agpsinf_init,
    hal2mnl_data_conn_open,    // GPS JNI will ignore its ret value
    hal2mnl_data_conn_closed,  // GPS JNI will ignore its ret value
    hal2mnl_data_conn_failed,  // GPS JNI will ignore its ret value
    agpsinf_set_server,
    agpsinf_data_conn_open_with_apn_ip_type
};


//=========================================================
// Gps Ni Interface

static void gpsni_init(GpsNiCallbacks *callbacks) {
    g_gpshal_ctx.gpsni_cbs = callbacks;
}

static void gpsni_respond(int notif_id, GpsUserResponseType user_response) {
    hal2mnl_ni_respond(notif_id, user_response);
}

const GpsNiInterface  mtk_gps_ni_inf = {
    sizeof(GpsNiInterface),
    gpsni_init,
    gpsni_respond
};


//=========================================================
// Agps Ril Interface

static void agps_ril_init(AGpsRilCallbacks* callbacks) {
    g_gpshal_ctx.agpsril_cbs = callbacks;
}

static void agps_ril_set_ref_location(
        const AGpsRefLocation *agps_reflocation,
        __unused size_t sz_struct) {
    uint16_t type = agps_reflocation->type;
    switch (type) {
        case AGPS_REF_LOCATION_TYPE_GSM_CELLID:
        case AGPS_REF_LOCATION_TYPE_UMTS_CELLID: {
            const AGpsRefLocationCellID cell = agps_reflocation->u.cellID;
            hal2mnl_set_ref_location(
                     type, cell.mcc, cell.mnc, cell.lac, cell.cid);
            break;
        }
        default:
            LOGW("%s: unsupported ref loc type: %d", __func__, type);
    }
}

static void agps_ril_set_set_id(AGpsSetIDType type, const char* setid) {
    hal2mnl_set_id(type, setid);
}

static void agps_ril_ni_message(uint8_t *msg, size_t len) {
    hal2mnl_ni_message((char*)msg, len);
}

static void agps_ril_update_network_state(int connected, int type, int roaming, const char* extra_info) {
    hal2mnl_update_network_state(connected, type, roaming, extra_info);
}

static void agps_ril_update_network_availability(int available, const char* apn) {
    hal2mnl_update_network_availability(available, apn);
}

const AGpsRilInterface mtk_agps_ril_inf = {
    sizeof(AGpsRilInterface),
    agps_ril_init,
    agps_ril_set_ref_location,
    agps_ril_set_set_id,
    agps_ril_ni_message,
    agps_ril_update_network_state,
    agps_ril_update_network_availability
};

