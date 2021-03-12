#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <sys/time.h>
#include <time.h>

#include <stddef.h> // offsetof
#include <stdarg.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/socket.h>
#include <string.h>
#include <fcntl.h>

#include <arpa/inet.h> //inet_addr
#include <sys/un.h> //struct sockaddr_un

#include <android/log.h>
#include <private/android_filesystem_config.h>

#include "agps2wifi_interface.h"
#include "data_coder.h"

static void tag_log(int type, const char* tag, const char *fmt, ...) {
    char out_buf[1100] = {0};
    char buf[1024] = {0};
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    sprintf(out_buf, "%s %s", tag, buf);
    if(type == 0) {
        __android_log_print(ANDROID_LOG_DEBUG, "agps", "%s", out_buf);
    } else {
        __android_log_print(ANDROID_LOG_ERROR, "agps", "%s", out_buf);
    }
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
#define LOGD(...) tag_log(0, "[AGPS2WIFI]", __VA_ARGS__);
#define LOGW(...) tag_log(0, " WARNING: [AGPS2WIFI]", __VA_ARGS__);
#define LOGE(...) tag_log(1, " ERR: [AGPS2WIFI]", __VA_ARGS__);

//-1 means failure
static int safe_recvfrom(int sockfd, char* buf, int len) {
    int ret = 0;
    int retry = 10;

    while((ret = recvfrom(sockfd, buf, len, 0,
         NULL, NULL)) == -1) {
        LOGW("ret=%d len=%d\n", ret, len);
        if(errno == EINTR) continue;
        if(errno == EAGAIN) {
            if(retry-- > 0) {
                usleep(100 * 1000);
                continue;
            }
        }
        LOGE("sendto reason=[%s]\n", strerror(errno));
        break;
    }
    return ret;
}

//-1 means failure
static int set_socket_blocking(int fd, int blocking) {
    if(fd < 0) {
        LOGE("set_socket_blocking  invalid fd=%d\n", fd);
        return -1;
    }

    int flags = fcntl(fd, F_GETFL, 0);
    if(flags < 0) {
        LOGE("set_socket_blocking  invalid flags=%d\n", flags);
        return -1;
    }

    flags = blocking ? (flags&~O_NONBLOCK) : (flags|O_NONBLOCK);
    return (fcntl(fd, F_SETFL, flags) == 0) ? 0 : -1;
}

static int bind_udp_socket(char* path) {
    int sockfd;
    struct sockaddr_un soc_addr;
    socklen_t addr_len;

    sockfd = socket(PF_LOCAL, SOCK_DGRAM, 0);
    if(sockfd < 0) {
        LOGE("socket failed reason=[%s]\n", strerror(errno));
        return -1;
    }

    strcpy(soc_addr.sun_path, path);
    soc_addr.sun_family = AF_UNIX;
    addr_len = (offsetof(struct sockaddr_un, sun_path) + strlen(soc_addr.sun_path) + 1);

    unlink(soc_addr.sun_path);
    if(bind(sockfd, (struct sockaddr *)&soc_addr, addr_len) < 0) {
        LOGE("bind failed path=[%s] reason=[%s]\n", path, strerror(errno));
        return -1;
    }

    chmod(path, 0660);

    return sockfd;
}
//============== implementation ===============

//-1 means failure
int wifi2agps_handler(int fd, wifi2agpsInterface* agps_interface) {
    char buff[WIFI2AGPS_MAX_BUFF_SIZE] = {0};
    int offset = 0;
    int ret;

    ret = safe_recvfrom(fd, buff, sizeof(buff));
    if(ret == -1) {
        LOGE("wifi2agps_handler  safe_recvfrom fail\n");
        return -1;
    }
    
    int version = get_int(buff, &offset);

    if(version != WIFI2AGPS_INTERFACE_VERSION) {
        LOGE("wifi_ver=%d agps_ver=%d\n",
            version, WIFI2AGPS_INTERFACE_VERSION);
    }
    
    wifi2agps_cmd_type type = get_int(buff, &offset);
    //LOGD("wifi2agps type=%d\n", type);

    switch(type) {
    case WIFI2AGPS_CMD_TYPE_ENABLED: {
        if(agps_interface->wifi_enabled) {
            agps_interface->wifi_enabled(); 
        } else {
            LOGE("wifi_enabled is NULL\n");
            return -1;
        }
        break;
    }
    case WIFI2AGPS_CMD_TYPE_DISABLED: {
        if(agps_interface->wifi_disabled) {
            agps_interface->wifi_disabled(); 
        } else {
            LOGE("wifi_disabled is NULL\n");
            return -1;
        }
        break;
    }
    case WIFI2AGPS_CMD_TYPE_ASSOCIATED: {
        if(agps_interface->wifi_associated) {
            wifi2agps_ap_info ap_info;
            memset(&ap_info, 0, sizeof(ap_info));
            int len = get_binary(buff, &offset, (char*)&ap_info);
            if(len != sizeof(ap_info)) {
                LOGE("WIFI2AGPS_CMD_TYPE_ASSOCIATED  length of wifi2agps_ap_info is incorrect, read=%d expected=%d\n",
                    len, sizeof(ap_info));
            }
            agps_interface->wifi_associated(&ap_info); 
        } else {
            LOGE("wifi_associated is NULL\n");
            return -1;
        }
        break;
    }
    case WIFI2AGPS_CMD_TYPE_DISASSOCIATED: {
        if(agps_interface->wifi_disassociated) {
            agps_interface->wifi_disassociated(); 
        } else {
            LOGE("wifi_disassociated is NULL\n");
            return -1;
        }
        break;
    }
    case WIFI2AGPS_CMD_TYPE_SCANNED: {
        if(agps_interface->wifi_scanned) {
            wifi2agps_ap_info_list ap_info_list;
            memset(&ap_info_list, 0, sizeof(ap_info_list));
            int len = get_binary(buff, &offset, (char*)&ap_info_list);
            if(len != sizeof(ap_info_list)) {
                LOGE("WIFI2AGPS_CMD_TYPE_SCANNED  length of wifi2agps_ap_info_list is incorrect, read=%d expected=%d\n",
                    len, sizeof(ap_info_list));
            }
            agps_interface->wifi_scanned(&ap_info_list); 
        } else {
            LOGE("wifi_scanned is NULL\n");
            return -1;
        }
        break;
    }
    default: {
        LOGE("wifi2agps unknown type=%d\n", type);
        return -1;
        break;
    }
    }
    return 0;
}

int create_wifi2agps_fd() {
    int fd = bind_udp_socket(WIFI_TO_AGPS);
    chown(WIFI_TO_AGPS, AID_GPS, AID_WIFI);
    set_socket_blocking(fd, 0);
    return fd;
}

