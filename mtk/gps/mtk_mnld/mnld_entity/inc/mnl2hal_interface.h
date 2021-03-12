#ifndef __MNL2HAL_INTERFACE_H__
#define __MNL2HAL_INTERFACE_H__

#include "hal_mnl_interface_common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    void (*hal_reboot)();

    void (*gps_init)();
    void (*gps_start)();
    void (*gps_stop)();
    void (*gps_cleanup)();

    void (*gps_inject_time)(int64_t time, int64_t time_reference, int uncertainty);
    void (*gps_inject_location)(double lat, double lng, float acc);
    void (*gps_delete_aiding_data)(int flags);
    void (*gps_set_position_mode)(gps_pos_mode mode, gps_pos_recurrence recurrence,
    int min_interval, int preferred_acc, int preferred_time);

    void (*data_conn_open)(const char* apn);
    void (*data_conn_open_with_apn_ip_type)(const char* apn, apn_ip_type ip_type);
    void (*data_conn_closed)();
    void (*data_conn_failed)();

    void (*set_server)(agps_type type, const char* hostname, int port);
    void (*set_ref_location)(cell_type type, int mcc, int mnc, int lac, int cid);
    void (*set_id)(agps_id_type type, const char* setid);

    void (*ni_message)(char* msg, int len);
    void (*ni_respond)(int notif_id, ni_user_response_type user_response);

    void (*update_network_state)(int connected, network_type type, int roaming,
        const char* extra_info);
    void (*update_network_availability)(int available, const char* apn);

    void (*set_gps_measurement)(bool enabled);
    void (*set_gps_navigation)(bool enabled);
} hal2mnl_interface;

int mnl2hal_mnld_reboot();

int mnl2hal_location(gps_location location);
int mnl2hal_gps_status(gps_status status);
int mnl2hal_gps_sv(gnss_sv_info sv);
int mnl2hal_nmea(int64_t timestamp, const char* nmea, int length);
int mnl2hal_gps_capabilities(gps_capabilites capabilities);
int mnl2hal_gps_measurements(gps_data data);
int mnl2hal_gps_navigation(gps_nav_msg msg);
int mnl2hal_gnss_measurements(gnss_data data);
int mnl2hal_gnss_navigation(gnss_nav_msg msg);


int mnl2hal_request_wakelock();
int mnl2hal_release_wakelock();

int mnl2hal_request_utc_time();

int mnl2hal_request_data_conn(struct sockaddr_storage addr);
int mnl2hal_release_data_conn();
int mnl2hal_request_ni_notify(int session_id, agps_notify_type type,
    const char* requestor_id, const char* client_name, ni_encoding_type requestor_id_encoding,
    ni_encoding_type client_name_encoding);
int mnl2hal_request_set_id(request_setid flags);
int mnl2hal_request_ref_loc(request_refloc flags);

// -1 means failure
int hal2mnl_hdlr(int fd, hal2mnl_interface* hdlr);

// -1 means failure
int create_hal2mnl_fd();

#ifdef __cplusplus
}
#endif

#endif
