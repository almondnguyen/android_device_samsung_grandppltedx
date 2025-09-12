#ifndef __unused
#define __unused  __attribute__((__unused__))
#endif

int ifc_set_throttle(const char *ifname __unused, int rxKbps __unused, int txKbps __unused) {
    return 0;
}

int ifc_set_txq_state(const char *ifname __unused, int state __unused) {
    return 0;
};
