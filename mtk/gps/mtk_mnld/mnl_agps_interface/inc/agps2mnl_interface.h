
#ifndef __AGPS2GPS_INTERFACE_H__
#define __AGPS2GPS_INTERFACE_H__

#include "mnl_agps_interface.h"

typedef struct {
    void (*mnl_reboot)();

    void (*open_gps_done)();
    void (*close_gps_done)();
    void (*reset_gps_done)();

    void (*gps_init)();
    void (*gps_cleanup)();
    void (*set_server)(int type, const char* hostname, int port);
    void (*delete_aiding_data)(int flags);
    void (*gps_open)(int assist_req);
    void (*gps_close)();
    void (*data_conn_open)(const char* apn);
    void (*data_conn_failed)();
    void (*data_conn_closed)();

    void (*ni_message)(const char* msg, int len);
    void (*ni_respond)(int session_id, int user_response);

    void (*set_ref_loc)(int type, int mcc, int mnc, int lac, int cid);
    void (*set_set_id)(int type, const char* setid);

    void (*update_network_state)(int connected, int type, int roaming, const char* extra_info);
    void (*update_network_availability)(int avaiable, const char* apn);

    void (*rcv_pmtk)(const char* pmtk);
    void (*raw_dbg)(int enabled);
    void (*reaiding_req)();
    void (*data_conn_open_ip_type)(const char* apn, int ip_type);
    void (*install_certificates)(int index, int total, const char* data, int len);
    void (*revoke_certificates)(const char* data, int len);
    void (*location_sync)(double lat, double lng, int acc);
} mnl2agps_interface;

int agps2mnl_agps_reboot();

int agps2mnl_agps_open_gps_req(int show_gps_icon);
int agps2mnl_agps_close_gps_req();
int agps2mnl_agps_reset_gps_req(int flags);

int agps2mnl_agps_session_done();

int agps2mnl_ni_notify(int session_id, mnl_agps_notify_type type, const char* requestor_id, const char* client_name);
int agps2mnl_ni_notify2(int session_id, mnl_agps_notify_type type, const char* requestor_id, const char* client_name,
    mnl_agps_ni_encoding_type requestor_id_encoding, mnl_agps_ni_encoding_type client_name_encoding);
int agps2mnl_data_conn_req(int ipaddr, int is_emergency);
int agps2mnl_data_conn_req2(struct sockaddr_storage* addr, int is_emergency);
int agps2mnl_data_conn_release();
// flags refer to REQUEST_SETID_IMSI and REQUEST_SETID_MSISDN
int agps2mnl_set_id_req(int flags);
// flags refer to REQUEST_REFLOC_CELLID and REQUEST_REFLOC_MAC
int agps2mnl_ref_loc_req(int flags);

int agps2mnl_pmtk(const char* pmtk);
int agps2mnl_gpevt(gpevt_type type);

int agps2mnl_agps_location(mnl_agps_agps_location* location);

void mnl2agps_hdlr(int fd, mnl2agps_interface* agps_interface);


int create_mnl2agps_fd();
// debug purpose
int create_mnl2agps_fd2(const char* agps2mnl_path);

#endif

