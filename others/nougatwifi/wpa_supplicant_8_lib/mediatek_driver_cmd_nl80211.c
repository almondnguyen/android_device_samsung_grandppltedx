/*
 * Driver interaction with extended Linux CFG8021
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Alternatively, this software may be distributed under the terms of BSD
 * license.
 *
 */
#include "includes.h"
#include <linux/wireless.h>
#include "netlink/genl/genl.h"

#include "common.h"
#include "driver_nl80211.h"
#include "linux_ioctl.h"
#include "wpa_supplicant_i.h"
#include "config.h"
#ifdef ANDROID
#include "android_drv.h"
#endif

#include "mediatek_driver_nl80211.h"
#include "driver_i.h"

#include "eloop.h"
#define PRIV_CMD_SIZE 512

typedef struct android_wifi_priv_cmd {
    char buf[PRIV_CMD_SIZE];
    int used_len;
    int total_len;
} android_wifi_priv_cmd;

static int drv_errors = 0;

static void wpa_driver_send_hang_msg(struct wpa_driver_nl80211_data *drv)
{
    drv_errors++;
    if (drv_errors > DRV_NUMBER_SEQUENTIAL_ERRORS) {
        drv_errors = 0;
        /* avoid the framework to handle  HANGED */
        /*
	 * wpa_msg(drv->ctx, MSG_INFO, WPA_EVENT_DRIVER_STATE "HANGED");
	 */
    }
}

static int testmode_sta_statistics_handler(struct nl_msg *msg, void *arg)
{
    struct nlattr *tb[NL80211_ATTR_MAX + 1];
    struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));
    struct nlattr *sinfo[NL80211_TESTMODE_STA_STATISTICS_NUM];
    struct wpa_driver_sta_statistics_s *sta_statistics = (struct wpa_driver_sta_statistics_s *)arg;
    unsigned char i = 0;
    static struct nla_policy policy[NL80211_TESTMODE_STA_STATISTICS_NUM] = {
        [NL80211_TESTMODE_STA_STATISTICS_VERSION]               = { .type = NLA_U8 },
        [NL80211_TESTMODE_STA_STATISTICS_MAC]                   = { .type = NLA_UNSPEC },
        [NL80211_TESTMODE_STA_STATISTICS_LINK_SCORE]            = { .type = NLA_U32 },
        [NL80211_TESTMODE_STA_STATISTICS_FLAG]                  = { .type = NLA_U32 },
        [NL80211_TESTMODE_STA_STATISTICS_PER]                   = { .type = NLA_U8 },
        [NL80211_TESTMODE_STA_STATISTICS_RSSI]                  = { .type = NLA_U8 },
        [NL80211_TESTMODE_STA_STATISTICS_PHY_MODE]              = { .type = NLA_U32 },
        [NL80211_TESTMODE_STA_STATISTICS_TX_RATE]               = { .type = NLA_U16 },
        [NL80211_TESTMODE_STA_STATISTICS_FAIL_CNT]              = { .type = NLA_U32 },
        [NL80211_TESTMODE_STA_STATISTICS_TIMEOUT_CNT]           = { .type = NLA_U32 },
        [NL80211_TESTMODE_STA_STATISTICS_AVG_AIR_TIME]          = { .type = NLA_U32 },
        [NL80211_TESTMODE_STA_STATISTICS_TOTAL_CNT]             = { .type = NLA_U32 },
        [NL80211_TESTMODE_STA_STATISTICS_THRESHOLD_CNT]         = { .type = NLA_U32 },

        [NL80211_TESTMODE_STA_STATISTICS_AVG_PROCESS_TIME]      = { .type = NLA_U32 },
        [NL80211_TESTMODE_STA_STATISTICS_MAX_PROCESS_TIME]		= { .type = NLA_U32 },
        [NL80211_TESTMODE_STA_STATISTICS_AVG_HIF_PROCESS_TIME]		= { .type = NLA_U32 },
        [NL80211_TESTMODE_STA_STATISTICS_MAX_HIF_PROCESS_TIME]		= { .type = NLA_U32 },

        [NL80211_TESTMODE_STA_STATISTICS_TC_EMPTY_CNT_ARRAY]    = { .type = NLA_UNSPEC },
        [NL80211_TESTMODE_STA_STATISTICS_TC_QUE_LEN_ARRAY]      = { .type = NLA_UNSPEC },
        [NL80211_TESTMODE_STA_STATISTICS_TC_AVG_QUE_LEN_ARRAY]  = { .type = NLA_UNSPEC },
        [NL80211_TESTMODE_STA_STATISTICS_TC_CUR_QUE_LEN_ARRAY]  = { .type = NLA_UNSPEC },
        /*
         * how many packages TX during statistics interval
         */
        [NL80211_TESTMODE_STA_STATISTICS_ENQUEUE]		= { .type = NLA_U32 },
        /*
         * how many packages this sta TX during statistics interval
         */
        [NL80211_TESTMODE_STA_STATISTICS_STA_ENQUEUE]		= { .type = NLA_U32 },

        /*
         * how many packages dequeue during statistics interval
         */
        [NL80211_TESTMODE_STA_STATISTICS_DEQUEUE]		= { .type = NLA_U32 },

        /*
         * how many packages this sta dequeue during statistics interval
         */
        [NL80211_TESTMODE_STA_STATISTICS_STA_DEQUEUE]		= { .type = NLA_U32 },

        /*
         * how many TC[0-3] resource back from firmware during
         * statistics interval
         */
        [NL80211_TESTMODE_STA_STATISTICS_RB_ARRAY]		= { .type = NLA_UNSPEC },
        [NL80211_TESTMODE_STA_STATISTICS_NO_TC_ARRAY]		= { .type = NLA_UNSPEC },
        [NL80211_TESTMODE_STA_STATISTICS_TC_USED_ARRAY]		= { .type = NLA_UNSPEC },
        [NL80211_TESTMODE_STA_STATISTICS_TC_WANTED_ARRAY]		= { .type = NLA_UNSPEC },

        [NL80211_TESTMODE_STA_STATISTICS_IRQ_ISR_CNT]		= { .type = NLA_U32 },
        [NL80211_TESTMODE_STA_STATISTICS_IRQ_ISR_PASS_CNT]	= { .type = NLA_U32 },
        [NL80211_TESTMODE_STA_STATISTICS_IRQ_TASK_CNT]		= { .type = NLA_U32 },
        [NL80211_TESTMODE_STA_STATISTICS_IRQ_AB_CNT]		= { .type = NLA_U32 },
        [NL80211_TESTMODE_STA_STATISTICS_IRQ_SW_CNT]		= { .type = NLA_U32 },
        [NL80211_TESTMODE_STA_STATISTICS_IRQ_TX_CNT]		= { .type = NLA_U32 },
        [NL80211_TESTMODE_STA_STATISTICS_IRQ_RX_CNT]		= { .type = NLA_U32 },
        [NL80211_TESTMODE_STA_STATISTICS_RESERVED_ARRAY]        = { .type = NLA_UNSPEC }
    };

    nla_parse(tb, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
        genlmsg_attrlen(gnlh, 0), NULL);

    if (!tb[NL80211_ATTR_TESTDATA] ||
        nla_parse_nested(sinfo, NL80211_TESTMODE_STA_STATISTICS_MAX, tb[NL80211_ATTR_TESTDATA], policy))
        return NL_SKIP;

    for (i=1; i < NL80211_TESTMODE_STA_STATISTICS_NUM; i++) {

        if (!sinfo[i])
            continue;

        switch(i) {
        case NL80211_TESTMODE_STA_STATISTICS_VERSION:
            sta_statistics->version = nla_get_u8(sinfo[i]);
            break;
        case NL80211_TESTMODE_STA_STATISTICS_MAC:
            nla_memcpy(sta_statistics->addr, sinfo[i], ETH_ALEN);
            break;
        case NL80211_TESTMODE_STA_STATISTICS_LINK_SCORE:
            sta_statistics->link_score = nla_get_u32(sinfo[i]);
            break;
        case NL80211_TESTMODE_STA_STATISTICS_FLAG:
            sta_statistics->flag = nla_get_u32(sinfo[i]);
            break;
        case NL80211_TESTMODE_STA_STATISTICS_PER:
            sta_statistics->per = nla_get_u8(sinfo[i]);
            break;
        case NL80211_TESTMODE_STA_STATISTICS_RSSI:
            sta_statistics->rssi = (((int)nla_get_u8(sinfo[i]) - 220) / 2);
            break;
        case NL80211_TESTMODE_STA_STATISTICS_PHY_MODE:
            sta_statistics->phy_mode = nla_get_u32(sinfo[i]);
            break;
        case NL80211_TESTMODE_STA_STATISTICS_TX_RATE:
            sta_statistics->tx_rate = (((double)nla_get_u16(sinfo[i])) / 2);
            break;
        case NL80211_TESTMODE_STA_STATISTICS_FAIL_CNT:
            sta_statistics->tx_fail_cnt = nla_get_u32(sinfo[i]);
            break;
        case NL80211_TESTMODE_STA_STATISTICS_TIMEOUT_CNT:
            sta_statistics->tx_timeout_cnt = nla_get_u32(sinfo[i]);
            break;
        case NL80211_TESTMODE_STA_STATISTICS_AVG_AIR_TIME:
            sta_statistics->tx_avg_air_time = nla_get_u32(sinfo[i]);
            break;
        case NL80211_TESTMODE_STA_STATISTICS_TOTAL_CNT:
            sta_statistics->tx_total_cnt = nla_get_u32(sinfo[i]);
            break;
        case NL80211_TESTMODE_STA_STATISTICS_THRESHOLD_CNT:
            sta_statistics->tx_exc_threshold_cnt = nla_get_u32(sinfo[i]);
            break;
        case NL80211_TESTMODE_STA_STATISTICS_AVG_PROCESS_TIME:
            sta_statistics->tx_avg_process_time = nla_get_u32(sinfo[i]);
            break;
        case NL80211_TESTMODE_STA_STATISTICS_MAX_PROCESS_TIME:
            sta_statistics->tx_max_process_time = nla_get_u32(sinfo[i]);
            break;
        case NL80211_TESTMODE_STA_STATISTICS_AVG_HIF_PROCESS_TIME:
            sta_statistics->tx_avg_hif_process_time = nla_get_u32(sinfo[i]);
            break;
        case NL80211_TESTMODE_STA_STATISTICS_MAX_HIF_PROCESS_TIME:
            sta_statistics->tx_max_hif_process_time = nla_get_u32(sinfo[i]);
            break;
        case NL80211_TESTMODE_STA_STATISTICS_TC_EMPTY_CNT_ARRAY:
            nla_memcpy(sta_statistics->tc_buf_full_cnt, sinfo[i], sizeof(sta_statistics->tc_buf_full_cnt));
            break;
        case NL80211_TESTMODE_STA_STATISTICS_TC_QUE_LEN_ARRAY:
            nla_memcpy(sta_statistics->tc_que_len, sinfo[i], sizeof(sta_statistics->tc_que_len));
            break;
        case NL80211_TESTMODE_STA_STATISTICS_TC_AVG_QUE_LEN_ARRAY:
            nla_memcpy(sta_statistics->tc_avg_que_len, sinfo[i], sizeof(sta_statistics->tc_avg_que_len));
            break;
        case NL80211_TESTMODE_STA_STATISTICS_TC_CUR_QUE_LEN_ARRAY:
            nla_memcpy(sta_statistics->tc_cur_que_len, sinfo[i], sizeof(sta_statistics->tc_cur_que_len));
            break;

        case NL80211_TESTMODE_STA_STATISTICS_ENQUEUE:
            sta_statistics->enqueue_total_cnt = nla_get_u32(sinfo[i]);
            break;

        case NL80211_TESTMODE_STA_STATISTICS_DEQUEUE:
            sta_statistics->dequeue_total_cnt = nla_get_u32(sinfo[i]);
            break;

        case NL80211_TESTMODE_STA_STATISTICS_STA_ENQUEUE:
            sta_statistics->enqueue_sta_total_cnt = nla_get_u32(sinfo[i]);
            break;

        case NL80211_TESTMODE_STA_STATISTICS_STA_DEQUEUE:
            sta_statistics->dequeue_sta_total_cnt = nla_get_u32(sinfo[i]);
            break;

        case NL80211_TESTMODE_STA_STATISTICS_IRQ_ISR_CNT:
            sta_statistics->isr_cnt = nla_get_u32(sinfo[i]);
            break;

        case NL80211_TESTMODE_STA_STATISTICS_IRQ_ISR_PASS_CNT:
            sta_statistics->isr_pass_cnt = nla_get_u32(sinfo[i]);
            break;
        case NL80211_TESTMODE_STA_STATISTICS_IRQ_TASK_CNT:
            sta_statistics->isr_task_cnt = nla_get_u32(sinfo[i]);
            break;

        case NL80211_TESTMODE_STA_STATISTICS_IRQ_AB_CNT:
            sta_statistics->isr_ab_cnt = nla_get_u32(sinfo[i]);
            break;

        case NL80211_TESTMODE_STA_STATISTICS_IRQ_SW_CNT:
            sta_statistics->isr_sw_cnt = nla_get_u32(sinfo[i]);
            break;

        case NL80211_TESTMODE_STA_STATISTICS_IRQ_TX_CNT:
            sta_statistics->isr_tx_cnt = nla_get_u32(sinfo[i]);
            break;

        case NL80211_TESTMODE_STA_STATISTICS_IRQ_RX_CNT:
            sta_statistics->isr_rx_cnt = nla_get_u32(sinfo[i]);
            break;

        case NL80211_TESTMODE_STA_STATISTICS_NO_TC_ARRAY:
            nla_memcpy(sta_statistics->dequeue_no_tc_res, sinfo[i],
                      sizeof(sta_statistics->dequeue_no_tc_res));
            break;

        case NL80211_TESTMODE_STA_STATISTICS_TC_USED_ARRAY:
            nla_memcpy(sta_statistics->tc_used_res, sinfo[i],
                      sizeof(sta_statistics->tc_used_res));
            break;
        case NL80211_TESTMODE_STA_STATISTICS_TC_WANTED_ARRAY:
            nla_memcpy(sta_statistics->tc_wanted_res, sinfo[i],
                      sizeof(sta_statistics->tc_wanted_res));
            break;

        case NL80211_TESTMODE_STA_STATISTICS_RB_ARRAY:
            nla_memcpy(sta_statistics->tc_back_count, sinfo[i],
                      sizeof(sta_statistics->tc_back_count));
            break;

        case NL80211_TESTMODE_STA_STATISTICS_RESERVED_ARRAY:
             nla_memcpy(sta_statistics->reserved, sinfo[i], sizeof(sta_statistics->reserved));
             break;
        default:
            break;
        }
    }

    return NL_SKIP;
}

static int wpa_driver_nl80211_testmode(void *priv, const u8 *data, size_t data_len)
{
    struct i802_bss *bss = priv;
    struct wpa_driver_nl80211_data *drv = bss->drv;
    struct nl_msg *msg, *cqm = NULL;
    struct wpa_driver_testmode_params *params;
    int index;

    msg = nlmsg_alloc();
    if (!msg)
        return -1;

    wpa_printf(MSG_DEBUG, "nl80211 test mode: ifindex=%d", drv->ifindex);

    nl80211_cmd(drv, msg, 0, NL80211_CMD_TESTMODE);

    NLA_PUT_U32(msg, NL80211_ATTR_IFINDEX, drv->ifindex);
    NLA_PUT(msg, NL80211_ATTR_TESTDATA, data_len, data);

    params = (struct wpa_driver_testmode_params *)data;

    /* Mask version field */
    index = params->hdr.index & BITS(0, 23);

    switch(index) {
        case 0x10:
        {
            struct wpa_driver_get_sta_statistics_params *sta_params =
                           (struct wpa_driver_get_sta_statistics_params *)data;
            return send_and_recv_msgs(drv, msg, testmode_sta_statistics_handler, sta_params->buf);
        }
        default:
        {
            int ret = send_and_recv_msgs(drv, msg, NULL, NULL);
            wpa_printf(MSG_DEBUG, "ret=%d, nl=%d", ret, drv->global->nl);
            return ret;
        }
    }

nla_put_failure:
    nlmsg_free(msg);
    return -ENOBUFS;
}

static int wpa_driver_nl80211_driver_sw_cmd(void *priv, int set, u32 *adr, u32 *dat)
{
    struct i802_bss *bss = priv;
    struct wpa_driver_nl80211_data *drv = bss->drv;
    struct wpa_driver_sw_cmd_params params;
    struct nl_msg *msg, *cqm = NULL;
    int ret = 0;

    os_memset(&params, 0, sizeof(params));

    params.hdr.index = NL80211_TESTMODE_SW_CMD;
    params.hdr.index = params.hdr.index | (0x01 << 24);
    params.hdr.buflen = sizeof(struct wpa_driver_sw_cmd_params);

    params.adr = *adr;
    params.data = *dat;

    if (set)
        params.set = 1;
    else
        params.set = 0;

    wpa_driver_nl80211_testmode(priv, (u8 *)&params, sizeof(struct wpa_driver_sw_cmd_params));
    return 0;
}

#ifdef CONFIG_HOTSPOT_MGR_SUPPORT
static int wpa_driver_hotspot_block_list_update(void *priv, const u8 *bssid, int blocked)
{
    struct wpa_driver_hotspot_params params;

    os_memset(&params, 0, sizeof(params));

    if (bssid)
        os_memcpy(params.bssid, bssid, ETH_ALEN);

    params.blocked = (u8)blocked;

    params.hdr.index = NL80211_TESTMODE_HS20;
    params.hdr.index = params.hdr.index | (0x01 << 24);
    params.hdr.buflen = sizeof(struct wpa_driver_hotspot_params);

    return wpa_driver_nl80211_testmode(priv, (u8 *)&params,
        sizeof(struct wpa_driver_hotspot_params));
}

static int wpa_driver_sta_block(void *priv, char *cmd)
{
    u8 bssid[ETH_ALEN];
    int blocked = 1;

    /* Block client device */
    if (hwaddr_aton(cmd, bssid)) {
        wpa_printf(MSG_DEBUG, "STA block: invalid DEVICE ADDRESS '%s'", cmd);
        return -1;
    }

    wpa_printf(MSG_DEBUG, "Block STA " MACSTR, MAC2STR(bssid));
    return wpa_driver_hotspot_block_list_update(priv, bssid, blocked);
}

static int wpa_driver_sta_unblock(void *priv, char *cmd)
{
    u8 bssid[ETH_ALEN];
    int blocked = 0;

    /* Unblock client device */
    if (hwaddr_aton(cmd, bssid)) {
        wpa_printf(MSG_DEBUG, "STA unblock : invalid DEVICE ADDRESS '%s'", cmd);
        return -1;
    }

    wpa_printf(MSG_DEBUG, "Unblock STA " MACSTR, MAC2STR(bssid));
    return wpa_driver_hotspot_block_list_update(priv, bssid, blocked);
}
#endif /* CONFIG_HOTSPOT_MGR_SUPPORT */

/**********************************************************************
* OVERLAPPED functins, previous defination is in driver_nl80211.c,
* it will be modified
***********************************************************************/

/**********************************************************************/
extern int wpa_config_write(const char *name, struct wpa_config *config);

static int wpa_driver_mediatek_set_country(void *priv, const char *alpha2_arg)
{
    struct i802_bss *bss = priv;
    struct wpa_driver_nl80211_data *drv = bss->drv;
    int ioctl_sock = -1;
    struct iwreq iwr;
    int ret = -1;
    char buf[11];
#ifdef MTK_TC1_FEATURE
    char replace_ifname[IFNAMSIZ+1];

    memset(replace_ifname, 0, IFNAMSIZ+1);
    os_strlcpy(replace_ifname, "wlan0", os_strlen("wlan0")+1);
#endif

    wpa_printf(MSG_DEBUG, "wpa_driver_nl80211_set_country");
    ioctl_sock = socket(PF_INET, SOCK_DGRAM, 0);
    if (ioctl_sock < 0) {
        wpa_printf(MSG_ERROR, "%s: socket(PF_INET,SOCK_DGRAM)", __func__);
        return -1;
    }
    os_memset(&iwr, 0, sizeof(iwr));
#ifdef MTK_TC1_FEATURE
    // convert 'p2p0' -> 'wlan0' :
    // when iface name is p2p0, COUNTRY driver command doesn't support in MTK solution.
    if (os_strncmp(drv->first_bss->ifname, "p2p0", os_strlen("p2p0")) == 0) {
        wpa_printf(MSG_DEBUG, "Change interface name : p2p0->wlan0");
        os_strlcpy(iwr.ifr_name, replace_ifname, IFNAMSIZ );
    } else {
    os_strlcpy(iwr.ifr_name, drv->first_bss->ifname, IFNAMSIZ);
    }
#else
    os_strlcpy(iwr.ifr_name, drv->first_bss->ifname, IFNAMSIZ);
#endif
    sprintf(buf, "COUNTRY %s", alpha2_arg);
    iwr.u.data.pointer = buf;
    iwr.u.data.length = strlen(buf);
    if ((ret = ioctl(ioctl_sock, 0x8B0C, &iwr)) < 0) {  // SIOCSIWPRIV
        wpa_printf(MSG_DEBUG, "ioctl[SIOCSIWPRIV]: %s", buf);
        close(ioctl_sock);
        return ret;
    }
    else {
        close(ioctl_sock);
        return 0;
    }

}

/*
* update channel list in wpa_supplicant
* if coutry code chanaged
*/
static void wpa_driver_notify_country_change(void *ctx, char *cmd)
{
    if (os_strncasecmp(cmd, "COUNTRY", 7) == 0) {
        union wpa_event_data event;

        os_memset(&event, 0, sizeof(event));
        event.channel_list_changed.initiator = REGDOM_SET_BY_USER;
        if (os_strncasecmp(cmd, "COUNTRY", 7) == 0) {
            event.channel_list_changed.type = REGDOM_TYPE_COUNTRY;
            if (os_strlen(cmd) > 9) {
                event.channel_list_changed.alpha2[0] = cmd[8];
                event.channel_list_changed.alpha2[1] = cmd[9];
            }
        } else
            event.channel_list_changed.type = REGDOM_TYPE_UNKNOWN;
        wpa_supplicant_event(ctx, EVENT_CHANNEL_LIST_CHANGED, &event);
    }
}

/**
 * mtk_p2p_get_device - Fetch a peer entry
 * @p2p: P2P module context from p2p_init()
 * @addr: P2P Device Address of the peer
 * Returns: Pointer to the device entry or %NULL if not found
 */
struct p2p_device *mtk_p2p_get_device(struct p2p_data *p2p, const u8 *addr)
{
    return NULL;
}
/*
 * we should use interface MAC address
 * instead of device MAC when query
 * STA statistics, as driver uses interface addr
 * to do TX/RX
 * In most cases, the interface addr and device addr
 * should be the same
 */
u8 *wpas_p2p_get_sta_mac(struct wpa_supplicant *wpa_s, u8 *org_addr)
{
    return NULL;
}

/* Move GET_STA_STATISTICS to "DRIVER GET_STA_STATISTICS", implement in 3rd part lib */
/* [ALPS00618361] [WFD Quality Enhancement] */
int wpas_get_sta_statistics(struct wpa_supplicant *wpa_s, u8 *sta_addr, u8 *buf)
{
    struct wpa_driver_get_sta_statistics_params params;

    os_memset(&params, 0, sizeof(params));

    if(sta_addr)
        os_memcpy(params.addr, sta_addr, ETH_ALEN);

    wpa_printf(MSG_DEBUG, "get_sta_statistics ["MACSTR"]", MAC2STR(params.addr));

    params.hdr.index = NL80211_TESTMODE_STATISTICS;
    params.hdr.index = params.hdr.index | (0x01 << 24);
    params.hdr.buflen = sizeof(struct wpa_driver_get_sta_statistics_params);

    /* buffer for return structure */
    params.buf = buf;

    return wpa_driver_nl80211_testmode(wpa_s->drv_priv, (u8 *)&params,
        sizeof(struct wpa_driver_get_sta_statistics_params));
}

/*  [ALPS00618361] [WFD Quality Enhancement] [changelist 1686130] */
static int print_sta_statistics(struct wpa_supplicant *wpa_s, struct wpa_driver_sta_statistics_s *sta_stats,
              unsigned long mask, char *buf, size_t buflen)
{
    size_t i;
    int ret;
    char *pos, *end;

    pos = buf;
    end = buf + buflen;

    ret = os_snprintf(pos, end - pos, "sta_addr="MACSTR"\n", MAC2STR(sta_stats->addr));
    if (ret < 0 || ret >= end - pos)
        return 0;
    pos += ret;

    ret = os_snprintf(pos, end - pos, "link_score=%d\n", sta_stats->link_score);
    if (ret < 0 || ret >= end - pos)
        return 0;
    pos += ret;

    ret = os_snprintf(pos, end - pos, "per=%d\n", sta_stats->per);
    if (ret < 0 || ret >= end - pos)
        return 0;
    pos += ret;

    ret = os_snprintf(pos, end - pos, "rssi=%d\n", sta_stats->rssi);
    if (ret < 0 || ret >= end - pos)
        return 0;
    pos += ret;

    ret = os_snprintf(pos, end - pos, "phy=0x%08X\n", sta_stats->phy_mode);
    if (ret < 0 || ret >= end - pos)
        return 0;
    pos += ret;

    ret = os_snprintf(pos, end - pos, "rate=%.1f\n", sta_stats->tx_rate);
    if (ret < 0 || ret >= end - pos)
        return 0;
    pos += ret;

    ret = os_snprintf(pos, end - pos, "total_cnt=%d\n", sta_stats->tx_total_cnt);
    if (ret < 0 || ret >= end - pos)
        return 0;
    pos += ret;

    ret = os_snprintf(pos, end - pos, "threshold_cnt=%d\n", sta_stats->tx_exc_threshold_cnt);
    if (ret < 0 || ret >= end - pos)
        return 0;
    pos += ret;

    ret = os_snprintf(pos, end - pos, "fail_cnt=%d\n", sta_stats->tx_fail_cnt);
    if (ret < 0 || ret >= end - pos)
        return 0;
    pos += ret;

    ret = os_snprintf(pos, end - pos, "timeout_cnt=%d\n", sta_stats->tx_timeout_cnt);
    if (ret < 0 || ret >= end - pos)
        return 0;
    pos += ret;

    ret = os_snprintf(pos, end - pos, "apt=%d\n", sta_stats->tx_avg_process_time);
    if (ret < 0 || ret >= end - pos)
        return 0;
    pos += ret;

    ret = os_snprintf(pos, end - pos, "aat=%d\n", sta_stats->tx_avg_air_time);
    if (ret < 0 || ret >= end - pos)
        return 0;
    pos += ret;

    ret = os_snprintf(pos, end - pos, "TC_buf_full_cnt=%d:%d:%d:%d\n",
                      sta_stats->tc_buf_full_cnt[TC0_INDEX],
                      sta_stats->tc_buf_full_cnt[TC1_INDEX],
                      sta_stats->tc_buf_full_cnt[TC2_INDEX],
                      sta_stats->tc_buf_full_cnt[TC3_INDEX]);
    if (ret < 0 || ret >= end - pos)
        return 0;
    pos += ret;

    ret = os_snprintf(pos, end - pos, "TC_sta_que_len=%d:%d:%d:%d\n",
                      sta_stats->tc_que_len[TC0_INDEX],
                      sta_stats->tc_que_len[TC1_INDEX],
                      sta_stats->tc_que_len[TC2_INDEX],
                      sta_stats->tc_que_len[TC3_INDEX]);
    if (ret < 0 || ret >= end - pos)
        return 0;
    pos += ret;

    ret = os_snprintf(pos, end - pos, "TC_avg_que_len=%d:%d:%d:%d\n",
                      sta_stats->tc_avg_que_len[TC0_INDEX],
                      sta_stats->tc_avg_que_len[TC1_INDEX],
                      sta_stats->tc_avg_que_len[TC2_INDEX],
                      sta_stats->tc_avg_que_len[TC3_INDEX]);
    if (ret < 0 || ret >= end - pos)
        return 0;
    pos += ret;

    ret = os_snprintf(pos, end - pos, "TC_cur_que_len=%d:%d:%d:%d\n",
                      sta_stats->tc_cur_que_len[TC0_INDEX],
                      sta_stats->tc_cur_que_len[TC1_INDEX],
                      sta_stats->tc_cur_que_len[TC2_INDEX],
                      sta_stats->tc_cur_que_len[TC3_INDEX]);
    if (ret < 0 || ret >= end - pos)
        return 0;
    pos += ret;

    ret = os_snprintf(pos, end - pos, "flag=0x%08X\n", sta_stats->flag);
    if (ret < 0 || ret >= end - pos)
        return 0;
    pos += ret;

    ret = os_snprintf(pos, end - pos, "reserved0=");
    if (ret < 0 || ret >= end - pos)
        return 0;
    pos += ret;
    for (i = 0; i < 16; i++) {
        ret = os_snprintf(pos, end - pos, "%02X", sta_stats->reserved[i]);
        if (ret < 0 || ret >= end - pos)
            return 0;
        pos += ret;

        if (((i + 1) % 4) == 0) {
            ret = os_snprintf(pos, end - pos, " ", sta_stats->reserved[i]);
            if (ret < 0 || ret >= end - pos)
                return 0;
            pos += ret;
        }
    }
    ret = os_snprintf(pos, end - pos, "\n", sta_stats->reserved[i]);
    if (ret < 0 || ret >= end - pos)
        return 0;
    pos += ret;

    ret = os_snprintf(pos, end - pos, "reserved1=");
    if (ret < 0 || ret >= end - pos)
        return 0;
    pos += ret;
    for (i = 16; i < 32; i++) {
        ret = os_snprintf(pos, end - pos, "%02X", sta_stats->reserved[i]);
        if (ret < 0 || ret >= end - pos)
            return 0;
        pos += ret;

        if (((i + 1) % 4) == 0) {
            ret = os_snprintf(pos, end - pos, " ", sta_stats->reserved[i]);
            if (ret < 0 || ret >= end - pos)
                return 0;
            pos += ret;
        }
    }
    ret = os_snprintf(pos, end - pos, "\n", sta_stats->reserved[i]);
    if (ret < 0 || ret >= end - pos)
        return 0;
    pos += ret;

    ret = os_snprintf(pos, end - pos, "====\n");
    if (ret < 0 || ret >= end - pos)
        return 0;
    pos += ret;

    return pos - buf;
}

/*  [ALPS00618361] [WFD Quality Enhancement] [changelist 1686130] */
static void format_sta_statistics(struct wpa_driver_sta_statistics_s *s)
{
	wpa_printf(MSG_DEBUG, "NWFD: Basic info* AVG:%4d:EN:%4d:DE:%4d:SEN:%4d:SDE:%4d:HIF:%4d",
		s->tx_avg_process_time,
		s->enqueue_total_cnt,
		s->dequeue_total_cnt,
		s->enqueue_sta_total_cnt,
		s->dequeue_sta_total_cnt,
		s->tx_total_cnt);

	wpa_printf(MSG_DEBUG, "NWFD: Time info* TTL:%4d:AVG:%4d:MAX:%4d:HIFAVG:%4d:HIFMAX:%4d",
		s->tx_total_cnt,
		s->tx_avg_process_time,
		s->tx_max_process_time,
		s->tx_avg_hif_process_time,
		s->tx_max_hif_process_time);

	wpa_printf(MSG_DEBUG, "NWFD: No TC RES* Score:%4d:EN:%4d#%4d#%4d#%4d:DE:%4d#%4d#%4d#%4d",
		s->link_score,
		s->tc_buf_full_cnt[TC0_INDEX],
		s->tc_buf_full_cnt[TC1_INDEX],
		s->tc_buf_full_cnt[TC2_INDEX],
		s->tc_buf_full_cnt[TC3_INDEX],
		s->dequeue_no_tc_res[TC0_INDEX],
		s->dequeue_no_tc_res[TC1_INDEX],
		s->dequeue_no_tc_res[TC2_INDEX],
		s->dequeue_no_tc_res[TC3_INDEX]);

	wpa_printf(MSG_DEBUG, "NWFD: Irq info* T:%4d:P:%4d:TT:%4d:A:%4d:S:%4d:R:%4d:T:%4d",
		s->isr_cnt,
		s->isr_pass_cnt,
		s->isr_task_cnt,
		s->isr_ab_cnt,
		s->isr_sw_cnt,
		s->isr_rx_cnt,
		s->isr_tx_cnt);

	/*
	 * TC resouce information: format:
	 * 1. how many TC resource wanted during statistics intervals
	 * 2. how many TC resource acquire successfully
	 * 3. how many TC resource back during statistics intervals
	 */
	wpa_printf(MSG_DEBUG, "NWFD: TC Res info[W:U:B]* Score:%4d:"
		"#%5d:%5d:%5d#"
		"#%5d:%5d:%5d#"
		"#%5d:%5d:%5d#"
		"#%5d:%5d:%5d#",
		s->link_score,
		s->tc_wanted_res[TC0_INDEX],
		s->tc_used_res[TC0_INDEX],
		s->tc_back_count[TC0_INDEX],

		s->tc_wanted_res[TC1_INDEX],
		s->tc_used_res[TC1_INDEX],
		s->tc_back_count[TC1_INDEX],

		s->tc_wanted_res[TC2_INDEX],
		s->tc_used_res[TC2_INDEX],
		s->tc_back_count[TC2_INDEX],

		s->tc_wanted_res[TC3_INDEX],
		s->tc_used_res[TC3_INDEX],
		s->tc_back_count[TC3_INDEX]);
}

int wpa_driver_get_sta_statistics(struct wpa_supplicant *wpa_s, char *addr,
                                char *buf, size_t buflen)
{
    char *str = NULL;
    int len = 0;
    u8 sta_addr[ETH_ALEN];
    u8 *mac = NULL;
    struct wpa_driver_sta_statistics_s sta_statistics;

    memset(&sta_statistics, 0 ,sizeof(sta_statistics));

    if (hwaddr_aton(addr, sta_addr)) {
        wpa_printf(MSG_DEBUG, "CTRL_IFACE GET_STA_STATISTICS: invalid "
                   "address '%s'", addr);
        return -1;
    }

    mac = wpas_p2p_get_sta_mac(wpa_s, sta_addr);

    if (wpas_get_sta_statistics(wpa_s, mac ? mac : sta_addr,
        (u8 *)&sta_statistics) < 0) {
        wpa_printf(MSG_DEBUG, "CTRL_IFACE GET_STA_STATISTICS: command failed");
        return -1;
    }
    len = print_sta_statistics(wpa_s, &sta_statistics, 0x00, buf, buflen);

    format_sta_statistics(&sta_statistics);
    return len;
}

#ifdef CONFIG_MTK_WFD_SINK
static int p2p_get_capability(struct wpa_supplicant *wpa_s, char *cmd, char *buf, size_t buflen)
{
    int ret = 0;
    struct p2p_data *p2p = wpa_s->global->p2p;
    wpa_printf(MSG_DEBUG, "%s %d, %d", __func__, __LINE__, p2p->dev_capab);
    if (os_strncmp(cmd, "p2p_dev_capa", os_strlen("p2p_dev_capa")) == 0) {
        ret = snprintf(buf, buflen, "p2p_dev_capa=%d\n", p2p->dev_capab);
        wpa_printf(MSG_DEBUG, "%s %d %d, %s", __func__, __LINE__, p2p->dev_capab, buf);
    } else if (os_strncmp(cmd, "p2p_group_capa", os_strlen("p2p_group_capa")) == 0) {
        wpa_printf(MSG_DEBUG, "%s not implement", __func__);
        ret = -1;
    }
    return ret;
}

static int p2p_set_capability(struct wpa_supplicant *wpa_s, char *cmd, char *buf, size_t buflen)
{
    int ret = 0;
    wpa_printf(MSG_DEBUG, "%s %d", __func__, __LINE__);
    struct p2p_data *p2p = wpa_s->global->p2p;
    if (os_strncmp(cmd, "p2p_dev_capa ", os_strlen("p2p_dev_capa ")) == 0) {
        int old_cap = p2p->dev_capab;
        int dev_cap = atoi(cmd + os_strlen("p2p_dev_capa "));
        p2p->dev_capab = dev_cap & 0xff;
        wpa_printf(MSG_DEBUG, "%s %d %d, %d", __func__, __LINE__, p2p->dev_capab,
                            old_cap);
    } else if (os_strncmp(cmd, "p2p_group_capa ", os_strlen("p2p_group_capa ")) == 0) {
        wpa_printf(MSG_DEBUG, "%s group not implement", __func__);
        ret = -1;
    }
    return ret;
}

/**
 * priv_p2p_freq_to_channel - Convert frequency into channel info
 * @op_class: Buffer for returning operating class
 * @channel: Buffer for returning channel number
 * Returns: 0 on success, -1 if the specified frequency is unknown
 */
static int priv_p2p_freq_to_channel(unsigned int freq, u8 *op_class, u8 *channel)
{
    /* TODO: more operating classes */
    if (freq >= 2412 && freq <= 2472) {
        if ((freq - 2407) % 5)
            return -1;

        *op_class = 81; /* 2.407 GHz, channels 1..13 */
        *channel = (freq - 2407) / 5;
        return 0;
    }

    if (freq == 2484) {
        *op_class = 82; /* channel 14 */
        *channel = 14;
        return 0;
    }

    if (freq >= 5180 && freq <= 5240) {
        if ((freq - 5000) % 5)
            return -1;

        *op_class = 115; /* 5 GHz, channels 36..48 */
        *channel = (freq - 5000) / 5;
        return 0;
    }
    if (freq >= 5745 && freq <= 5805) {
        if ((freq - 5000) % 5)
            return -1;

        *op_class = 124; /* 5 GHz, channels 149..161 */
        *channel = (freq - 5000) / 5;
        return 0;
    }
    return -1;
}

static int p2p_wfd_sink_config_scc(struct wpa_supplicant *wpa_s, int scc, unsigned int oper_freq)
{
    int ret = 0;
    u8 op_reg_class, op_channel;
    unsigned int r;
    struct wpa_supplicant *iface;
    struct p2p_data *p2p = wpa_s->global->p2p;
    wpa_printf(MSG_DEBUG, "%s %d", __func__, __LINE__);
    if (!p2p) {
        wpa_printf(MSG_DEBUG, "Not support p2p.");
        return ret;
    }
    if (scc && oper_freq) {
        priv_p2p_freq_to_channel(oper_freq, &op_reg_class, &op_channel);
        p2p->op_channel = op_channel;
        p2p->op_reg_class = op_reg_class;
        p2p->channels.reg_classes = 1;
        p2p->channels.reg_class[0].channels = 1;
        p2p->channels.reg_class[0].reg_class = op_reg_class;
        p2p->channels.reg_class[0].channel[0] = op_channel;
        wpa_printf(MSG_DEBUG, "Enable SCC in WFD sink mode class %d, channel %d",
                op_reg_class, op_channel);
        return ret;
    }
    /* Get back to MCC */
    wpa_printf(MSG_DEBUG, "Config MCC");
    if (wpa_s->conf->p2p_oper_reg_class &&
        wpa_s->conf->p2p_oper_channel) {
        p2p->op_reg_class = wpa_s->conf->p2p_oper_reg_class;
        p2p->op_channel = wpa_s->conf->p2p_oper_channel;
        p2p->cfg->cfg_op_channel = 1;
    } else {
        p2p->op_reg_class = 81;
        os_get_random((u8 *)&r, sizeof(r));
        p2p->op_channel = 1 + (r % 3) * 5;
        p2p->cfg->cfg_op_channel = 0;
    }

    os_memcpy(&p2p->channels, &p2p->cfg->channels, sizeof(struct p2p_channels));
    return ret;
}


static int mtk_get_shared_radio_freqs_data(struct wpa_supplicant *wpa_s,
				struct wpa_used_freq_data *freqs_data,
				unsigned int len)
{
    struct wpa_supplicant *ifs;
    u8 bssid[ETH_ALEN];
    int freq;
    unsigned int idx = 0, i;

    wpa_dbg(wpa_s, MSG_DEBUG,
	    "Determining shared radio frequencies (max len %u)", len);
    os_memset(freqs_data, 0, sizeof(struct wpa_used_freq_data) * len);

    dl_list_for_each(ifs, &wpa_s->radio->ifaces, struct wpa_supplicant,
                    radio_list) {
        wpa_printf(MSG_DEBUG, "Get shared freqs ifname %s", ifs->ifname);
        if (idx == len)
            break;

        if (ifs->current_ssid == NULL || ifs->assoc_freq == 0)
            continue;

        if (ifs->current_ssid->mode == WPAS_MODE_AP ||
            ifs->current_ssid->mode == WPAS_MODE_P2P_GO)
            freq = ifs->current_ssid->frequency;
        else if (wpa_drv_get_bssid(ifs, bssid) == 0)
            freq = ifs->assoc_freq;
        else
            continue;

        /* Hold only distinct freqs */
        for (i = 0; i < idx; i++)
            if (freqs_data[i].freq == freq)
                break;

        if (i == idx)
            freqs_data[idx++].freq = freq;

        if (ifs->current_ssid->mode == WPAS_MODE_INFRA) {
            freqs_data[i].flags = ifs->current_ssid->p2p_group ?
                                WPA_FREQ_USED_BY_P2P_CLIENT :
                                WPA_FREQ_USED_BY_INFRA_STATION;
        }
    }

    return idx;
}


static int mtk_get_shared_radio_freqs(struct wpa_supplicant *wpa_s,
			   int *freq_array, unsigned int len)
{
    struct wpa_used_freq_data *freqs_data;
    int num, i;

    os_memset(freq_array, 0, sizeof(int) * len);

    freqs_data = os_calloc(len, sizeof(struct wpa_used_freq_data));
    if (!freqs_data)
        return -1;

    num = mtk_get_shared_radio_freqs_data(wpa_s, freqs_data, len);
    for (i = 0; i < num; i++)
        freq_array[i] = freqs_data[i].freq;

    os_free(freqs_data);

    return num;
}
#endif

static void wpa_driver_nl80211_scan_loading_timeout(void *eloop_ctx, void *timeout_ctx)
{
    struct wpa_driver_nl80211_data *drv = eloop_ctx;

    wpa_printf(MSG_DEBUG, "Scan loading timeout - update channel list");
    wpa_supplicant_event(drv->ctx, EVENT_CHANNEL_LIST_CHANGED, NULL);
}

int wpa_driver_nl80211_driver_cmd(void *priv, char *cmd, char *buf,
                  size_t buf_len )
{
    struct i802_bss *bss = priv;
    struct wpa_driver_nl80211_data *drv = bss->drv;
    struct ifreq ifr;
    android_wifi_priv_cmd priv_cmd;
    struct wpa_supplicant *wpa_s;
    struct hostapd_data *hapd;
    int handled = 0;
    int cmd_len = 0;
    union wpa_event_data event;
    static int user_force_band = 0;
    int ret = -1;

    if (drv == NULL) {
        wpa_printf(MSG_ERROR, "%s: drv is NULL, exit", __func__);
        return -1;
    }
    if (drv->ctx == NULL) {
        wpa_printf(MSG_ERROR, "%s: drv->ctx is NULL, exit", __func__);
        return -1;
    }

    if (os_strcmp(bss->ifname, "ap0") == 0) {
        hapd = (struct hostapd_data *)(drv->ctx);
    }
    else {
        wpa_s = (struct wpa_supplicant *)(drv->ctx);
        if (wpa_s->conf == NULL) {
            wpa_printf(MSG_ERROR, "%s: wpa_s->conf is NULL, exit", __func__);
            return -1;
        }
    }

    wpa_printf(MSG_DEBUG, "%s: %s recv cmd %s", __func__, bss->ifname, cmd);
    handled = 1;

    if (os_strncasecmp(cmd, "POWERMODE ", 10) == 0) {
        int state;
        state = atoi(cmd + 10);
        wpa_printf(MSG_DEBUG, "POWERMODE=%d", state);
    }  else if (os_strncasecmp(cmd, "GET_STA_STATISTICS ", 19) == 0) {
        ret = wpa_driver_get_sta_statistics(wpa_s, cmd + 19, buf, buf_len);
    }  else if (os_strncmp(cmd, "MACADDR", os_strlen("MACADDR")) == 0) {
        u8 macaddr[ETH_ALEN] = {};
        os_memcpy(&macaddr, wpa_s->own_addr, ETH_ALEN);
        ret = snprintf(buf, buf_len, "Macaddr = " MACSTR "\n", MAC2STR(macaddr));
        wpa_printf(MSG_DEBUG, "%s", buf);
    } else if(os_strncasecmp(cmd, "COUNTRY", os_strlen("COUNTRY")) == 0) {
        if (os_strlen(cmd) != os_strlen("COUNTRY") + 3) {
            wpa_printf(MSG_DEBUG, "Ignore COUNTRY cmd %s", cmd);
            ret = 0;
        } else {
            wpa_printf(MSG_INFO, "Set country: %s", cmd + 8);
            // ret = wpa_drv_set_country(wpa_s, cmd + 8);
            ret = wpa_driver_mediatek_set_country(priv, cmd + 8);
            if (ret == 0) {
                wpa_printf(MSG_DEBUG, "Update channel list after country code changed");
                wpa_driver_notify_country_change(wpa_s, cmd);
            }
        }
    } else if (os_strcasecmp(cmd, "start") == 0) {
        if (ret = linux_set_iface_flags(drv->global->ioctl_sock,
            drv->first_bss->ifname, 1)) {
            wpa_printf(MSG_INFO, "nl80211: Could not set interface UP, ret=%d \n", ret);
        } else {
            wpa_msg(drv->ctx, MSG_INFO, "CTRL-EVENT-DRIVER-STATE STARTED");
        }
    } else if (os_strcasecmp(cmd, "stop") == 0) {
        if (drv->associated) {
            ret = wpa_drv_deauthenticate(wpa_s, drv->bssid, WLAN_REASON_DEAUTH_LEAVING);
            if (ret != 0)
                wpa_printf(MSG_DEBUG, "DRIVER-STOP error, ret=%d", ret);
        } else {
            wpa_printf(MSG_INFO, "nl80211: not associated, no need to deauthenticate \n");
        }

        if (ret = linux_set_iface_flags(drv->global->ioctl_sock,
            drv->first_bss->ifname, 0)) {
            wpa_printf(MSG_INFO, "nl80211: Could not set interface Down, ret=%d \n", ret);
        } else {
            wpa_msg(drv->ctx, MSG_INFO, "CTRL-EVENT-DRIVER-STATE STOPPED");
        }
    } else if (os_strncasecmp(cmd, "getpower", 8) == 0) {
        u32 mode;
        // ret = wpa_driver_wext_driver_get_power(drv, &mode);
        if (ret == 0) {
            ret = snprintf(buf, buf_len, "powermode = %u\n", mode);
            wpa_printf(MSG_DEBUG, "%s", buf);
            if (ret < (int)buf_len)
                return ret;
        }
    } else if (os_strncasecmp(cmd, "get-rts-threshold", 17) == 0) {
        u32 thd;
        // ret = wpa_driver_wext_driver_get_rts(drv, &thd);
        if (ret == 0) {
            ret = snprintf(buf, buf_len, "rts-threshold = %u\n", thd);
            wpa_printf(MSG_DEBUG, "%s", buf);
            if (ret < (int)buf_len)
                return ret;
        }
    } else if (os_strncasecmp(cmd, "set-rts-threshold", 17) == 0) {
        u32 thd = 0;
        char *cp = cmd + 17;
        char *endp;
        if (*cp != '\0') {
            thd = (u32)strtol(cp, &endp, 0);
            // if (endp != cp)
                // ret = wpa_driver_wext_driver_set_rts(drv, thd);
        }
    } else if (os_strncasecmp(cmd, "rxfilter-add", 12) == 0) {
        u32 sw_cmd = 0x9F000000;
        u32 idx = 0;
        char *cp = cmd + 12;
        char *endp;

        if (*cp != '\0') {
            idx = (u32)strtol(cp, &endp, 0);
            if (endp != cp) {
                idx += 0x00900200;
                wpa_driver_nl80211_driver_sw_cmd(priv, 1, &sw_cmd, &idx);
                ret = 0;
            }
        }
    } else if (os_strncasecmp(cmd, "rxfilter-remove", 15) == 0) {
        u32 sw_cmd = 0x9F000000;
        u32 idx = 0;
        char *cp = cmd + 15;
        char *endp;

        if (*cp != '\0') {
            idx = (u32)strtol(cp, &endp, 0);
            if (endp != cp) {
                idx += 0x00900300;
                wpa_driver_nl80211_driver_sw_cmd(priv, 1, &sw_cmd, &idx);
                ret = 0;
            }
        }
    } else if (os_strncasecmp(cmd, "rxfilter-stop", 13) == 0) {
        u32 sw_cmd = 0x9F000000;
        u32 idx = 0x00900000;
        wpa_driver_nl80211_driver_sw_cmd(priv, 1, &sw_cmd, &idx);
        ret = 0;
    } else if (os_strncasecmp(cmd, "rxfilter-start", 14) == 0) {
        u32 sw_cmd = 0x9F000000;
        u32 idx = 0x00900100;
        wpa_driver_nl80211_driver_sw_cmd(priv, 1, &sw_cmd, &idx);
        ret = 0;
    } else if (os_strcasecmp(cmd, "btcoexscan-start") == 0) {
        ret = 0; /* mt5921 linux driver not implement yet */
    } else if (os_strcasecmp(cmd, "btcoexscan-stop") == 0) {
        ret = 0; /* mt5921 linux driver not implement yet */
    } else if (os_strncasecmp(cmd, "btcoexmode", 10) == 0) {
        ret = 0; /* mt5921 linux driver not implement yet */
    } else if (os_strncasecmp(cmd, "smt-rate", 8) == 0 ) {
        u32 sw_cmd = 0xFFFF0123;
        u32 idx = 0;
        char *cp = cmd + 8;
        char *endp;

        if (*cp != '\0') {
            idx = (u32)strtol(cp, &endp, 0);
            if (endp != cp) {
                wpa_driver_nl80211_driver_sw_cmd(priv, 1, &sw_cmd, &idx);
                ret = 0;
            }
        }
    } else if (os_strncasecmp(cmd, "smt-test-on", 11) == 0 ) {
        u32 sw_cmd = 0xFFFF1234;
        u32 idx = 0;
        wpa_driver_nl80211_driver_sw_cmd(priv, 1, &sw_cmd, &idx);
        ret = 0;
    } else if (os_strncasecmp(cmd, "smt-test-off", 12) == 0 ) {
        u32 sw_cmd = 0xFFFF1235;
        u32 idx = 0;
        wpa_driver_nl80211_driver_sw_cmd(priv, 1, &sw_cmd, &idx);
        ret = 0;
    } else if (os_strncasecmp(cmd, "smt-test-scan-on", 16) == 0 ) {
        u32 sw_cmd = 0xFFFF1260;
        u32 idx = 0;
        wpa_driver_nl80211_driver_sw_cmd(priv, 1, &sw_cmd, &idx);
        ret = 0;
    } else if (os_strncasecmp(cmd, "smt-test-scan-off", 17) == 0 ) {
        u32 sw_cmd = 0xFFFF1261;
        u32 idx = 0;
        wpa_driver_nl80211_driver_sw_cmd(priv, 1, &sw_cmd, &idx);
        ret = 0;
    } else if (os_strncasecmp(cmd, "smt-test-roam-on", 16) == 0 ) {
        u32 sw_cmd = 0xFFFF1262;
        u32 idx = 0;
        wpa_driver_nl80211_driver_sw_cmd(priv, 1, &sw_cmd, &idx);
        ret = 0;
    } else if (os_strncasecmp(cmd, "smt-test-roam-off", 17) == 0 ) {
        u32 sw_cmd = 0xFFFF1263;
        u32 idx = 0;
        wpa_driver_nl80211_driver_sw_cmd(priv, 1, &sw_cmd, &idx);
        ret = 0;
    } else if (os_strncasecmp(cmd, "smt-test-cam-on", 15) == 0 ) {
        u32 sw_cmd = 0xFFFF1264;
        u32 idx = 0;
        wpa_driver_nl80211_driver_sw_cmd(priv, 1, &sw_cmd, &idx);
        ret = 0;
    } else if (os_strncasecmp(cmd, "smt-test-cam-off", 16) == 0 ) {
        u32 sw_cmd = 0xFFFF1265;
        u32 idx = 0;
        wpa_driver_nl80211_driver_sw_cmd(priv, 1, &sw_cmd, &idx);
        ret = 0;
    } else if (os_strncasecmp(cmd, "smt-test-bcn-on", 15) == 0 ) {
        u32 sw_cmd = 0xFFFF1266;
        u32 idx = 0;
        wpa_driver_nl80211_driver_sw_cmd(priv, 1, &sw_cmd, &idx);
        ret = 0;
    } else if (os_strncasecmp(cmd, "smt-test-bcn-off", 16) == 0 ) {
        u32 sw_cmd = 0xFFFF1267;
        u32 idx = 0;
        wpa_driver_nl80211_driver_sw_cmd(priv, 1, &sw_cmd, &idx);
        ret = 0;
    } else if (os_strncasecmp(cmd, "smt-test-power-on", 17) == 0 ) {
        u32 sw_cmd = 0xFFFF1268;
        u32 idx = 0;
        wpa_driver_nl80211_driver_sw_cmd(priv, 1, &sw_cmd, &idx);
        ret = 0;
    } else if (os_strncasecmp(cmd, "smt-test-power-off", 18) == 0 ) {
        u32 sw_cmd = 0xFFFF1269;
        u32 idx = 0;
        wpa_driver_nl80211_driver_sw_cmd(priv, 1, &sw_cmd, &idx);
        ret = 0;
    } else if (os_strncasecmp(cmd, "smt-test-fifo-on", 16) == 0 ) {
        u32 sw_cmd = 0xFFFF1270;
        u32 idx = 0;
        wpa_driver_nl80211_driver_sw_cmd(priv, 1, &sw_cmd, &idx);
        ret = 0;
    } else if (os_strncasecmp(cmd, "smt-test-fifo-off", 17) == 0 ) {
        u32 sw_cmd = 0xFFFF1271;
        u32 idx = 0;
        wpa_driver_nl80211_driver_sw_cmd(priv, 1, &sw_cmd, &idx);
        ret = 0;
#ifdef CONFIG_HOTSPOT_MGR_SUPPORT
    } else if (os_strncmp(cmd, "STA-BLOCK ", 10) == 0) {
        if (wpa_driver_sta_block(priv, cmd + 10)) {
            ret = -1;
        } else {
            ret = 0;
        }
    } else if (os_strncmp(cmd, "STA-UNBLOCK ", 12) == 0) {
        if (wpa_driver_sta_unblock(priv, cmd + 12)) {
            ret = -1;
        } else {
            ret = 0;
        }
#endif /* CONFIG_HOTSPOT_MGR_SUPPORT */
#ifdef CONFIG_MTK_WFD_SINK
    } else if (os_strncmp(cmd, "p2p_get_cap ", os_strlen("p2p_get_cap ")) == 0) {
        struct p2p_data *p2p = wpa_s->global->p2p;
        if (p2p) {
            wpa_printf(MSG_DEBUG, "%s %d, %d ",
                    __func__, __LINE__, p2p->dev_capab);
            ret = p2p_get_capability(wpa_s, cmd + os_strlen("p2p_get_cap "),
                          buf, buf_len);
        }
    } else if (os_strncmp(cmd, "p2p_set_cap ", os_strlen("p2p_set_cap ")) == 0) {
        struct p2p_data *p2p = wpa_s->global->p2p;
        if (p2p) {
            wpa_printf(MSG_DEBUG, "%s %d", __func__, __LINE__);
            ret = p2p_set_capability(wpa_s, cmd + os_strlen("p2p_set_cap "),
                          buf, buf_len);
        }
    } else if (os_strncmp(cmd, "MIRACAST ", os_strlen("MIRACAST ")) == 0) {
        unsigned char miracast = atoi(cmd + os_strlen("MIRACAST "));
        char *pos = os_strstr(cmd, " freq=");
        unsigned int freq = 0;
        int num;
        wpa_printf(MSG_DEBUG, "MIRACAST %d", miracast);
        switch (miracast) {
        case 0:
        case 1:
            num = mtk_get_shared_radio_freqs(wpa_s, &freq, 1);
            if (num > 0 && freq > 0) {
                wpa_printf(MSG_DEBUG, "AIS connected %d", freq);
                p2p_wfd_sink_config_scc(wpa_s, 1, freq);
            } else
                p2p_wfd_sink_config_scc(wpa_s, 0, 0);
            handled = 0; /* DRIVER MIRACAST used as private cmd*/
            break;
        case 2:
            if (pos) {
                pos += 6;
                freq = atoi(pos);
                wpa_printf(MSG_DEBUG, "MIRACAST freq %d", freq);
                p2p_wfd_sink_config_scc(wpa_s, 1, freq);
                /* rebuild DRIVER MIRACAST 2  cmd */
                os_memset(cmd, 0, os_strlen(cmd));
                os_memcpy(cmd, "MIRACAST 2", os_strlen("MIRACAST 2"));
            } else {
                num = mtk_get_shared_radio_freqs(wpa_s, &freq, 1);
                if (num > 0 && freq > 0) {
                    wpa_printf(MSG_DEBUG, "AIS connected %d", freq);
                    p2p_wfd_sink_config_scc(wpa_s, 1, freq);
                } else
                    p2p_wfd_sink_config_scc(wpa_s, 0, 0);
            }

            handled = 0; /* DRIVER MIRACAST used as private cmd*/

            break;
        default:
            wpa_printf(MSG_DEBUG, "Unknown MIRACAST value %d", miracast);
            handled = 0; /* DRIVER MIRACAST used as private cmd*/
            break;
        }

#endif
#ifdef CONFIG_MTK_SCC_MCC
    } else if (os_strncasecmp(cmd, "p2p_use_mcc=", os_strlen("p2p_use_mcc=")) == 0) {
        unsigned char use_mcc = atoi(cmd + os_strlen("p2p_use_mcc="));
        wpa_printf(MSG_DEBUG, "p2p_use_mcc %d", use_mcc);
        wpa_s->global->p2p->p2p_use_mcc = use_mcc;
        if (use_mcc) {
            wpa_printf(MSG_DEBUG, "SCC_MCC, config MCC");
            p2p_wfd_sink_config_scc(wpa_s, 0, 0);
        } else {
            int shared_freq;
            int num = 0;
            wpa_printf(MSG_DEBUG, "use_mcc=0");
            num = mtk_get_shared_radio_freqs(wpa_s, &shared_freq, 1);
            if (num > 0 && shared_freq > 0) {
                wpa_printf(MSG_DEBUG, "p2p disconnected, AIS connected %d", shared_freq);
                p2p_wfd_sink_config_scc(wpa_s, 1, shared_freq);
            } else
                p2p_wfd_sink_config_scc(wpa_s, 0, 0);
        }
#endif

#ifdef CONFIG_MTK_STAGE_SCAN
    } else if (os_strncasecmp(cmd, "set_band_dual", 13) == 0) {
        u32 sw_cmd = 0xFFFF1250;
        u32 idx = 0;
        if (user_force_band == 0) {  // STAGE_SCAN be enabled only if no force band is selected
            wpa_driver_nl80211_driver_sw_cmd(priv, 1, &sw_cmd, &idx);
            wpa_printf(MSG_DEBUG, "[STAGE_SCAN] Set Band = DUAL (sw_cmd: 0x%08X)", sw_cmd);
        }
        ret = 0;
    } else if (os_strncasecmp(cmd, "set_band_2g4", 12) == 0) {
        u32 sw_cmd = 0xFFFF1251;
        u32 idx = 0;
        if (user_force_band == 0) {  // STAGE_SCAN be enabled only if no force band is selected
            wpa_driver_nl80211_driver_sw_cmd(priv, 1, &sw_cmd, &idx);
            wpa_printf(MSG_DEBUG, "[STAGE_SCAN] Set Band = 2G4 (sw_cmd: 0x%08X)", sw_cmd);
        }
        ret = 0;
    } else if (os_strncasecmp(cmd, "set_band_5g", 11) == 0) {
        u32 sw_cmd = 0xFFFF1252;
        u32 idx = 0;
        if (user_force_band == 0) {  // STAGE_SCAN be enabled only if no force band is selected
            wpa_driver_nl80211_driver_sw_cmd(priv, 1, &sw_cmd, &idx);
            wpa_printf(MSG_DEBUG, "[STAGE_SCAN] Set Band = 5G (sw_cmd: 0x%08X)", sw_cmd);
        }
        ret = 0;
#if 0
    } else if (os_strncasecmp(cmd, "SETBAND", 7) == 0) {
        u32 sw_cmd = 0xFFFF1250;
        u32 idx = 0;
        char *value;
        int band = 0;

        value = os_strchr(cmd, ' ');
        if (value == NULL)
            return -1;
        *value++ = '\0';

        band = atoi(value);
        switch (band) {
            case 1:
                sw_cmd = 0xFFFF1252;
                wpa_printf(MSG_DEBUG, "[STAGE_SCAN] WIFI_FREQUENCY_BAND_5GHZ (sw_cmd: 0x%08X)", sw_cmd);
                user_force_band = 1;
                wpa_driver_nl80211_driver_sw_cmd(priv, 1, &sw_cmd, &idx);
                break;
            case 2:
                sw_cmd = 0xFFFF1251;
                wpa_printf(MSG_DEBUG, "[STAGE_SCAN] WIFI_FREQUENCY_BAND_2GHZ (sw_cmd: 0x%08X)", sw_cmd);
                user_force_band = 2;
                wpa_driver_nl80211_driver_sw_cmd(priv, 1, &sw_cmd, &idx);
                break;
            default:
                sw_cmd = 0xFFFF1250;
                wpa_printf(MSG_DEBUG, "[STAGE_SCAN] WIFI_FREQUENCY_BAND_AUTO (sw_cmd: 0x%08X)", sw_cmd);
                user_force_band = 0;
                wpa_driver_nl80211_driver_sw_cmd(priv, 1, &sw_cmd, &idx);
        }

        ret = 0;
#endif
    } else if (os_strncasecmp(cmd, "set_scan_loading", 16) == 0) {
        u32 sw_cmd = 0xFFFF1253;
        u32 idx = 0;

        wpa_printf(MSG_DEBUG, "[STAGE_SCAN] Set scan loading");
        wpa_driver_nl80211_driver_sw_cmd(priv, 1, &sw_cmd, &idx);

        wpa_printf(MSG_DEBUG, "[STAGE_SCAN] Scan loading timeout= 1 second");
        eloop_cancel_timeout(wpa_driver_nl80211_scan_loading_timeout, drv, drv->ctx);
        eloop_register_timeout(10, 0, wpa_driver_nl80211_scan_loading_timeout,
                   drv, drv->ctx);
        ret = 0;
#endif
    } else if (os_strncasecmp(cmd, "SETSUSPENDMODE ", 15) == 0) {
        struct wpa_driver_suspendmode_params params;
        params.hdr.index = NL80211_TESTMODE_SUSPEND;
        params.hdr.index = params.hdr.index | (0x01 << 24);
        params.hdr.buflen = sizeof(params);
        params.suspend = *(cmd+15)-'0';
        wpa_driver_nl80211_testmode(priv, &params, sizeof(params));
        handled = 0; /* 6630 driver handled this command in driver, so give a chance to 6630 driver */
    }else if(os_strncasecmp(cmd, "mtk_rx_packet_filter ", 21) == 0) {
        char buf[9] = {0}, errChar = 0;
        char *pos = NULL;
        /* mtk_rx_packet_filter 00000000000000FE 0000000000000000 0000000000000000 */
        struct wpa_driver_rx_filter_params params;
        params.hdr.index = NL80211_TESTMODE_RXFILTER;
        params.hdr.index = params.hdr.index | (0x01 << 24);
        params.hdr.buflen = sizeof(params);

        pos = cmd;
        pos = pos + 21;
        if (pos == NULL || strlen(cmd) != 71 ) {
            wpa_printf(MSG_DEBUG, "[mtk_rx_packet_filter] Error! \n");
            return -1;
        }

        os_memcpy(buf,pos,8);
        buf[8] = '\0';
        params.Ipv4FilterHigh = strtol(buf,&errChar,16);
        wpa_printf(MSG_DEBUG, "[mtk_rx_packet_filter]params.Ipv4FilterHigh (0x%08x),errChar [%c]\n", params.Ipv4FilterHigh,errChar);

        pos = pos + 8;
        os_memcpy(buf,pos,8);
        buf[8] = '\0';
        params.Ipv4FilterLow = strtol(buf,&errChar,16);
        wpa_printf(MSG_DEBUG, "[mtk_rx_packet_filter]params.Ipv4FilterLow (0x%08x),errChar [%c]\n", params.Ipv4FilterLow,errChar);

        pos = pos + 9;
        os_memcpy(buf,pos,8);
        buf[8] = '\0';
        params.Ipv6FilterHigh = strtol(buf,&errChar,16);
        wpa_printf(MSG_DEBUG, "[mtk_rx_packet_filter]params.Ipv6FilterHigh (0x%08x),errChar [%c]\n", params.Ipv6FilterHigh,errChar);

        pos = pos + 8;
        os_memcpy(buf,pos,8);
        buf[8] = '\0';
        params.Ipv6FilterLow = strtol(buf,&errChar,16);
        wpa_printf(MSG_DEBUG, "[mtk_rx_packet_filter]params.Ipv6FilterLow (0x%08x),errChar [%c]\n", params.Ipv6FilterLow,errChar);

        pos = pos + 9;
        os_memcpy(buf,pos,8);
        buf[8] = '\0';
        params.SnapFilterHigh = strtol(buf,&errChar,16);
        wpa_printf(MSG_DEBUG, "[mtk_rx_packet_filter]params.SnapFilterHigh (0x%08x),errChar [%c]\n", params.SnapFilterHigh,errChar);

        pos = pos + 8;
        os_memcpy(buf,pos,8);
        buf[8] = '\0';
        params.SnapFilterLow = strtol(buf,&errChar,16);
        wpa_printf(MSG_DEBUG, "[mtk_rx_packet_filter]params.SnapFilterLow (0x%08x),errChar [%c]\n", params.SnapFilterLow,errChar);

        ret = wpa_driver_nl80211_testmode(priv, &params, sizeof(params));
    } else {
        u8 buffer[100];
        struct wpa_driver_test_mode_info *params = (struct wpa_driver_test_mode_info *)buffer;
        params->index = NL80211_TESTMODE_STR_CMD | (0x01 << 24);
        params->buflen = sizeof(*params) + strlen(cmd);
        strncpy((u8*)(params+1), cmd, sizeof(buffer)-sizeof(*params));
        ret = wpa_driver_nl80211_testmode(priv, buffer, params->buflen);
        handled = 0;
        wpa_printf(MSG_INFO, "Transparent command for driver nl80211, ret=%d", ret);
    }

    if (handled == 0) {
        cmd_len = strlen(cmd);

        memset(&ifr, 0, sizeof(ifr));
        memset(&priv_cmd, 0, sizeof(priv_cmd));
        memset(buf, 0, buf_len);
        strncpy(ifr.ifr_name, bss->ifname, IFNAMSIZ);

        if (cmd_len >= PRIV_CMD_SIZE) {
            wpa_printf(MSG_INFO, "%s: cmd: %s overflow",
                      __func__, cmd);
            cmd_len = PRIV_CMD_SIZE - 1;
        }

        memcpy(priv_cmd.buf, cmd, cmd_len + 1);
        priv_cmd.used_len = cmd_len + 1;
        priv_cmd.total_len = PRIV_CMD_SIZE;
        ifr.ifr_data = &priv_cmd;

        ret = ioctl(drv->global->ioctl_sock, SIOCDEVPRIVATE + 1, &ifr);
        if (ret < 0) {
            wpa_printf(MSG_ERROR, "%s: failed to issue private commands,"
                    " error msg: %s\n", __func__, strerror(errno));
            wpa_driver_send_hang_msg(drv);
            ret = snprintf(buf, buf_len, "%s\n", "FAIL");
        } else {

            wpa_printf(MSG_DEBUG, "%s: ret = %d used = %u total = %u",
                    __func__, ret , priv_cmd.used_len, priv_cmd.total_len);

            drv_errors = 0;
            ret = snprintf(buf, buf_len, "%s\n", "OK");
            if ((os_strncasecmp(cmd, "WLS_BATCHING", 12) == 0))
                ret = strlen(buf);
            /*
	     * There no need to call wpa_supplicant_event func
	     * on which the cmd is SETBAND
	     */
            if (os_strncasecmp(cmd, "SETBAND", 7) == 0) {
                /*
                 * wpa_supplicant_event(drv->ctx,
                 * 		EVENT_CHANNEL_LIST_CHANGED, NULL);
                 */
                wpa_printf(MSG_INFO, "%s: Unsupported command SETBAND\n", __func__);
            }
        }
    } /* handled == 0 */

    return ret;
}

int wpa_driver_set_p2p_noa(void *priv, u8 count, int start, int duration)
{
    struct i802_bss *bss = priv;
    struct wpa_driver_nl80211_data *drv = bss->drv;

    wpa_printf(MSG_DEBUG, "iface %s P2P_SET_NOA %d %d %d, ignored", bss->ifname, count, start, duration);
    return -1;
}

int wpa_driver_get_p2p_noa(void *priv, u8 *buf, size_t len)
{
    struct i802_bss *bss = priv;
    struct wpa_driver_nl80211_data *drv = bss->drv;

    wpa_printf(MSG_DEBUG, "iface %s P2P_GET_NOA, ignored", bss->ifname);
    return -1;
}

int wpa_driver_set_p2p_ps(void *priv, int legacy_ps, int opp_ps, int ctwindow)
{
    struct i802_bss *bss = priv;
    struct wpa_driver_nl80211_data *drv = bss->drv;

    wpa_printf(MSG_DEBUG, "iface %s P2P_SET_PS, ignored", bss->ifname);
    return -1;
}

int wpa_driver_set_ap_wps_p2p_ie(void *priv, const struct wpabuf *beacon,
                 const struct wpabuf *proberesp,
                 const struct wpabuf *assocresp)
{
    struct i802_bss *bss = priv;
    struct wpa_driver_nl80211_data *drv = bss->drv;

    wpa_printf(MSG_DEBUG, "iface %s set_ap_wps_p2p_ie, ignored", bss->ifname);
    return 0;
}

