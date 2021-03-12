#include <stdlib.h>
#include <unistd.h>
#include "nl80211.h"
#include "event_loop.h"

//#define CONFIG_DEBUG_WITH_SHELL
int main(int argc, char **argv) {
	struct wifi_callback_funcs wifi2agps_cb;
	/* if it is not called by init, do nothing*/
#ifndef CONFIG_DEBUG_WITH_SHELL
	if (getppid() != 1)
		return 1;
#endif
	wifi2agps_cb.wifi_enabled = wifi2agps_enabled;
	wifi2agps_cb.wifi_disabled = wifi2agps_disabled;
	wifi2agps_cb.wifi_associated = wifi2agps_associated;
	wifi2agps_cb.wifi_disassociated = wifi2agps_disassociated;
	wifi2agps_cb.wifi_scan_results = wifi2agps_scanned;
	wifi2agps_init(&wifi2agps_cb);
	event_loop_run();
	return 0;
}
