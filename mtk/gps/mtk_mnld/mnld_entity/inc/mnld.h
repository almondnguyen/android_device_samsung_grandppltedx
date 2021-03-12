#ifndef __MNLD_H__
#define __MNLD_H__

#include <time.h>
#include <stdint.h>
#include <stdbool.h>
#include "mnl2hal_interface.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MNLD_MAIN_SOCKET                "mnld_main_socket"
#define MNLD_GPS_CONTROL_SOCKET         "mnld_gps_control_socket"
#define MNLD_EPO_DOWNLOAD_SOCKET        "mnld_epo_download_socket"
#define MNLD_QEPO_DOWNLOAD_SOCKET       "mnld_qepo_download_socket"
#define MNLD_OP01_LOG_WRITER_SOCKET     "mnld_op01_log_write_socket"

#if defined(__ANDROID_OS__)
#define MNLD_OP01_LOG_PATH              "/sdcard/GPS.LOG"
#else
#define MNLD_OP01_LOG_PATH              "GPS.LOG"
#endif

#define MNLD_MAIN_HANDLER_TIMEOUT       (10 * 1000)
#define MNLD_GPS_HANDLER_TIMEOUT        (10 * 1000)
#define MNLD_EPO_HANDLER_TIMEOUT        (60 * 60 * 1000)
#define MNLD_EPO_RETRY_HANDLER_TIMEOUT  (10 * 1000)
#define MNLD_QEPO_HANDLER_TIMEOUT       (60 * 60 * 1000)
#define MNLD_OP01_HANDLER_TIMEOUT       (20 * 1000)
#define MNLD_MPE_HANDLER_TIMEOUT        (10 * 1000)
#define MNLD_GPS_START_TIMEOUT          (15 * 1000)
#define MNLD_GPS_STOP_TIMEOUT           (10 * 1000)
#define MNLD_GPS_RESET_TIMEOUT          (10 * 1000)
#define MNLD_GPS_NMEA_DATA_TIMEOUT      (10 * 1000)

#define MNLD_INTERNAL_BUFF_SIZE         (8 * 1024)

typedef enum {
    MNLD_GPS_STATE_IDLE         = 0,
    MNLD_GPS_STATE_STARTING     = 1,
    MNLD_GPS_STATE_STARTED      = 2,
    MNLD_GPS_STATE_STOPPING     = 3,
} mnld_gps_state;

typedef enum {
    GPS_EVENT_START         = 0,
    GPS_EVENT_STOP          = 1,
    GPS_EVENT_RESET         = 2,
    GPS_EVENT_START_DONE    = 3,
    GPS_EVENT_STOP_DONE     = 4,
} mnld_gps_event;

typedef enum {
    GPS2MAIN_EVENT_START_DONE               = 0,
    GPS2MAIN_EVENT_STOP_DONE                = 1,
    GPS2MAIN_EVENT_RESET_DONE               = 2,
    GPS2MAIN_EVENT_NMEA_TIMEOUT             = 3,
    GPS2MAIN_EVENT_UPDATE_LOCATION          = 4,
    EPO2MAIN_EVENT_EPO_DONE                 = 5,
    QEPO2MAIN_EVENT_QEPO_DONE               = 6,
} main_internal_event;

typedef enum {
    EPO_DOWNLOAD_RESULT_SUCCESS     = 0,
    EPO_DOWNLOAD_RESULT_FAIL        = -1,
} epo_download_result;

typedef struct {
    int fd_hal;
    int fd_agps;
    int fd_flp;
    int fd_flp_test;
    int fd_at_cmd;
    int fd_int;
    int fd_mtklogger;
    int fd_mtklogger_client;
} mnld_fds;

typedef struct {
    bool        gps_used;
    bool        need_open_ack;
    bool        need_close_ack;
    bool        need_reset_ack;
} mnld_gps_client;

typedef struct {
    mnld_gps_client     hal;
    mnld_gps_client     agps;
    mnld_gps_client     flp;
    mnld_gps_client     flp_test;
    mnld_gps_client     at_cmd_test;
    mnld_gps_client     factory;
} mnld_gps_clients;

typedef struct {
    mnld_gps_clients    clients;
    mnld_gps_state      gps_state;
    bool                is_gps_init;
    bool                is_gps_meas_enabled;
    bool                is_gps_navi_enabled;
    int                 delete_aiding_flag;

    timer_t             timer_start;
    timer_t             timer_stop;
    timer_t             timer_reset;
    timer_t             timer_nmea_monitor;

    time_t              gps_start_time;
    time_t              gps_stop_time;
    time_t              gps_ttff;
    bool                wait_first_location;
} mnld_gps_status;

typedef struct {
    bool            is_network_connected;
    bool            is_wifi_connected;
    bool            is_epo_downloading;
} mnl_epo_status;

typedef struct {
    mnld_fds            fds;
    mnld_gps_status     gps_status;
    mnl_epo_status      epo_status;
} mnld_context;

// GPS Control -> MAIN
int mnld_gps_start_done(bool is_assist_req);
int mnld_gps_stop_done();
int mnld_gps_reset_done();
int mnld_gps_update_location(gps_location location);

// EPO Download -> MAIN
int mnld_epo_download_done(epo_download_result result);
int mnld_qepo_download_done(epo_download_result result);
void hal_start_gps_trigger_epo_download();
bool is_network_connected();
bool is_wifi_network_connected();

// Provided for GPS Control to check the status
bool mnld_is_gps_started();
bool mnld_is_gps_started_done();
bool mnld_is_gps_meas_enabled();
bool mnld_is_gps_navi_enabled();
bool mnld_is_gps_stopped();

int mnld_gps_controller_mnl_nmea_timeout(void);
int mnld_gps_start_nmea_monitor(void);
int mnld_gps_stop_nmea_monitor(void);
void gps_mnld_restart_mnl_process(void);

int mtk_gps_get_gps_user(void);

void factory_mnld_gps_start(void);
void factory_mnld_gps_stop(void);

void flp_test2mnl_gps_start(void);
void flp_test2mnl_gps_stop(void);

int is_flp_user_exist();

#ifdef __cplusplus
}
#endif

#endif

