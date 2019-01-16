#include <stdint.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/ctrl.h>
#include <linux/rtnetlink.h>
#include <netpacket/packet.h>
#include <linux/filter.h>
#include <linux/errqueue.h>
#include <errno.h>
#include <string.h>

#include <linux/pkt_sched.h>
#include <netlink/object-api.h>
#include <netlink/netlink.h>
#include <netlink/socket.h>
#include <netlink/attr.h>
#include <netlink/handlers.h>
#include <netlink/msg.h>

#include <dirent.h>
#include <net/if.h>

#include "sync.h"

#define LOG_TAG  "WifiHAL"

#include <utils/Log.h>

#include "wifi_hal.h"
#include "common.h"
#include "cpp_bindings.h"
#include "cutils/properties.h"

/*
 BUGBUG: normally, libnl allocates ports for all connections it makes; but
 being a static library, it doesn't really know how many other netlink connections
 are made by the same process, if connections come from different shared libraries.
 These port assignments exist to solve that problem - temporarily. We need to fix
 libnl to try and allocate ports across the entire process.
 */

#define FEATURE_SET                  0
#define FEATURE_SET_MATRIX           1

typedef enum {
    WIFI_SUBCMD_GET_CHANNEL_LIST = ANDROID_NL80211_SUBCMD_WIFI_RANGE_START,

    WIFI_SUBCMD_GET_FEATURE_SET,                      /* 0x0002 */
    WIFI_SUBCMD_GET_FEATURE_SET_MATRIX,               /* 0x0003 */
    WIFI_SUBCMD_SET_PNO_RANDOM_MAC_OUI,               /* 0x0004 */
    WIFI_SUBCMD_NODFS_SET,                            /* 0x0005 */
    WIFI_SUBCMD_SET_COUNTRY_CODE,                     /* 0x0006 */

    WIFI_SUBCMD_SET_RSSI_MONITOR,                     /* 0x0007 */
    /* Add more sub commands here */

} WIFI_SUB_COMMAND;

typedef enum {
    WIFI_ATTRIBUTE_BAND = 1,
    WIFI_ATTRIBUTE_NUM_CHANNELS,
    WIFI_ATTRIBUTE_CHANNEL_LIST,

    WIFI_ATTRIBUTE_NUM_FEATURE_SET,
    WIFI_ATTRIBUTE_FEATURE_SET,
    WIFI_ATTRIBUTE_PNO_RANDOM_MAC_OUI,
    WIFI_ATTRIBUTE_NODFS_VALUE,
    WIFI_ATTRIBUTE_COUNTRY_CODE,

    WIFI_ATTRIBUTE_MAX_RSSI,
    WIFI_ATTRIBUTE_MIN_RSSI,
    WIFI_ATTRIBUTE_RSSI_MONITOR_START

} WIFI_ATTRIBUTE;


#define WIFI_HAL_CMD_SOCK_PORT       644
#define WIFI_HAL_EVENT_SOCK_PORT     645

static int internal_no_seq_check(nl_msg *msg, void *arg);
static int internal_valid_message_handler(nl_msg *msg, void *arg);
static int wifi_get_multicast_id(wifi_handle handle, const char *name, const char *group);
static wifi_error wifi_init_interfaces(wifi_handle handle);
static void wifi_internal_cleanup(wifi_handle handle);
static wifi_error wifi_start_rssi_monitoring(wifi_request_id id, wifi_interface_handle
                        iface, s8 max_rssi, s8 min_rssi, wifi_rssi_event_handler eh);
static wifi_error wifi_stop_rssi_monitoring(wifi_request_id id, wifi_interface_handle iface);

/*****************************************************************************
* socket pair to wake up wifi_event_loop from a blocking poll for termination
*   - exit_sockets[0]: write socket, trigger from wifi_cleanup
*   - exit_sockets[1]: read socket, monitored in poll of wifi_event_loop
*****************************************************************************/
static int exit_sockets[2] = {-1, -1};


/////////////////////////////////////////////////////////////////////////
/* Initialize vendor function pointer table with MTK HAL API */
wifi_error init_wifi_vendor_hal_func_table(wifi_hal_fn *fn)
{
    if (fn == NULL) {
        return WIFI_ERROR_UNKNOWN;
    }
    fn->wifi_initialize = wifi_initialize;
    fn->wifi_cleanup = wifi_cleanup;
    fn->wifi_event_loop = wifi_event_loop;
    fn->wifi_get_supported_feature_set = wifi_get_supported_feature_set;
    fn->wifi_get_concurrency_matrix = wifi_get_concurrency_matrix;
    fn->wifi_set_scanning_mac_oui =  wifi_set_scanning_mac_oui;
    fn->wifi_get_ifaces = wifi_get_ifaces;
    fn->wifi_get_iface_name = wifi_get_iface_name;
    fn->wifi_start_gscan = wifi_start_gscan;
    fn->wifi_stop_gscan = wifi_stop_gscan;
    fn->wifi_get_cached_gscan_results = wifi_get_cached_gscan_results;
    fn->wifi_set_bssid_hotlist = wifi_set_bssid_hotlist;
    fn->wifi_reset_bssid_hotlist = wifi_reset_bssid_hotlist;
    fn->wifi_set_significant_change_handler = wifi_set_significant_change_handler;
    fn->wifi_reset_significant_change_handler = wifi_reset_significant_change_handler;
    fn->wifi_get_gscan_capabilities = wifi_get_gscan_capabilities;
    fn->wifi_get_link_stats = wifi_get_link_stats;
    fn->wifi_get_valid_channels = wifi_get_valid_channels;
    fn->wifi_rtt_range_request = wifi_rtt_range_request;
    fn->wifi_rtt_range_cancel = wifi_rtt_range_cancel;
    fn->wifi_get_rtt_capabilities = wifi_get_rtt_capabilities;
    fn->wifi_set_nodfs_flag = wifi_set_nodfs_flag;
    //fn->wifi_start_logging = wifi_start_logging;
    fn->wifi_set_epno_list = wifi_set_epno_list;
    fn->wifi_set_country_code = wifi_set_country_code;
    /*fn->wifi_get_firmware_memory_dump = wifi_get_firmware_memory_dump;
    fn->wifi_set_log_handler = wifi_set_log_handler;
    fn->wifi_reset_log_handler = wifi_reset_log_handler;
    fn->wifi_set_alert_handler = wifi_set_alert_handler;
    fn->wifi_get_firmware_version = wifi_get_firmware_version;
    fn->wifi_get_ring_buffers_status = wifi_get_ring_buffers_status;
    fn->wifi_get_logger_supported_feature_set = wifi_get_logger_supported_feature_set;
    fn->wifi_get_ring_data = wifi_get_ring_data;
    fn->wifi_get_driver_version = wifi_get_driver_version;*/
    fn->wifi_set_bssid_blacklist = wifi_set_bssid_blacklist;
    fn->wifi_start_rssi_monitoring = wifi_start_rssi_monitoring;
    fn->wifi_stop_rssi_monitoring = wifi_stop_rssi_monitoring;
    fn->wifi_start_sending_offloaded_packet = wifi_start_sending_offloaded_packet;
    fn->wifi_stop_sending_offloaded_packet = wifi_stop_sending_offloaded_packet;
    return WIFI_SUCCESS;
}

void wifi_socket_set_local_port(struct nl_sock *sock, uint32_t port)
{
    uint32_t pid = getpid() & 0x3FFFFF;
    nl_socket_set_local_port(sock, pid + (port << 22));
}

static nl_sock *wifi_create_nl_socket(int port)
{
    struct nl_sock *sock;

    // ALOGD("Creating netlink socket, local port[%d]", port);
    sock = nl_socket_alloc();
    if (sock == NULL) {
        ALOGE("Could not create netlink socket: %s(%d)", strerror(errno), errno);
        return NULL;
    }

    wifi_socket_set_local_port(sock, port);

    // ALOGD("Connecting to socket");
    if (nl_connect(sock, NETLINK_GENERIC)) {
        ALOGE("Could not connect to netlink socket: %s(%d)", strerror(errno), errno);
        nl_socket_free(sock);
        return NULL;
    }

    return sock;
}

static int wifi_add_membership(wifi_handle handle, const char *group)
{
    hal_info *info = getHalInfo(handle);

    int id = wifi_get_multicast_id(handle, "nl80211", group);
    if (id < 0) {
        ALOGE("Could not find group %s", group);
        return id;
    }

    int ret = nl_socket_add_membership(info->event_sock, id);
    if (ret < 0) {
        ALOGE("Could not add membership to group %s", group);
    }

    ALOGD("Add membership for group %s successfully", group);
    return ret;
}

wifi_error wifi_initialize(wifi_handle *handle)
{
    ALOGI("Wifi HAL initializing");

    hal_info *info = (hal_info *)malloc(sizeof(hal_info));
    if (info == NULL) {
        ALOGE("Could not allocate hal_info");
        return WIFI_ERROR_OUT_OF_MEMORY;
    }

    memset(info, 0, sizeof(*info));
    *handle = (wifi_handle)info;

    info->cmd_sock = wifi_create_nl_socket(WIFI_HAL_CMD_SOCK_PORT);
    if (info->cmd_sock == NULL) {
        ALOGE("Could not create command socket");
        wifi_internal_cleanup(*handle);
        *handle = NULL;
        return WIFI_ERROR_UNKNOWN;
    }

    info->event_sock = wifi_create_nl_socket(WIFI_HAL_EVENT_SOCK_PORT);
    if (info->event_sock == NULL) {
        ALOGE("Could not create event socket");
        wifi_internal_cleanup(*handle);
        *handle = NULL;
        return WIFI_ERROR_UNKNOWN;
    }

    struct nl_cb *cb = nl_socket_get_cb(info->event_sock);
    // ALOGI("cb->refcnt = %d", cb->cb_refcnt);
    nl_cb_set(cb, NL_CB_SEQ_CHECK, NL_CB_CUSTOM, internal_no_seq_check, info);
    nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, internal_valid_message_handler, info);
    nl_cb_put(cb);

    info->nl80211_family_id = genl_ctrl_resolve(info->cmd_sock, "nl80211");
    if (info->nl80211_family_id < 0) {
        ALOGE("Could not resolve nl80211 family id");
        wifi_internal_cleanup(*handle);
        *handle = NULL;
        return WIFI_ERROR_UNKNOWN;
    }

    info->clean_up = false;
    info->in_event_loop = false;

    info->event_cb = (cb_info *)malloc(sizeof(cb_info) * DEFAULT_EVENT_CB_SIZE);
    info->alloc_event_cb = DEFAULT_EVENT_CB_SIZE;
    info->num_event_cb = 0;
    if (info->event_cb == NULL) {
        ALOGE("Could not allocate cb_info array");
        wifi_internal_cleanup(*handle);
        *handle = NULL;
        return WIFI_ERROR_OUT_OF_MEMORY;
    }

    info->cmd = (cmd_info *)malloc(sizeof(cmd_info) * DEFAULT_CMD_SIZE);
    info->alloc_cmd = DEFAULT_CMD_SIZE;
    info->num_cmd = 0;
    if (info->cmd == NULL) {
        ALOGE("Could not allocate cmd_info array");
        wifi_internal_cleanup(*handle);
        *handle = NULL;
        return WIFI_ERROR_OUT_OF_MEMORY;
    }

    pthread_mutex_init(&info->cb_lock, NULL);

    wifi_add_membership(*handle, "scan");
    wifi_add_membership(*handle, "mlme");
    wifi_add_membership(*handle, "regulatory");
    wifi_add_membership(*handle, "vendor");

    wifi_init_interfaces(*handle);
    ALOGD("Found %d interfaces", info->num_interfaces);

    ALOGD("Wifi HAL initialized successfully: nl80211_family_id=%d", info->nl80211_family_id);
    return WIFI_SUCCESS;
}

/////////////////////////////////////////////////////////////////////////

static bool is_wifi_interface(const char *name)
{
    if (strncmp(name, "wlan", 4) != 0 && strncmp(name, "p2p", 3) != 0) {
        /* Not a wifi interface; ignore it */
        return false;
    } else {
        return true;
    }
}

int get_interface(const char *name, interface_info *info)
{
    strcpy(info->name, name);
    info->id = if_nametoindex(name);
    ALOGD("found an interface : %s, id = %d", name, info->id);
    return WIFI_SUCCESS;
}

wifi_error wifi_init_interfaces(wifi_handle handle)
{
    hal_info *info = (hal_info *)handle;
    struct dirent *de;

    DIR *d = opendir("/sys/class/net");
    if (d == 0)
        return WIFI_ERROR_UNKNOWN;

    int n = 0;
    while ((de = readdir(d))) {
        if (de->d_name[0] == '.')
            continue;
        if (is_wifi_interface(de->d_name) ) {
            n++;
        }
    }

    closedir(d);

    d = opendir("/sys/class/net");
    if (d == 0)
        return WIFI_ERROR_UNKNOWN;

    info->interfaces = (interface_info **)malloc(sizeof(interface_info *) * n);
    if (!info->interfaces) {
        closedir(d);
        return WIFI_ERROR_OUT_OF_MEMORY;
    }

    int i = 0;
    while ((de = readdir(d))) {
        if (de->d_name[0] == '.')
            continue;
        if (is_wifi_interface(de->d_name) && i < n) {
            interface_info *ifinfo = (interface_info *)malloc(sizeof(interface_info));
            if (!ifinfo)
                continue;
            if (get_interface(de->d_name, ifinfo) != WIFI_SUCCESS) {
                free(ifinfo);
                continue;
            }
            ifinfo->handle = handle;
            info->interfaces[i] = ifinfo;
            i++;
        }
    }

    closedir(d);

    info->num_interfaces = (i < n) ? i : n;
    return WIFI_SUCCESS;
}

wifi_error wifi_get_ifaces(wifi_handle handle, int *num, wifi_interface_handle **interfaces)
{
    hal_info *info = (hal_info *)handle;

    *interfaces = (wifi_interface_handle *)info->interfaces;
    *num = info->num_interfaces;

    return WIFI_SUCCESS;
}

wifi_error wifi_get_iface_name(wifi_interface_handle handle, char *name, size_t size)
{
    interface_info *info = (interface_info *)handle;
    strcpy(name, info->name);
    return WIFI_SUCCESS;
}

/////////////////////////////////////////////////////////////////////////

static void wifi_internal_cleanup(wifi_handle handle)
{
    hal_info *info = getHalInfo(handle);
    if (info == NULL)
        return;

    if (info->interfaces) {
        int i = 0;
        for (; i < info->num_interfaces; i++)
            free(info->interfaces[i]);
        free(info->interfaces);
    }

    pthread_mutex_destroy(&info->cb_lock);

    if (info->event_cb)
        free(info->event_cb);

    if (info->cmd)
        free(info->cmd);

    if (info->cmd_sock)
        nl_socket_free(info->cmd_sock);

    if (info->event_sock)
        nl_socket_free(info->event_sock);

    if (info->cleaned_up_handler)
        info->cleaned_up_handler(handle);

    free(info);

    if (exit_sockets[0] != -1) {
        close(exit_sockets[0]);
        exit_sockets[0] = -1;
    }
    if (exit_sockets[1] != -1) {
        close(exit_sockets[1]);
        exit_sockets[1] = -1;
    }

    ALOGD("Internal cleanup completed");
    return;
}

void wifi_cleanup(wifi_handle handle, wifi_cleaned_up_handler handler)
{
    hal_info *info = getHalInfo(handle);
    info->cleaned_up_handler = handler;
    info->clean_up = true;

    if (info->in_event_loop && exit_sockets[0] != -1) {
        char sig = 'T';
        write(exit_sockets[0], &sig, sizeof(sig));
        ALOGD("Signal wifi_event_loop to exit");
    } else {
        wifi_internal_cleanup(handle);
    }
}

/////////////////////////////////////////////////////////////////////////

static int internal_event_handler(wifi_handle handle)
{
    hal_info *info = getHalInfo(handle);
    struct nl_cb *cb = nl_socket_get_cb(info->event_sock);
    int res = nl_recvmsgs(info->event_sock, cb);
    // ALOGD("nl_recvmsgs returned %d", res);
    nl_cb_put(cb);
    return res;
}

void wifi_event_loop(wifi_handle handle)
{
    hal_info *info = getHalInfo(handle);
    if (info->in_event_loop)
        return;

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, exit_sockets) < 0) {
        ALOGE("Create socketpair failed, errno %d", errno);
        return;
    }

    info->in_event_loop = true;

    struct pollfd fds[2];
    /* TODO: Add support for timeout */
    int timeout = -1; /* Infinite timeout */

    do {
        fds[0].fd = exit_sockets[1];
        fds[0].events = POLLIN;
        fds[0].revents = 0;
        fds[1].fd = nl_socket_get_fd(info->event_sock);
        fds[1].events = POLLIN;
        fds[1].revents = 0;

        int result = poll(fds, 2, -1);
        if (result < 0) {
            ALOGE("wifi_event_loop: poll error result=%d, errno=%s(%d)", result, strerror(errno), errno);
            if (errno == EINTR) /* ignore EINTR */
                continue;
            break;
        } else if (fds[0].revents & POLLIN) {
            char sig;
            int sz;
            sz = read(fds[0].fd, &sig, sizeof(sig));
            if (sz == -1) {
                ALOGE("read fail errno=%s(%d)\n", strerror(errno), errno);
                break;
            }
            if (sig == 'T') {
                ALOGD("Wifi HAL stopped!!!");
                break;
            }
        } else if (fds[1].revents & POLLIN) {
            // ALOGI("Receive HAL events");
            internal_event_handler(handle);
        } else {
            ALOGE("Unknown returned event");
        }
    } while (!info->clean_up);

    info->in_event_loop = false;

    if (info->clean_up)
        wifi_internal_cleanup(handle);

    ALOGD("Leaving wifi_event_loop");
}

static int internal_no_seq_check(struct nl_msg *msg, void *arg)
{
    return NL_OK;
}

static int internal_valid_message_handler(nl_msg *msg, void *arg)
{
    wifi_handle handle = (wifi_handle)arg;
    hal_info *info = getHalInfo(handle);

    WifiEvent event(msg);
    int res = event.parse();
    if (res < 0) {
        ALOGE("Failed to parse event: %d", res);
        return NL_SKIP;
    }

    int cmd = event.get_cmd();
    uint32_t vendor_id = 0;
    int subcmd = 0;

    if (cmd == NL80211_CMD_VENDOR) {
        vendor_id = event.get_u32(NL80211_ATTR_VENDOR_ID);
        subcmd = event.get_u32(NL80211_ATTR_VENDOR_SUBCMD);
        ALOGV("event received %s, vendor_id = 0x%0x, subcmd = 0x%0x",
                event.get_cmdString(), vendor_id, subcmd);
    } else {
        ALOGV("event received %s", event.get_cmdString());
    }

    bool dispatched = false;

    pthread_mutex_lock(&info->cb_lock);

    for (int i = 0; i < info->num_event_cb; i++) {
        if (cmd == info->event_cb[i].nl_cmd) {
            if (cmd == NL80211_CMD_VENDOR
                && ((vendor_id != info->event_cb[i].vendor_id)
                || (subcmd != info->event_cb[i].vendor_subcmd)))
            {
                /* event for a different vendor, ignore it */
                continue;
            }

            cb_info *cbi = &(info->event_cb[i]);
            nl_recvmsg_msg_cb_t cb_func = cbi->cb_func;
            void *cb_arg = cbi->cb_arg;
            WifiCommand *cmd = (WifiCommand *)cbi->cb_arg;
            if (cmd != NULL) {
                cmd->addRef();
            }

            pthread_mutex_unlock(&info->cb_lock);

            (*cb_func)(msg, cb_arg);
            if (cmd != NULL) {
                cmd->releaseRef();
            }

            return NL_OK;
        }
    }

    pthread_mutex_unlock(&info->cb_lock);
    return NL_OK;
}

///////////////////////////////////////////////////////////////////////////////////

class GetMulticastIdCommand : public WifiCommand
{
private:
    const char *mName;
    const char *mGroup;
    int   mId;

public:
    GetMulticastIdCommand(wifi_handle handle, const char *name, const char *group)
        : WifiCommand(handle, 0)
    {
        mName = name;
        mGroup = group;
        mId = -1;
    }

    int getId() {
        return mId;
    }

    virtual int create() {
        int nlctrlFamily = genl_ctrl_resolve(mInfo->cmd_sock, "nlctrl");
        // ALOGD("ctrl family = %d", nlctrlFamily);
        int ret = mMsg.create(nlctrlFamily, CTRL_CMD_GETFAMILY, 0, 0);
        if (ret < 0) {
            return ret;
        }
        ret = mMsg.put_string(CTRL_ATTR_FAMILY_NAME, mName);
        return ret;
    }

    virtual int handleResponse(WifiEvent& reply) {

        // ALOGD("handling reponse in %s", __func__);

        struct nlattr **tb = reply.attributes();
        struct genlmsghdr *gnlh = reply.header();
        struct nlattr *mcgrp = NULL;
        int i;

        if (!tb[CTRL_ATTR_MCAST_GROUPS]) {
            ALOGD("No multicast groups found");
            return NL_SKIP;
        } else {
            // ALOGD("Multicast groups attr size = %d", nla_len(tb[CTRL_ATTR_MCAST_GROUPS]));
        }

        for_each_attr(mcgrp, tb[CTRL_ATTR_MCAST_GROUPS], i) {

            // ALOGD("Processing group");
            struct nlattr *tb2[CTRL_ATTR_MCAST_GRP_MAX + 1];
            nla_parse(tb2, CTRL_ATTR_MCAST_GRP_MAX, (nlattr *)nla_data(mcgrp),
                nla_len(mcgrp), NULL);
            if (!tb2[CTRL_ATTR_MCAST_GRP_NAME] || !tb2[CTRL_ATTR_MCAST_GRP_ID]) {
                continue;
            }

            char *grpName = (char *)nla_data(tb2[CTRL_ATTR_MCAST_GRP_NAME]);
            int grpNameLen = nla_len(tb2[CTRL_ATTR_MCAST_GRP_NAME]);

            // ALOGD("Found group name %s", grpName);

            if (strncmp(grpName, mGroup, grpNameLen) != 0)
                continue;

            mId = nla_get_u32(tb2[CTRL_ATTR_MCAST_GRP_ID]);
            break;
        }

        return NL_SKIP;
    }
};

static int wifi_get_multicast_id(wifi_handle handle, const char *name, const char *group)
{
    GetMulticastIdCommand cmd(handle, name, group);
    int res = cmd.requestResponse();
    if (res < 0)
        return res;
    else
        return cmd.getId();
}

class GetChannelListCommand : public WifiCommand
{
private:
    wifi_channel *mChannels;
    int mMaxChannels;
    int *mNumOfChannel;
    int mBand;

public:
    GetChannelListCommand(wifi_interface_handle handle, int band, int max_channels,
        wifi_channel *channels, int *num_channels)
        : WifiCommand(handle, 0)
    {
        mBand = band;
        mMaxChannels = max_channels;
        mChannels = channels;
        mNumOfChannel = num_channels;
        memset(mChannels, 0, sizeof(wifi_channel) * mMaxChannels);
    }

    virtual int create() {
        int ret;

        ret = mMsg.create(GOOGLE_OUI, WIFI_SUBCMD_GET_CHANNEL_LIST);
        if (ret < 0) {
            return ret;
        }

        ALOGI("In GetChannelList::mBand=%d", mBand);

        nlattr *data = mMsg.attr_start(NL80211_ATTR_VENDOR_DATA);
        ret = mMsg.put_u32(WIFI_ATTRIBUTE_BAND, mBand);
        if (ret < 0) {
            return ret;
        }

        mMsg.attr_end(data);
        return WIFI_SUCCESS;
    }

protected:
    virtual int handleResponse(WifiEvent& reply) {

        ALOGV("In GetChannelList::handleResponse");

        if (reply.get_cmd() != NL80211_CMD_VENDOR) {
            ALOGE("Ignore reply with cmd 0x%x", reply.get_cmd());
            return NL_SKIP;
        }

        int vendor_id = reply.get_vendor_id();
        int subcmd = reply.get_vendor_subcmd();
        ALOGV("vendor_id = 0x%x, subcmd = 0x%x", vendor_id, subcmd);

        nlattr *vendor = reply.get_attribute(NL80211_ATTR_VENDOR_DATA);
        int len = reply.get_vendor_data_len();
        if (vendor == NULL || len == 0) {
            ALOGE("No vendor data in GetChannelList response, ignore it");
            return NL_SKIP;
        }

        int num_channels = 0;
        for (nl_iterator it(vendor); it.has_next(); it.next()) {
            if (it.get_type() == WIFI_ATTRIBUTE_NUM_CHANNELS) {
                num_channels = it.get_u32();
                ALOGI("Get channel list with %d channels", num_channels);
                if (num_channels > mMaxChannels)
                    num_channels = mMaxChannels;
                *mNumOfChannel = num_channels;
            } else if (it.get_type() == WIFI_ATTRIBUTE_CHANNEL_LIST && num_channels) {
                memcpy(mChannels, it.get_data(), sizeof(wifi_channel) * num_channels);
            } else {
                ALOGW("Ignore invalid attribute type = %d, size = %d",
                        it.get_type(), it.get_len());
            }
        }

        ALOGD("mChannels[0]=%d mChannels[1]=%d", *mChannels, *(mChannels + 1));

        return NL_OK;
    }
};

wifi_error wifi_get_valid_channels(wifi_interface_handle handle,
        int band, int max_channels, wifi_channel *channels, int *num_channels)
{
    GetChannelListCommand command(handle, band, max_channels, channels, num_channels);
    return (wifi_error) command.requestResponse();
}

class GetFeatureSetCommand : public WifiCommand
{
private:
    int feature_type;
    feature_set *fset;
    feature_set *feature_matrix;
    int *fm_size;
    int set_size_max;

public:
    GetFeatureSetCommand(wifi_interface_handle handle, int feature, feature_set *set,
         feature_set set_matrix[], int *size, int max_size)
        : WifiCommand(handle, 0)
    {
        feature_type = feature;
        fset = set;
        feature_matrix = set_matrix;
        fm_size = size;
        set_size_max = max_size;
    }

    virtual int create() {
        int ret;

        if(feature_type == FEATURE_SET) {
            ret = mMsg.create(GOOGLE_OUI, WIFI_SUBCMD_GET_FEATURE_SET);
        } else if (feature_type == FEATURE_SET_MATRIX) {
            ret = mMsg.create(GOOGLE_OUI, WIFI_SUBCMD_GET_FEATURE_SET_MATRIX);
        } else {
            ALOGE("Unknown feature type %d", feature_type);
            return -1;
        }

        if (ret < 0) {
            ALOGE("Can't create subcmd message to driver, ret=%d", ret);
        }

        return ret;
    }

protected:
    virtual int handleResponse(WifiEvent& reply) {

        ALOGD("In GetFeatureSetCommand::handleResponse");

        if (reply.get_cmd() != NL80211_CMD_VENDOR) {
            ALOGD("Ignore reply with cmd 0x%x", reply.get_cmd());
            return NL_SKIP;
        }

        int vendor_id = reply.get_vendor_id();
        int subcmd = reply.get_vendor_subcmd();
        ALOGD("vendor_id = 0x%x, subcmd = 0x%x", vendor_id, subcmd);

        nlattr *vendor_data = reply.get_attribute(NL80211_ATTR_VENDOR_DATA);
        int len = reply.get_vendor_data_len();
        if (vendor_data == NULL || len == 0) {
            ALOGE("No vendor data in GetFeatureSetCommand response, ignore it");
            return NL_SKIP;
        }

        if (feature_type == FEATURE_SET) {
            void *data = reply.get_vendor_data();
            if (!fset) {
                ALOGE("feature_set pointer is not set");
                return NL_SKIP;
            }
            memcpy(fset, data, min(len, (int) sizeof(*fset)));
        }
        else {
            int num_features_set = 0;
            int i = 0;

            if(!feature_matrix || !fm_size) {
                ALOGE("feature_set pointer is not set");
                return NL_SKIP;
            }

            for (nl_iterator it(vendor_data); it.has_next(); it.next()) {
                if (it.get_type() == WIFI_ATTRIBUTE_NUM_FEATURE_SET) {
                    num_features_set = it.get_u32();
                    ALOGI("Get feature list with %d concurrent sets", num_features_set);
                    if(set_size_max && (num_features_set > set_size_max))
                        num_features_set = set_size_max;
                    *fm_size = num_features_set;
                } else if ((it.get_type() == WIFI_ATTRIBUTE_FEATURE_SET) &&
                             i < num_features_set) {
                    feature_matrix[i] = it.get_u32();
                    i++;
                } else {
                    ALOGW("Ignore invalid attribute type = %d, size = %d",
                            it.get_type(), it.get_len());
                }
            }
        }

        return NL_OK;
    }
};

wifi_error wifi_get_supported_feature_set(wifi_interface_handle handle, feature_set *pset)
{
#if 0
    GetFeatureSetCommand command(handle, FEATURE_SET, set, NULL, NULL, 1);
    return (wifi_error)command.requestResponse();
#else
    feature_set set = 0;
    char prop_buf[PROPERTY_VALUE_MAX];

    property_get("ro.wlan.mtk.wifi.5g", prop_buf, NULL);
    if (!strcmp(prop_buf, "1"))
        set |= WIFI_FEATURE_INFRA_5G;

    set |= WIFI_FEATURE_P2P;
    set |= WIFI_FEATURE_SOFT_AP;
    set |= WIFI_FEATURE_TDLS;

#ifdef CONFIG_PNO_SUPPORT
    set |= WIFI_FEATURE_PNO;
#endif

    memcpy(pset, &set, sizeof(feature_set));

    ALOGI("[WIFI HAL]wifi_get_supported_feature_set: handle=%p, feature_set=0x%x", handle, *pset);
    return WIFI_SUCCESS;
#endif
}

wifi_error wifi_get_concurrency_matrix(wifi_interface_handle handle, int set_size_max,
       feature_set set[], int *set_size)
{
    GetFeatureSetCommand command(handle, FEATURE_SET_MATRIX, NULL, set, set_size, set_size_max);
    return (wifi_error)command.requestResponse();
}

class SetPnoMacAddrOuiCommand : public WifiCommand
{
private:
    byte *mOui;
    feature_set *fset;
    feature_set *feature_matrix;
    int *fm_size;
    int set_size_max;

public:
    SetPnoMacAddrOuiCommand(wifi_interface_handle handle, oui scan_oui)
        : WifiCommand(handle, 0)
    {
        mOui = scan_oui;
    }

    int createRequest(WifiRequest& request, int subcmd, byte *scan_oui) {
        int result = request.create(GOOGLE_OUI, subcmd);
        if (result < 0) {
            return result;
        }

        nlattr *data = request.attr_start(NL80211_ATTR_VENDOR_DATA);
        result = request.put(WIFI_ATTRIBUTE_PNO_RANDOM_MAC_OUI, scan_oui, DOT11_OUI_LEN);
        if (result < 0) {
            return result;
        }

        request.attr_end(data);
        return WIFI_SUCCESS;

    }

    int start() {
        ALOGD("[WIFI HAL]Sending mac address OUI");
        WifiRequest request(familyId(), ifaceId());
        int result = createRequest(request, WIFI_SUBCMD_SET_PNO_RANDOM_MAC_OUI, mOui);
        if (result != WIFI_SUCCESS) {
            ALOGE("Failed to create request, result=%d", result);
            return result;
        }

        result = requestResponse(request);
        if (result != WIFI_SUCCESS) {
            ALOGE("[WIFI HAL]Failed to set scanning mac OUI, result=%d", result);
        }

        return result;
    }

protected:
    virtual int handleResponse(WifiEvent& reply) {
         ALOGD("Request complete!");
        /* Nothing to do on response! */
        return NL_SKIP;
    }
};

wifi_error wifi_set_scanning_mac_oui(wifi_interface_handle handle, oui scan_oui)
{
#if 0
    SetPnoMacAddrOuiCommand command(handle, scan_oui);
    return (wifi_error)command.start();
#else
    return WIFI_ERROR_NOT_SUPPORTED;
#endif
}

class SetNodfsCommand : public WifiCommand
{
private:
    u32 mNoDfs;

public:
    SetNodfsCommand(wifi_interface_handle handle, u32 nodfs)
        : WifiCommand(handle, 0)
    {
        mNoDfs = nodfs;
    }

    virtual int create() {
        int ret;

        ret = mMsg.create(GOOGLE_OUI, WIFI_SUBCMD_NODFS_SET);
        if (ret < 0) {
            ALOGE("Can't create subcmd message to driver, ret=%d", ret);
            return ret;
        }

        nlattr *data = mMsg.attr_start(NL80211_ATTR_VENDOR_DATA);
        ret = mMsg.put_u32(WIFI_ATTRIBUTE_NODFS_VALUE, mNoDfs);
        if (ret < 0) {
             return ret;
        }

        mMsg.attr_end(data);
        return WIFI_SUCCESS;
    }
};

wifi_error wifi_set_nodfs_flag(wifi_interface_handle handle, u32 nodfs)
{
#if 0
    SetNodfsCommand command(handle, nodfs);
    return (wifi_error)command.requestResponse();
#else
    return WIFI_ERROR_NOT_SUPPORTED;
#endif
}

class SetCountryCodeCommand : public WifiCommand
{
private:
    const char *mCountryCode;

public:
    SetCountryCodeCommand(wifi_interface_handle handle, const char *country_code)
        : WifiCommand(handle, 0)
    {
        mCountryCode = country_code;
    }

    virtual int create() {
        int ret;

        ret = mMsg.create(GOOGLE_OUI, WIFI_SUBCMD_SET_COUNTRY_CODE);
        if (ret < 0) {
             ALOGE("Can't create subcmd message to driver, ret=%d", ret);
             return ret;
        }

        nlattr *data = mMsg.attr_start(NL80211_ATTR_VENDOR_DATA);
        ret = mMsg.put_string(WIFI_ATTRIBUTE_COUNTRY_CODE, mCountryCode);
        if (ret < 0) {
            return ret;
        }

        mMsg.attr_end(data);
        return WIFI_SUCCESS;
    }
};

wifi_error wifi_set_country_code(wifi_interface_handle handle, const char *country_code)
{
    SetCountryCodeCommand command(handle, country_code);
    return (wifi_error) command.requestResponse();
}

class SetRSSIMonitorCommand : public WifiCommand
{
private:
    s8 mMax_rssi;
    s8 mMin_rssi;
    wifi_rssi_event_handler mHandler;

public:
    SetRSSIMonitorCommand(wifi_request_id id, wifi_interface_handle handle,
                s8 max_rssi, s8 min_rssi, wifi_rssi_event_handler eh)
        : WifiCommand(handle, id), mMax_rssi(max_rssi), mMin_rssi(min_rssi), mHandler(eh)
    {
    }

    int createRequest(WifiRequest& request, int enable) {
        int result = request.create(GOOGLE_OUI, WIFI_SUBCMD_SET_RSSI_MONITOR);
        if (result < 0) {
            return result;
        }

        ALOGI("set RSSI Monitor, mMax_rssi=%d, mMin_rssi=%d, enable=%d", mMax_rssi, mMin_rssi, enable);

        nlattr *data = request.attr_start(NL80211_ATTR_VENDOR_DATA);
        result = request.put_u32(WIFI_ATTRIBUTE_MAX_RSSI, (enable? mMax_rssi: 0));
        if (result < 0) {
            return result;
        }
        result = request.put_u32(WIFI_ATTRIBUTE_MIN_RSSI, (enable? mMin_rssi: 0));
        if (result < 0) {
            return result;
        }
        result = request.put_u32(WIFI_ATTRIBUTE_RSSI_MONITOR_START, enable);
        if (result < 0) {
            return result;
        }

        request.attr_end(data);
        return result;
    }

    int start() {
        WifiRequest request(familyId(), ifaceId());
        int result = createRequest(request, 1);
        if (result < 0) {
            return result;
        }

        result = requestResponse(request);
        if (result < 0) {
            ALOGE("Failed to set RSSI Monitor, result=%d", result);
            return result;
        }
        ALOGD("Successfully set RSSI monitoring");

        registerVendorHandler(GOOGLE_OUI, WIFI_EVENT_RSSI_MONITOR);

        if (result < 0) {
            unregisterVendorHandler(GOOGLE_OUI, WIFI_EVENT_RSSI_MONITOR);
            return result;
        }

        return result;
    }

    virtual int cancel() {
        WifiRequest request(familyId(), ifaceId());
        int result = createRequest(request, 0);
        if (result != WIFI_SUCCESS) {
            ALOGE("Failed to create request, result=%d", result);
        } else {
            result = requestResponse(request);
            if (result != WIFI_SUCCESS) {
                ALOGE("Failed to stop RSSI monitoring, result=%d", result);
            }
        }
        unregisterVendorHandler(GOOGLE_OUI, WIFI_EVENT_RSSI_MONITOR);
        return WIFI_SUCCESS;
    }

    virtual int handleResponse(WifiEvent& reply) {
        /* Nothing to do on response! */
        return NL_SKIP;
    }

    virtual int handleEvent(WifiEvent& event) {
        ALOGD("Got a RSSI monitor event");

        //nlattr *vendor_data = event.get_attribute(NL80211_ATTR_VENDOR_DATA);
        struct nlattr *vendor_data = (struct nlattr *)event.get_vendor_data();
        int len = event.get_vendor_data_len();

        if (vendor_data == NULL || len == 0) {
            ALOGE("RSSI monitor: No data");
            return NL_SKIP;
        }
        /* driver<->HAL event structure */
        #define RSSI_MONITOR_EVT_VERSION   1
        typedef struct {
            u8 version;
            s8 cur_rssi;
            mac_addr BSSID;
        } rssi_monitor_evt;

        rssi_monitor_evt *data = NULL;
        if (vendor_data->nla_type == WIFI_EVENT_RSSI_MONITOR)
            data = (rssi_monitor_evt *)nla_data(vendor_data);
        else
            return NL_SKIP;

        ALOGI("data: version=%d, cur_rssi=%d BSSID=" MACSTR "\r\n",
            data->version, data->cur_rssi, MAC2STR(data->BSSID));

        if (data->version != RSSI_MONITOR_EVT_VERSION) {
            ALOGE("Event version mismatch %d, expected %d", data->version, RSSI_MONITOR_EVT_VERSION);
            return NL_SKIP;
        }

        if (*mHandler.on_rssi_threshold_breached) {
            (*mHandler.on_rssi_threshold_breached)(id(), data->BSSID, data->cur_rssi);
        } else {
            ALOGW("No RSSI monitor handler registered");
        }

        return NL_SKIP;
    }
};

static wifi_error wifi_start_rssi_monitoring(wifi_request_id id, wifi_interface_handle
                        iface, s8 max_rssi, s8 min_rssi, wifi_rssi_event_handler eh)
{
    ALOGD("Start RSSI monitoring %d", id);
    wifi_handle handle = getWifiHandle(iface);
    SetRSSIMonitorCommand *cmd = new SetRSSIMonitorCommand(id, iface, max_rssi, min_rssi, eh);
    wifi_register_cmd(handle, id, cmd);
    return (wifi_error)cmd->start();
}

static wifi_error wifi_stop_rssi_monitoring(wifi_request_id id, wifi_interface_handle iface)
{
    ALOGD("Stopping RSSI monitoring");

    if (id == -1) {
        wifi_rssi_event_handler handler;
        s8 max_rssi = 0, min_rssi = 0;
        wifi_handle handle = getWifiHandle(iface);
        memset(&handler, 0, sizeof(handler));
        SetRSSIMonitorCommand *cmd = new SetRSSIMonitorCommand(id, iface, max_rssi, min_rssi, handler);
        cmd->cancel();
        cmd->releaseRef();
        return WIFI_SUCCESS;
    }

    return wifi_cancel_cmd(id, iface);
}

/////////////////////////////////////////////////////////////////////////////
