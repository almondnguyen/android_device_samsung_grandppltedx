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

#include "wifi2agps_interface.h"
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
#define LOGD(...) tag_log(0, "[WIFI2AGPS]", __VA_ARGS__);
#define LOGW(...) tag_log(0, " WARNING: [WIFI2AGPS]", __VA_ARGS__);
#define LOGE(...) tag_log(1, " ERR: [WIFI2AGPS]", __VA_ARGS__);


//-1 means failure
static int safe_sendto(int sockfd, const char* dest, const char* buf, int size) {
    int len = 0;
    struct sockaddr_un soc_addr;
    socklen_t addr_len;
    int retry = 10;

    strcpy(soc_addr.sun_path, dest);
    soc_addr.sun_family = AF_UNIX;
    addr_len = (offsetof(struct sockaddr_un, sun_path) + strlen(soc_addr.sun_path) + 1);

    while((len = sendto(sockfd, buf, size, 0,
        (const struct sockaddr *)&soc_addr, (socklen_t)addr_len)) == -1) {
        if(errno == EINTR) continue;
        if(errno == EAGAIN) {
            if(retry-- > 0) {
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

//-1 means failure
static int send2agps(const char* buff, int len) {
    int ret = 0;
    int sockfd = socket(PF_LOCAL, SOCK_DGRAM, 0);
    if(safe_sendto(sockfd, WIFI_TO_AGPS, buff, len) < 0) {
        LOGE("external_snd_cmd safe_sendto failed\n");
        ret = -1;
    }
    close(sockfd);
    return ret;
}

//============== implementation ===============

//-1 means failure
int wifi2agps_enabled() {
    LOGD("wifi2agps_enabled");
    char buff[WIFI2AGPS_MAX_BUFF_SIZE] = {0};
    int offset = 0;

    put_int(buff, &offset, WIFI2AGPS_INTERFACE_VERSION);
    put_int(buff, &offset, WIFI2AGPS_CMD_TYPE_ENABLED);
    
    return send2agps(buff, offset);
}

//-1 means failure
int wifi2agps_disabled() {
    LOGD("wifi2agps_disabled");
    char buff[WIFI2AGPS_MAX_BUFF_SIZE] = {0};
    int offset = 0;

    put_int(buff, &offset, WIFI2AGPS_INTERFACE_VERSION);
    put_int(buff, &offset, WIFI2AGPS_CMD_TYPE_DISABLED);
    
    return send2agps(buff, offset);
}

//-1 means failure
int wifi2agps_associated(wifi2agps_ap_info* ap_info) {
    LOGD("wifi2agps_associated");
    char buff[WIFI2AGPS_MAX_BUFF_SIZE] = {0};
    int offset = 0;

    put_int(buff, &offset, WIFI2AGPS_INTERFACE_VERSION);
    put_int(buff, &offset, WIFI2AGPS_CMD_TYPE_ASSOCIATED);
    put_binary(buff, &offset, (const char*)ap_info, sizeof(wifi2agps_ap_info));
    
    return send2agps(buff, offset);
}

//-1 means failure
int wifi2agps_disassociated() {
    LOGD("wifi2agps_disassociated");
    char buff[WIFI2AGPS_MAX_BUFF_SIZE] = {0};
    int offset = 0;

    put_int(buff, &offset, WIFI2AGPS_INTERFACE_VERSION);
    put_int(buff, &offset, WIFI2AGPS_CMD_TYPE_DISASSOCIATED);
    
    return send2agps(buff, offset);
}

//-1 means failure
int wifi2agps_scanned(wifi2agps_ap_info_list* ap_info_list) {
    LOGD("wifi2agps_scanned");
    char buff[WIFI2AGPS_MAX_BUFF_SIZE] = {0};
    int offset = 0;

    put_int(buff, &offset, WIFI2AGPS_INTERFACE_VERSION);
    put_int(buff, &offset, WIFI2AGPS_CMD_TYPE_SCANNED);
    put_binary(buff, &offset, (const char*)ap_info_list, sizeof(wifi2agps_ap_info_list));
    
    return send2agps(buff, offset);
}

void dump_ap_info(wifi2agps_ap_info* ap_info) {
    LOGD("ap_mac_addr=%02x:%02x:%02x:%02x:%02x:%02x\n", 
        ap_info->ap_mac_addr[0],
        ap_info->ap_mac_addr[1],
        ap_info->ap_mac_addr[2],
        ap_info->ap_mac_addr[3],
        ap_info->ap_mac_addr[4],
        ap_info->ap_mac_addr[5]);
    LOGD("is_used=%d ap_transmit_power=%d (-127..128)\n", ap_info->ap_transmit_power_used, ap_info->ap_transmit_power);
    LOGD("is_used=%d ap_antenna_gain=%d (-127..128)\n", ap_info->ap_antenna_gain_used, ap_info->ap_antenna_gain);
    LOGD("is_used=%d ap_signal_noise=%d (-127..128)\n", ap_info->ap_signal_noise_used, ap_info->ap_signal_noise);
    LOGD("is_used=%d ap_device_type=%d (0..2)\n", ap_info->ap_device_type_used, ap_info->ap_device_type);
    LOGD("is_used=%d ap_signal_strength=%d (-127..128)\n", ap_info->ap_signal_strength_used, ap_info->ap_signal_strength);
    LOGD("is_used=%d ap_channel_frequency=%d (0..256)\n", ap_info->ap_channel_frequency_used, ap_info->ap_channel_frequency);
    LOGD("is_used=%d ap_round_trip_delay rtd_value=%d (0..16777216) rtd_units=%d (0..4) rtd_accuracy=%d (0..255)\n", 
        ap_info->ap_round_trip_delay_used,
        ap_info->ap_round_trip_delay.rtd_value,
        ap_info->ap_round_trip_delay.rtd_units,
        ap_info->ap_round_trip_delay.rtd_accuracy);
    LOGD("is_used=%d set_transmit_power=%d (-127..128)\n", ap_info->set_transmit_power_used, ap_info->set_transmit_power);
    LOGD("is_used=%d set_antenna_gain=%d (-127..128)\n", ap_info->set_antenna_gain_used, ap_info->set_antenna_gain);
    LOGD("is_used=%d set_signal_to_noise=%d (-127..128)\n", ap_info->set_signal_to_noise_used, ap_info->set_signal_to_noise);
    LOGD("is_used=%d set_signal_strength=%d (-127..128)\n", ap_info->set_signal_strength_used, ap_info->set_signal_strength);
    LOGD("is_used=%d reported_location loc_encode_descriptor=%d (0..1) is_used=%d location_accuracy=%d (0..4294967295) loc_value_len=%d\n", 
        ap_info->reported_location_used,
        ap_info->reported_location.loc_encode_descriptor,
        ap_info->reported_location.location_data.location_accuracy_used,
        ap_info->reported_location.location_data.location_accuracy,
        ap_info->reported_location.location_data.location_value_length);
    //ap_info->reported_location.location_data.location_value
}

void dump_ap_info_list(wifi2agps_ap_info_list* ap_info_list) {
    int i = 0;
    LOGD("num=%d (1..32)\n", ap_info_list->num);
    for(i = 0; i < ap_info_list->num; i++) {
        LOGD("i=%d\n", i);
        dump_ap_info(&ap_info_list->list[i]);
    }
}

