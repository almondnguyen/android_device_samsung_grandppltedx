#ifndef __MNL_FLP_TEST_INTERFACE_H__
#define __MNL_FLP_TEST_INTERFACE_H__

#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <sys/socket.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    void (*flp_test_gps_start)();
    void (*flp_test_gps_stop)();
    int (*flp_test_lpbk_start)();
    int (*flp_test_lpbk_stop)();
} flp_test2mnl_interface;

//======================================================
// GPS FLP test -> MNLD
//======================================================
#define MTK_FLP_TEST2MNL "mtk_flp_test2mnl"

int flp_test2mnl_hdlr(int fd, flp_test2mnl_interface* hdlr);
int create_flp_test2mnl_fd(void);
void flp_mnl_emi_download(void);

#ifdef __cplusplus
}
#endif

#endif

