#ifndef _WIFI2AGPS_NL80211_H_
#define _WIFI2AGPS_NL80211_H_
#include "common.h"
#include "interface/wifi2agps_interface.h"

struct wifi_callback_funcs {
	int (*wifi_enabled)();
	int (*wifi_disabled)();
	int (*wifi_associated)(wifi2agps_ap_info* ap_info);
	int (*wifi_disassociated)();
	int (*wifi_scan_results)(wifi2agps_ap_info_list* ap_info_list);
};

int wifi2apgs_deinit();
int wifi2agps_init(struct wifi_callback_funcs *callback);
#endif
