#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <sys/time.h>
#include <time.h>

#include <stddef.h>  // offsetof
#include <stdarg.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/socket.h>
#include <string.h>
#include <fcntl.h>

#include <arpa/inet.h>  // inet_addr
#include <sys/un.h>  // struct sockaddr_un

#if defined(__TIZEN_OS__)
#include <dlog/dlog.h>
#endif

#include "mnl2agps_interface.h"
#include "data_coder.h"

#ifndef UNUSED
#define UNUSED(x) (x)=(x)
#endif

static void tag_log(int type, const char* tag, const char *fmt, ...) {
    char out_buf[1100] = {0};
    char buf[1024] = {0};
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    sprintf(out_buf, "%s %s", tag, buf);

#if defined(__ANDROID_OS__)
    if (type == 0) {
        __android_log_print(ANDROID_LOG_DEBUG, "agps", "%s", out_buf);
    } else {
        __android_log_print(ANDROID_LOG_ERROR, "agps", "%s", out_buf);
    }
#elif defined(__LINUX_OS__)
    UNUSED(type);
    printf("%s\n", out_buf);
#elif defined(__TIZEN_OS__)
    UNUSED(type);
    dlog_print(DLOG_DEBUG, "%s\n", out_buf);
    printf("%s\n", out_buf);
#else
    UNUSED(type);
#endif
}

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
#define LOGD(...) tag_log(1, "[MNL2AGPS]", __VA_ARGS__);
#define LOGW(...) tag_log(1, " WARNING: [MNL2AGPS]", __VA_ARGS__);
#define LOGE(...) tag_log(1, " ERR: [MNL2AGPS]", __VA_ARGS__);
#else
#define LOG_TAG "MNL2AGPS"
#include <cutils/sockets.h>
#include <cutils/log.h>     /*logging in logcat*/
#define LOGD(fmt, arg ...) ALOGD("%s: " fmt, __FUNCTION__ , ##arg)
#define LOGW(fmt, arg ...) ALOGW("%s: " fmt, __FUNCTION__ , ##arg)
#define LOGE(fmt, arg ...) ALOGE("%s: " fmt, __FUNCTION__ , ##arg)
#endif
// -1 means failure
static int safe_recvfrom(int sockfd, char* buf, int len) {
    int ret = 0;
    int retry = 10;

    while ((ret = recvfrom(sockfd, buf, len, 0,
         NULL, NULL)) == -1) {
        LOGW("ret=%d len=%d\n", ret, len);
        if (errno == EINTR) continue;
        if (errno == EAGAIN) {
            if (retry-- > 0) {
                usleep(100 * 1000);
                continue;
            }
        }
        LOGE("sendto reason=[%s]\n", strerror(errno));
        break;
    }
    return ret;
}

// -1 means failure
static int set_socket_blocking(int fd, int blocking) {
    if (fd < 0) {
        LOGE("set_socket_blocking  invalid fd=%d\n", fd);
        return -1;
    }

    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) {
        LOGE("set_socket_blocking  invalid flags=%d\n", flags);
        return -1;
    }

    flags = blocking ? (flags&~O_NONBLOCK) : (flags|O_NONBLOCK);
    return (fcntl(fd, F_SETFL, flags) == 0) ? 0 : -1;
}

// -1 means failure
static int safe_sendto(int sockfd, const char* dest, const char* buf, int size) {
    int len = 0;
    struct sockaddr_un soc_addr;
    socklen_t addr_len;
    int retry = 10;

    strcpy(soc_addr.sun_path, dest);
    soc_addr.sun_family = AF_UNIX;
    addr_len = (offsetof(struct sockaddr_un, sun_path) + strlen(soc_addr.sun_path) + 1);

    while ((len = sendto(sockfd, buf, size, 0,
        (const struct sockaddr *)&soc_addr, (socklen_t)addr_len)) == -1) {
        if (errno == EINTR) continue;
        if (errno == EAGAIN) {
            if (retry-- > 0) {
                usleep(100 * 1000);
                continue;
            }
        }
        LOGE("sendto dest=[%s] len=%d reason=[%s]\n",
            dest, size, strerror(errno));
        break;
    }
    return len;
}

// -1 means failure
static int send2agps(const char* buff, int len) {
    int ret = 0;
    int sockfd = socket(PF_LOCAL, SOCK_DGRAM, 0);
    if (safe_sendto(sockfd, MNL_TO_AGPS, buff, len) < 0) {
        LOGE("external_snd_cmd safe_sendto failed\n");
        ret = -1;
    }
    close(sockfd);
    return ret;
}

static int bind_udp_socket(char* path) {
    int fd;
    struct sockaddr_un addr;
    socklen_t addr_len;

    fd = socket(PF_LOCAL, SOCK_DGRAM, 0);
    if (fd < 0) {
        LOGE("socket_bind_udp() socket() failed reason=[%s]%d",
            strerror(errno), errno);
        return -1;
    }
    strcpy(addr.sun_path, path);
    addr.sun_family = AF_UNIX;
    addr_len = (offsetof(struct sockaddr_un, sun_path) + strlen(addr.sun_path) + 1);
    unlink(addr.sun_path);
    if (bind(fd, (struct sockaddr *)&addr, addr_len) < 0) {
        LOGE("bind failed path=[%s] reason=[%s]\n", path, strerror(errno));
        close(fd);
        return -1;
    }
    if (chmod(path, 0660) < 0) {
        LOGE("chmod err = [%s]\n", strerror(errno));
    }

    return fd;
}

int mnl2agps_mnl_reboot() {
    LOGD("mnl2agps_mnl_reboot");
    char buff[MNL_AGPS_MAX_BUFF_SIZE] = {0};
    int offset = 0;

    put_int(buff, &offset, MNL_AGPS_INTERFACE_VERSION);
    put_int(buff, &offset, MNL_AGPS_TYPE_MNL_REBOOT);

    return send2agps(buff, offset);
}

int mnl2agps_open_gps_done() {
    LOGD("mnl2agps_open_gps_done");
    char buff[MNL_AGPS_MAX_BUFF_SIZE] = {0};
    int offset = 0;

    put_int(buff, &offset, MNL_AGPS_INTERFACE_VERSION);
    put_int(buff, &offset, MNL_AGPS_TYPE_AGPS_OPEN_GPS_DONE);

    return send2agps(buff, offset);
}
int mnl2agps_close_gps_done() {
    LOGD("mnl2agps_close_gps_done");
    char buff[MNL_AGPS_MAX_BUFF_SIZE] = {0};
    int offset = 0;

    put_int(buff, &offset, MNL_AGPS_INTERFACE_VERSION);
    put_int(buff, &offset, MNL_AGPS_TYPE_AGPS_CLOSE_GPS_DONE);

    return send2agps(buff, offset);
}
int mnl2agps_reset_gps_done() {
    LOGD("mnl2agps_reset_gps_done");
    char buff[MNL_AGPS_MAX_BUFF_SIZE] = {0};
    int offset = 0;

    put_int(buff, &offset, MNL_AGPS_INTERFACE_VERSION);
    put_int(buff, &offset, MNL_AGPS_TYPE_AGPS_RESET_GPS_DONE);

    return send2agps(buff, offset);
}

int mnl2agps_gps_init() {
    LOGD("mnl2agps_gps_init");
    char buff[MNL_AGPS_MAX_BUFF_SIZE] = {0};
    int offset = 0;

    put_int(buff, &offset, MNL_AGPS_INTERFACE_VERSION);
    put_int(buff, &offset, MNL_AGPS_TYPE_GPS_INIT);

    return send2agps(buff, offset);
}
int mnl2agps_gps_cleanup() {
    LOGD("mnl2agps_gps_cleanup");
    char buff[MNL_AGPS_MAX_BUFF_SIZE] = {0};
    int offset = 0;

    put_int(buff, &offset, MNL_AGPS_INTERFACE_VERSION);
    put_int(buff, &offset, MNL_AGPS_TYPE_GPS_CLEANUP);

    return send2agps(buff, offset);
}
// type:AGpsType
int mnl2agps_set_server(int type, const char* hostname, int port) {
    LOGD("mnl2agps_set_server, hostname = %s, port = %d\n", hostname, port);
    char buff[MNL_AGPS_MAX_BUFF_SIZE] = {0};
    int offset = 0;

    put_int(buff, &offset, MNL_AGPS_INTERFACE_VERSION);
    put_int(buff, &offset, MNL_AGPS_TYPE_SET_SERVER);
    put_int(buff, &offset, type);
    put_string(buff, &offset, hostname);
    put_int(buff, &offset, port);

    return send2agps(buff, offset);
}
// flags:GpsAidingData
int mnl2agps_delete_aiding_data(int flags) {
    LOGD("mnl2agps_delete_aiding_data");
    char buff[MNL_AGPS_MAX_BUFF_SIZE] = {0};
    int offset = 0;

    put_int(buff, &offset, MNL_AGPS_INTERFACE_VERSION);
    put_int(buff, &offset, MNL_AGPS_TYPE_DELETE_AIDING_DATA);
    put_int(buff, &offset, flags);

    return send2agps(buff, offset);
}
int mnl2agps_gps_open(int assist_req) {
    LOGD("mnl2agps_gps_open");
    char buff[MNL_AGPS_MAX_BUFF_SIZE] = {0};
    int offset = 0;

    put_int(buff, &offset, MNL_AGPS_INTERFACE_VERSION);
    put_int(buff, &offset, MNL_AGPS_TYPE_GPS_OPEN);
    put_int(buff, &offset, assist_req);

    return send2agps(buff, offset);
}
int mnl2agps_gps_close() {
    LOGD("mnl2agps_gps_close");
    char buff[MNL_AGPS_MAX_BUFF_SIZE] = {0};
    int offset = 0;

    put_int(buff, &offset, MNL_AGPS_INTERFACE_VERSION);
    put_int(buff, &offset, MNL_AGPS_TYPE_GPS_CLOSE);

    return send2agps(buff, offset);
}
int mnl2agps_data_conn_open(const char* apn) {
    LOGD("mnl2agps_data_conn_open, apn = %s\n", apn);
    char buff[MNL_AGPS_MAX_BUFF_SIZE] = {0};
    int offset = 0;

    put_int(buff, &offset, MNL_AGPS_INTERFACE_VERSION);
    put_int(buff, &offset, MNL_AGPS_TYPE_DATA_CONN_OPEN);
    put_string(buff, &offset, apn);

    return send2agps(buff, offset);
}

int mnl2agps_data_conn_open_ip_type(const char* apn, int ip_type) {
    LOGD("mnl2agps_data_conn_open  apn=%s ip_type=%d\n", apn, ip_type);

    char buff[MNL_AGPS_MAX_BUFF_SIZE] = {0};
    int offset = 0;

    put_int(buff, &offset, MNL_AGPS_INTERFACE_VERSION);
    put_int(buff, &offset, MNL_AGPS_TYPE_DATA_CONN_OPEN_IP_TYPE);
    put_string(buff, &offset, apn);
    put_int(buff, &offset, ip_type);

    return send2agps(buff, offset);
}

int mnl2agps_install_certificates(int index, int total, const char* data, int len) {
    LOGD("mnl2agps_install_certificates  (%d/%d) len=%d\n", index, total, len);

    char buff[MNL_AGPS_MAX_BUFF_SIZE] = {0};
    int offset = 0;

    put_int(buff, &offset, MNL_AGPS_INTERFACE_VERSION);
    put_int(buff, &offset, MNL_AGPS_TYPE_INSTALL_CERTIFICATES);
    put_int(buff, &offset, index);
    put_int(buff, &offset, total);
    put_binary(buff, &offset, data, len);

    return send2agps(buff, offset);
}

int mnl2agps_revoke_certificates(const char* data, int len) {
    LOGD("mnl2agps_revoke_certificates  len=%d item=%d\n", len, len/20);

    char buff[MNL_AGPS_MAX_BUFF_SIZE] = {0};
    int offset = 0;

    put_int(buff, &offset, MNL_AGPS_INTERFACE_VERSION);
    put_int(buff, &offset, MNL_AGPS_TYPE_REVOKE_CERTIFICATES);
    put_binary(buff, &offset, data, len);

    return send2agps(buff, offset);
}

int mnl2agps_data_conn_failed() {
    LOGD("mnl2agps_data_conn_failed");
    char buff[MNL_AGPS_MAX_BUFF_SIZE] = {0};
    int offset = 0;

    put_int(buff, &offset, MNL_AGPS_INTERFACE_VERSION);
    put_int(buff, &offset, MNL_AGPS_TYPE_DATA_CONN_FAILED);

    return send2agps(buff, offset);
}
int mnl2agps_data_conn_closed() {
    LOGD("mnl2agps_data_conn_closed");
    char buff[MNL_AGPS_MAX_BUFF_SIZE] = {0};
    int offset = 0;

    put_int(buff, &offset, MNL_AGPS_INTERFACE_VERSION);
    put_int(buff, &offset, MNL_AGPS_TYPE_DATA_CONN_CLOSED);

    return send2agps(buff, offset);
}
int mnl2agps_ni_message(const char* msg, int len) {
    LOGD("mnl2agps_ni_message");
    char buff[MNL_AGPS_MAX_BUFF_SIZE] = {0};
    int offset = 0;

    put_int(buff, &offset, MNL_AGPS_INTERFACE_VERSION);
    put_int(buff, &offset, MNL_AGPS_TYPE_NI_MESSAGE);
    put_binary(buff, &offset, msg, len);

    return send2agps(buff, offset);
}

/*
ACCEPT = 1
DENY = 2
NO_RSP = 3
*/
int mnl2agps_ni_respond(int session_id, int user_response) {
    char buff[MNL_AGPS_MAX_BUFF_SIZE] = {0};
    int offset = 0;

    LOGD("session_id: %d, user_response: %d\n", session_id, user_response);
    put_int(buff, &offset, MNL_AGPS_INTERFACE_VERSION);
    put_int(buff, &offset, MNL_AGPS_TYPE_NI_RESPOND);
    put_int(buff, &offset, session_id);
    put_int(buff, &offset, user_response);

    return send2agps(buff, offset);
}
int mnl2agps_set_ref_loc(int type, int mcc, int mnc, int lac, int cid) {
    char buff[MNL_AGPS_MAX_BUFF_SIZE] = {0};
    int offset = 0;
    LOGD("type=%d, mcc=%d, mnc=%d, lac=%d, cid=%d\n", type, mcc, mnc, lac, cid);

    put_int(buff, &offset, MNL_AGPS_INTERFACE_VERSION);
    put_int(buff, &offset, MNL_AGPS_TYPE_SET_REF_LOC);
    put_int(buff, &offset, type);
    put_int(buff, &offset, mcc);
    put_int(buff, &offset, mnc);
    put_int(buff, &offset, lac);
    put_int(buff, &offset, cid);

    return send2agps(buff, offset);
}
int mnl2agps_set_set_id(int type, const char* setid) {
    char buff[MNL_AGPS_MAX_BUFF_SIZE] = {0};
    int offset = 0;

    LOGD("type=%d, setid = %s\n", type, setid);
    put_int(buff, &offset, MNL_AGPS_INTERFACE_VERSION);
    put_int(buff, &offset, MNL_AGPS_TYPE_SET_SET_ID);
    put_int(buff, &offset, type);
    put_string(buff, &offset, setid);

    return send2agps(buff, offset);
}
int mnl2agps_update_network_state(int connected, int type, int roaming, const char* extra_info) {
    char buff[MNL_AGPS_MAX_BUFF_SIZE] = {0};
    int offset = 0;

    LOGD("connected=%d, type = %d, roaming=%d, extra_info=%s\n", connected, type, roaming, extra_info);
    put_int(buff, &offset, MNL_AGPS_INTERFACE_VERSION);
    put_int(buff, &offset, MNL_AGPS_TYPE_UPDATE_NETWORK_STATE);
    put_int(buff, &offset, connected);
    put_int(buff, &offset, type);
    put_int(buff, &offset, roaming);
    put_string(buff, &offset, extra_info);

    return send2agps(buff, offset);
}
int mnl2agps_update_network_availability(int avaiable, const char* apn) {
    char buff[MNL_AGPS_MAX_BUFF_SIZE] = {0};
    int offset = 0;

    LOGD("avaiable=%d, apn=%s\n", avaiable, apn);
    put_int(buff, &offset, MNL_AGPS_INTERFACE_VERSION);
    put_int(buff, &offset, MNL_AGPS_TYPE_UPDATE_NETWORK_AVAILABILITY);
    put_int(buff, &offset, avaiable);
    put_string(buff, &offset, apn);

    return send2agps(buff, offset);
}

int mnl2agps_pmtk(const char* pmtk) {
    char buff[MNL_AGPS_MAX_BUFF_SIZE] = {0};
    int offset = 0;

    put_int(buff, &offset, MNL_AGPS_INTERFACE_VERSION);
    put_int(buff, &offset, MNL_AGPS_TYPE_MNL2AGPS_PMTK);
    put_string(buff, &offset, pmtk);

    return send2agps(buff, offset);
}
int mnl2agps_raw_dbg(int enabled) {
    char buff[MNL_AGPS_MAX_BUFF_SIZE] = {0};
    int offset = 0;

    put_int(buff, &offset, MNL_AGPS_INTERFACE_VERSION);
    put_int(buff, &offset, MNL_AGPS_TYPE_RAW_DBG);
    put_int(buff, &offset, enabled);

    return send2agps(buff, offset);
}

int mnl2agps_reaiding_req() {
    char buff[MNL_AGPS_MAX_BUFF_SIZE] = {0};
    int offset = 0;

    put_int(buff, &offset, MNL_AGPS_INTERFACE_VERSION);
    put_int(buff, &offset, MNL_AGPS_TYPE_REAIDING);

    return send2agps(buff, offset);
}

int mnl2agps_location_sync(double lat, double lng, int acc) {
    char buff[MNL_AGPS_MAX_BUFF_SIZE] = {0};
    int offset = 0;

    put_int(buff, &offset, MNL_AGPS_INTERFACE_VERSION);
    put_int(buff, &offset, MNL_AGPS_TYPE_LOCATION_SYNC);
    put_double(buff, &offset, lat);
    put_double(buff, &offset, lng);
    put_int(buff, &offset, acc);

    return send2agps(buff, offset);
}
int mnl2agps_agps_settings_ack(mnl_agps_gnss_settings* settings) {
    char buff[MNL_AGPS_MAX_BUFF_SIZE] = {0};
    int offset = 0;

    put_int(buff, &offset, MNL_AGPS_INTERFACE_VERSION);
    put_int(buff, &offset, MNL_AGPS_TYPE_SETTINGS_ACK);
    put_int(buff, &offset, settings->gps_satellite_support);
    put_int(buff, &offset, settings->glonass_satellite_support);
    put_int(buff, &offset, settings->beidou_satellite_support);
    put_int(buff, &offset, settings->galileo_satellite_support);

    return send2agps(buff, offset);
}
void agps2mnl_hdlr(int fd, agps2mnl_interface* mnl_interface) {
    char buff[MNL_AGPS_MAX_BUFF_SIZE] = {0};
    int offset = 0;
    int ret;

    ret = safe_recvfrom(fd, buff, sizeof(buff));
    if (ret <= 0) {
        LOGE("mnl2agps_handler() safe_recvfrom() failed ret=%d", ret);
        return;
    }
    int version = get_int(buff, &offset);

    if (version != MNL_AGPS_INTERFACE_VERSION) {
        LOGE("agps_ver=%d mnl_ver=%d\n",
            version, MNL_AGPS_INTERFACE_VERSION);
    }

    mnl_agps_type type = get_int(buff, &offset);
    // LOGD("agps2mnl [%s]\n", get_mnl_agps_type_str(type));

    switch (type) {
    case MNL_AGPS_TYPE_AGPS_REBOOT: {
        if (mnl_interface->agps_reboot) {
            mnl_interface->agps_reboot();
        } else {
            LOGE("agps_reboot is NULL\n");
        }
        break;
    }
    case MNL_AGPS_TYPE_AGPS_OPEN_GPS_REQ: {
        int show_gps_icon = get_int(buff, &offset);
        if (mnl_interface->agps_open_gps_req) {
            mnl_interface->agps_open_gps_req(show_gps_icon);
        } else {
            LOGE("agps_open_gps_req is NULL\n");
        }
        break;
    }
    case MNL_AGPS_TYPE_AGPS_CLOSE_GPS_REQ: {
        if (mnl_interface->agps_close_gps_req) {
            mnl_interface->agps_close_gps_req();
        } else {
            LOGE("agps_close_gps_req is NULL\n");
        }
        break;
    }
    case MNL_AGPS_TYPE_AGPS_RESET_GPS_REQ: {
        int flags = get_int(buff, &offset);
        if (mnl_interface->agps_reset_gps_req) {
            mnl_interface->agps_reset_gps_req(flags);
        } else {
            LOGE("agps_reset_gps_req is NULL\n");
        }
        break;
    }
    case MNL_AGPS_TYPE_AGPS_SESSION_DONE: {
        if (mnl_interface->agps_session_done) {
            mnl_interface->agps_session_done();
        } else {
            LOGE("agps_session_done is NULL\n");
        }
        break;
    }
    case MNL_AGPS_TYPE_NI_NOTIFY: {
        int session_id = get_int(buff, &offset);
        mnl_agps_notify_type type = get_int(buff, &offset);
        const char* requestor_id = get_string(buff, &offset);
        const char* client_name = get_string(buff, &offset);
        if (mnl_interface->ni_notify) {
            mnl_interface->ni_notify(session_id, type, requestor_id, client_name);
        } else {
            LOGE("ni_notify is NULL\n");
        }
        break;
    }
    case MNL_AGPS_TYPE_NI_NOTIFY_2: {
        int session_id = get_int(buff, &offset);
        mnl_agps_notify_type type = get_int(buff, &offset);
        const char* requestor_id = get_string(buff, &offset);
        const char* client_name = get_string(buff, &offset);
        int requestor_id_encoding = get_int(buff, &offset);
        int client_name_encoding = get_int(buff, &offset);
        if (mnl_interface->ni_notify2) {
            mnl_interface->ni_notify2(session_id, type, requestor_id,
               client_name, requestor_id_encoding, client_name_encoding);
        } else {
            LOGE("ni_notify2 is NULL\n");
        }
        break;
    }
    case MNL_AGPS_TYPE_DATA_CONN_REQ: {
        int ipaddr = get_int(buff, &offset);
        int is_emergency = get_int(buff, &offset);
        if (mnl_interface->data_conn_req) {
            mnl_interface->data_conn_req(ipaddr, is_emergency);
        } else {
            LOGE("data_conn_req is NULL\n");
        }
        break;
    }
    case MNL_AGPS_TYPE_DATA_CONN_REQ2: {
        struct sockaddr_storage addr;
        get_binary(buff, &offset, (char*)&addr);
        int is_emergency = get_int(buff, &offset);
        if (mnl_interface->data_conn_req2) {
            mnl_interface->data_conn_req2(&addr, is_emergency);
        } else {
            LOGE("data_conn_req2 is NULL\n");
        }
        break;
    }
    case MNL_AGPS_TYPE_DATA_CONN_RELEASE: {
        if (mnl_interface->data_conn_release) {
            mnl_interface->data_conn_release();
        } else {
            LOGE("data_conn_release is NULL\n");
        }
        break;
    }
    case MNL_AGPS_TYPE_SET_ID_REQ: {
        int flags = get_int(buff, &offset);
        if (mnl_interface->set_id_req) {
            mnl_interface->set_id_req(flags);
        } else {
            LOGE("set_id_req is NULL\n");
        }
        break;
    }
    case MNL_AGPS_TYPE_REF_LOC_REQ: {
        int flags = get_int(buff, &offset);
        if (mnl_interface->ref_loc_req) {
            mnl_interface->ref_loc_req(flags);
        } else {
            LOGE("ref_loc_req is NULL\n");
        }
        break;
    }
    case MNL_AGPS_TYPE_AGPS2MNL_PMTK: {
        const char* pmtk = get_string(buff, &offset);
        if (mnl_interface->rcv_pmtk) {
            mnl_interface->rcv_pmtk(pmtk);
        } else {
            LOGE("rcv_pmtk is NULL\n");
        }
        break;
    }
    case MNL_AGPS_TYPE_GPEVT: {
        gpevt_type type = get_int(buff, &offset);
        if (mnl_interface->gpevt) {
            mnl_interface->gpevt(type);
        } else {
            LOGE("gpevt is NULL\n");
        }
        break;
    }
    case MNL_AGPS_TYPE_AGPS_LOC: {
        mnl_agps_agps_location agps_location;
        get_binary(buff, &offset, (char*)&agps_location);
        if (mnl_interface->agps_location) {
            mnl_interface->agps_location(&agps_location);
        } else {
            LOGE("agps_location is NULL\n");
        }
        break;
    }
    case MNL_AGPS_TYPE_SETTINGS_SYNC: {
        mnl_agps_agps_settings agps_settings;
        agps_settings.sib8_16_enable = get_int(buff, &offset);
        agps_settings.gps_satellite_enable = get_int(buff, &offset);
        agps_settings.glonass_satellite_enable = get_int(buff, &offset);
        agps_settings.beidou_satellite_enable = get_int(buff, &offset);
        agps_settings.galileo_satellite_enable = get_int(buff, &offset);
        agps_settings.a_glonass_satellite_enable = get_int(buff, &offset);
        if (mnl_interface->agps_settings_sync) {
            mnl_interface->agps_settings_sync(&agps_settings);
        } else {
            LOGE("agps_settings_sync is NULL\n");
        }
        break;
    }
    default:
        LOGE("agps2mnl unknown type=%d\n", type);
        break;
    }
}

int create_agps2mnl_fd() {
    int fd = bind_udp_socket(AGPS_TO_MNL);
    set_socket_blocking(fd, 0);
    return fd;
}

