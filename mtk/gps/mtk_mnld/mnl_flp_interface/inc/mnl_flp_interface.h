#ifndef __MNL_FLP_INTERFACE_H__
#define __MNL_FLP_INTERFACE_H__

#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <sys/socket.h>
#include "mtk_gps_type.h"
#ifdef __cplusplus
extern "C" {
#endif

#define FLP_MNL_BUFF_SIZE           (1 * 1024)
#define FLP_MNL_INTERFACE_VERSION   1
#define OFFLOAD_PAYLOAD_LEN (600)
#define OFL_VER "OFL_VER_2015.01.15-01"
#define IOCTL_MNL_IMAGE_FILE_TO_MEM  1
#define IOCTL_MNL_NVRAM_FILE_TO_MEM  2
#define IOCTL_MNL_NVRAM_MEM_TO_FILE  3

//======================================================
// GPS FLP -> MNLD
//======================================================
#define MTK_FLP2MNL "/data/gps_mnl/flp2mnld"

typedef enum {
    FLPMNL_CMD_UNKNOWN = -1,
    FLPMNL_GPS_START = 0x00,
    FLPMNL_GPS_STOP = 0x01,
    FLPMNL_GPS_LPBK = 0x02,
    FLPMNL_DATA_SEND = 0x03,
    FLPMNL_DATA_RECV = 0x04,
    FLPMNL_CMD_NETWORK_STATUS = 0x05,
    FLPMNL_REBOOT_DONE = 0x06,
    FLPMNL_GPS_OPEN_DONE = 0x07,
    FLPMNL_GPS_STOP_DONE = 0x08,
    FLPMNL_GPS_REPORT_LOC = 0x09,
} flp2mnl_cmd;

typedef struct {
    UINT32 type;
    UINT32 length;
    UINT8 data[OFFLOAD_PAYLOAD_LEN];
} MTK_FLP_OFFLOAD_MSG_T;

typedef struct {
    void (*flp_reboot)();

    void (*gps_start)();
    void (*gps_stop)();
    int (*flp_lpbk)();
    int (*flp_data)();
} flp2mnl_interface;

int mnl2flp_mnld_reboot();
int mnl2flp_gps_open_done();
int mnl2flp_gps_close_done();
int mnl2flp_data_send(UINT8 *buf, UINT32 len);

// -1 means failure
int flp2mnl_hdlr(int fd, flp2mnl_interface* hdlr);

// -1 means failure
int create_flp2mnl_fd();
int create_mnl2flp_fd();
//======================================================
// MNLD -> GPS FLP
//======================================================
#define MTK_MNL2FLP "/data/gps_mnl/mnld2flp"

#ifdef __cplusplus
}
#endif

#endif
