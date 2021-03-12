#ifndef __GPS_CONTROLER_H__
#define __GPS_CONTROLER_H__

// #include <time.h>
// #include <stdint.h>
#include <stdbool.h>


#ifdef __cplusplus
extern "C" {
#endif

int gps_control_init();

int gps_control_gps_start(int delete_aiding_data_flags);
int gps_control_gps_stop();
int gps_control_gps_reset(int delete_aiding_data_flags);

bool mnl_offload_is_enabled(void);
void mnl_offload_set_enabled(void);
void mnl_offload_clear_enabled(void);
int mnl_init(void);
void linux_signal_handler(int signo);
int mnld_factory_test_entry(int argc, char** argv);
int linux_setup_signal_handler(void);

void gps_controller_agps_session_done(void);
void gps_controller_rcv_pmtk(const char* pmtk);

void get_chip_sv_support_capability(unsigned char* sv_type);

int gps_driver_state_init(void);
void gps_control_mnl_set_status(void);
void gps_control_mnl_set_pwrctl(void);
#if ANDROID_MNLD_PROP_SUPPORT
int get_gps_cmcc_log_enabled();
#endif
#ifdef __cplusplus
}
#endif

#endif


