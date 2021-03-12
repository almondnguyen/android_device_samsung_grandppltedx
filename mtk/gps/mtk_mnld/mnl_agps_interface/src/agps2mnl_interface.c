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

#include "agps2mnl_interface.h"
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
#define LOGD(...) tag_log(1, "[agps][n][AGPS] [MNL]", __VA_ARGS__);
#define LOGW(...) tag_log(1, "[agps] WARNING: [MNL]", __VA_ARGS__);
#define LOGE(...) tag_log(1, "[agps] ERR: [MNL]", __VA_ARGS__);
#else
#define LOG_TAG "agps"
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

static char g_agps2mnl_path[128] = AGPS_TO_MNL;

// -1 means failure
static int send2mnl(const char* buff, int len) {
    int ret = 0;
    int sockfd = socket(PF_LOCAL, SOCK_DGRAM, 0);
    if (safe_sendto(sockfd, g_agps2mnl_path, buff, len) < 0) {
        LOGE("external_snd_cmd safe_sendto failed\n");
        ret = -1;
    }
    close(sockfd);
    return ret;
}

static int bind_udp_socket(char* path) {
    int sockfd;
    struct sockaddr_un soc_addr;
    socklen_t addr_len;

    sockfd = socket(PF_LOCAL, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        LOGE("socket failed reason=[%s]\n", strerror(errno));
        return -1;
    }

    strcpy(soc_addr.sun_path, path);
    soc_addr.sun_family = AF_UNIX;
    addr_len = (offsetof(struct sockaddr_un, sun_path) + strlen(soc_addr.sun_path) + 1);

    unlink(soc_addr.sun_path);
    if (bind(sockfd, (struct sockaddr *)&soc_addr, addr_len) < 0) {
        LOGE("bind failed path=[%s] reason=[%s]\n", path, strerror(errno));
        close(sockfd);
        return -1;
    }

    if (chmod(path, 0660) < 0) {
        LOGE("chmod err = [%s]\n", strerror(errno));
    }
    return sockfd;
}

int agps2mnl_agps_reboot() {
    char buff[MNL_AGPS_MAX_BUFF_SIZE] = {0};
    int offset = 0;

    LOGD("write  agps_reboot\n");

    put_int(buff, &offset, MNL_AGPS_INTERFACE_VERSION);
    put_int(buff, &offset, MNL_AGPS_TYPE_AGPS_REBOOT);

    return send2mnl(buff, offset);
}

int agps2mnl_agps_open_gps_req(int show_gps_icon) {
    char buff[MNL_AGPS_MAX_BUFF_SIZE] = {0};
    int offset = 0;

    LOGD("write  agps_open_gps_req  show_gps_icon=%d\n", show_gps_icon);

    put_int(buff, &offset, MNL_AGPS_INTERFACE_VERSION);
    put_int(buff, &offset, MNL_AGPS_TYPE_AGPS_OPEN_GPS_REQ);
    put_int(buff, &offset, show_gps_icon);

    return send2mnl(buff, offset);
}
int agps2mnl_agps_close_gps_req() {
    char buff[MNL_AGPS_MAX_BUFF_SIZE] = {0};
    int offset = 0;

    LOGD("write  agps_close_gps_req\n");

    put_int(buff, &offset, MNL_AGPS_INTERFACE_VERSION);
    put_int(buff, &offset, MNL_AGPS_TYPE_AGPS_CLOSE_GPS_REQ);

    return send2mnl(buff, offset);
}
int agps2mnl_agps_reset_gps_req(int flags) {
    char buff[MNL_AGPS_MAX_BUFF_SIZE] = {0};
    int offset = 0;

    LOGD("write  agps_reset_gps_req  flags=0x%x\n", flags);

    put_int(buff, &offset, MNL_AGPS_INTERFACE_VERSION);
    put_int(buff, &offset, MNL_AGPS_TYPE_AGPS_RESET_GPS_REQ);
    put_int(buff, &offset, flags);

    return send2mnl(buff, offset);
}

int agps2mnl_agps_session_done() {
    char buff[MNL_AGPS_MAX_BUFF_SIZE] = {0};
    int offset = 0;

    LOGD("write  agps_session_done\n");

    put_int(buff, &offset, MNL_AGPS_INTERFACE_VERSION);
    put_int(buff, &offset, MNL_AGPS_TYPE_AGPS_SESSION_DONE);

    return send2mnl(buff, offset);
}

int agps2mnl_ni_notify(int session_id, mnl_agps_notify_type type, const char* requestor_id, const char* client_name) {
    char buff[MNL_AGPS_MAX_BUFF_SIZE] = {0};
    int offset = 0;

    LOGD("write  ni_notify  session_id=%d type=%d requestor_id=[%s] client_name=[%s]\n",
                 session_id, type, requestor_id, client_name);
    put_int(buff, &offset, MNL_AGPS_INTERFACE_VERSION);
    put_int(buff, &offset, MNL_AGPS_TYPE_NI_NOTIFY);
    put_int(buff, &offset, session_id);
    put_int(buff, &offset, type);
    put_string(buff, &offset, requestor_id);
    put_string(buff, &offset, client_name);

    return send2mnl(buff, offset);
}

int agps2mnl_ni_notify2(int session_id, mnl_agps_notify_type type, const char* requestor_id, const char* client_name,
    mnl_agps_ni_encoding_type requestor_id_encoding, mnl_agps_ni_encoding_type client_name_encoding) {

    char buff[MNL_AGPS_MAX_BUFF_SIZE] = {0};
    int offset = 0;

    LOGD("write  ni_notify2  session_id=%d type=%d requestor_id=[%s] type=%d client_name=[%s] type=%d\n",
        session_id, type, requestor_id, requestor_id_encoding, client_name, client_name_encoding);

    put_int(buff, &offset, MNL_AGPS_INTERFACE_VERSION);
    put_int(buff, &offset, MNL_AGPS_TYPE_NI_NOTIFY_2);
    put_int(buff, &offset, session_id);
    put_int(buff, &offset, type);
    put_string(buff, &offset, requestor_id);
    put_string(buff, &offset, client_name);
    put_int(buff, &offset, requestor_id_encoding);
    put_int(buff, &offset, client_name_encoding);

    return send2mnl(buff, offset);
}

int agps2mnl_data_conn_req(int ipaddr, int is_emergency) {
    char buff[MNL_AGPS_MAX_BUFF_SIZE] = {0};
    int offset = 0;

    LOGD("write  data_conn_req  ipaddr=0x%x is_emergency=%d\n", ipaddr, is_emergency);

    put_int(buff, &offset, MNL_AGPS_INTERFACE_VERSION);
    put_int(buff, &offset, MNL_AGPS_TYPE_DATA_CONN_REQ);
    put_int(buff, &offset, ipaddr);
    put_int(buff, &offset, is_emergency);

    return send2mnl(buff, offset);
}

int agps2mnl_data_conn_req2(struct sockaddr_storage* addr, int is_emergency) {
    char buff[MNL_AGPS_MAX_BUFF_SIZE] = {0};
    int offset = 0;

    LOGD("write  data_conn_req2  is_emergency=%d\n", is_emergency);

    put_int(buff, &offset, MNL_AGPS_INTERFACE_VERSION);
    put_int(buff, &offset, MNL_AGPS_TYPE_DATA_CONN_REQ2);
    put_binary(buff, &offset, (const char*)addr, sizeof(*addr));
    put_int(buff, &offset, is_emergency);

    return send2mnl(buff, offset);
}

int agps2mnl_data_conn_release() {
    char buff[MNL_AGPS_MAX_BUFF_SIZE] = {0};
    int offset = 0;

    LOGD("write  data_conn_release\n");

    put_int(buff, &offset, MNL_AGPS_INTERFACE_VERSION);
    put_int(buff, &offset, MNL_AGPS_TYPE_DATA_CONN_RELEASE);

    return send2mnl(buff, offset);
}

int agps2mnl_set_id_req(int flags) {
    char buff[MNL_AGPS_MAX_BUFF_SIZE] = {0};
    int offset = 0;

    LOGD("write  set_id_req  flags=0x%x\n", flags);

    put_int(buff, &offset, MNL_AGPS_INTERFACE_VERSION);
    put_int(buff, &offset, MNL_AGPS_TYPE_SET_ID_REQ);
    put_int(buff, &offset, flags);

    return send2mnl(buff, offset);
}

int agps2mnl_ref_loc_req(int flags) {
    char buff[MNL_AGPS_MAX_BUFF_SIZE] = {0};
    int offset = 0;

    LOGD("write  ref_loc_req  flags=0x%x\n", flags);

    put_int(buff, &offset, MNL_AGPS_INTERFACE_VERSION);
    put_int(buff, &offset, MNL_AGPS_TYPE_REF_LOC_REQ);
    put_int(buff, &offset, flags);

    return send2mnl(buff, offset);
}

int agps2mnl_pmtk(const char* pmtk) {
    char buff[MNL_AGPS_MAX_BUFF_SIZE] = {0};
    int offset = 0;

    LOGD("write  pmtk  [%s]\n", pmtk);

    put_int(buff, &offset, MNL_AGPS_INTERFACE_VERSION);
    put_int(buff, &offset, MNL_AGPS_TYPE_AGPS2MNL_PMTK);
    put_string(buff, &offset, pmtk);

    return send2mnl(buff, offset);
}
int agps2mnl_gpevt(gpevt_type type) {
    char buff[MNL_AGPS_MAX_BUFF_SIZE] = {0};
    int offset = 0;

    LOGD("write  gpevt  type=%d\n", type);

    put_int(buff, &offset, MNL_AGPS_INTERFACE_VERSION);
    put_int(buff, &offset, MNL_AGPS_TYPE_GPEVT);
    put_int(buff, &offset, type);

    return send2mnl(buff, offset);
}

int agps2mnl_agps_location(mnl_agps_agps_location* location) {
    char buff[MNL_AGPS_MAX_BUFF_SIZE] = {0};
    int offset = 0;

    LOGD("write  agps_location  lat=%f lng=%f\n", location->latitude, location->longitude);

    put_int(buff, &offset, MNL_AGPS_INTERFACE_VERSION);
    put_int(buff, &offset, MNL_AGPS_TYPE_AGPS_LOC);
    put_binary(buff, &offset, (const char*)location, sizeof(mnl_agps_agps_location));

    return send2mnl(buff, offset);
}

void mnl2agps_hdlr(int fd, mnl2agps_interface* agps_interface) {
    char buff[MNL_AGPS_MAX_BUFF_SIZE] = {0};
    int offset = 0;
    int ret;

    ret = safe_recvfrom(fd, buff, sizeof(buff));
    if (ret <= 0) {
        LOGE("agps2mnl_handler() safe_recvfrom() failed ret=%d", ret);
        return;
    }

    int version = get_int(buff, &offset);

    if (version != MNL_AGPS_INTERFACE_VERSION) {
        LOGE("mnl_ver=%d agps_ver=%d\n",
            version, MNL_AGPS_INTERFACE_VERSION);
    }

    mnl_agps_type type = get_int(buff, &offset);

    switch (type) {
    case MNL_AGPS_TYPE_MNL_REBOOT: {
        if (agps_interface->mnl_reboot) {
           agps_interface->mnl_reboot();
        } else {
            LOGE("mnl_reboot is NULL\n");
        }
        break;
    }
    case MNL_AGPS_TYPE_AGPS_OPEN_GPS_DONE: {
        if (agps_interface->open_gps_done) {
           agps_interface->open_gps_done();
        } else {
            LOGE("open_gps_done is NULL\n");
        }
        break;
    }
    case MNL_AGPS_TYPE_AGPS_CLOSE_GPS_DONE: {
        if (agps_interface->close_gps_done) {
           agps_interface->close_gps_done();
        } else {
            LOGE("close_gps_done is NULL\n");
        }
        break;
    }
    case MNL_AGPS_TYPE_AGPS_RESET_GPS_DONE: {
        if (agps_interface->reset_gps_done) {
           agps_interface->reset_gps_done();
        } else {
            LOGE("reset_gps_done is NULL\n");
        }
        break;
    }
    case MNL_AGPS_TYPE_GPS_INIT: {
        if (agps_interface->gps_init) {
           agps_interface->gps_init();
        } else {
            LOGE("gps_init is NULL\n");
        }
        break;
    }
    case MNL_AGPS_TYPE_GPS_CLEANUP: {
        if (agps_interface->gps_cleanup) {
           agps_interface->gps_cleanup();
        } else {
            LOGE("gps_cleanup is NULL\n");
        }
        break;
    }
    case MNL_AGPS_TYPE_SET_SERVER: {
        int type = get_int(buff, &offset);
        const char* hostname = get_string(buff, &offset);
        int port = get_int(buff, &offset);
        if (agps_interface->set_server) {
           agps_interface->set_server(type, hostname, port);
        } else {
            LOGE("set_server is NULL\n");
        }
        break;
    }
    case MNL_AGPS_TYPE_DELETE_AIDING_DATA: {
        int flags = get_int(buff, &offset);
        if (agps_interface->delete_aiding_data) {
           agps_interface->delete_aiding_data(flags);
        } else {
            LOGE("delete_aiding_data is NULL\n");
        }
        break;
    }
    case MNL_AGPS_TYPE_GPS_OPEN: {
        int assist_req = get_int(buff, &offset);
        if (agps_interface->gps_open) {
           agps_interface->gps_open(assist_req);
        } else {
            LOGE("gps_open is NULL\n");
        }
        break;
    }
    case MNL_AGPS_TYPE_GPS_CLOSE: {
        if (agps_interface->gps_close) {
           agps_interface->gps_close();
        } else {
            LOGE("gps_close is NULL\n");
        }
        break;
    }
    case MNL_AGPS_TYPE_DATA_CONN_OPEN: {
        const char* apn = get_string(buff, &offset);
        if (agps_interface->data_conn_open) {
           agps_interface->data_conn_open(apn);
        } else {
            LOGE("data_conn_open is NULL\n");
        }
        break;
    }
    case MNL_AGPS_TYPE_DATA_CONN_OPEN_IP_TYPE: {
        const char* apn = get_string(buff, &offset);
        int ip_type = get_int(buff, &offset);
        if (agps_interface->data_conn_open_ip_type) {
           agps_interface->data_conn_open_ip_type(apn, ip_type);
        } else {
            LOGE("data_conn_open_ip_type is NULL\n");
        }
        break;
    }
    case MNL_AGPS_TYPE_INSTALL_CERTIFICATES: {
        int index = get_int(buff, &offset);
        int total = get_int(buff, &offset);
        char data[MNL_AGPS_MAX_BUFF_SIZE] = {0};
        int len = get_binary(buff, &offset, data);
        if (agps_interface->install_certificates) {
           agps_interface->install_certificates(index, total, data, len);
        } else {
            LOGE("install_certificates is NULL\n");
        }
        break;
    }
    case MNL_AGPS_TYPE_REVOKE_CERTIFICATES: {
        char data[MNL_AGPS_MAX_BUFF_SIZE] = {0};
        int len = get_binary(buff, &offset, data);
        if (agps_interface->revoke_certificates) {
           agps_interface->revoke_certificates(data, len);
        } else {
            LOGE("revoke_certificates is NULL\n");
        }
        break;
    }
    case MNL_AGPS_TYPE_DATA_CONN_FAILED: {
        if (agps_interface->data_conn_failed) {
           agps_interface->data_conn_failed();
        } else {
            LOGE("data_conn_failed is NULL\n");
        }
        break;
    }
    case MNL_AGPS_TYPE_DATA_CONN_CLOSED: {
        if (agps_interface->data_conn_closed) {
           agps_interface->data_conn_closed();
        } else {
            LOGE("data_conn_closed is NULL\n");
        }
        break;
    }
    case MNL_AGPS_TYPE_NI_MESSAGE: {
        char msg[1024] = {0};
        int len;
        len = get_binary(buff, &offset, msg);
        if (agps_interface->ni_message) {
           agps_interface->ni_message(msg, len);
        } else {
            LOGE("ni_message is NULL\n");
        }
        break;
    }
    case MNL_AGPS_TYPE_NI_RESPOND: {
        int session_id = get_int(buff, &offset);
        int user_response = get_int(buff, &offset);
        if (agps_interface->ni_respond) {
           agps_interface->ni_respond(session_id, user_response);
        } else {
            LOGE("ni_respond is NULL\n");
        }
        break;
    }
    case MNL_AGPS_TYPE_SET_REF_LOC: {
        int type = get_int(buff, &offset);
        int mcc = get_int(buff, &offset);
        int mnc = get_int(buff, &offset);
        int lac = get_int(buff, &offset);
        int cid = get_int(buff, &offset);
        if (agps_interface->set_ref_loc) {
           agps_interface->set_ref_loc(type, mcc, mnc, lac, cid);
        } else {
            LOGE("set_ref_loc is NULL\n");
        }
        break;
    }
    case MNL_AGPS_TYPE_SET_SET_ID: {
        int type = get_int(buff, &offset);
        const char* setid = get_string(buff, &offset);
        if (agps_interface->set_set_id) {
           agps_interface->set_set_id(type, setid);
        } else {
            LOGE("set_set_id is NULL\n");
        }
        break;
    }
    case MNL_AGPS_TYPE_UPDATE_NETWORK_STATE: {
        int connected = get_int(buff, &offset);
        int type = get_int(buff, &offset);
        int roaming = get_int(buff, &offset);
        const char* extra_info = get_string(buff, &offset);
        if (agps_interface->update_network_state) {
           agps_interface->update_network_state(connected, type, roaming, extra_info);
        } else {
            LOGE("update_network_state is NULL\n");
        }
        break;
    }
    case MNL_AGPS_TYPE_UPDATE_NETWORK_AVAILABILITY: {
        int avaiable = get_int(buff, &offset);
        const char* apn = get_string(buff, &offset);
        if (agps_interface->update_network_availability) {
           agps_interface->update_network_availability(avaiable, apn);
        } else {
            LOGE("update_network_availability is NULL\n");
        }
        break;
    }
    case MNL_AGPS_TYPE_MNL2AGPS_PMTK: {
        const char* pmtk = get_string(buff, &offset);
        if (agps_interface->rcv_pmtk) {
           agps_interface->rcv_pmtk(pmtk);
        } else {
            LOGE("rcv_pmtk is NULL\n");
        }
        break;
    }
    case MNL_AGPS_TYPE_RAW_DBG: {
        int enabled = get_int(buff, &offset);
        if (agps_interface->raw_dbg) {
           agps_interface->raw_dbg(enabled);
        } else {
            LOGE("raw_dbg is NULL\n");
        }
        break;
    }
    case MNL_AGPS_TYPE_REAIDING: {
        if (agps_interface->reaiding_req) {
            agps_interface->reaiding_req();
        } else {
            LOGE("reaiding_req is NULL\n");
        }
        break;
    }
    case MNL_AGPS_TYPE_LOCATION_SYNC: {
        if (agps_interface->location_sync) {
            double lat = get_double(buff, &offset);
            double lng = get_double(buff, &offset);
            int acc = get_int(buff, &offset);
            agps_interface->location_sync(lat, lng, acc);
        } else {
            LOGE("location_sync is NULL\n");
        }
        break;
    }
    default:
        LOGE("mnl2agps unknown type=%d\n", type);
        break;
    }
}

int create_mnl2agps_fd() {
    int fd = bind_udp_socket(MNL_TO_AGPS);
    set_socket_blocking(fd, 0);
    return fd;
}

int create_mnl2agps_fd2(const char* agps2mnl_path) {
    int fd = bind_udp_socket(MNL_TO_AGPS);
    set_socket_blocking(fd, 0);
    strcpy(g_agps2mnl_path, agps2mnl_path);
    return fd;
}

