#ifndef __QEPO_H__
#define __QEPO_H__


#ifdef __cplusplus
extern "C" {
#endif

#define QEPO_FILE                    "/data/misc/gps/QEPO.DAT"
#define QEPO_UPDATE_FILE             "/data/misc/gps/QEPOTMP.DAT"
#define QEPO_UPDATE_HAL             "/data/misc/gps/QEPOHAL.DAT"


int qepo_downloader_init();

int qepo_downloader_start();

int is_qepo_download_finished();

void qepo_update_quarter_epo_file(int qepo_valid);

void gps_mnl_set_gps_time(int wn, int tow, int sys_time);

#ifdef __cplusplus
}
#endif

#endif



