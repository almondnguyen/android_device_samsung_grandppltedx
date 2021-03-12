#ifndef __MTK_LBS_UTILITY_H__
#define __MTK_LBS_UTILITY_H__

#include <time.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*************************************************
* Basic Utilities
**************************************************/
#ifndef UNUSED
#define UNUSED(x) (x)=(x)
#endif

void tag_log(int type, const char* tag, const char *fmt, ...);

#ifdef LOGD
#undef LOGD
#endif
#ifdef LOGW
#undef LOGW
#endif
#ifdef LOGE
#undef LOGE
#endif
#define LOGD(...) tag_log(1, "[gpshal][Default]", __VA_ARGS__);
#define LOGW(...) tag_log(1, "[gpshal] WARNING: [Default]", __VA_ARGS__);
#define LOGE(...) tag_log(1, "[gpshal] ERR: [Default]", __VA_ARGS__);
void msleep(int interval);

// in millisecond
time_t get_tick();

// in millisecond
time_t get_time_in_millisecond();

/*************************************************
* Epoll
**************************************************/
// -1 means failure
int epoll_add_fd(int epfd, int fd);

// -1 failed
int epoll_add_fd2(int epfd, int fd, uint32_t events);

// -1 failed
int epoll_del_fd(int epfd, int fd);

int epoll_mod_fd(int epfd, int fd, uint32_t events);

/*************************************************
* Local UDP Socket
**************************************************/
// -1 means failure
int socket_bind_udp(const char* path);

// -1 means failure
int socket_set_blocking(int fd, int blocking);

// -1 means failure
int safe_sendto(const char* path, const char* buff, int len);

// -1 means failure
int safe_recvfrom(int fd, char* buff, int len);

#ifdef __cplusplus
}
#endif

#endif
