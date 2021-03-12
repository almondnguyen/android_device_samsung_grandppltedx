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
#include <sys/epoll.h>  // epoll_create, epoll_event
#include <errno.h>     // errno
#include <string.h>    // strerror
#include "hal2mnl_interface.h"
#include "mtk_lbs_utility.h"
//=========================================================

#define GPSHAL_WORKER_THREAD_TIMEOUT (30*1000)

#define fieldp_copy(dst, src, f)  (dst)->f  = (src)->f
#define field_copy(dst, src, f)  (dst).f   = (src).f

//=========================================================

static void update_mnld_reboot() {
    gpshal_state state = g_gpshal_ctx.gps_state_intent;
    LOGD("state: (%d)", state);
    switch (state) {
        case GPSHAL_STATE_INIT:
        case GPSHAL_STATE_STOP:
            gpshal2mnl_gps_init();
            break;
        case GPSHAL_STATE_START:
            gpshal2mnl_gps_init();
            gpshal2mnl_gps_start();
            break;
        case GPSHAL_STATE_CLEANUP:
        default:
            LOGW("Current gps_state_intent: %s (%d)",
                gpshal_state_to_string(state), state);
    }
}
static void update_location(gps_location location) {
    GpsLocation loc;
    dump_gps_location(location);
    gpshal_state state = g_gpshal_ctx.gps_state_intent;
    LOGD("state: (%d)", state);
    if (GPSHAL_STATE_START == state) {
        loc.size      = sizeof(loc);
        loc.flags     = location.flags;
        loc.latitude  = location.lat;
        loc.longitude = location.lng;
        loc.altitude  = location.alt;
        loc.speed     = location.speed;
        loc.bearing   = location.bearing;
        loc.accuracy  = location.accuracy;
        loc.timestamp = location.timestamp;
        g_gpshal_ctx.gps_cbs->location_cb(&loc);
    } else {
        // Do not report this location to GLP to avoid strange TTFF time issue
        LOGW("we have a location when gps_state_intent: %s (%d)",
                gpshal_state_to_string(state), state);
    }
}
static void update_gps_status(gps_status status) {
    GpsStatus s;
    LOGD("  status=%d", status);
    s.size   = sizeof(s);
    s.status = status;
    g_gpshal_ctx.gps_cbs->status_cb(&s);
}

static void update_gps_sv(gnss_sv_info sv) {

    int i;
    GnssSvStatus gss;
    dump_gnss_sv_info(sv);
    gss.size = sizeof(gss);
    gss.num_svs = sv.num_svs;
    for (i = 0; i < gss.num_svs; i++) {
        GnssSvInfo* dst = &gss.gnss_sv_list[i];
        gnss_sv*    src = &sv.sv_list[i];
        dst->size = sizeof(*dst);
        fieldp_copy(dst, src, svid);
        fieldp_copy(dst, src, constellation);
        fieldp_copy(dst, src, c_n0_dbhz);
        fieldp_copy(dst, src, elevation);
        fieldp_copy(dst, src, azimuth);
        fieldp_copy(dst, src, flags);
    }
    g_gpshal_ctx.gps_cbs->gnss_sv_status_cb(&gss);
}

static void update_nmea(int64_t timestamp, const char* nmea, int length) {
    LOGD("  timestamp=%lld nmea=[%s] length=%d",
        timestamp, nmea, length);
    g_gpshal_ctx.gps_cbs->nmea_cb(timestamp, nmea, length);
}
static void update_gps_capabilities(gps_capabilites capabilities) {
    LOGD("  capabilities=0x%x", capabilities);
    g_gpshal_ctx.gps_cbs->set_capabilities_cb(capabilities);
}
static void update_gps_measurements(gps_data data) {
    size_t i;
    GpsData gd;
    dump_gps_data(data);
    gd.size = sizeof(gd);
    field_copy(gd, data, measurement_count);
    for (i = 0; i < GPS_MAX_MEASUREMENT; i++) {
        GpsMeasurement*  dst = &gd.measurements[i];
        gps_measurement* src = &data.measurements[i];
        dst->size  = sizeof(*dst);
        fieldp_copy(dst, src, flags);
        fieldp_copy(dst, src, prn);
        fieldp_copy(dst, src, time_offset_ns);
        fieldp_copy(dst, src, state);
        fieldp_copy(dst, src, received_gps_tow_ns);
        fieldp_copy(dst, src, received_gps_tow_uncertainty_ns);
        fieldp_copy(dst, src, c_n0_dbhz);
        fieldp_copy(dst, src, pseudorange_rate_mps);
        fieldp_copy(dst, src, pseudorange_rate_uncertainty_mps);
        fieldp_copy(dst, src, accumulated_delta_range_state);
        fieldp_copy(dst, src, accumulated_delta_range_m);
        fieldp_copy(dst, src, accumulated_delta_range_uncertainty_m);
        fieldp_copy(dst, src, pseudorange_m);
        fieldp_copy(dst, src, pseudorange_uncertainty_m);
        fieldp_copy(dst, src, code_phase_chips);
        fieldp_copy(dst, src, code_phase_uncertainty_chips);
        fieldp_copy(dst, src, carrier_frequency_hz);
        fieldp_copy(dst, src, carrier_cycles);
        fieldp_copy(dst, src, carrier_phase);
        fieldp_copy(dst, src, carrier_phase_uncertainty);
        fieldp_copy(dst, src, loss_of_lock);
        fieldp_copy(dst, src, bit_number);
        fieldp_copy(dst, src, time_from_last_bit_ms);
        fieldp_copy(dst, src, doppler_shift_hz);
        fieldp_copy(dst, src, doppler_shift_uncertainty_hz);
        fieldp_copy(dst, src, multipath_indicator);
        fieldp_copy(dst, src, snr_db);
        fieldp_copy(dst, src, elevation_deg);
        fieldp_copy(dst, src, elevation_uncertainty_deg);
        fieldp_copy(dst, src, azimuth_deg);
        fieldp_copy(dst, src, azimuth_uncertainty_deg);
        fieldp_copy(dst, src, used_in_fix);
    }

    // To do things like field_copy(gd, data, clock)
    {
        GpsClock*  dst = &gd.clock;
        gps_clock* src = &data.clock;
        dst->size = sizeof(*dst);
        fieldp_copy(dst, src, flags);
        fieldp_copy(dst, src, leap_second);
        fieldp_copy(dst, src, type);
        fieldp_copy(dst, src, time_ns);
        fieldp_copy(dst, src, time_uncertainty_ns);
        fieldp_copy(dst, src, full_bias_ns);
        fieldp_copy(dst, src, bias_ns);
        fieldp_copy(dst, src, bias_uncertainty_ns);
        fieldp_copy(dst, src, drift_nsps);
        fieldp_copy(dst, src, drift_uncertainty_nsps);
    }
    g_gpshal_ctx.meas_cbs->measurement_callback(&gd);
}
static void update_gps_navigation(gps_nav_msg msg) {
    GpsNavigationMessage gnm;
    dump_gps_nav_msg(msg);
    gnm.size = sizeof(gnm);
    field_copy(gnm, msg, prn);
    field_copy(gnm, msg, type);
    field_copy(gnm, msg, status);
    field_copy(gnm, msg, message_id);
    field_copy(gnm, msg, submessage_id);
    field_copy(gnm, msg, data_length);
    gnm.data = (uint8_t*) msg.data;
    g_gpshal_ctx.navimsg_cbs->navigation_message_callback(&gnm);
}

static void update_gnss_measurements(gnss_data data) {
    size_t i;
    GnssData gd;
    dump_gnss_data(data);
    gd.size = sizeof(gd);
    field_copy(gd, data, measurement_count);
    for (i = 0; i < GNSS_MAX_MEASUREMENT; i++) {
        GnssMeasurement*  dst = &gd.measurements[i];
        gnss_measurement*  src = &data.measurements[i];
        dst->size  = sizeof(*dst);
        fieldp_copy(dst, src, flags);
        fieldp_copy(dst, src, svid);
        fieldp_copy(dst, src, constellation);
        fieldp_copy(dst, src, time_offset_ns);
        fieldp_copy(dst, src, state);
        fieldp_copy(dst, src, received_sv_time_in_ns);
        fieldp_copy(dst, src, received_sv_time_uncertainty_in_ns);
        fieldp_copy(dst, src, c_n0_dbhz);
        fieldp_copy(dst, src, pseudorange_rate_mps);
        fieldp_copy(dst, src, pseudorange_rate_uncertainty_mps);
        fieldp_copy(dst, src, accumulated_delta_range_state);
        fieldp_copy(dst, src, accumulated_delta_range_m);
        fieldp_copy(dst, src, accumulated_delta_range_uncertainty_m);
        fieldp_copy(dst, src, carrier_frequency_hz);
        fieldp_copy(dst, src, carrier_cycles);
        fieldp_copy(dst, src, carrier_phase);
        fieldp_copy(dst, src, carrier_phase_uncertainty);
        fieldp_copy(dst, src, multipath_indicator);
        fieldp_copy(dst, src, snr_db);
    }

    // To do things like field_copy(gd, data, clock)
    {
        GnssClock*  dst = &gd.clock;
        gnss_clock* src = &data.clock;
        dst->size = sizeof(*dst);
        fieldp_copy(dst, src, flags);
        fieldp_copy(dst, src, leap_second);
        fieldp_copy(dst, src, time_ns);
        fieldp_copy(dst, src, time_uncertainty_ns);
        fieldp_copy(dst, src, full_bias_ns);
        fieldp_copy(dst, src, bias_ns);
        fieldp_copy(dst, src, bias_uncertainty_ns);
        fieldp_copy(dst, src, drift_nsps);
        fieldp_copy(dst, src, drift_uncertainty_nsps);
        fieldp_copy(dst, src, hw_clock_discontinuity_count);
    }

    g_gpshal_ctx.meas_cbs->gnss_measurement_callback(&gd);
}

static void update_gnss_navigation(gnss_nav_msg msg) {
    GnssNavigationMessage gnm;
    dump_gnss_nav_msg(msg);
    gnm.size = sizeof(gnm);
    field_copy(gnm, msg, svid);
    field_copy(gnm, msg, type);
    field_copy(gnm, msg, status);
    field_copy(gnm, msg, message_id);
    field_copy(gnm, msg, submessage_id);
    field_copy(gnm, msg, data_length);
    gnm.data = (uint8_t*) msg.data;
    g_gpshal_ctx.navimsg_cbs->gnss_navigation_message_callback(&gnm);
}

static void request_wakelock() {
    g_gpshal_ctx.gps_cbs->acquire_wakelock_cb();
}
static void release_wakelock() {
    g_gpshal_ctx.gps_cbs->release_wakelock_cb();
}
static void request_utc_time() {
    g_gpshal_ctx.gps_cbs->request_utc_time_cb();
}
static void request_data_conn(__unused struct sockaddr_storage* addr) {
    UNUSED(addr);
    AGpsStatus as;
    as.size   = sizeof(AGpsStatus);  // our imp supports v3
    as.type   = AGPS_TYPE_SUPL;
    as.status = GPS_REQUEST_AGPS_DATA_CONN;
    as.addr   = *addr;
    g_gpshal_ctx.agps_cbs->status_cb(&as);
}
static void release_data_conn() {
    AGpsStatus as;
    as.size   = sizeof(AGpsStatus_v1);  // use v1 size to omit optional fields
    as.type   = AGPS_TYPE_SUPL;
    as.status = GPS_RELEASE_AGPS_DATA_CONN;
    g_gpshal_ctx.agps_cbs->status_cb(&as);
}
static void request_ni_notify(int session_id, agps_notify_type type, const char* requestor_id,
        const char* client_name, ni_encoding_type requestor_id_encoding,
        ni_encoding_type client_name_encoding) {
    LOGD("  session_id=%d type=%d requestor_id=[%s] client_name=[%s] requestor_id_encoding=%d client_name_encoding=%d",
        session_id, type, requestor_id, client_name, requestor_id_encoding, client_name_encoding);
    GpsNiNotification gnn;
    gnn.size = sizeof(gnn);
    gnn.notification_id = session_id;
    gnn.ni_type = GPS_NI_TYPE_UMTS_SUPL;
    switch (type) {
        case AGPS_NOTIFY_TYPE_NOTIFY_ONLY:
            gnn.notify_flags = GPS_NI_NEED_NOTIFY;
            break;
        case AGPS_NOTIFY_TYPE_NOTIFY_ALLOW_NO_ANSWER:
            gnn.notify_flags = GPS_NI_NEED_NOTIFY|GPS_NI_NEED_VERIFY;
            break;
        case AGPS_NOTIFY_TYPE_NOTIFY_DENY_NO_ANSWER:
            gnn.notify_flags = GPS_NI_NEED_NOTIFY|GPS_NI_NEED_VERIFY;
            break;
        case AGPS_NOTIFY_TYPE_PRIVACY:
            gnn.notify_flags = GPS_NI_PRIVACY_OVERRIDE;
            break;
        default:
            gnn.notify_flags = 0;
    }
    gnn.timeout          = 8;
    gnn.default_response = GPS_NI_RESPONSE_NORESP;
    strcpy(gnn.requestor_id, requestor_id);
    strcpy(gnn.text,         client_name);
    gnn.requestor_id_encoding = requestor_id_encoding;
    gnn.text_encoding         = client_name_encoding;
    gnn.extras[0] = 0;  // be an empty string ""
    g_gpshal_ctx.gpsni_cbs->notify_cb(&gnn);
}
static void request_set_id(request_setid flags) {
    LOGD("  flags=0x%x", flags);
    g_gpshal_ctx.agpsril_cbs->request_setid(flags);
}
static void request_ref_loc(request_refloc flags) {
    LOGD("  flags=0x%x", flags);
    g_gpshal_ctx.agpsril_cbs->request_refloc(flags);
}

static mnl2hal_interface gpshal_mnl2hal_interface = {
    update_mnld_reboot,
    update_location,
    update_gps_status,
    update_gps_sv,
    update_nmea,
    update_gps_capabilities,
    update_gps_measurements,
    update_gps_navigation,
    update_gnss_measurements,
    update_gnss_navigation,
    request_wakelock,
    release_wakelock,
    request_utc_time,
    request_data_conn,
    release_data_conn,
    request_ni_notify,
    request_set_id,
    request_ref_loc,
};

//=========================================================

void gpshal_worker_thread(__unused void *arg) {
    struct epoll_event events[MAX_EPOLL_EVENT];
    UNUSED(arg);

    while (true) {
        int i;
        int n;

        n = epoll_wait(g_gpshal_ctx.fd_worker_epoll, events, MAX_EPOLL_EVENT , -1);
        if (n == -1) {
            if (errno == EINTR) {
                continue;
            } else {
                LOGE("%s() epoll_wait failure reason=[%s]%d",
                        __func__, strerror(errno), errno);
                return;
            }
        }

        for (i = 0; i < n; i++) {
            if (events[i].data.fd == g_gpshal_ctx.fd_mnl2hal) {
                if (events[i].events & EPOLLIN) {
                    mnl2hal_hdlr(g_gpshal_ctx.fd_mnl2hal,
                            &gpshal_mnl2hal_interface);
                }
            } else {
                LOGE("%s() unknown fd=%d",
                        __func__, events[i].data.fd);
            }
        }
    }  // of while (true)
}
