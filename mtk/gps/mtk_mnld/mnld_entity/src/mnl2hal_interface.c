#include "mnl2hal_interface.h"
#include "mtk_lbs_utility.h"
#include "data_coder.h"
#include "mpe.h"

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
#define LOGD(...) tag_log(1, "[mnl2hal]", __VA_ARGS__);
#define LOGW(...) tag_log(1, "[mnl2hal] WARNING: ", __VA_ARGS__);
#define LOGE(...) tag_log(1, "[mnl2hal] ERR: ", __VA_ARGS__);
#else
#define LOG_TAG "mnl2hal"
#include <cutils/sockets.h>
#include <cutils/log.h>     /*logging in logcat*/
#define LOGD(fmt, arg ...) ALOGD("%s: " fmt, __FUNCTION__ , ##arg)
#define LOGW(fmt, arg ...) ALOGW("%s: " fmt, __FUNCTION__ , ##arg)
#define LOGE(fmt, arg ...) ALOGE("%s: " fmt, __FUNCTION__ , ##arg)
#endif


int mnl2hal_mnld_reboot() {
    LOGD("mnl2hal_mnld_reboot");
    char buff[HAL_MNL_BUFF_SIZE] = {0};
    int offset = 0;
    put_int(buff, &offset, HAL_MNL_INTERFACE_VERSION);

    put_int(buff, &offset, MNL2HAL_MNLD_REBOOT);

    return safe_sendto(MTK_MNL2HAL, buff, offset);
}

int mnl2hal_location(gps_location location) {
    LOGD("mnl2hal_location");
    char buff[HAL_MNL_BUFF_SIZE] = {0};
    int offset = 0;
    put_int(buff, &offset, HAL_MNL_INTERFACE_VERSION);

    put_int(buff, &offset, MNL2HAL_LOCATION);
    put_binary(buff, &offset, (const char*)&location, sizeof(location));

    return safe_sendto(MTK_MNL2HAL, buff, offset);
}

int mnl2hal_gps_status(gps_status status) {
    LOGD("mnl2hal_gps_status  status=%d", status);
    char buff[HAL_MNL_BUFF_SIZE] = {0};
    int offset = 0;
    put_int(buff, &offset, HAL_MNL_INTERFACE_VERSION);

    put_int(buff, &offset, MNL2HAL_GPS_STATUS);
    put_binary(buff, &offset, (const char*)&status, sizeof(status));

    return safe_sendto(MTK_MNL2HAL, buff, offset);
}

int mnl2hal_gps_sv(gnss_sv_info sv) {
    LOGD("mnl2hal_gps_sv  num=%d", sv.num_svs);
    char buff[HAL_MNL_BUFF_SIZE] = {0};
    int offset = 0;
    put_int(buff, &offset, HAL_MNL_INTERFACE_VERSION);

    put_int(buff, &offset, MNL2HAL_GPS_SV);
    put_binary(buff, &offset, (const char*)&sv, sizeof(sv));

    return safe_sendto(MTK_MNL2HAL, buff, offset);
}

int mnl2hal_nmea(int64_t timestamp, const char* nmea, int length) {
    LOGD("mnl2hal_nmea  timestamp=%llu nmea=[%s] length=%d",
        timestamp, nmea, length);
    char buff[HAL_MNL_BUFF_SIZE] = {0};
    int offset = 0;
    put_int(buff, &offset, HAL_MNL_INTERFACE_VERSION);

    put_int(buff, &offset, MNL2HAL_NMEA);
    put_long(buff, &offset, timestamp);
    put_string(buff, &offset, nmea);
    put_int(buff, &offset, length);
    return safe_sendto(MTK_MNL2HAL, buff, offset);
}

int mnl2hal_gps_capabilities(gps_capabilites capabilities) {
    LOGD("mnl2hal_gps_capabilities  capabilities=0x%x", capabilities);
    char buff[HAL_MNL_BUFF_SIZE] = {0};
    int offset = 0;
    put_int(buff, &offset, HAL_MNL_INTERFACE_VERSION);

    put_int(buff, &offset, MNL2HAL_GPS_CAPABILITIES);
    put_binary(buff, &offset, (const char*)&capabilities, sizeof(capabilities));

    return safe_sendto(MTK_MNL2HAL, buff, offset);
}

int mnl2hal_gps_measurements(gps_data data) {
    LOGD("mnl2hal_gps_measurements");
    char buff[HAL_MNL_BUFF_SIZE] = {0};
    int offset = 0;
    put_int(buff, &offset, HAL_MNL_INTERFACE_VERSION);

    put_int(buff, &offset, MNL2HAL_GPS_MEASUREMENTS);
    put_binary(buff, &offset, (const char*)&data, sizeof(data));

    return safe_sendto(MTK_MNL2HAL, buff, offset);
}

int mnl2hal_gps_navigation(gps_nav_msg msg) {
    LOGD("mnl2hal_gps_navigation");
    char buff[HAL_MNL_BUFF_SIZE] = {0};
    int offset = 0;
    put_int(buff, &offset, HAL_MNL_INTERFACE_VERSION);

    put_int(buff, &offset, MNL2HAL_GPS_NAVIGATION);
    put_binary(buff, &offset, (const char*)&msg, sizeof(msg));

    return safe_sendto(MTK_MNL2HAL, buff, offset);
}

int mnl2hal_gnss_measurements(gnss_data data) {
    LOGD("mnl2hal_gnss_measurements");
    char buff[HAL_MNL_BUFF_SIZE] = {0};
    int offset = 0;
    put_int(buff, &offset, HAL_MNL_INTERFACE_VERSION);

    put_int(buff, &offset, MNL2HAL_GNSS_MEASUREMENTS);
    put_binary(buff, &offset, (const char*)&data, sizeof(data));

    return safe_sendto(MTK_MNL2HAL, buff, offset);
}

int mnl2hal_gnss_navigation(gnss_nav_msg msg) {
    LOGD("mnl2hal_gnss_navigation");
    char buff[HAL_MNL_BUFF_SIZE] = {0};
    int offset = 0;
    put_int(buff, &offset, HAL_MNL_INTERFACE_VERSION);

    put_int(buff, &offset, MNL2HAL_GNSS_NAVIGATION);
    put_binary(buff, &offset, (const char*)&msg, sizeof(msg));

    return safe_sendto(MTK_MNL2HAL, buff, offset);
}


int mnl2hal_request_wakelock() {
    LOGD("mnl2hal_request_wakelock");
    char buff[HAL_MNL_BUFF_SIZE] = {0};
    int offset = 0;
    put_int(buff, &offset, HAL_MNL_INTERFACE_VERSION);

    put_int(buff, &offset, MNL2HAL_REQUEST_WAKELOCK);

    return safe_sendto(MTK_MNL2HAL, buff, offset);
}

int mnl2hal_release_wakelock() {
    LOGD("mnl2hal_release_wakelock");
    char buff[HAL_MNL_BUFF_SIZE] = {0};
    int offset = 0;
    put_int(buff, &offset, HAL_MNL_INTERFACE_VERSION);

    put_int(buff, &offset, MNL2HAL_RELEASE_WAKELOCK);

    return safe_sendto(MTK_MNL2HAL, buff, offset);
}

int mnl2hal_request_utc_time() {
    LOGD("mnl2hal_request_utc_time");
    char buff[HAL_MNL_BUFF_SIZE] = {0};
    int offset = 0;
    put_int(buff, &offset, HAL_MNL_INTERFACE_VERSION);

    put_int(buff, &offset, MNL2HAL_REQUEST_UTC_TIME);

    return safe_sendto(MTK_MNL2HAL, buff, offset);
}

int mnl2hal_request_data_conn(struct sockaddr_storage addr) {
    LOGD("mnl2hal_request_data_conn");
    char buff[HAL_MNL_BUFF_SIZE] = {0};
    int offset = 0;
    put_int(buff, &offset, HAL_MNL_INTERFACE_VERSION);

    put_int(buff, &offset, MNL2HAL_REQUEST_DATA_CONN);
    put_binary(buff, &offset, (const char*)&addr, sizeof(addr));

    return safe_sendto(MTK_MNL2HAL, buff, offset);
}

int mnl2hal_release_data_conn() {
    LOGD("mnl2hal_release_data_conn");
    char buff[HAL_MNL_BUFF_SIZE] = {0};
    int offset = 0;
    put_int(buff, &offset, HAL_MNL_INTERFACE_VERSION);

    put_int(buff, &offset, MNL2HAL_RELEASE_DATA_CONN);

    return safe_sendto(MTK_MNL2HAL, buff, offset);
}

int mnl2hal_request_ni_notify(int session_id, agps_notify_type type, const char* requestor_id,
    const char* client_name, ni_encoding_type requestor_id_encoding,
    ni_encoding_type client_name_encoding) {
    LOGD("mnl2hal_request_ni_notify");
    char buff[HAL_MNL_BUFF_SIZE] = {0};
    int offset = 0;
    put_int(buff, &offset, HAL_MNL_INTERFACE_VERSION);

    put_int(buff, &offset, MNL2HAL_REQUEST_NI_NOTIFY);
    put_int(buff, &offset, session_id);
    put_int(buff, &offset, type);
    put_string(buff, &offset, requestor_id);
    put_string(buff, &offset, client_name);
    put_int(buff, &offset, requestor_id_encoding);
    put_int(buff, &offset, client_name_encoding);

    return safe_sendto(MTK_MNL2HAL, buff, offset);
}

int mnl2hal_request_set_id(request_setid flags) {
    LOGD("mnl2hal_request_set_id  flag=0x%x", flags);
    char buff[HAL_MNL_BUFF_SIZE] = {0};
    int offset = 0;
    put_int(buff, &offset, HAL_MNL_INTERFACE_VERSION);

    put_int(buff, &offset, MNL2HAL_REQUEST_SET_ID);
    put_int(buff, &offset, flags);

    return safe_sendto(MTK_MNL2HAL, buff, offset);
}

int mnl2hal_request_ref_loc(request_refloc flags) {
    LOGD("mnl2hal_request_ref_loc  flag=0x%x", flags);
    char buff[HAL_MNL_BUFF_SIZE] = {0};
    int offset = 0;
    put_int(buff, &offset, HAL_MNL_INTERFACE_VERSION);

    put_int(buff, &offset, MNL2HAL_REQUEST_REF_LOC);
    put_int(buff, &offset, flags);

    return safe_sendto(MTK_MNL2HAL, buff, offset);
}

// -1 means failure
int hal2mnl_hdlr(int fd, hal2mnl_interface* hdlr) {
    char buff[HAL_MNL_BUFF_SIZE] = {0};
    int offset = 0;
    int ver;
    hal2mnl_cmd cmd;
    int read_len;
    int ret = 0;

    read_len = safe_recvfrom(fd, buff, sizeof(buff));
    if (read_len <= 0) {
        LOGE("hal2mnl_hdlr() safe_recvfrom() failed read_len=%d", read_len);
        return -1;
    }
    ver = get_int(buff, &offset);
    UNUSED(ver);
    cmd = get_int(buff, &offset);

    switch (cmd) {
    case HAL2MNL_HAL_REBOOT: {
        if (hdlr->hal_reboot) {
            hdlr->hal_reboot();
        } else {
            LOGE("hal2mnl_hdlr() hal_reboot is NULL");
            ret = -1;
        }
        break;
    }
    case HAL2MNL_GPS_INIT: {
        if (hdlr->gps_init) {
            hdlr->gps_init();
        } else {
            LOGE("hal2mnl_hdlr() gps_init is NULL");
            ret = -1;
        }
        break;
    }
    case HAL2MNL_GPS_START: {
        if (hdlr->gps_start) {
            hdlr->gps_start();
        } else {
            LOGE("hal2mnl_hdlr() gps_start is NULL");
            ret = -1;
        }
        break;
    }
    case HAL2MNL_GPS_STOP: {
        if (hdlr->gps_stop) {
            hdlr->gps_stop();
        } else {
            LOGE("hal2mnl_hdlr() gps_stop is NULL");
            ret = -1;
        }
        break;
    }
    case HAL2MNL_GPS_CLEANUP: {
        if (hdlr->gps_cleanup) {
            hdlr->gps_cleanup();
        } else {
            LOGE("hal2mnl_hdlr() gps_cleanup is NULL");
            ret = -1;
        }
        break;
    }
    case HAL2MNL_GPS_INJECT_TIME: {
        if (hdlr->gps_inject_time) {
            int64_t time = get_long(buff, &offset);
            int64_t time_reference = get_long(buff, &offset);
            int uncertainty = get_int(buff, &offset);
            hdlr->gps_inject_time(time, time_reference, uncertainty);
        } else {
            LOGE("hal2mnl_hdlr() gps_inject_time is NULL");
            ret = -1;
        }
        break;
    }
    case HAL2MNL_GPS_INJECT_LOCATION: {
        if (hdlr->gps_inject_location) {
            double lat = get_double(buff, &offset);
            double lng = get_double(buff, &offset);
            float acc = get_float(buff, &offset);
            hdlr->gps_inject_location(lat, lng, acc);
        } else {
            LOGE("hal2mnl_hdlr() gps_inject_location is NULL");
            ret = -1;
        }
        break;
    }
    case HAL2MNL_GPS_DELETE_AIDING_DATA: {
        if (hdlr->gps_delete_aiding_data) {
            int flags = get_int(buff, &offset);
            hdlr->gps_delete_aiding_data(flags);
        } else {
            LOGE("hal2mnl_hdlr() gps_delete_aiding_data is NULL");
            ret = -1;
        }
        break;
    }
    case HAL2MNL_GPS_SET_POSITION_MODE: {
        if (hdlr->gps_set_position_mode) {
            gps_pos_mode mode = get_int(buff, &offset);
            gps_pos_recurrence recurrence = get_int(buff, &offset);
            int min_interval = get_int(buff, &offset);
            int preferred_acc = get_int(buff, &offset);
            int preferred_time = get_int(buff, &offset);
            hdlr->gps_set_position_mode(mode, recurrence, min_interval,
                preferred_acc, preferred_time);
        } else {
            LOGE("hal2mnl_hdlr() gps_set_position_mode is NULL");
            ret = -1;
        }
        break;
    }
    case HAL2MNL_DATA_CONN_OPEN: {
        if (hdlr->data_conn_open) {
            char* apn = get_string(buff, &offset);
            hdlr->data_conn_open(apn);
        } else {
            LOGE("hal2mnl_hdlr() data_conn_open is NULL");
            ret = -1;
        }
        break;
    }
    case HAL2MNL_DATA_CONN_OPEN_WITH_APN_IP_TYPE: {
        if (hdlr->data_conn_open_with_apn_ip_type) {
            char* apn = get_string(buff, &offset);
            apn_ip_type ip_type = get_int(buff, &offset);
            hdlr->data_conn_open_with_apn_ip_type(apn, ip_type);
        } else {
            LOGE("hal2mnl_hdlr() data_conn_open_with_apn_ip_type is NULL");
            ret = -1;
        }
        break;
    }
    case HAL2MNL_DATA_CONN_CLOSED: {
        if (hdlr->data_conn_closed) {
            hdlr->data_conn_closed();
        } else {
            LOGE("hal2mnl_hdlr() data_conn_closed is NULL");
            ret = -1;
        }
        break;
    }
    case HAL2MNL_DATA_CONN_FAILED: {
        if (hdlr->data_conn_failed) {
            hdlr->data_conn_failed();
        } else {
            LOGE("hal2mnl_hdlr() data_conn_failed is NULL");
            ret = -1;
        }
        break;
    }
    case HAL2MNL_SET_SERVER: {
        if (hdlr->set_server) {
            agps_type type = get_int(buff, &offset);
            char* hostname = get_string(buff, &offset);
            int port = get_int(buff, &offset);
            hdlr->set_server(type, hostname, port);
        } else {
            LOGE("hal2mnl_hdlr() set_server is NULL");
            ret = -1;
        }
        break;
    }
    case HAL2MNL_SET_REF_LOCATION: {
        if (hdlr->set_ref_location) {
            cell_type type = get_int(buff, &offset);
            int mcc = get_int(buff, &offset);
            int mnc = get_int(buff, &offset);
            int lac = get_int(buff, &offset);
            int cid = get_int(buff, &offset);
            hdlr->set_ref_location(type, mcc, mnc, lac, cid);
        } else {
            LOGE("hal2mnl_hdlr() set_ref_location is NULL");
            ret = -1;
        }
        break;
    }
    case HAL2MNL_SET_ID: {
        if (hdlr->set_id) {
            agps_id_type type = get_int(buff, &offset);
            char* setid = get_string(buff, &offset);
            hdlr->set_id(type, setid);
        } else {
            LOGE("hal2mnl_hdlr() set_id is NULL");
            ret = -1;
        }
        break;
    }
    case HAL2MNL_NI_MESSAGE: {
        if (hdlr->ni_message) {
            char msg[1024] = {0};
            int len = get_binary(buff, &offset, msg);
            hdlr->ni_message(msg, len);
        } else {
            LOGE("hal2mnl_hdlr() ni_message is NULL");
            ret = -1;
        }
        break;
    }
    case HAL2MNL_NI_RESPOND: {
        if (hdlr->ni_respond) {
            int notif_id = get_int(buff, &offset);
            ni_user_response_type user_response  = get_int(buff, &offset);
            hdlr->ni_respond(notif_id, user_response);
        } else {
            LOGE("hal2mnl_hdlr() ni_respond is NULL");
            ret = -1;
        }
        break;
    }
    case HAL2MNL_UPDATE_NETWORK_STATE: {
        if (hdlr->update_network_state) {
            int connected = get_int(buff, &offset);
            network_type type = get_int(buff, &offset);
            int roaming  = get_int(buff, &offset);
            char* extra_info = get_string(buff, &offset);
            hdlr->update_network_state(connected, type, roaming, extra_info);
        } else {
            LOGE("hal2mnl_hdlr() update_network_state is NULL");
            ret = -1;
        }
        break;
    }
    case HAL2MNL_UPDATE_NETWORK_AVAILABILITY: {
        if (hdlr->update_network_availability) {
            int available = get_int(buff, &offset);
            char* apn = get_string(buff, &offset);
            hdlr->update_network_availability(available, apn);
        } else {
            LOGE("hal2mnl_hdlr() update_network_availability is NULL");
            ret = -1;
        }
        break;
    }
    case HAL2MNL_GPS_MEASUREMENT: {
        if (hdlr->set_gps_measurement) {
            bool enabled = get_int(buff, &offset);
            hdlr->set_gps_measurement(enabled);
        } else {
            LOGE("hal2mnl_hdlr() set_gps_measurement is NULL");
            ret = -1;
        }
        break;
    }
    case HAL2MNL_GPS_NAVIGATION: {
        if (hdlr->set_gps_navigation) {
            bool enabled = get_int(buff, &offset);
            hdlr->set_gps_navigation(enabled);
        } else {
            LOGE("hal2mnl_hdlr() set_gps_navigation is NULL");
            ret = -1;
        }
        break;
    }
    default: {
        LOGE("hal2mnl_hdlr() unknown cmd=%d", cmd);
        ret = -1;
        break;
    }
    }

    return ret;
}

// -1 means failure
int create_hal2mnl_fd() {
    int fd = socket_bind_udp(MTK_HAL2MNL);
    socket_set_blocking(fd, 0);
    return fd;
}

