
#ifndef __GPS2AGPS_INTERFACE_H__
#define __GPS2AGPS_INTERFACE_H__

#include <sys/socket.h>
#include "mnl_agps_interface.h"

typedef struct {
    void (*agps_reboot)();

    void (*agps_open_gps_req)(int show_gps_icon);
    void (*agps_close_gps_req)();
    void (*agps_reset_gps_req)(int flags);

    void (*agps_session_done)();

    void (*ni_notify)(int session_id, mnl_agps_notify_type type, const char* requestor_id, const char* client_name);
    void (*data_conn_req)(int ipaddr, int is_emergency);
    void (*data_conn_release)();

    void (*set_id_req)(int flags);
    void (*ref_loc_req)(int flags);

    void (*rcv_pmtk)(const char* pmtk);
    void (*gpevt)(gpevt_type type);

    void (*agps_location)(mnl_agps_agps_location* location);
    void (*ni_notify2)(int session_id, mnl_agps_notify_type type, const char* requestor_id, const char* client_name,
        mnl_agps_ni_encoding_type requestor_id_encoding, mnl_agps_ni_encoding_type client_name_encoding);
    void (*data_conn_req2)(struct sockaddr_storage* addr, int is_emergency);
    void (*agps_settings_sync)(mnl_agps_agps_settings* settings);
} agps2mnl_interface;

// MNL -> AGPS
int mnl2agps_mnl_reboot();

int mnl2agps_open_gps_done();
int mnl2agps_close_gps_done();
int mnl2agps_reset_gps_done();

int mnl2agps_gps_init();
int mnl2agps_gps_cleanup();
// type:AGpsType
int mnl2agps_set_server(int type, const char* hostname, int port);
// flags:GpsAidingData
int mnl2agps_delete_aiding_data(int flags);
int mnl2agps_gps_open(int assist_req);
int mnl2agps_gps_close();
int mnl2agps_data_conn_open(const char* apn);
int mnl2agps_data_conn_open_ip_type(const char* apn, int ip_type);
int mnl2agps_data_conn_failed();
int mnl2agps_data_conn_closed();
int mnl2agps_ni_message(const char* msg, int len);
int mnl2agps_ni_respond(int session_id, int user_response);
int mnl2agps_set_ref_loc(int type, int mcc, int mnc, int lac, int cid);
int mnl2agps_set_set_id(int type, const char* setid);
int mnl2agps_update_network_state(int connected, int type, int roaming, const char* extra_info);
int mnl2agps_update_network_availability(int avaiable, const char* apn);
int mnl2agps_install_certificates(int index, int total, const char* data, int len);
int mnl2agps_revoke_certificates(const char* data, int len);

int mnl2agps_pmtk(const char* pmtk);
int mnl2agps_raw_dbg(int enabled);
int mnl2agps_reaiding_req();
int mnl2agps_location_sync(double lat, double lng, int acc);
int mnl2agps_agps_settings_ack(mnl_agps_gnss_settings* settings);

void agps2mnl_hdlr(int fd, agps2mnl_interface* mnl_interface);


int create_agps2mnl_fd();

#endif

