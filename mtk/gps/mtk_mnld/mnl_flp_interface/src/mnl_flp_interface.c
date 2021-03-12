#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "mnl_flp_interface.h"
#include "mtk_lbs_utility.h"
#include "data_coder.h"
#include "gps_controller.h"
#include "mtk_gps.h"
#include "mnld.h"


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
#define LOGD(...) tag_log(1, "[mnl2flp]", __VA_ARGS__);
#define LOGW(...) tag_log(1, "[mnl2flp] WARNING: ", __VA_ARGS__);
#define LOGE(...) tag_log(1, "[mnl2flp] ERR: ", __VA_ARGS__);
#else
#define LOG_TAG "mnl2flp"
#include <cutils/sockets.h>
#include <cutils/log.h>     /*logging in logcat*/
#define LOGD(fmt, arg ...) ALOGD("%s: " fmt, __FUNCTION__ , ##arg)
#define LOGW(fmt, arg ...) ALOGW("%s: " fmt, __FUNCTION__ , ##arg)
#define LOGE(fmt, arg ...) ALOGE("%s: " fmt, __FUNCTION__ , ##arg)
#endif

int ofl_lpbk_tst_start = 0;
int ofl_lpbk_tst_size = 300;

/*****************************************************************************/
INT32 mtk_gps_ofl_sys_rst_stpgps_req() {
    int ret = 0;

    LOGD("");
    gps_mnld_restart_mnl_process();
    return ret;
}
/*****************************************************************************/
INT32 mtk_gps_ofl_sys_submit_flp_data(UINT8 *buf, UINT32 len) {
    LOGD("[OFL]flp submit to host, buf=%p, len=%d", buf, len);
    if (ofl_lpbk_tst_start == 1) {
        static UINT8 tmp_buf[1024];
        static int  tmp_buf_w = 0;
        memcpy(&tmp_buf[tmp_buf_w], buf, len);
        tmp_buf_w+=len;
        if (tmp_buf_w >= ofl_lpbk_tst_size) {
            LOGD("[OFL] lpbk recv enougth data, actual=%d, except=%d, lpbk restart...",
                tmp_buf_w, ofl_lpbk_tst_size);
            mtk_gps_ofl_send_flp_data(&tmp_buf[0], ofl_lpbk_tst_size);
            tmp_buf_w = 0;
        }
    } else if ((buf != NULL) && (len != 0) && (len < 1023)) {
        UINT8 buff[1024] = {0};
        MTK_FLP_OFFLOAD_MSG_T *prMsg = (MTK_FLP_OFFLOAD_MSG_T *)&buff[0];

        prMsg->type = FLPMNL_DATA_RECV;
        prMsg->length = len;
        if (len < sizeof(prMsg->data)) {
            memcpy(&prMsg->data[0], buf, len);
        } else {
            LOGE("memcpy buf to prMsg->data out of bounds");
        }

        if (-1 == mnl2flp_data_send(buff, sizeof(buff))) {
            LOGE("[FLP2MNLD]Send to FLP failed, %s\n", strerror(errno));
            return MTK_GPS_ERROR;
        }
        else {
            LOGD("[FLP2MNLD]Send to FLP successfully\n");
        }
    }
    return MTK_GPS_SUCCESS;
}

int mnl2flp_mnld_reboot() {
    LOGD("mnl2flp_mnld_reboot");
    char buff[FLP_MNL_BUFF_SIZE] = {0};
    int offset = 0;
    int * payload = NULL;
    MTK_FLP_OFFLOAD_MSG_T *prMsg = (MTK_FLP_OFFLOAD_MSG_T *)&buff[0];

    prMsg->type = FLPMNL_REBOOT_DONE;
    prMsg->length = sizeof(int);
    payload = (int *)&prMsg->data[0];
    *payload = FLP_MNL_INTERFACE_VERSION;
    return mnld_sendto_flp(MTK_MNL2FLP, buff, sizeof(MTK_FLP_OFFLOAD_MSG_T));
}

int mnl2flp_gps_open_done() {
    LOGD("mnl2flp_gps_open_done");
    char buff[FLP_MNL_BUFF_SIZE] = {0};
    MTK_FLP_OFFLOAD_MSG_T *prMsg = (MTK_FLP_OFFLOAD_MSG_T *)&buff[0];

    prMsg->type = FLPMNL_GPS_OPEN_DONE;
    prMsg->length = 0;
    return mnld_sendto_flp(MTK_MNL2FLP, buff, sizeof(MTK_FLP_OFFLOAD_MSG_T));
}

int mnl2flp_gps_close_done() {
    LOGD("mnl2flp_gps_close_done");
    char buff[FLP_MNL_BUFF_SIZE] = {0};
    MTK_FLP_OFFLOAD_MSG_T *prMsg = (MTK_FLP_OFFLOAD_MSG_T *)&buff[0];

    prMsg->type = FLPMNL_GPS_STOP_DONE;
    prMsg->length = 0;
    return mnld_sendto_flp(MTK_MNL2FLP, buff, sizeof(MTK_FLP_OFFLOAD_MSG_T));
}

int mnl2flp_data_send(UINT8 *buf, UINT32 len) {
    int ret = 0;

    ret = mnld_sendto_flp(MTK_MNL2FLP, (char *)buf, len);
    return ret;
}

// Add for dbg usage
#define MAX_DUMP_CHAR 128
void mnl_flp_dump_buf(char *p, int len) {
    int i, r, n;
    char str[MAX_DUMP_CHAR] = {0};
    for (i = 0, n = 0; i < len ; i++) {
        r = snprintf(&str[n], MAX_DUMP_CHAR - n, "%3d,", p[i]);
        if (r + n >= MAX_DUMP_CHAR || r <= 0) {
            LOGD("[FLP2MNLD]data from flp: data=%s, error return", str);
            return;
    }

    n += r;
    if ((i % 8 == 7) || i + 1 == len) {
            LOGD("[FLP2MNLD]data from flp: data=%s", str);
            n = 0;
            str[0] = '0';
        }
    }
}
// -1 means failure
int flp2mnl_hdlr(int fd, flp2mnl_interface* hdlr) {
    char buff[FLP_MNL_BUFF_SIZE] = {0};
    int read_len;
    int ret = 0;
    MTK_FLP_OFFLOAD_MSG_T *prMsg = NULL;

    read_len = safe_recvfrom(fd, buff, sizeof(buff));
    if (read_len <= 0) {
        LOGE("flp2mnl_hdlr() safe_recvfrom() failed read_len=%d", read_len);
        return -1;
    }
    prMsg = (MTK_FLP_OFFLOAD_MSG_T *)&buff[0];
    LOGD("[FLP2MNLD]data from flp: type=%d, len=%d, read_len = %d\n", prMsg->type, prMsg->length, read_len);
    mnl_flp_dump_buf(&buff[0], read_len);
    switch (prMsg->type) {
    case FLPMNL_REBOOT_DONE: {
        if (hdlr->flp_reboot) {
            hdlr->flp_reboot();
        } else {
            LOGE("flp2mnl_hdlr() flp_reboot is NULL");
            ret = -1;
        }
        break;
    }
    case FLPMNL_GPS_START: {
        int *mnl_mode_pointer = (int *)&prMsg->data[0];
        int flp_mnl_mode = *mnl_mode_pointer;
        LOGD("flp_mnl_mode = %d\n", flp_mnl_mode);
        if (flp_mnl_mode == 1) {
             mnl_offload_set_enabled();
        } else {
             mnl_offload_clear_enabled();
        }
        if (hdlr->gps_start) {
            hdlr->gps_start();
        } else {
            LOGE("flp2mnl_hdlr() gps_start is NULL");
            ret = -1;
        }
        break;
    }
    case FLPMNL_GPS_STOP: {
        int *mnl_mode_pointer = (int *)&prMsg->data[0];
        int flp_mnl_mode = *mnl_mode_pointer;
        LOGD("flp_mnl_mode = %d\n", flp_mnl_mode);
        if (flp_mnl_mode == 1) {
             mnl_offload_set_enabled();
        } else {
             mnl_offload_clear_enabled();
        }
        if (hdlr->gps_stop) {
            hdlr->gps_stop();
        } else {
            LOGE("flp2mnl_hdlr() gps_stop is NULL");
            ret = -1;
        }
        break;
    }
    case FLPMNL_GPS_LPBK: {
        if (hdlr->flp_lpbk) {
            hdlr->flp_lpbk(buff, sizeof(buff));
        } else {
            LOGE("flp2mnl_hdlr() flp_lpbk is NULL");
            ret = -1;
        }
        break;
    }
    case FLPMNL_DATA_SEND: {
        if (hdlr->flp_data) {
            hdlr->flp_data(prMsg);
        } else {
            LOGE("flp2mnl_hdlr() flp_data is NULL");
            ret = -1;
        }
        break;
    }
    default: {
        LOGE("flp2mnl_hdlr() unknown cmd=%d", prMsg->type);
        ret = -1;
        break;
    }
    }

    return ret;
}

// -1 means failure
int create_flp2mnl_fd() {
    int fd = mnld_flp_to_mnld_fd_init(MTK_FLP2MNL);
    socket_set_blocking(fd, 0);
    return fd;
}

int create_mnl2flp_fd() {
    int fd = socket_bind_udp(MTK_MNL2FLP);
    socket_set_blocking(fd, 0);
    return fd;
}
