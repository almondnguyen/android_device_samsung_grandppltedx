#include <pthread.h>

pid_t __pthread_gettid(pthread_t t) {
    return pthread_gettid_np(t);
}
