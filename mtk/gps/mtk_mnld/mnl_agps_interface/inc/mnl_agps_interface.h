#ifndef __GPS_AGPS_INTERFACE_H__
#define __GPS_AGPS_INTERFACE_H__

// #include <hardware/gps.h>

#if defined(__ANDROID_OS__)
#include <android/log.h>
#endif

#if defined(ANDROID)
#define AGPS_TO_MNL "/data/agps_supl/agps_to_mnl"
#define MNL_TO_AGPS "/data/agps_supl/mnl_to_agps"
#else
#define AGPS_TO_MNL "agps_to_mnl"
#define MNL_TO_AGPS "mnl_to_agps"
#endif

#define MNL_AGPS_MAX_BUFF_SIZE (8 * 1024)

#define MNL_AGPS_INTERFACE_VERSION 1

#define REQUEST_SETID_IMSI     (1<<0L)
#define REQUEST_SETID_MSISDN   (1<<1L)

#define REQUEST_REFLOC_CELLID  (1<<0L)
#define REQUEST_REFLOC_MAC     (1<<1L)   // not ready

typedef enum {
    MNL_AGPS_NOTIFY_TYPE_NONE = 0,
    MNL_AGPS_NOTIFY_TYPE_NOTIFY_ONLY,
    MNL_AGPS_NOTIFY_TYPE_NOTIFY_ALLOW_NO_ANSWER,
    MNL_AGPS_NOTIFY_TYPE_NOTIFY_DENY_NO_ANSWER,
    MNL_AGPS_NOTIFY_TYPE_PRIVACY,
} mnl_agps_notify_type;

typedef enum {
    MNL_AGPS_NI_ENCODING_TYPE_GSM7 = 1,
    MNL_AGPS_NI_ENCODING_TYPE_UTF8,
    MNL_AGPS_NI_ENCODING_TYPE_UCS2,
} mnl_agps_ni_encoding_type;

typedef enum {
  GPEVT_TYPE_UNKNOWN = 0,                      //  0
  GPEVT_SUPL_SLP_CONNECT_BEGIN,                //  1
  GPEVT_SUPL_SLP_CONNECTED,                    //  2
  GPEVT_SUPL_SSL_CONNECT_BEGIN,                //  3
  GPEVT_SUPL_SSL_CONNECTED,                    //  4
  GPEVT_SUPL_ASSIST_DATA_RECEIVED,             //  5
  GPEVT_SUPL_ASSIST_DATA_VALID,                //  6
  GPEVT_SUPL_FIRST_POS_FIX,                    //  7
  GPEVT_SUPL_MEAS_TIME_OUT,                    //  8
  GPEVT_SUPL_MEAS_RESPONSE_SENT,               //  9
  GPEVT_SUPL_SSL_CLOSED,                       // 10
  GPEVT_SUPL_SLP_DISCONNECTED,                 // 11

  GPEVT_CP_MOLR_SENT,                          // 12
  GPEVT_CP_MTLR_RECEIVED,                      // 13
  GPEVT_CP_ASSIST_DATA_RECEIVED,               // 14
  GPEVT_CP_ASSIST_DATA_VALID,                  // 15
  GPEVT_CP_FIRST_POS_FIX,                      // 16
  GPEVT_CP_MEAS_TIME_OUT,                      // 17
  GPEVT_CP_MEAS_RESPONSE_SENT,                 // 18

  GPEVT_GNSS_HW_START,                         // 19
  GPEVT_GNSS_HW_STOP,                          // 20
  GPEVT_GNSS_RESET_STORED_SATELLITE_DATA,      // 21

  GPEVT_EPO_SERVER_CONNECT_BEGIN,              // 22
  GPEVT_EPO_SERVER_CONNECTED,                  // 23
  GPEVT_EPO_DATA_RECEIVED,                     // 24
  GPEVT_EPO_SERVER_DISCONNECTED,               // 25
  GPEVT_EPO_DATA_VALID,                        // 26

  GPEVT_HOT_STILL_DATA_VALID,                  // 27

  GPEVT_TYPE_MAX                               // 28
}gpevt_type;

typedef enum {
    //---------------------------------------
    // MNL -> AGPS
    MNL_AGPS_TYPE_MNL_REBOOT  = 100,

    MNL_AGPS_TYPE_AGPS_OPEN_GPS_DONE = 110,
    MNL_AGPS_TYPE_AGPS_CLOSE_GPS_DONE,
    MNL_AGPS_TYPE_AGPS_RESET_GPS_DONE,

    MNL_AGPS_TYPE_GPS_INIT = 120,
    MNL_AGPS_TYPE_GPS_CLEANUP,
    MNL_AGPS_TYPE_DELETE_AIDING_DATA,
    MNL_AGPS_TYPE_GPS_OPEN,
    MNL_AGPS_TYPE_GPS_CLOSE,
    MNL_AGPS_TYPE_DATA_CONN_OPEN,
    MNL_AGPS_TYPE_DATA_CONN_FAILED,
    MNL_AGPS_TYPE_DATA_CONN_CLOSED,
    MNL_AGPS_TYPE_NI_MESSAGE,
    MNL_AGPS_TYPE_NI_RESPOND,
    MNL_AGPS_TYPE_SET_REF_LOC,
    MNL_AGPS_TYPE_SET_SET_ID,
    MNL_AGPS_TYPE_SET_SERVER,
    MNL_AGPS_TYPE_UPDATE_NETWORK_STATE,
    MNL_AGPS_TYPE_UPDATE_NETWORK_AVAILABILITY,
    MNL_AGPS_TYPE_DATA_CONN_OPEN_IP_TYPE,
    MNL_AGPS_TYPE_INSTALL_CERTIFICATES,
    MNL_AGPS_TYPE_REVOKE_CERTIFICATES,

    MNL_AGPS_TYPE_MNL2AGPS_PMTK = 150,
    MNL_AGPS_TYPE_RAW_DBG,
    MNL_AGPS_TYPE_REAIDING,
    MNL_AGPS_TYPE_LOCATION_SYNC,
    MNL_AGPS_TYPE_SETTINGS_ACK,

    // ---------------------------------------
    // AGPS -> MNL
    MNL_AGPS_TYPE_AGPS_REBOOT    = 200,

    MNL_AGPS_TYPE_AGPS_OPEN_GPS_REQ = 210,
    MNL_AGPS_TYPE_AGPS_CLOSE_GPS_REQ,
    MNL_AGPS_TYPE_AGPS_RESET_GPS_REQ,

    MNL_AGPS_TYPE_AGPS_SESSION_DONE = 220,

    MNL_AGPS_TYPE_NI_NOTIFY = 230,
    MNL_AGPS_TYPE_DATA_CONN_REQ,
    MNL_AGPS_TYPE_DATA_CONN_RELEASE,
    MNL_AGPS_TYPE_SET_ID_REQ,
    MNL_AGPS_TYPE_REF_LOC_REQ,
    MNL_AGPS_TYPE_AGPS_LOC,
    MNL_AGPS_TYPE_NI_NOTIFY_2,
    MNL_AGPS_TYPE_DATA_CONN_REQ2,

    MNL_AGPS_TYPE_AGPS2MNL_PMTK = 250,
    MNL_AGPS_TYPE_GPEVT,
    MNL_AGPS_TYPE_SETTINGS_SYNC,
} mnl_agps_type;

typedef struct {
    double          latitude;           // Represents latitude in degrees
    double          longitude;          // Represents longitude in degrees
    char            altitude_used;      // 0=disabled 1=enabled
    double          altitude;           // Represents altitude in meters above the WGS 84 reference
    char            speed_used;         // 0=disabled 1=enabled
    float           speed;              // Represents speed in meters per second
    char            bearing_used;       // 0=disabled 1=enabled
    float           bearing;            // Represents heading in degrees
    char            accuracy_used;      // 0=disabled 1=enabled
    float           accuracy;           // Represents expected accuracy in meters
    char            timestamp_used;     // 0=disabled 1=enabled
    long long       timestamp;          // Milliseconds since January 1, 1970
} mnl_agps_agps_location;

typedef struct {
    char            sib8_16_enable;
    char            gps_satellite_enable;
    char            glonass_satellite_enable;
    char            beidou_satellite_enable;
    char            galileo_satellite_enable;
    char            a_glonass_satellite_enable;
} mnl_agps_agps_settings;

typedef struct {
    char            gps_satellite_support;
    char            glonass_satellite_support;
    char            beidou_satellite_support;
    char            galileo_satellite_support;
} mnl_agps_gnss_settings;
const char* get_mnl_agps_type_str(mnl_agps_type input);

#endif

