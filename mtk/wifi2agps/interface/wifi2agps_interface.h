#ifndef __WIFI2AGPS_INTERFACE_H__
#define __WIFI2AGPS_INTERFACE_H__

#define WIFI_TO_AGPS "/data/agps_supl/wifi_2_agps"
#define WIFI2AGPS_MAX_BUFF_SIZE 10000
#define WIFI2AGPS_INTERFACE_VERSION 1
#define WIFI_AP_LIST_NUM_MAX 32

typedef enum {
    WIFI2AGPS_CMD_TYPE_ENABLED = 0,
    WIFI2AGPS_CMD_TYPE_DISABLED,
    WIFI2AGPS_CMD_TYPE_ASSOCIATED,
    WIFI2AGPS_CMD_TYPE_DISASSOCIATED,
    WIFI2AGPS_CMD_TYPE_SCANNED,
} wifi2agps_cmd_type;

typedef enum {
    AP_DEVICE_TYPE_802_11A = 0,
    AP_DEVICE_TYPE_802_11B,
    AP_DEVICE_TYPE_802_11G,
} wifi2agps_ap_device_type_e;

typedef enum {
    RTD_UNITS_MICRO_SEC = 0,
    RTD_UNITS_HUNDREDS_OF_NANO_SEC,
    RTD_UNITS_TENS_OF_NANO_SEC,
    RTD_UNITS_NANO_SEC,
    RTD_UNITS_TENTHS_OF_NANO_SEC,
} wifi2agps_rtd_units_e;

typedef enum {
    LOC_ENCODE_DESCRIPTOR_LCI = 0,
    LOC_ENCODE_DESCRIPTOR_ASN1,
} wifi2agps_loc_encode_descriptor_e;

typedef struct {
    int rtd_value;      //0..16777216
    wifi2agps_rtd_units_e rtd_units;
    int rtd_accuracy;   //0..255
} wifi2agps_rtd;

typedef struct {
    char location_accuracy_used;
    unsigned int location_accuracy; //0..4294967295

    char location_value_length; //1..128
    char location_value[128];
} wifi2agps_location_data;

typedef struct {
    wifi2agps_loc_encode_descriptor_e loc_encode_descriptor;
    wifi2agps_location_data location_data;
} wifi2agps_reported_location;

typedef struct {
    char ap_mac_addr[6];
    
    char ap_transmit_power_used;
    int ap_transmit_power;  //-127..128
    
    char ap_antenna_gain_used;
    int ap_antenna_gain;    //-127..128

    char ap_signal_noise_used;
    int ap_signal_noise;    //-127..128

    char ap_device_type_used;
    wifi2agps_ap_device_type_e ap_device_type;

    char ap_signal_strength_used;
    int ap_signal_strength; //-127..128

    char ap_channel_frequency_used;
    int ap_channel_frequency;   //0..256

    char ap_round_trip_delay_used;
    wifi2agps_rtd ap_round_trip_delay;

    char set_transmit_power_used;
    int set_transmit_power; //-127..128

    char set_antenna_gain_used;
    int set_antenna_gain;   //-127..128

    char set_signal_to_noise_used;
    int set_signal_to_noise;    //-127..128

    char set_signal_strength_used;
    int set_signal_strength;    //-127..128

    char reported_location_used;
    wifi2agps_reported_location reported_location;
} wifi2agps_ap_info;
//sizeof(wifi2agps_ap_info)=248

typedef struct {
    char num;   //1..32
    wifi2agps_ap_info list[WIFI_AP_LIST_NUM_MAX];
} wifi2agps_ap_info_list;
//sizeof(wifi2agps_ap_info_list)=7940

//-1 means failure
int wifi2agps_enabled();

//-1 means failure
int wifi2agps_disabled();

//-1 means failure
int wifi2agps_associated(wifi2agps_ap_info* ap_info);

//-1 means failure
int wifi2agps_disassociated();

//-1 means failure
int wifi2agps_scanned(wifi2agps_ap_info_list* ap_info_list);

void dump_ap_info(wifi2agps_ap_info* ap_info);
void dump_ap_info_list(wifi2agps_ap_info_list* ap_info_list);

#endif
