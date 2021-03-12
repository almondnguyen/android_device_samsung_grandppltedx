#ifndef __EPO_H__
#define __EPO_H__

#include "curl.h"
#ifdef __cplusplus
extern "C" {
#endif

#define EPO_FILE                    "/data/misc/gps/EPO.DAT"
#define EPO_UPDATE_FILE             "/data/misc/gps/EPOTMP.DAT"
#define EPO_UPDATE_HAL              "/data/misc/gps/EPOHAL.DAT"

extern char* getEpoUrl(char* filename, char* key);

int epo_downloader_init();
int epo_read_cust_config();
int epo_downloader_is_file_invalid();
int epo_downloader_start();
void epo_update_epo_file();
int epo_is_wifi_trigger_enabled();
int epo_is_epo_download_enabled();
CURLcode curl_easy_download(char* url, char* filename);
int mtk_gps_sys_epo_period_start(int fd, unsigned int* u4GpsSecs, time_t* uSecond);
void GpsToUtcTime(int i2Wn, double dfTow, time_t* uSecond);

#ifdef __cplusplus
}
#endif

#endif



