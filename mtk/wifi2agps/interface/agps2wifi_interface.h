#ifndef __AGPS2WIFI_INTERFACE_H__
#define __AGPS2WIFI_INTERFACE_H__

#include "wifi2agps_interface.h"

typedef struct {
    void (*wifi_enabled)();
    void (*wifi_disabled)();
    void (*wifi_associated)(wifi2agps_ap_info* ap_info);
    void (*wifi_disassociated)();
    void (*wifi_scanned)(wifi2agps_ap_info_list* ap_info_list);
} wifi2agpsInterface;

//-1 means failure
int wifi2agps_handler(int fd, wifi2agpsInterface* agps_interface);
int create_wifi2agps_fd();

#endif
