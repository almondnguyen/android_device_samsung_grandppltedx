/* //device/libs/telephony/ril.cpp
**
** Copyright 2017, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
**
** Oreo RIL library for MTK devices - Version (0.9).
** by: daniel_hk(https://github.com/danielhk)
**
** 2017/7/29: initial port for MT6572			by: daniel_hk
** 2017/8/5: initial service porting			by: daniel_hk
** 2017/8/6: divert the 3 callbacks to librilmtk	by: daniel_hk
** 2017/8/12: handle extra MTK commands and responses	by: daniel_hk
** 2017/8/12: emulate MTK's proxyID			by: daniel_hk
** 2017/8/19: stick with 1.0 for now			by: daniel_hk
** 2017/8/19: use my own RIL_requestProxyTimedCallback	by: daniel_hk
** 2017/8/26: try RIL_CHANNEL_QUEUING method		by: daniel_hk
** 2017/8/26: my own approach for queuing	 	by: daniel_hk
** 2017/8/27: add RIL_UNSOL_PENDING for testing		by: daniel_hk
** 2017/9/2: try to figure out the channelID locally	by: daniel_hk
** 2017/9/2: another way to restart the service		by: daniel_hk
** 2017/9/3: revert to MTK's queuing		 	by: daniel_hk
** 2017/9/3: new approach for callbacks			by: daniel_hk
** 2017/9/9: use my own proxyID methods			by: daniel_hk
** 2017/9/9: better use of local commands		by: daniel_hk
** 2017/9/10: use MTK's property to restart the service	by: daniel_hk
** 2017/9/10: write my own my_enqueue			by: daniel_hk
** 2017/9/16: disable RIL_CHANNEL_QUEUING for now	by: daniel_hk
** 2017/9/16: patch APN crash in mtkril			by: daniel_hk
** 2017/9/17: write my own isInitialAttachAPN		by: daniel_hk
** 2017/9/17: revert to my queuing, RIL working now!	by: daniel_hk
** 2017/9/23: handle only mtk unsol commands now	by: daniel_hk
** 2017/9/23: use custom spn list to match plmn		by: daniel_hk
** 2020/01/05: handle ril identity in a better way		by: bilux (i.bilux@gmail.com)
*/

#define LOG_TAG "RILC"

#include <hardware_legacy/power.h>
#include <telephony/ril.h>
#include <telephony/ril_cdma_sms.h>
#include <cutils/sockets.h>
#include <cutils/jstring.h>
#include <telephony/record_stream.h>
#include <utils/Log.h>
#include <utils/SystemClock.h>
#include <pthread.h>
#include <cutils/jstring.h>
#include <sys/types.h>
#include <sys/limits.h>
#include <sys/system_properties.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <assert.h>
#include <ctype.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <cutils/properties.h>
#include <RilSapSocket.h>
#include <ril_service.h>
#include <sap_service.h>

extern "C" void
RIL_onRequestComplete(RIL_Token t, RIL_Errno e, void *response, size_t responselen);

extern "C" bool
IMS_isRilRequestFromIms(RIL_Token t);

extern "C"
void IMS_RIL_onUnsolicitedResponseSocket(int unsolResponse, const void *data,
				size_t datalen, RIL_SOCKET_ID socket_id);

extern "C" void
RIL_onRequestAck(RIL_Token t);
namespace android {

#define PHONE_PROCESS "radio"
#define BLUETOOTH_PROCESS "bluetooth"

#define ANDROID_WAKE_LOCK_NAME "radio-interface"

#define ANDROID_WAKE_LOCK_SECS 0
#define ANDROID_WAKE_LOCK_USECS 200000

#define PROPERTY_RIL_IMPL "gsm.version.ril-impl"

// match with constant in RIL.java
#define MAX_COMMAND_BYTES (8 * 1024)

// Basically: memset buffers that the client library
// shouldn't be using anymore in an attempt to find
// memory usage issues sooner.
#define MEMSET_FREED 1

#define NUM_ELEMS(a)     (sizeof (a) / sizeof (a)[0])

/* Negative values for private RIL errno's */
#define RIL_ERRNO_INVALID_RESPONSE (-1)
#define RIL_ERRNO_NO_MEMORY (-12)

// request, response, and unsolicited msg print macro
#define PRINTBUF_SIZE 8096

enum WakeType {DONT_WAKE, WAKE_PARTIAL};

typedef struct {
    int requestNumber;
    int (*responseFunction) (int slotId, int responseType, int token,
	    RIL_Errno e, void *response, size_t responselen);
    WakeType wakeType;
} UnsolResponseInfo;

typedef struct UserCallbackInfo {
    RIL_TimedCallback p_callback;
    void *userParam;
    struct ril_event event;
    struct UserCallbackInfo *p_next;
    RILChannelId cid;	// For command dispatch after onRequest()
} UserCallbackInfo;

typedef struct {
    int unsolResponse;
    void *data;
    size_t datalen;
    RIL_SOCKET_ID socket_id;
} USRbuffer;

typedef struct ProxyReqInfo {
    struct RequestInfo *pRI;
    struct ProxyReqInfo *pNext;
    void *buf;
    size_t buflen;
    BUF_FMTS bf;
} ProxyReqInfo;

typedef struct {
    int cid;
    long pthread;
} ThreadProxyId;

extern "C" const char * failCauseToString(RIL_Errno);
extern "C" const char * callStateToString(RIL_CallState);
extern "C" const char * radioStateToString(RIL_RadioState);
extern "C" const char * rilSocketIdToString(RIL_SOCKET_ID socket_id);
extern "C" const char * proxyString (RILChannelId id);

extern "C"
char ril_service_name_base[MAX_SERVICE_NAME_LENGTH] = RIL_SERVICE_NAME_BASE;
extern "C"
char ril_service_name[MAX_SERVICE_NAME_LENGTH] = RIL1_SERVICE_NAME;

// from librilmtk
extern "C" void
RIL_requestProxyTimedCallback (RIL_TimedCallback callback,
			       void *param, const struct timeval *relativeTime, int proxyId);
extern "C" RILChannelId RIL_queryMyChannelId(RIL_Token t);
extern "C" int RIL_queryMyProxyIdByThread();
extern "C" int isRequestTokFromMal(RIL_Token t);
extern void RIL_startRILProxys();
// replaced in mtk-ril
extern "C" void getIaCache(char* cache);
// see what it is doing
extern void enqueue(RequestInfo* pRI, void *buffer, size_t buflen,
		    UserCallbackInfo* pUCI, RIL_SOCKET_ID socket_id);

/*******************************************************************/

//RIL_RadioFunctions s_callbacks = {0, NULL, NULL, NULL, NULL, NULL};
RIL_RadioFunctionsSocket s_callbacksSocket = {0, NULL, NULL, NULL, NULL, NULL};
static int s_registerCalled = 0;

static pthread_t s_tid_dispatch;
static pthread_t s_tid_reader;
static int s_started = 0;

static int s_fdDebug = -1;
static int s_fdDebug_socket2 = -1;

static int mtk_command_start = 0;
static int mtk_unsol_start = 0;

static int s_fdWakeupRead;
static int s_fdWakeupWrite;

int s_wakelock_count = 0;

static struct ril_event s_wakeupfd_event;

static pthread_mutex_t s_pendingRequestsMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t s_writeMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t s_wakeLockCountMutex = PTHREAD_MUTEX_INITIALIZER;
static RequestInfo *s_pendingRequests = NULL;

#if (SIM_COUNT >= 2)
static pthread_mutex_t s_pendingRequestsMutex_socket2  = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t s_writeMutex_socket2	    = PTHREAD_MUTEX_INITIALIZER;
static RequestInfo *s_pendingRequests_socket2	  = NULL;
#endif

#if (SIM_COUNT >= 3)
static pthread_mutex_t s_pendingRequestsMutex_socket3  = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t s_writeMutex_socket3	    = PTHREAD_MUTEX_INITIALIZER;
static RequestInfo *s_pendingRequests_socket3	  = NULL;
#endif

#if (SIM_COUNT >= 4)
static pthread_mutex_t s_pendingRequestsMutex_socket4  = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t s_writeMutex_socket4	    = PTHREAD_MUTEX_INITIALIZER;
static RequestInfo *s_pendingRequests_socket4	  = NULL;
#endif

static struct ril_event s_wake_timeout_event;
static struct ril_event s_debug_event;

static const struct timeval TIMEVAL_WAKE_TIMEOUT = {ANDROID_WAKE_LOCK_SECS,ANDROID_WAKE_LOCK_USECS};

static pthread_mutex_t s_startupMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t s_startupCond = PTHREAD_COND_INITIALIZER;

static pthread_mutex_t s_dispatchMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t s_dispatchCond = PTHREAD_COND_INITIALIZER;

static RequestInfo *s_toDispatchHead = NULL;
static RequestInfo *s_toDispatchTail = NULL;

static UserCallbackInfo *s_last_wake_timeout_info = NULL;

static void *s_lastNITZTimeData = NULL;
static size_t s_lastNITZTimeDataSize;

#if RILC_LOG
    static char printBuf[PRINTBUF_SIZE];
#endif

/*******************************************
	MTK porting
*******************************************/

#ifdef RIL_CHANNEL_QUEUING
// TODO:handle SIM_COUNT > 2??
static ProxyReqInfo *proxyCmd1[SIM_COUNT] = {NULL, NULL};
static ProxyReqInfo *proxyCmd3[SIM_COUNT] = {NULL, NULL};
static bool cmdProxy1busy[SIM_COUNT] = {false, false};
static bool cmdProxy3busy[SIM_COUNT] = {false, false};
#endif

static void *s_prevEcopsData[SIM_COUNT] = {NULL};
static size_t s_prevEcopsDataSize[SIM_COUNT];

#ifdef RIL_UNSOL_PENDING
#define MAX_PENDING_USR	16
static int pendingUSRcnt = 0;
static USRbuffer pendingUSR[MAX_PENDING_USR];
#endif

// device identity
typedef struct {
    char *imei;
    char *imeisv;
    char *esnHex;
    char *meidHex;
} RIL_IDENTITY;

static RIL_IDENTITY Device_ID[SIM_COUNT];

#if (SIM_COUNT > 3)
  #define MAX_RIL_CHANNELS	24
#elif (SIM_COUNT > 2)
  #define MAX_RIL_CHANNELS	18
#elif (SIM_COUNT > 1)
  #define MAX_RIL_CHANNELS	12
#else
  #define MAX_RIL_CHANNELS	6
#endif
// for RIL_myChannelId (aka RIL_queryMyChannelId)
static ThreadProxyId threadPid[MAX_RIL_CHANNELS]; // MAX_RIL_CHANNELS may be good enough

/*******************************************************************/
static void grabPartialWakeLock();
void releaseWakeLock();
static void wakeTimeoutCallback(void *);

#ifdef RIL_SHLIB
#if defined(ANDROID_MULTI_SIM)
extern "C" void RIL_onUnsolicitedResponse(int unsolResponse, const void *data,
				size_t datalen, RIL_SOCKET_ID socket_id);
#else
extern "C" void RIL_onUnsolicitedResponse(int unsolResponse, const void *data,
				size_t datalen);
#endif
#endif

#ifdef MTK_HARDWARE
#define RIL_UNSOL_RESPONSE(a, b, c, d) RIL_onUnsolicitedResponseSocket((a), (b), (c), (d))
#define CALL_ONREQUEST(a, b, c, d, e) s_callbacksSocket.onRequest((a), (b), (c), (d), (e))
#define CALL_ONSTATEREQUEST(a) s_callbacksSocket.onStateRequest(a)
#else
#if defined(ANDROID_MULTI_SIM)
#define RIL_UNSOL_RESPONSE(a, b, c, d) RIL_onUnsolicitedResponse((a), (b), (c), (d))
#define CALL_ONREQUEST(a, b, c, d, e) s_callbacksSocket.onRequest((a), (b), (c), (d), (e))
#define CALL_ONSTATEREQUEST(a) s_callbacksSocket.onStateRequest(a)
#else
#define RIL_UNSOL_RESPONSE(a, b, c, d) RIL_onUnsolicitedResponse((a), (b), (c))
#define CALL_ONREQUEST(a, b, c, d, e) s_callbacksSocket.onRequest((a), (b), (c), (d))
#define CALL_ONSTATEREQUEST(a) s_callbacksSocket.onStateRequest()
#endif
#endif

static UserCallbackInfo * internalRequestTimedCallback
    (RIL_TimedCallback callback, void *param, const struct timeval *relativeTime, int cid);

/** Index == requestNumber */
static CommandInfo s_commands[] = {
#include "ril_commands.h"
//#include "mtk_ril_commands.h"
};

static UnsolResponseInfo s_unsolResponses[] = {
#include "ril_unsol_commands.h"
#include "mtk_ril_unsol_commands.h"
};

char * RIL_getServiceName() {
    return ril_service_name;
}

void addThreadPid(int cid, long tid)
{
   int j,i=0;

   while ((i < MAX_RIL_CHANNELS) &&
	  (threadPid[i].pthread != 0) &&
	  (threadPid[i].pthread != tid))
	i++;
    /* if (threadPid[i].pthread == 0) or same tid
	add/move to top and replace the cid */
    if (i == MAX_RIL_CHANNELS) {
	// reach max. drop the last entry
	i = MAX_RIL_CHANNELS-1;
    }
    j = i;
    while (j > 0) {
	threadPid[j] = threadPid[j-1];
	j--;
    }
    //put on top
    threadPid[0].pthread = tid;
    threadPid[0].cid = cid;
}

extern "C"
void RIL_setServiceName(const char * s) {
    strncpy(ril_service_name, s, MAX_SERVICE_NAME_LENGTH);
}
// MTK functions in librilmtk
extern "C" void RIL_myProxyTimedCallback (RIL_TimedCallback callback, void *param,
				const struct timeval *relativeTime, int proxyId)
{
#if VDBG
    RLOGD("RIL_requestProxyTimedCallback:***proxyId=%d,pthread_self()=%lu",
					proxyId, pthread_self());
#endif
    RIL_requestProxyTimedCallback(callback, param, relativeTime, proxyId);
// replaced
//	internalRequestTimedCallback(callback, param, relativeTime, proxyId);
    if (proxyId > -1) {
	RLOGD("internalRequestTimedCallback, ***pthread_self()=%lu cid=%d",
				pthread_self(), proxyId);
	addThreadPid(proxyId, pthread_self());	// ** guess ??
    }
}

extern "C" RILChannelId RIL_myChannelId(RIL_Token t)
{
    RequestInfo* r = (RequestInfo*)t;
    //TODO:handle more than 2 sims
    int cid = r->socket_id;
    RILChannelId id = RIL_queryMyChannelId(t);
/*    if (cid == RIL_SOCKET_1)
	cid = r->pCI->proxyId;
    else cid = r->pCI->proxyId + RIL_CHANNEL_OFFSET;*/
    cid = r->cid;
#if VDBG
    RLOGD("RIL_queryMyChannelId:***cid=%d,id=%d,pthread_self()=%lu", cid, (int)id, pthread_self());
#endif
//    addThreadPid(cid, pthread_self());
    return (RILChannelId)cid;
}

extern "C" int RIL_myProxyIdByThread()
{
    int cid;
    int i=0;
    while ((threadPid[i].pthread != pthread_self()) && (i < MAX_RIL_CHANNELS))
	i++;
    cid = (i < MAX_RIL_CHANNELS)? threadPid[i].cid : -1;
    i = RIL_queryMyProxyIdByThread();
#if VDBG
    RLOGD("RIL_queryMyProxyIdByThread:***cid=%d,i=%d,pthread_self()=%lu", cid, i, pthread_self());
#endif
    return i;
}

// for mtk-ril
extern "C"
int isRequestTokFromMal(RIL_Token t)
{
    RLOGD("isRequestTokFromMal:token:%p", t);
    return 0;
}

static char PROPERTY_ICCID_SIM[SIM_COUNT][16] =
{
    "ril.iccid.sim1",
#if (SIM_COUNT > 1)
    "ril.iccid.sim2",
#if (SIM_COUNT > 2)
    "ril.iccid.sim3",
#if (SIM_COUNT > 3)
    "ril.iccid.sim4"
#endif
#endif
#endif
};

struct RILChannelCtx;
extern "C"
int isInitialAttachAPN(const char *requestedApn, const char * protocol,
		int authType, const char *username, const char* password, RILChannelCtx *pChannel)
{
    char iaProperty[PROPERTY_VALUE_MAX * 2] = { 0 };
    getIaCache(iaProperty);
    RLOGD("[RILData_GSM_IRAT]: isInitialAttachApn IaCache=%s", iaProperty);
    if (strlen(iaProperty) == 0) {
	// No initial attach APN, return false.
	return 0;
    }
#if VDBG
    RLOGD("isInitialAttachApn: reqApn:%s, ptcol:%s, authT:%d, uname:%s, pw:%s",
		requestedApn, protocol, authType, username, password);
#endif
    // Check if current IA property is different with what we want to set
    char apnParameter[PROPERTY_VALUE_MAX * 2] = { 0 };
    int canHandleIms = 0;
    if (password == NULL || strlen(password) == 0) {
	char iccid[PROPERTY_VALUE_MAX] = { 0 };
	property_get(PROPERTY_ICCID_SIM[0], iccid, "");
	sprintf(apnParameter, "%s,%s,%d,%s,0,%s", iccid, protocol,
			authType, username, requestedApn);
    } else {
	//when password is set, iccid is not recorded, so we do not need to compare iccid
	printf(apnParameter, "%s,%s,%d,%s,0,%s", "", protocol,
			authType, username, requestedApn);
    }

    RLOGD("[RILData_GSM_IRAT]: isInitialAttachApn IaCache=%s, apnParameter=%s.", iaProperty,
			apnParameter);
    if (strcmp(apnParameter, iaProperty) == 0 || strcmp(requestedApn, "ctnet") == 0) {
	return 1;
    }
    return 0;
}

// for rild_prop in rild
extern "C"
void RIL_MDStateChange(RIL_MDState state)
{
    int i = 0;
    int state_tmp = (int) state;

    /* for each connected socket, rild should send onsolicited notification to RILJ(phone)*/
    for (i=0; i < SIM_COUNT; i++)
    {
	/*if the socket is not connected (-1), the unsol message will be ignore*/
	RIL_UNSOL_RESPONSE(RIL_UNSOL_MD_STATE_CHANGE, &state_tmp, sizeof(int),
				(RIL_SOCKET_ID) (RIL_SOCKET_1+i));
    }
}
// end MTK

RequestInfo *
addRequestToList(int serial, int slotId, int request) {
    RequestInfo *pRI;
    int ret;
    RIL_SOCKET_ID socket_id = (RIL_SOCKET_ID) slotId;
    /* Hook for current context */
    /* pendingRequestsMutextHook refer to &s_pendingRequestsMutex */
    pthread_mutex_t* pendingRequestsMutexHook = &s_pendingRequestsMutex;
    /* pendingRequestsHook refer to &s_pendingRequests */
    RequestInfo**    pendingRequestsHook = &s_pendingRequests;

#if (SIM_COUNT >= 2)
    if (socket_id == RIL_SOCKET_2) {
	pendingRequestsMutexHook = &s_pendingRequestsMutex_socket2;
	pendingRequestsHook = &s_pendingRequests_socket2;
    }
#if (SIM_COUNT >= 3)
    else if (socket_id == RIL_SOCKET_3) {
	pendingRequestsMutexHook = &s_pendingRequestsMutex_socket3;
	pendingRequestsHook = &s_pendingRequests_socket3;
    }
#endif
#if (SIM_COUNT >= 4)
    else if (socket_id == RIL_SOCKET_4) {
	pendingRequestsMutexHook = &s_pendingRequestsMutex_socket4;
	pendingRequestsHook = &s_pendingRequests_socket4;
    }
#endif
#endif

    pRI = (RequestInfo *)calloc(1, sizeof(RequestInfo));
    if (pRI == NULL) {
	RLOGE("Memory allocation failed for request %s", requestToString(request));
	return NULL;
    }

    if (serial == -1)
	pRI->local = 1;
    pRI->token = serial;
    ret = (request < RIL_REQUEST_VENDOR_BASE) ? 
	ret = request :
 	ret = request - RIL_REQUEST_VENDOR_BASE + mtk_command_start;
    pRI->pCI = &(s_commands[ret]);
    pRI->socket_id = socket_id;
    //TODO:handle more than 2 sims
    ret = pRI->pCI->proxyId;
    if (slotId > RIL_SOCKET_1)
	ret += RIL_CHANNEL_OFFSET;
    pRI->cid = (RILChannelId)ret;

    ret = pthread_mutex_lock(pendingRequestsMutexHook);
    assert (ret == 0);

    pRI->p_next = *pendingRequestsHook;
    *pendingRequestsHook = pRI;

    ret = pthread_mutex_unlock(pendingRequestsMutexHook);
    assert (ret == 0);

    return pRI;
}

static void triggerEvLoop() {
    int ret;
    if (!pthread_equal(pthread_self(), s_tid_dispatch)) {
	/* trigger event loop to wakeup. No reason to do this,
	 * if we're in the event loop thread */
	 do {
	    ret = write (s_fdWakeupWrite, " ", 1);
	 } while (ret < 0 && errno == EINTR);
    }
}

static void rilEventAddWakeup(struct ril_event *ev) {
    ril_event_add(ev);
    triggerEvLoop();
}

/**
 * A write on the wakeup fd is done just to pop us out of select()
 * We empty the buffer here and then ril_event will reset the timers on the
 * way back down
 */
static void processWakeupCallback(int fd, short flags, void *param) {
    char buff[16];
    int ret;

    RLOGV("processWakeupCallback");

    /* empty our wakeup socket out */
    do {
	ret = read(s_fdWakeupRead, &buff, sizeof(buff));
    } while (ret > 0 || (ret < 0 && errno == EINTR));
}

static void resendLastNITZTimeData(RIL_SOCKET_ID socket_id) {
    if (s_lastNITZTimeData != NULL) {
	int responseType = (s_callbacksSocket.version >= 13)
			   ? RESPONSE_UNSOLICITED_ACK_EXP
			   : RESPONSE_UNSOLICITED;
	int ret = radio::nitzTimeReceivedInd(
	    (int)socket_id, responseType, 0,
	    RIL_E_SUCCESS, s_lastNITZTimeData, s_lastNITZTimeDataSize);
	if (ret == 0) {
	    free(s_lastNITZTimeData);
	    s_lastNITZTimeData = NULL;
	}
    }
}

void onNewCommandConnect(RIL_SOCKET_ID socket_id) {
    // Inform we are connected and the ril version
    int rilVer = s_callbacksSocket.version;
    char prop[PROPERTY_VALUE_MAX];
    RLOGD("**onNewCommandConnect,socket=%d,pthread=%lu",socket_id, pthread_self());

#ifdef MTK_HARDWARE
#define GSM_RIL_INIT	"gsm.ril.init"
    do {
	sleep(1);  // sleep 1s
	// wait until init callbacks finished, OR haven't started
	property_get(GSM_RIL_INIT, prop, "1");
    } while (strcmp(prop, "1"));
#endif
    RIL_UNSOL_RESPONSE(RIL_UNSOL_RIL_CONNECTED, &rilVer, sizeof(rilVer), socket_id);

    // implicit radio state changed
    RIL_UNSOL_RESPONSE(RIL_UNSOL_RESPONSE_RADIO_STATE_CHANGED, NULL, 0, socket_id);

    // Send last NITZ time data, in case it was missed
    if (s_lastNITZTimeData != NULL) {
	resendLastNITZTimeData(socket_id);
    }

    // Send previous  ECOPS data in case EOS happened before worldphone send resume camping
// **HOW??
//    if (s_prevEcopsData[socket_id] != NULL) {
//        sendResponseRaw(s_prevEcopsData[socket_id], s_prevEcopsDataSize[socket_id], socket_id);
//    }

    // Get version string
    if (s_callbacksSocket.getVersion != NULL) {
	const char *version;
	version = s_callbacksSocket.getVersion();
	RLOGI("RIL Daemon version: %s\n", version);

	property_set(PROPERTY_RIL_IMPL, version);
    } else {
	RLOGI("RIL Daemon version: unavailable\n");
	property_set(PROPERTY_RIL_IMPL, "unavailable");
    }

#ifdef MTK_HARDWARE
//    RIL_UNSOL_RESPONSE(RIL_UNSOL_SET_ATTACH_APN, NULL, 0, socket_id); // reset apn??
// MTK modem stuff
#define RIL_MUXREP_CASE	"ril.mux.report.case"
#define RIL_MUXREPORT	"ril.muxreport"
    if ( 0 == property_get(RIL_MUXREPORT, prop, NULL) && (socket_id == RIL_SOCKET_2)) {
	RLOGD("**reset modem**");
	property_set(GSM_RIL_INIT, "0");		// clear ril init
	// reset modem and service once after boot
	property_set(RIL_MUXREP_CASE, "2");		// reset modem 1
	property_set("ctl.start", "muxreport-daemon");	// activate
	property_set(RIL_MUXREPORT, "0");		// clear
    }
#endif
}

//static 
void userTimerCallback (int fd, short flags, void *param) {
    UserCallbackInfo *p_info;

    p_info = (UserCallbackInfo *)param;

// Register callback too ??
    if (p_info->cid >= RIL_URC) {
	RLOGD("userTimerCallback: call directly, cid=%u, pthread=%lu",
		p_info->cid, pthread_self());
#if 1
	enqueue(NULL, NULL, 0, p_info, RIL_SOCKET_1);
	return;
#else
//	int cid = (int)p_info->cid;
//	threadPid[cid] = pthread_self();	// ** guess ??
//	addThreadPid(cid, pthread_self());
	p_info->p_callback(p_info->userParam);
#endif
    } else
	p_info->p_callback(p_info->userParam);

    // FIXME generalize this...there should be a cancel mechanism
    if (s_last_wake_timeout_info != NULL && s_last_wake_timeout_info == p_info) {
	s_last_wake_timeout_info = NULL;
    }

    free(p_info);
}

static void *
eventLoop(void *param) {
    int ret;
    int filedes[2];

    ril_event_init();

    pthread_mutex_lock(&s_startupMutex);

    s_started = 1;
    pthread_cond_broadcast(&s_startupCond);

    pthread_mutex_unlock(&s_startupMutex);

    ret = pipe(filedes);

    if (ret < 0) {
	RLOGE("Error in pipe() errno:%d", errno);
	return NULL;
    }

    s_fdWakeupRead = filedes[0];
    s_fdWakeupWrite = filedes[1];

    fcntl(s_fdWakeupRead, F_SETFL, O_NONBLOCK);

    ril_event_set (&s_wakeupfd_event, s_fdWakeupRead, true,
		processWakeupCallback, NULL);

    rilEventAddWakeup (&s_wakeupfd_event);

    // Only returns on error
    ril_event_loop();
    RLOGE ("error in event_loop_base errno:%d", errno);
    // kill self to restart on error
    kill(0, SIGKILL);

    return NULL;
}
#ifdef RIL_UNSOL_PENDING
// daniel added functions
void clrUSRBuffer(void)
{
    if (pendingUSRcnt > 0) {
	RLOGI("clear the USR buffer");
	for (int i = 0; i < pendingUSRcnt; i++) {
	 // only those with data is pending, so won't be NULL
	    free(pendingUSR[i].data);
	}
	pendingUSRcnt = 0;
    }
}
#endif

void memsetAndFreeStrings(int numPointers, ...) {
    va_list ap;
    va_start(ap, numPointers);
    for (int i = 0; i < numPointers; i++) {
	char *ptr = va_arg(ap, char *);
	if (ptr) {
#ifdef MEMSET_FREED
#define MAX_STRING_LENGTH 4096
	    memset(ptr, 0, strnlen(ptr, MAX_STRING_LENGTH));
#endif
	    free(ptr);
	}
    }
    va_end(ap);
}

void clearPendingBuffer(BUF_FMTS bf, void *buf, size_t buflen)
{
    switch (bf) {
	case FMT_INTS: {
#ifdef MEMSET_FREED
		memset(buf, 0, buflen);
#endif
	    }
	    break;
	case FMT_STR: {
		char *pString = (char*)buf;
		memsetAndFreeStrings(1, pString);
	    }
	    break;
	case FMT_STRS: {
		char **pStrings = (char **)buf;
		int countStrings = buflen / sizeof(char *);
		for (int i = 0 ; i < countStrings ; i++) {
		memsetAndFreeStrings(1, pStrings[i]);
		}
	    }
	    break;
	case FMT_CallFWST: {
		RIL_CallForwardInfo *cf = (RIL_CallForwardInfo *)buf;
		memsetAndFreeStrings(1, cf->number);
	    }
	    break;
	case FMT_IccAPDU: {
		RIL_SIM_APDU *apdu = (RIL_SIM_APDU *)buf;
		memsetAndFreeStrings(1, apdu->data);
	    }
	    break;
	case FMT_SIMIO: {
		RIL_SIM_IO_v6 *rilIccIo = (RIL_SIM_IO_v6 *)buf;
		memsetAndFreeStrings(4, rilIccIo->path, rilIccIo->data,
					rilIccIo->pin2, rilIccIo->aidPtr);
	    }
	    break;
	case FMT_WrSMSSIM: {
		RIL_SMS_WriteArgs *args = (RIL_SMS_WriteArgs *)buf;
		memsetAndFreeStrings(2, args->smsc, args->pdu);
	    }
	    break;
	case FMT_AttchAPN: {
		RIL_InitialAttachApn *iaa = (RIL_InitialAttachApn *)buf;
		memsetAndFreeStrings(5, iaa->apn, iaa->protocol, iaa->username,
					iaa->password, iaa->operatorNumeric);
	    }
	    break;
	case FMT_ImsSms: {
		RIL_IMS_SMS_Message *rism = (RIL_IMS_SMS_Message *)buf;
		char **pStrings = (char **)rism->message.gsmMessage;
		memsetAndFreeStrings(2, pStrings[0], pStrings[1]);
		free(pStrings);
	    }
	    break;
	case FMT_SIMAUTH: {
		RIL_SimAuthentication *pf = (RIL_SimAuthentication *)buf;
		memsetAndFreeStrings(2, pf->authData, pf->aid);
	    }
	    break;
	case FMT_DATAPROF: {
		RIL_DataProfileInfo **dataProfilePtrs = (RIL_DataProfileInfo **)buf;
		int numProfiles = buflen / sizeof(RIL_DataProfileInfo *);
		for (int i = 0; i < numProfiles; i++) {
		    RIL_DataProfileInfo *dp = dataProfilePtrs[i];
		    memsetAndFreeStrings(4, dp->apn, dp->protocol, dp->user, dp->password);
		    free(dp);
		}
	    }
	    break;
	default:
	    // FMT_RAW , no extra clean needed
	    break;
    }
}

#ifdef RIL_CHANNEL_QUEUING
void my_dequeue(int slot, RILChannelId cid)
{
    // TODO: check mux lock
    ProxyReqInfo *p = NULL;

    if (cid == RIL_CMD_PROXY_1) {
	p = proxyCmd1[slot];
	if (p) {
	    proxyCmd1[slot] = proxyCmd1[slot]->pNext;
//	    if (p->pRI->pCI->requestNumber == RIL_REQUEST_RADIO_POWER)
//		cmdProxy1busy[0] = true; // force dispatch for RIL_REQUEST_RADIO_POWER in MTK-RIL
//	    else
		cmdProxy1busy[slot] = true; // force dispatch for RIL_REQUEST_RADIO_POWER in MTK-RIL
	} else
	    cmdProxy1busy[slot] = false;
    }
    else if (cid == RIL_CMD_PROXY_3) {
	p = proxyCmd3[slot];
	if (p) {
	    proxyCmd3[slot] = proxyCmd3[slot]->pNext;
	    cmdProxy3busy[slot] = true;
	}
	else
	    cmdProxy3busy[slot] = false;
    }
#if VDBG
    RLOGD("my_dequeue, slot=%d, cid=%d, p=%p", slot, (int)cid, p);
#endif
    if (p) {
	RLOGD("dequeue: send pending request %s to %s",
		requestToString(p->pRI->pCI->requestNumber), proxyString(cid));
	s_callbacksSocket.onRequest(p->pRI->pCI->requestNumber,
			p->buf, p->buflen, p->pRI, p->pRI->socket_id);
	if ((p->bf != FMT_VOID) && (p->buflen != 0)) {
	    clearPendingBuffer(p->bf, p->buf, p->buflen);
	    free (p->buf);
	}
	free (p);
    }
}
#endif

void my_enqueue(int request, void *buf, size_t buflen, BUF_FMTS bf, RequestInfo *pRI)
{
    ProxyReqInfo *p = NULL;
    RIL_SOCKET_ID socket_id = pRI->socket_id;
    RILChannelId proxyId = pRI->pCI->proxyId;
#ifdef RIL_CHANNEL_QUEUING
    int slot = 0;
    bool pending = false, nullp = false;

    if (bf != FMT_IGNORE) {
	if (socket_id == RIL_SOCKET_2) {
	    slot = 1;
	}

	if (proxyId == RIL_CMD_PROXY_1) {
	    p = proxyCmd1[slot];
	    if (request == RIL_REQUEST_RADIO_POWER) {
		pending = cmdProxy1busy[0];
		cmdProxy1busy[0] = true; // force dispatch for RIL_REQUEST_RADIO_POWER in MTK-RIL
	    }
	    else {
		pending = cmdProxy1busy[slot];
		cmdProxy1busy[slot] = true;
	    }
	    if (pending && p==NULL) {
		p = (ProxyReqInfo *)calloc(1, sizeof(ProxyReqInfo));
		proxyCmd1[slot] = p;
		nullp = true;
	    }
	}
	else if (proxyId == RIL_CMD_PROXY_3) {
	    p = proxyCmd3[slot];
	    pending = cmdProxy3busy[slot];
	    cmdProxy3busy[slot] = true;
	    if (pending && p==NULL) {
		p = (ProxyReqInfo *)calloc(1, sizeof(ProxyReqInfo));
		proxyCmd3[slot] = p;
		nullp = true;
	    }
	}
    }
#if VDBG
    RLOGD("my_enqueue:request=%s,RIL_CMD%s_%s,buflen:%d,bf=%u,pending=%s",requestToString(request),
		(socket_id == RIL_SOCKET_1) ? "" : "2",	proxyString(proxyId), 
		buflen, bf, (pending)?"true":"false");
#endif
    if (pending) {
	int i = 1;
	while (p->pNext) {
	    RLOGI("#%d:%s is queued in RIL_CMD%s_%s", i++,
			requestToString(p->pRI->pCI->requestNumber),
			(slot == 0) ? "" : "2", proxyString(proxyId));
	    p = p->pNext;
	}
	RLOGD("Channel busy, pending request: %s to queue #%d", requestToString(request), i);
	if (!nullp) {
	    p->pNext = (ProxyReqInfo *)calloc(1, sizeof(ProxyReqInfo));
	    p = p->pNext;
	}
	p->pRI = pRI;
	p->pNext = NULL;	// should be 0 in calloc, clearer only
	p->buflen = buflen;
	p->bf = bf;
	if ((bf != FMT_VOID) && (buflen != 0)) {
	    // allocate and copy the whole buffer
	    p->buf = calloc(1, buflen);
	    memcpy(p->buf, buf, buflen);
	}
    }
    else
#else	// RIL_CHANNEL_QUEUING
#if VDBG
    RLOGD("my_enqueue:send request=%s,RIL_CMD%s_%s,buflen:%u",requestToString(request),
		(socket_id == RIL_SOCKET_1) ? "" : "2",	proxyString(proxyId), buflen);
#endif
#endif	// RIL_CHANNEL_QUEUING
    {
	s_callbacksSocket.onRequest(request, buf, buflen, pRI, socket_id);
	if (bf != FMT_IGNORE) { // cleanup the memory if not FMT_IGNORE
	    clearPendingBuffer(bf, buf, buflen);
	}
    }
}
// daniel added end

extern "C" bool
IMS_isRilRequestFromIms(RIL_Token t)
{ return false; }

extern "C"
void IMS_RIL_onUnsolicitedResponseSocket(int unsolResponse, const void *data,
				size_t datalen, RIL_SOCKET_ID socket_id)
{}

extern "C" void
RIL_startEventLoop(void) {
    RIL_startRILProxys();

    /* spin up eventLoop thread and wait for it to get started */
    s_started = 0;
    pthread_mutex_lock(&s_startupMutex);

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    int result = pthread_create(&s_tid_dispatch, &attr, eventLoop, NULL);
    if (result != 0) {
	RLOGE("Failed to create dispatch thread: %s", strerror(result));
	goto done;
    }

    while (s_started == 0) {
	pthread_cond_wait(&s_startupCond, &s_startupMutex);
    }

done:
    pthread_mutex_unlock(&s_startupMutex);
}

// Used for testing purpose only.
extern "C" void RIL_setcallbacks (const RIL_RadioFunctionsSocket *callbacks) {
    memcpy(&s_callbacksSocket, callbacks, sizeof (RIL_RadioFunctionsSocket));
}

extern "C" void
RIL_register (const RIL_RadioFunctions *callbacks) {
    int ret;
    int flags;

    RLOGI("SIM_COUNT: %d", SIM_COUNT);

    if (callbacks == NULL) {
	RLOGE("RIL_register: RIL_RadioFunctions * null");
	return;
    }
    if (callbacks->version < RIL_VERSION_MIN) {
	RLOGE("RIL_register: version %d is to old, min version is %d",
	     callbacks->version, RIL_VERSION_MIN);
	return;
    }

    RLOGE("RIL_register: RIL version %d", callbacks->version);

    if (s_registerCalled > 0) {
	RLOGE("RIL_register has been called more than once. "
		"Subsequent call ignored");
	return;
    }

    memcpy(&s_callbacksSocket, callbacks, sizeof (RIL_RadioFunctionsSocket));

    s_registerCalled = 1;
#ifdef RIL_UNSOL_PENDING
    clrUSRBuffer();
#endif
    memset(threadPid, 0, sizeof(threadPid));

    RLOGI("s_registerCalled flag set, %d", s_started);
    // Little self-check

    for (int i = 0; i < (int)NUM_ELEMS(s_commands); i++) {
	int r = s_commands[i].requestNumber;
	if (r < RIL_REQUEST_VENDOR_BASE) {
	    assert(i == r);
	} else {
	    if (mtk_command_start == 0) mtk_command_start = i;
	    assert(i + RIL_REQUEST_VENDOR_BASE - mtk_command_start == r);
	}
    }

    for (int i = 0; i < (int)NUM_ELEMS(s_unsolResponses); i++) {
	int r = s_unsolResponses[i].requestNumber;
	if (r < RIL_UNSOL_VENDOR_BASE) {
	    assert(i + RIL_UNSOL_RESPONSE_BASE == r);
	} else {
	    if (mtk_unsol_start == 0) mtk_unsol_start = i;
	    assert(i + RIL_UNSOL_VENDOR_BASE - mtk_unsol_start == r);
	}
    }

    radio::registerService(&s_callbacksSocket, s_commands);
    RLOGI("RILHIDL called registerService");

}

extern "C" void
RIL_registerSocket (const RIL_RadioFunctionsSocket *callbacks) {
    int ret;
    int flags;
    int i = 0;

    RLOGI("SIM_COUNT: %d", SIM_COUNT);

    if (callbacks == NULL) {
	RLOGE("RIL_registerSocket: RIL_RadioFunctions * null");
	return;
    }
    if (callbacks->version < RIL_VERSION_MIN) {
	RLOGE("RIL_registerSocket: version %d is to old, min version is %d",
	     callbacks->version, RIL_VERSION_MIN);
	return;
    }

    RLOGI("RIL_registerSocket: RIL version %d", callbacks->version);

    if (s_registerCalled > 0) {
	RLOGE("RIL_registerSocket has been called more than once. "
		"Subsequent call ignored");
	return;
    }

    memcpy(&s_callbacksSocket, callbacks, sizeof (RIL_RadioFunctionsSocket));

    s_registerCalled = 1;
#ifdef RIL_UNSOL_PENDING
    clrUSRBuffer();
#endif
    memset(threadPid, 0, sizeof(threadPid));

    RLOGI("s_registerCalled flag set, %d", s_started);
    // Little self-check

    for (int i = 0; i < (int)NUM_ELEMS(s_commands); i++) {
	int r = s_commands[i].requestNumber;
	if (r < RIL_REQUEST_VENDOR_BASE) {
	    assert(i == r);
	} else {
	    if (mtk_command_start == 0) mtk_command_start = i;
	    assert(i + RIL_REQUEST_VENDOR_BASE - mtk_command_start == r);
	}
    }
    RLOGD("s_commands size: %d, mtk_command_start=%d",
		(int)NUM_ELEMS(s_commands), mtk_command_start);

    for (int i = 0; i < (int)NUM_ELEMS(s_unsolResponses); i++) {
	int r = s_unsolResponses[i].requestNumber;
	if (r < RIL_UNSOL_VENDOR_BASE) {
	    assert(i + RIL_UNSOL_RESPONSE_BASE == r);
	} else {
	    if (mtk_unsol_start == 0) mtk_unsol_start = i;
	    assert(i + RIL_UNSOL_VENDOR_BASE - mtk_unsol_start == r);
	}
    }
    RLOGD("s_unsolResponses size: %d, mtk_unsol_start=%d",
		(int)NUM_ELEMS(s_unsolResponses), mtk_unsol_start);

    radio::registerService(&s_callbacksSocket, s_commands);
    RLOGI("RILHIDL called registerService");
}

extern "C" void
RIL_register_socket (RIL_RadioFunctions *(*Init)(const struct RIL_Env *, int, char **),
	RIL_SOCKET_TYPE socketType, int argc, char **argv) {

    RIL_RadioFunctions* UimFuncs = NULL;

    if(Init) {
	UimFuncs = Init(&RilSapSocket::uimRilEnv, argc, argv);

	switch(socketType) {
	    case RIL_SAP_SOCKET:
		RilSapSocket::initSapSocket(RIL1_SERVICE_NAME, UimFuncs);

#if (SIM_COUNT >= 2)
		RilSapSocket::initSapSocket(RIL2_SERVICE_NAME, UimFuncs);
#endif

#if (SIM_COUNT >= 3)
		RilSapSocket::initSapSocket(RIL3_SERVICE_NAME, UimFuncs);
#endif

#if (SIM_COUNT >= 4)
		RilSapSocket::initSapSocket(RIL4_SERVICE_NAME, UimFuncs);
#endif
		break;
	    default:;
	}

	RLOGI("RIL_register_socket: calling registerService");
	sap::registerService(UimFuncs);
    }
}

// Check and remove RequestInfo if its a response and not just ack sent back
static int
checkAndDequeueRequestInfoIfAck(struct RequestInfo *pRI, bool isAck) {
    int ret = 0;
    /* Hook for current context
       pendingRequestsMutextHook refer to &s_pendingRequestsMutex */
    pthread_mutex_t* pendingRequestsMutexHook = &s_pendingRequestsMutex;
    /* pendingRequestsHook refer to &s_pendingRequests */
    RequestInfo ** pendingRequestsHook = &s_pendingRequests;

    if (pRI == NULL) {
	return 0;
    }

#if (SIM_COUNT >= 2)
    if (pRI->socket_id == RIL_SOCKET_2) {
	pendingRequestsMutexHook = &s_pendingRequestsMutex_socket2;
	pendingRequestsHook = &s_pendingRequests_socket2;
    }
#if (SIM_COUNT >= 3)
	if (pRI->socket_id == RIL_SOCKET_3) {
	    pendingRequestsMutexHook = &s_pendingRequestsMutex_socket3;
	    pendingRequestsHook = &s_pendingRequests_socket3;
	}
#endif
#if (SIM_COUNT >= 4)
    if (pRI->socket_id == RIL_SOCKET_4) {
	pendingRequestsMutexHook = &s_pendingRequestsMutex_socket4;
	pendingRequestsHook = &s_pendingRequests_socket4;
    }
#endif
#endif
    pthread_mutex_lock(pendingRequestsMutexHook);

    for(RequestInfo **ppCur = pendingRequestsHook
	; *ppCur != NULL
	; ppCur = &((*ppCur)->p_next)
    ) {
	if (pRI == *ppCur) {
	    ret = 1;
	    if (isAck) { // Async ack
		if (pRI->wasAckSent == 1) {
		    RLOGD("Ack was already sent for %s", requestToString(pRI->pCI->requestNumber));
		} else {
		    pRI->wasAckSent = 1;
		}
	    } else {
		*ppCur = (*ppCur)->p_next;
	    }
	    break;
	}
    }

    pthread_mutex_unlock(pendingRequestsMutexHook);

    return ret;
}

extern "C" void
RIL_onRequestAck(RIL_Token t) {
    RequestInfo *pRI;
    int ret;

    size_t errorOffset;
    RIL_SOCKET_ID socket_id = RIL_SOCKET_1;

    pRI = (RequestInfo *)t;

    if (!checkAndDequeueRequestInfoIfAck(pRI, true)) {
	RLOGE ("RIL_onRequestAck: invalid RIL_Token");
	return;
    }

    socket_id = pRI->socket_id;

#if VDBG
    RLOGD("Request Ack, %s", rilSocketIdToString(socket_id));
#endif

    appendPrintBuf("Ack [%04d]< %s", pRI->token, requestToString(pRI->pCI->requestNumber));

    if (pRI->cancelled == 0) {
	pthread_rwlock_t *radioServiceRwlockPtr = radio::getRadioServiceRwlock(
		(int) socket_id);
	int rwlockRet = pthread_rwlock_rdlock(radioServiceRwlockPtr);
	assert(rwlockRet == 0);

	radio::acknowledgeRequest((int) socket_id, pRI->token);

	rwlockRet = pthread_rwlock_unlock(radioServiceRwlockPtr);
	assert(rwlockRet == 0);
    }
}

extern "C" void
RIL_onRequestComplete(RIL_Token t, RIL_Errno s_e, void *s_response, size_t s_responselen) {
    RequestInfo *pRI;
    int ret;
    size_t errorOffset;
    RIL_SOCKET_ID socket_id = RIL_SOCKET_1;
    RIL_Errno e = s_e;
    void *response = s_response;
    size_t responselen = s_responselen;

    pRI = (RequestInfo *)t;

    if (!checkAndDequeueRequestInfoIfAck(pRI, false)) {
	RLOGE ("RIL_onRequestComplete: invalid RIL_Token");
	return;
    }

    socket_id = pRI->socket_id;
#ifdef RIL_CHANNEL_QUEUING
#if VDBG
    RLOGD("RequestComplete, %s: RIL_CMD%s_%s, e=%u", rilSocketIdToString(socket_id),
	(socket_id == RIL_SOCKET_1) ? "" : "2", proxyString(pRI->pCI->proxyId), e);
#endif
#endif

    if (pRI->local > 0) {
	// Locally issued command...void only!
	// response does not go back up the command socket
	RLOGD("C[locl]< %s", requestToString(pRI->pCI->requestNumber));
	if (pRI->pCI->requestNumber == RIL_REQUEST_GET_IMEI) {
	    int i = (int)socket_id;
	    RLOGD("SOCKET_ID_IMEI= %d", i);
            Device_ID[i].imei = (char*) response;
	    RLOGD("IMEI=%s", Device_ID[i].imei);
	}
	else if (pRI->pCI->requestNumber == RIL_REQUEST_GET_IMEISV) {
	    int i = (int)socket_id;
	    RLOGD("SOCKET_ID_IMEI_SV= %d", i);
            Device_ID[i].imeisv = (char*) response;
	    RLOGD("IMEISV=%s", Device_ID[i].imeisv);
	}
	goto done;
    }

// *** handle unsupported but necessary requests
    if (pRI->pCI->requestNumber == RIL_REQUEST_DEVICE_IDENTITY) {
	RLOGD("Overriding RIL_REQUEST_DEVICE_IDENTITY");
	    int i = (int)socket_id;
	    RLOGD("SOCKET_ID_IDENTITY= %d", i);
	    response = &(Device_ID[i]);
	    responselen = 4 * sizeof(char*);
	    e = RIL_E_SUCCESS;
    }

    appendPrintBuf("[%04d]< %s",
	pRI->token, requestToString(pRI->pCI->requestNumber));
//*** handle MTK start, seems handled in service now
#if 0
    if(pRI->pCI->requestNumber == RIL_LOCAL_REQUEST_QUERY_MODEM_THERMAL){
	LOGD("[THERMAL] local request for THERMAL returned ");
	char* strResult = NULL;
	if(RIL_E_SUCCESS == e){
	    asprintf(&strResult, "%s",(char*)response);
	} else {
	    asprintf(&strResult, "ERROR");
	}

	if(s_THERMAL_fd > 0){
	    LOGD("[THERMAL] s_THERMAL_fd is valid strResult is %s", strResult);

	    int len = (int)strlen(strResult);
	    ret = send(s_THERMAL_fd, strResult, len, MSG_NOSIGNAL);
	    if (ret != len) {
		LOGD("[THERMAL] lose data when send response. ");
	    }
	    free(strResult);
	    goto done;
	} else {
	    LOGD("[EAP] s_THERMAL_fd is < 0");
	    free(strResult);
	    goto done;
	}
    }

    if (pRI->pCI->requestNumber == RIL_LOCAL_REQUEST_SET_MODEM_THERMAL) {
	    LOGD("[MDTM_TOG] Not need to send response");
	    goto done;
    }

    /*handle response for local request for response*/
    if(pRI->pCI->requestNumber == RIL_LOCAL_REQUEST_SIM_AUTHENTICATION
	|| pRI->pCI->requestNumber == RIL_LOCAL_REQUEST_USIM_AUTHENTICATION){

	LOGD("[EAP] local request for EAP returned ");
	char* strResult = NULL;
	char* res = (char*)response;
	if(RIL_E_SUCCESS == e){
	    if(pRI->pCI->requestNumber == RIL_LOCAL_REQUEST_SIM_AUTHENTICATION
		 && strlen(res) > 24){
		strResult = new char[25];
		strResult[0] = '\0';
		if(res[0] == '0' && res[1] == '4'){
		    strncpy(strResult, res+2, 8);
		    if(res[10] == '0' && res[11] == '8'){
			strncpy(strResult+8, res+12, 16);
			strResult[24] = '\0';
		    }else{
			LOGE("The length of KC is not valid.");
		    }
		}else{
		   LOGE("The length of SRES is not valid.");
		}
	    }else{
		asprintf(&strResult, "%s",res);
	    }
	}else{
	    asprintf(&strResult, "ERROR:%s", failCauseToString(e));
	}
	if(s_EAPSIMAKA_fd > 0){
	    LOGD("[EAP] s_EAPSIMAKA_fd is valid strResult is %s", strResult);
	    int len = (int)strlen(strResult);
	    ret = send(s_EAPSIMAKA_fd, &len, sizeof(int), 0);
	    if (ret != sizeof(int)) {
		LOGD("Socket write Error: when sending arg length");
	    }else{
		ret = send(s_EAPSIMAKA_fd, strResult, len, 0);
		if (ret != len) {
		    LOGD("[EAP]lose data when send response. ");
		}
	    }
	    close(s_EAPSIMAKA_fd);
	    s_EAPSIMAKA_fd = -1;
	    free(strResult);
	    goto done;
	}

	if (strResult != NULL) {
	    free(strResult);
	}
    }

   if(pRI->pCI->requestNumber == RIL_LOCAL_REQUEST_GET_SHARED_KEY
      || pRI->pCI->requestNumber == RIL_LOCAL_REQUEST_UPDATE_SIM_LOCK_SETTINGS
      || pRI->pCI->requestNumber == RIL_LOCAL_REQUEST_GET_SIM_LOCK_INFO
      || pRI->pCI->requestNumber == RIL_LOCAL_REQUEST_RESET_SIM_LOCK_SETTINGS
      || pRI->pCI->requestNumber == RIL_LOCAL_REQUEST_GET_MODEM_STATUS){

	LOGD("[SIM_LOCK] local request for SIM_LOCK returned ");

	char* strResult = NULL;
	if(RIL_E_SUCCESS == e){
	    asprintf(&strResult, "%s",(char*)response);
	} else {
	    asprintf(&strResult, "ERROR:%d", e);
	}

	if(s_SIMLOCK_fd > 0){
	    LOGD("[SIM_LOCK] s_SIMLOCK_fd is valid strResult is %s", strResult);

	    int len = (int)strlen(strResult);

	    ret = send(s_SIMLOCK_fd, &len, sizeof(int), 0);
	    ret = send(s_SIMLOCK_fd, strResult, len, 0);
	    if (ret != len) {
		LOGD("[SIM_LOCK] lose data when send response. ");
	    }
	    close(s_SIMLOCK_fd);
	    s_SIMLOCK_fd = -1;
	    free(strResult);
	    goto done;
	} else {
	    LOGD("[SIM_LOCK] s_SIMLOCK_fd is < 0");
	    free(strResult);
	    goto done;
	}
    }
#endif
    /* RIL_REQUEST_RESUME_REGISTRATION clear backup data start */
    if (pRI->pCI->requestNumber == RIL_REQUEST_RESUME_REGISTRATION) {
	//LOGD("[WPO] ECOPS backup dataSize:<%d>, data:<%s>", s_prevEcopsDataSize[socket_id], s_prevEcopsData[socket_id]);
	if (s_prevEcopsData[socket_id] != NULL) {
	    LOGD("[WPO] ECOPS data is not NULL, so clear it.");
	    free(s_prevEcopsData[socket_id]);
	    s_prevEcopsData[socket_id] = NULL;
	    s_prevEcopsDataSize[socket_id] = 0;
	}
    }
    /* RIL_REQUEST_RESUME_REGISTRATION clear backup data end */
//*** handle MTK end

    if (pRI->cancelled == 0) {
	int responseType;
	if (s_callbacksSocket.version >= 13 && pRI->wasAckSent == 1) {
	    // If ack was already sent, then this call is an asynchronous response. So we need to
	    // send id indicating that we expect an ack from RIL.java as we acquire wakelock here.
	    responseType = RESPONSE_SOLICITED_ACK_EXP;
	    grabPartialWakeLock();
	} else {
	    responseType = RESPONSE_SOLICITED;
	}

	// there is a response payload, no matter success or not.
#if VDBG
	RLOGI ("Calling responseFunction() for token %d", pRI->token);
#endif
	pthread_rwlock_t *radioServiceRwlockPtr = radio::getRadioServiceRwlock((int) socket_id);
	int rwlockRet = pthread_rwlock_rdlock(radioServiceRwlockPtr);
	assert(rwlockRet == 0);


	if (pRI->pCI->responseFunction == NULL) {
	    RLOGW("**Request: %s response not handled!",
		requestToString(pRI->pCI->requestNumber));
	}
	else
	    ret = pRI->pCI->responseFunction((int) socket_id,
			responseType, pRI->token, e, response, responselen);

	rwlockRet = pthread_rwlock_unlock(radioServiceRwlockPtr);
	assert(rwlockRet == 0);
    }
done:
#ifdef RIL_CHANNEL_QUEUING
    ret = (socket_id == RIL_SOCKET_1) ? 0 : 1;
    if (pRI->pCI->proxyId == RIL_CMD_PROXY_1) {
//	if (pRI->pCI->requestNumber == RIL_REQUEST_RADIO_POWER)
//	    cmdProxy1busy[0] = false; // force dispatch for RIL_REQUEST_RADIO_POWER in MTK-RIL
//	else
	    cmdProxy1busy[ret] = false; // force dispatch for RIL_REQUEST_RADIO_POWER in MTK-RIL
	my_dequeue(ret, pRI->pCI->proxyId);
    } else if (pRI->pCI->proxyId == RIL_CMD_PROXY_3) {
	cmdProxy3busy[ret] = false; // force dispatch for RIL_REQUEST_RADIO_POWER in MTK-RIL
	my_dequeue(ret, pRI->pCI->proxyId);
    }
#endif
    free(pRI);
}

static void
grabPartialWakeLock() {
    if (s_callbacksSocket.version >= 13) {
	int ret;
	ret = pthread_mutex_lock(&s_wakeLockCountMutex);
	assert(ret == 0);
	acquire_wake_lock(PARTIAL_WAKE_LOCK, ANDROID_WAKE_LOCK_NAME);

	UserCallbackInfo *p_info = internalRequestTimedCallback(wakeTimeoutCallback,
					 NULL, &TIMEVAL_WAKE_TIMEOUT, -1);
	if (p_info == NULL) {
	    release_wake_lock(ANDROID_WAKE_LOCK_NAME);
	} else {
	    s_wakelock_count++;
	    if (s_last_wake_timeout_info != NULL) {
		s_last_wake_timeout_info->userParam = (void *)1;
	    }
	    s_last_wake_timeout_info = p_info;
	}
	ret = pthread_mutex_unlock(&s_wakeLockCountMutex);
	assert(ret == 0);
    } else {
	acquire_wake_lock(PARTIAL_WAKE_LOCK, ANDROID_WAKE_LOCK_NAME);
    }
}

void
releaseWakeLock() {
    if (s_callbacksSocket.version >= 13) {
	int ret;
	ret = pthread_mutex_lock(&s_wakeLockCountMutex);
	assert(ret == 0);

	if (s_wakelock_count > 1) {
	    s_wakelock_count--;
	} else {
	    s_wakelock_count = 0;
	    release_wake_lock(ANDROID_WAKE_LOCK_NAME);
	    if (s_last_wake_timeout_info != NULL) {
		s_last_wake_timeout_info->userParam = (void *)1;
	    }
	}

	ret = pthread_mutex_unlock(&s_wakeLockCountMutex);
	assert(ret == 0);
    } else {
	release_wake_lock(ANDROID_WAKE_LOCK_NAME);
    }
}

/**
 * Timer callback to put us back to sleep before the default timeout
 */
static void
wakeTimeoutCallback (void *param) {
    // We're using "param != NULL" as a cancellation mechanism
    if (s_callbacksSocket.version >= 13) {
	if (param == NULL) {
	    int ret;
	    ret = pthread_mutex_lock(&s_wakeLockCountMutex);
	    assert(ret == 0);
	    s_wakelock_count = 0;
	    release_wake_lock(ANDROID_WAKE_LOCK_NAME);
	    ret = pthread_mutex_unlock(&s_wakeLockCountMutex);
	    assert(ret == 0);
	}
    } else {
	if (param == NULL) {
	    releaseWakeLock();
	}
    }
}

static void send_unsolResponse(int unsolResponse, const void *data,
				size_t datalen, RIL_SOCKET_ID socket_id)
{
    int unsolResponseIndex;
    int ret;
    bool shouldScheduleTimeout = false;
    RIL_SOCKET_ID soc_id = socket_id;
    WakeType waketype = WAKE_PARTIAL;

    unsolResponseIndex = (unsolResponse < RIL_UNSOL_VENDOR_BASE) ?
	unsolResponse - RIL_UNSOL_RESPONSE_BASE :
	unsolResponse - RIL_UNSOL_VENDOR_BASE + mtk_unsol_start;

    if (unsolResponseIndex < 0) {
	RLOGE("unsupported unsolicited response code %d", unsolResponse);
	return;
    } else if (unsolResponse > RIL_UNSOL_VENDOR_BASE + 80) { // not handle byond 80
	RLOGE("unsupported mtk unsolicited response code %d", unsolResponse);
	return;
    }

    waketype = s_unsolResponses[unsolResponseIndex].wakeType;

    // Grab a wake lock if needed for this reponse,
    // as we exit we'll either release it immediately
    // or set a timer to release it later.
    switch (waketype) {
	case WAKE_PARTIAL:
	    grabPartialWakeLock();
	    shouldScheduleTimeout = true;
	break;

	case DONT_WAKE:
	default:
	    // No wake lock is grabed so don't set timeout
	    shouldScheduleTimeout = false;
	    break;
    }

    appendPrintBuf("[UNSL]< %s", requestToString(unsolResponse));

    int responseType;
    if (s_callbacksSocket.version >= 13
		&& waketype == WAKE_PARTIAL) {
	responseType = RESPONSE_UNSOLICITED_ACK_EXP;
    } else {
	responseType = RESPONSE_UNSOLICITED;
    }

    pthread_rwlock_t *radioServiceRwlockPtr = radio::getRadioServiceRwlock((int) soc_id);
    int rwlockRet = pthread_rwlock_rdlock(radioServiceRwlockPtr);
    assert(rwlockRet == 0); 
#if VDBG
    RLOGI("%s UNSOLICITED: %s data:%p,length:%zu,pthread=%lu", rilSocketIdToString(soc_id),
	    requestToString(unsolResponse), data, datalen, pthread_self());
#endif

    if (s_unsolResponses[unsolResponseIndex].responseFunction == NULL) {
	RLOGW("**UNSOLICITED: %s not handled!", requestToString(unsolResponse));
	ret = 0;
    }
    else
	ret = s_unsolResponses[unsolResponseIndex].responseFunction((int) soc_id,
			responseType, 0, RIL_E_SUCCESS, const_cast<void*>(data), datalen);

    rwlockRet = pthread_rwlock_unlock(radioServiceRwlockPtr);
    assert(rwlockRet == 0);

    if (s_callbacksSocket.version < 13) {
	if (shouldScheduleTimeout) {
	    UserCallbackInfo *p_info = internalRequestTimedCallback(wakeTimeoutCallback,
					NULL, &TIMEVAL_WAKE_TIMEOUT, -1);

	    if (p_info == NULL) {
		goto error_exit;
	    } else {
		// Cancel the previous request
		if (s_last_wake_timeout_info != NULL) {
		    s_last_wake_timeout_info->userParam = (void *)1;
		}
		s_last_wake_timeout_info = p_info;
	    }
	}
    }

    if (ret != 0 && unsolResponse == RIL_UNSOL_NITZ_TIME_RECEIVED) {
	// Unfortunately, NITZ time is not poll/update like everything
	// else in the system. So, if the upstream client isn't connected,
	// keep a copy of the last NITZ response (with receive time noted
	// above) around so we can deliver it when it is connected

	if (s_lastNITZTimeData != NULL) {
	    free(s_lastNITZTimeData);
	    s_lastNITZTimeData = NULL;
	}

	s_lastNITZTimeData = calloc(datalen, 1);
	if (s_lastNITZTimeData == NULL) {
	    RLOGE("Memory allocation failed in RIL_onUnsolicitedResponse");
	    goto error_exit;
	}
	s_lastNITZTimeDataSize = datalen;
	memcpy(s_lastNITZTimeData, data, datalen);
    }

    // Normal exit
    return;

error_exit:
    if (shouldScheduleTimeout) {
	releaseWakeLock();
    }
}

#if defined(ANDROID_MULTI_SIM)
extern "C"
void RIL_onUnsolicitedResponse(int unsolResponse, const void *data,
				size_t datalen, RIL_SOCKET_ID socket_id)
{
    RIL_onUnsolicitedResponseSocket(unsolResponse, data, datalen, socket_id);
}
#else
extern "C"
void RIL_onUnsolicitedResponse(int unsolResponse, const void *data,
				size_t datalen)
{
    RIL_onUnsolicitedResponseSocket(unsolResponse, data, datalen, RIL_SOCKET_1);
}
#endif

extern "C"
void RIL_onUnsolicitedResponseSocket(int unsolResponse, const void *data,
				size_t datalen, RIL_SOCKET_ID socket_id)
{
    if (s_registerCalled == 0) {
	// Ignore RIL_onUnsolicitedResponse before RIL_register
	RLOGW("RIL_onUnsolicitedResponse called before RIL_register");
	return;
    }

    /* RIL.java not handled in N but still worked
	Only handle those in MediaTekRIL for the time being
	TODO:***Check later */
    if (unsolResponse == RIL_UNSOL_MAL_AT_INFO ||
	unsolResponse == RIL_UNSOL_MAIN_SIM_INFO ||
	unsolResponse == RIL_UNSOL_DATA_ALLOWED ||
	unsolResponse == RIL_UNSOL_PHB_READY_NOTIFICATION ||
	unsolResponse == RIL_UNSOL_GPRS_DETACH ||
	unsolResponse == RIL_UNSOL_CRSS_NOTIFICATION ||
	unsolResponse == RIL_UNSOL_RESPONSE_PLMN_CHANGED ||
	unsolResponse == RIL_UNSOL_VOLTE_EPS_NETWORK_FEATURE_SUPPORT ||
	unsolResponse == RIL_UNSOL_EMERGENCY_BEARER_SUPPORT_NOTIFY ||
	unsolResponse == RIL_UNSOL_SMS_READY_NOTIFICATION ||
	unsolResponse == RIL_UNSOL_CALL_FORWARDING) {
	//ignore
	RLOGD("***unsolResponse=%d,socket_id=%d ignore (%s)",
	    	unsolResponse, (int)socket_id, requestToString(unsolResponse));
	return;
    }

    RLOGD("***unsolResponse=%d,datalen=%d,socket_id=%d (%s)",
	    unsolResponse,(int)datalen, (int)socket_id, requestToString(unsolResponse));

#ifndef RIL_UNSOL_PENDING
    send_unsolResponse(unsolResponse, data, datalen, socket_id);
#else
/*** need to check if service ready,
	if no, cache it
	if yes, flush cache
   *** following go to a function send_unsolResponse()
*/
    if (radio::isServiceConnected(0) && radio::isServiceConnected(1)) { // check socket_id 1
	// empty cache if exist
	if (pendingUSRcnt > 0) {
	    RLOGI("flush the pending USRs");
	    for (int i = 0; i < pendingUSRcnt; i++) {
		RLOGD("send unsolResponse %d(%s), response length:%zu, socket:%d",
			pendingUSR[i].unsolResponse, requestToString(pendingUSR[i].unsolResponse),
			pendingUSR[i].datalen, pendingUSR[i].socket_id);
		send_unsolResponse(pendingUSR[i].unsolResponse, pendingUSR[i].data,
				pendingUSR[i].datalen, pendingUSR[i].socket_id);
		free(pendingUSR[i].data);
	    }
	    pendingUSRcnt = 0;
	}
	send_unsolResponse(unsolResponse, data, datalen, socket_id);
    }
    else {
	// cached if data != NULL
	if (data == NULL || datalen == 0 ||
	    unsolResponse == RIL_UNSOL_RESPONSE_VOICE_NETWORK_STATE_CHANGED)  {
	    RLOGW("service not started, ignored %d (%s) on socket %d", 
		unsolResponse, requestToString(unsolResponse), (int)socket_id);
	}
	else {
	    if (pendingUSRcnt < MAX_PENDING_USR) {
		pendingUSR[pendingUSRcnt].unsolResponse = unsolResponse;
		pendingUSR[pendingUSRcnt].data = calloc(1, datalen);
		if (pendingUSR[pendingUSRcnt].data == NULL) {
		    RLOGE("Memory allocation failed for USR cache!");
		    return;
		}
		memcpy (pendingUSR[pendingUSRcnt].data, data, datalen);
		pendingUSR[pendingUSRcnt].datalen = datalen;
		pendingUSR[pendingUSRcnt++].socket_id = socket_id;
		RLOGW("wait for service, cached %d (%s) on socket %d, cache size=%d", 
		    unsolResponse, requestToString(unsolResponse), (int)socket_id, pendingUSRcnt);
	    }
	    else RLOGE("service not started, USR cache full!");
	}
    }
#endif
}

/** FIXME generalize this if you track UserCAllbackInfo, clear it
    when the callback occurs
*/
static UserCallbackInfo *
internalRequestTimedCallback (RIL_TimedCallback callback, void *param,
			const struct timeval *relativeTime, int cid)
{
    struct timeval myRelativeTime;
    UserCallbackInfo *p_info;

    p_info = (UserCallbackInfo *) calloc(1, sizeof(UserCallbackInfo));
    if (p_info == NULL) {
	RLOGE("Memory allocation failed in internalRequestTimedCallback");
	return p_info;

    }

    p_info->p_callback = callback;
    p_info->userParam = param;
    p_info->cid = (RILChannelId)cid;
    if (cid > -1) {
	RLOGD("internalRequestTimedCallback, ***pthread_self()=%lu cid=%d",
				pthread_self(), cid);
	addThreadPid(cid, pthread_self());	// ** guess ??
    }
    if (relativeTime == NULL) {
	/* treat null parameter as a 0 relative time */
	memset (&myRelativeTime, 0, sizeof(myRelativeTime));
    } else {
	/* FIXME I think event_add's tv param is really const anyway */
	memcpy (&myRelativeTime, relativeTime, sizeof(myRelativeTime));
    }

    ril_event_set(&(p_info->event), -1, false, userTimerCallback, p_info);

    ril_timer_add(&(p_info->event), &myRelativeTime);

    triggerEvLoop();
    return p_info;
}


extern "C" void
RIL_requestTimedCallback (RIL_TimedCallback callback, void *param,
				const struct timeval *relativeTime) {
    internalRequestTimedCallback (callback, param, relativeTime, -1);
}

const char *
failCauseToString(RIL_Errno e) {
    switch(e) {
	case RIL_E_SUCCESS: return "E_SUCCESS";
	case RIL_E_RADIO_NOT_AVAILABLE: return "E_RADIO_NOT_AVAILABLE";
	case RIL_E_GENERIC_FAILURE: return "E_GENERIC_FAILURE";
	case RIL_E_PASSWORD_INCORRECT: return "E_PASSWORD_INCORRECT";
	case RIL_E_SIM_PIN2: return "E_SIM_PIN2";
	case RIL_E_SIM_PUK2: return "E_SIM_PUK2";
	case RIL_E_REQUEST_NOT_SUPPORTED: return "E_REQUEST_NOT_SUPPORTED";
	case RIL_E_CANCELLED: return "E_CANCELLED";
	case RIL_E_OP_NOT_ALLOWED_DURING_VOICE_CALL: return "E_OP_NOT_ALLOWED_DURING_VOICE_CALL";
	case RIL_E_OP_NOT_ALLOWED_BEFORE_REG_TO_NW: return "E_OP_NOT_ALLOWED_BEFORE_REG_TO_NW";
	case RIL_E_SMS_SEND_FAIL_RETRY: return "E_SMS_SEND_FAIL_RETRY";
	case RIL_E_SIM_ABSENT:return "E_SIM_ABSENT";
	case RIL_E_ILLEGAL_SIM_OR_ME:return "E_ILLEGAL_SIM_OR_ME";
#ifdef FEATURE_MULTIMODE_ANDROID
	case RIL_E_SUBSCRIPTION_NOT_AVAILABLE:return "E_SUBSCRIPTION_NOT_AVAILABLE";
	case RIL_E_MODE_NOT_SUPPORTED:return "E_MODE_NOT_SUPPORTED";
#endif
	case RIL_E_FDN_CHECK_FAILURE: return "E_FDN_CHECK_FAILURE";
	case RIL_E_MISSING_RESOURCE: return "E_MISSING_RESOURCE";
	case RIL_E_NO_SUCH_ELEMENT: return "E_NO_SUCH_ELEMENT";
	case RIL_E_DIAL_MODIFIED_TO_USSD: return "E_DIAL_MODIFIED_TO_USSD";
	case RIL_E_DIAL_MODIFIED_TO_SS: return "E_DIAL_MODIFIED_TO_SS";
	case RIL_E_DIAL_MODIFIED_TO_DIAL: return "E_DIAL_MODIFIED_TO_DIAL";
	case RIL_E_USSD_MODIFIED_TO_DIAL: return "E_USSD_MODIFIED_TO_DIAL";
	case RIL_E_USSD_MODIFIED_TO_SS: return "E_USSD_MODIFIED_TO_SS";
	case RIL_E_USSD_MODIFIED_TO_USSD: return "E_USSD_MODIFIED_TO_USSD";
	case RIL_E_SS_MODIFIED_TO_DIAL: return "E_SS_MODIFIED_TO_DIAL";
	case RIL_E_SS_MODIFIED_TO_USSD: return "E_SS_MODIFIED_TO_USSD";
	case RIL_E_SUBSCRIPTION_NOT_SUPPORTED: return "E_SUBSCRIPTION_NOT_SUPPORTED";
	case RIL_E_SS_MODIFIED_TO_SS: return "E_SS_MODIFIED_TO_SS";
	case RIL_E_LCE_NOT_SUPPORTED: return "E_LCE_NOT_SUPPORTED";
	case RIL_E_NO_MEMORY: return "E_NO_MEMORY";
	case RIL_E_INTERNAL_ERR: return "E_INTERNAL_ERR";
	case RIL_E_SYSTEM_ERR: return "E_SYSTEM_ERR";
	case RIL_E_MODEM_ERR: return "E_MODEM_ERR";
	case RIL_E_INVALID_STATE: return "E_INVALID_STATE";
	case RIL_E_NO_RESOURCES: return "E_NO_RESOURCES";
	case RIL_E_SIM_ERR: return "E_SIM_ERR";
	case RIL_E_INVALID_ARGUMENTS: return "E_INVALID_ARGUMENTS";
	case RIL_E_INVALID_SIM_STATE: return "E_INVALID_SIM_STATE";
	case RIL_E_INVALID_MODEM_STATE: return "E_INVALID_MODEM_STATE";
	case RIL_E_INVALID_CALL_ID: return "E_INVALID_CALL_ID";
	case RIL_E_NO_SMS_TO_ACK: return "E_NO_SMS_TO_ACK";
	case RIL_E_NETWORK_ERR: return "E_NETWORK_ERR";
	case RIL_E_REQUEST_RATE_LIMITED: return "E_REQUEST_RATE_LIMITED";
	case RIL_E_SIM_BUSY: return "E_SIM_BUSY";
	case RIL_E_SIM_FULL: return "E_SIM_FULL";
	case RIL_E_NETWORK_REJECT: return "E_NETWORK_REJECT";
	case RIL_E_OPERATION_NOT_ALLOWED: return "E_OPERATION_NOT_ALLOWED";
	case RIL_E_EMPTY_RECORD: return "E_EMPTY_RECORD";
	case RIL_E_INVALID_SMS_FORMAT: return "E_INVALID_SMS_FORMAT";
	case RIL_E_ENCODING_ERR: return "E_ENCODING_ERR";
	case RIL_E_INVALID_SMSC_ADDRESS: return "E_INVALID_SMSC_ADDRESS";
	case RIL_E_NO_SUCH_ENTRY: return "E_NO_SUCH_ENTRY";
	case RIL_E_NETWORK_NOT_READY: return "E_NETWORK_NOT_READY";
	case RIL_E_NOT_PROVISIONED: return "E_NOT_PROVISIONED";
	case RIL_E_NO_SUBSCRIPTION: return "E_NO_SUBSCRIPTION";
	case RIL_E_NO_NETWORK_FOUND: return "E_NO_NETWORK_FOUND";
	case RIL_E_DEVICE_IN_USE: return "E_DEVICE_IN_USE";
	case RIL_E_ABORTED: return "E_ABORTED";
	case RIL_E_INVALID_RESPONSE: return "INVALID_RESPONSE";
	case RIL_E_OEM_ERROR_1: return "E_OEM_ERROR_1";
	case RIL_E_OEM_ERROR_2: return "E_OEM_ERROR_2";
	case RIL_E_OEM_ERROR_3: return "E_OEM_ERROR_3";
	case RIL_E_OEM_ERROR_4: return "E_OEM_ERROR_4";
	case RIL_E_OEM_ERROR_5: return "E_OEM_ERROR_5";
	case RIL_E_OEM_ERROR_6: return "E_OEM_ERROR_6";
	case RIL_E_OEM_ERROR_7: return "E_OEM_ERROR_7";
	case RIL_E_OEM_ERROR_8: return "E_OEM_ERROR_8";
	case RIL_E_OEM_ERROR_9: return "E_OEM_ERROR_9";
	case RIL_E_OEM_ERROR_10: return "E_OEM_ERROR_10";
	case RIL_E_OEM_ERROR_11: return "E_OEM_ERROR_11";
	case RIL_E_OEM_ERROR_12: return "E_OEM_ERROR_12";
	case RIL_E_OEM_ERROR_13: return "E_OEM_ERROR_13";
	case RIL_E_OEM_ERROR_14: return "E_OEM_ERROR_14";
	case RIL_E_OEM_ERROR_15: return "E_OEM_ERROR_15";
	case RIL_E_OEM_ERROR_16: return "E_OEM_ERROR_16";
	case RIL_E_OEM_ERROR_17: return "E_OEM_ERROR_17";
	case RIL_E_OEM_ERROR_18: return "E_OEM_ERROR_18";
	case RIL_E_OEM_ERROR_19: return "E_OEM_ERROR_19";
	case RIL_E_OEM_ERROR_20: return "E_OEM_ERROR_20";
	case RIL_E_OEM_ERROR_21: return "E_OEM_ERROR_21";
	case RIL_E_OEM_ERROR_22: return "E_OEM_ERROR_22";
	case RIL_E_OEM_ERROR_23: return "E_OEM_ERROR_23";
	case RIL_E_OEM_ERROR_24: return "E_OEM_ERROR_24";
	case RIL_E_OEM_ERROR_25: return "E_OEM_ERROR_25";
	default: return "<unknown error>";
    }
}

const char *
radioStateToString(RIL_RadioState s) {
    switch(s) {
	case RADIO_STATE_OFF: return "RADIO_OFF";
	case RADIO_STATE_UNAVAILABLE: return "RADIO_UNAVAILABLE";
	case RADIO_STATE_ON:return"RADIO_ON";
	default: return "<unknown state>";
    }
}

const char *
callStateToString(RIL_CallState s) {
    switch(s) {
	case RIL_CALL_ACTIVE : return "ACTIVE";
	case RIL_CALL_HOLDING: return "HOLDING";
	case RIL_CALL_DIALING: return "DIALING";
	case RIL_CALL_ALERTING: return "ALERTING";
	case RIL_CALL_INCOMING: return "INCOMING";
	case RIL_CALL_WAITING: return "WAITING";
	default: return "<unknown state>";
    }
}

const char *
requestToString(int request) {
/*
 cat libs/telephony/ril_commands.h \
 | egrep "^ *	case RIL_" \
 | sed -re 's/\	case RIL_([^,]+),[^,]+,([^}]+).+/case RIL_\1: return "\1";/'


 cat libs/telephony/ril_unsol_commands.h \
 | egrep "^ *	case RIL_" \
 | sed -re 's/\	case RIL_([^,]+),([^}]+).+/case RIL_\1: return "\1";/'

*/
    switch(request) {
	case RIL_REQUEST_GET_SIM_STATUS: return "GET_SIM_STATUS";
	case RIL_REQUEST_ENTER_SIM_PIN: return "ENTER_SIM_PIN";
	case RIL_REQUEST_ENTER_SIM_PUK: return "ENTER_SIM_PUK";
	case RIL_REQUEST_ENTER_SIM_PIN2: return "ENTER_SIM_PIN2";
	case RIL_REQUEST_ENTER_SIM_PUK2: return "ENTER_SIM_PUK2";
	case RIL_REQUEST_CHANGE_SIM_PIN: return "CHANGE_SIM_PIN";
	case RIL_REQUEST_CHANGE_SIM_PIN2: return "CHANGE_SIM_PIN2";
	case RIL_REQUEST_ENTER_NETWORK_DEPERSONALIZATION: return "ENTER_NETWORK_DEPERSONALIZATION";
	case RIL_REQUEST_GET_CURRENT_CALLS: return "GET_CURRENT_CALLS";
	case RIL_REQUEST_DIAL: return "DIAL";
	case RIL_REQUEST_GET_IMSI: return "GET_IMSI";
	case RIL_REQUEST_HANGUP: return "HANGUP";
	case RIL_REQUEST_HANGUP_WAITING_OR_BACKGROUND: return "HANGUP_WAITING_OR_BACKGROUND";
	case RIL_REQUEST_HANGUP_FOREGROUND_RESUME_BACKGROUND: return "HANGUP_FOREGROUND_RESUME_BACKGROUND";
	case RIL_REQUEST_SWITCH_WAITING_OR_HOLDING_AND_ACTIVE: return "SWITCH_WAITING_OR_HOLDING_AND_ACTIVE";
	case RIL_REQUEST_CONFERENCE: return "CONFERENCE";
	case RIL_REQUEST_UDUB: return "UDUB";
	case RIL_REQUEST_LAST_CALL_FAIL_CAUSE: return "LAST_CALL_FAIL_CAUSE";
	case RIL_REQUEST_SIGNAL_STRENGTH: return "SIGNAL_STRENGTH";
	case RIL_REQUEST_VOICE_REGISTRATION_STATE: return "VOICE_REGISTRATION_STATE";
	case RIL_REQUEST_DATA_REGISTRATION_STATE: return "DATA_REGISTRATION_STATE";
	case RIL_REQUEST_OPERATOR: return "OPERATOR";
	case RIL_REQUEST_RADIO_POWER: return "RADIO_POWER";
	case RIL_REQUEST_DTMF: return "DTMF";
	case RIL_REQUEST_SEND_SMS: return "SEND_SMS";
	case RIL_REQUEST_SEND_SMS_EXPECT_MORE: return "SEND_SMS_EXPECT_MORE";
	case RIL_REQUEST_SETUP_DATA_CALL: return "SETUP_DATA_CALL";
	case RIL_REQUEST_SIM_IO: return "SIM_IO";
	case RIL_REQUEST_SEND_USSD: return "SEND_USSD";
	case RIL_REQUEST_CANCEL_USSD: return "CANCEL_USSD";
	case RIL_REQUEST_GET_CLIR: return "GET_CLIR";
	case RIL_REQUEST_SET_CLIR: return "SET_CLIR";
	case RIL_REQUEST_QUERY_CALL_FORWARD_STATUS: return "QUERY_CALL_FORWARD_STATUS";
	case RIL_REQUEST_SET_CALL_FORWARD: return "SET_CALL_FORWARD";
	case RIL_REQUEST_QUERY_CALL_WAITING: return "QUERY_CALL_WAITING";
	case RIL_REQUEST_SET_CALL_WAITING: return "SET_CALL_WAITING";
	case RIL_REQUEST_SMS_ACKNOWLEDGE: return "SMS_ACKNOWLEDGE";
	case RIL_REQUEST_GET_IMEI: return "GET_IMEI";
	case RIL_REQUEST_GET_IMEISV: return "GET_IMEISV";
	case RIL_REQUEST_ANSWER: return "ANSWER";
	case RIL_REQUEST_DEACTIVATE_DATA_CALL: return "DEACTIVATE_DATA_CALL";
	case RIL_REQUEST_QUERY_FACILITY_LOCK: return "QUERY_FACILITY_LOCK";
	case RIL_REQUEST_SET_FACILITY_LOCK: return "SET_FACILITY_LOCK";
	case RIL_REQUEST_CHANGE_BARRING_PASSWORD: return "CHANGE_BARRING_PASSWORD";
	case RIL_REQUEST_QUERY_NETWORK_SELECTION_MODE: return "QUERY_NETWORK_SELECTION_MODE";
	case RIL_REQUEST_SET_NETWORK_SELECTION_AUTOMATIC: return "SET_NETWORK_SELECTION_AUTOMATIC";
	case RIL_REQUEST_SET_NETWORK_SELECTION_MANUAL: return "SET_NETWORK_SELECTION_MANUAL";
	case RIL_REQUEST_QUERY_AVAILABLE_NETWORKS: return "QUERY_AVAILABLE_NETWORKS";
	case RIL_REQUEST_DTMF_START: return "DTMF_START";
	case RIL_REQUEST_DTMF_STOP: return "DTMF_STOP";
	case RIL_REQUEST_BASEBAND_VERSION: return "BASEBAND_VERSION";
	case RIL_REQUEST_SEPARATE_CONNECTION: return "SEPARATE_CONNECTION";
	case RIL_REQUEST_SET_MUTE: return "SET_MUTE";
	case RIL_REQUEST_GET_MUTE: return "GET_MUTE";
	case RIL_REQUEST_QUERY_CLIP: return "QUERY_CLIP";
	case RIL_REQUEST_LAST_DATA_CALL_FAIL_CAUSE: return "LAST_DATA_CALL_FAIL_CAUSE";
	case RIL_REQUEST_DATA_CALL_LIST: return "DATA_CALL_LIST";
	case RIL_REQUEST_RESET_RADIO: return "RESET_RADIO";
	case RIL_REQUEST_OEM_HOOK_RAW: return "OEM_HOOK_RAW";
	case RIL_REQUEST_OEM_HOOK_STRINGS: return "OEM_HOOK_STRINGS";
	case RIL_REQUEST_SCREEN_STATE: return "SCREEN_STATE";
	case RIL_REQUEST_SET_SUPP_SVC_NOTIFICATION: return "SET_SUPP_SVC_NOTIFICATION";
	case RIL_REQUEST_WRITE_SMS_TO_SIM: return "WRITE_SMS_TO_SIM";
	case RIL_REQUEST_DELETE_SMS_ON_SIM: return "DELETE_SMS_ON_SIM";
	case RIL_REQUEST_SET_BAND_MODE: return "SET_BAND_MODE";
	case RIL_REQUEST_QUERY_AVAILABLE_BAND_MODE: return "QUERY_AVAILABLE_BAND_MODE";
	case RIL_REQUEST_STK_GET_PROFILE: return "STK_GET_PROFILE";
	case RIL_REQUEST_STK_SET_PROFILE: return "STK_SET_PROFILE";
	case RIL_REQUEST_STK_SEND_ENVELOPE_COMMAND: return "STK_SEND_ENVELOPE_COMMAND";
	case RIL_REQUEST_STK_SEND_TERMINAL_RESPONSE: return "STK_SEND_TERMINAL_RESPONSE";
	case RIL_REQUEST_STK_HANDLE_CALL_SETUP_REQUESTED_FROM_SIM: return "STK_HANDLE_CALL_SETUP_REQUESTED_FROM_SIM";
	case RIL_REQUEST_EXPLICIT_CALL_TRANSFER: return "EXPLICIT_CALL_TRANSFER";
	case RIL_REQUEST_SET_PREFERRED_NETWORK_TYPE: return "SET_PREFERRED_NETWORK_TYPE";
	case RIL_REQUEST_GET_PREFERRED_NETWORK_TYPE: return "GET_PREFERRED_NETWORK_TYPE";
	case RIL_REQUEST_GET_NEIGHBORING_CELL_IDS: return "GET_NEIGHBORING_CELL_IDS";
	case RIL_REQUEST_SET_LOCATION_UPDATES: return "SET_LOCATION_UPDATES";
	case RIL_REQUEST_CDMA_SET_SUBSCRIPTION_SOURCE: return "CDMA_SET_SUBSCRIPTION_SOURCE";
	case RIL_REQUEST_CDMA_SET_ROAMING_PREFERENCE: return "CDMA_SET_ROAMING_PREFERENCE";
	case RIL_REQUEST_CDMA_QUERY_ROAMING_PREFERENCE: return "CDMA_QUERY_ROAMING_PREFERENCE";
	case RIL_REQUEST_SET_TTY_MODE: return "SET_TTY_MODE";
	case RIL_REQUEST_QUERY_TTY_MODE: return "QUERY_TTY_MODE";
	case RIL_REQUEST_CDMA_SET_PREFERRED_VOICE_PRIVACY_MODE: return "CDMA_SET_PREFERRED_VOICE_PRIVACY_MODE";
	case RIL_REQUEST_CDMA_QUERY_PREFERRED_VOICE_PRIVACY_MODE: return "CDMA_QUERY_PREFERRED_VOICE_PRIVACY_MODE";
	case RIL_REQUEST_CDMA_FLASH: return "CDMA_FLASH";
	case RIL_REQUEST_CDMA_BURST_DTMF: return "CDMA_BURST_DTMF";
	case RIL_REQUEST_CDMA_VALIDATE_AND_WRITE_AKEY: return "CDMA_VALIDATE_AND_WRITE_AKEY";
	case RIL_REQUEST_CDMA_SEND_SMS: return "CDMA_SEND_SMS";
	case RIL_REQUEST_CDMA_SMS_ACKNOWLEDGE: return "CDMA_SMS_ACKNOWLEDGE";
	case RIL_REQUEST_GSM_GET_BROADCAST_SMS_CONFIG: return "GSM_GET_BROADCAST_SMS_CONFIG";
	case RIL_REQUEST_GSM_SET_BROADCAST_SMS_CONFIG: return "GSM_SET_BROADCAST_SMS_CONFIG";
	case RIL_REQUEST_GSM_SMS_BROADCAST_ACTIVATION: return "GSM_SMS_BROADCAST_ACTIVATION";
	case RIL_REQUEST_CDMA_GET_BROADCAST_SMS_CONFIG: return "CDMA_GET_BROADCAST_SMS_CONFIG";
	case RIL_REQUEST_CDMA_SET_BROADCAST_SMS_CONFIG: return "CDMA_SET_BROADCAST_SMS_CONFIG";
	case RIL_REQUEST_CDMA_SMS_BROADCAST_ACTIVATION: return "CDMA_SMS_BROADCAST_ACTIVATION";
	case RIL_REQUEST_CDMA_SUBSCRIPTION: return "CDMA_SUBSCRIPTION";
	case RIL_REQUEST_CDMA_WRITE_SMS_TO_RUIM: return "CDMA_WRITE_SMS_TO_RUIM";
	case RIL_REQUEST_CDMA_DELETE_SMS_ON_RUIM: return "CDMA_DELETE_SMS_ON_RUIM";
	case RIL_REQUEST_DEVICE_IDENTITY: return "DEVICE_IDENTITY";
	case RIL_REQUEST_EXIT_EMERGENCY_CALLBACK_MODE: return "EXIT_EMERGENCY_CALLBACK_MODE";
	case RIL_REQUEST_GET_SMSC_ADDRESS: return "GET_SMSC_ADDRESS";
	case RIL_REQUEST_SET_SMSC_ADDRESS: return "SET_SMSC_ADDRESS";
	case RIL_REQUEST_REPORT_SMS_MEMORY_STATUS: return "REPORT_SMS_MEMORY_STATUS";
	case RIL_REQUEST_REPORT_STK_SERVICE_IS_RUNNING: return "REPORT_STK_SERVICE_IS_RUNNING";
	case RIL_REQUEST_CDMA_GET_SUBSCRIPTION_SOURCE: return "CDMA_GET_SUBSCRIPTION_SOURCE";
	case RIL_REQUEST_ISIM_AUTHENTICATION: return "ISIM_AUTHENTICATION";
	case RIL_REQUEST_ACKNOWLEDGE_INCOMING_GSM_SMS_WITH_PDU: return "ACKNOWLEDGE_INCOMING_GSM_SMS_WITH_PDU";
	case RIL_REQUEST_STK_SEND_ENVELOPE_WITH_STATUS: return "STK_SEND_ENVELOPE_WITH_STATUS";
	case RIL_REQUEST_VOICE_RADIO_TECH: return "VOICE_RADIO_TECH";
	case RIL_REQUEST_GET_CELL_INFO_LIST: return "GET_CELL_INFO_LIST";
	case RIL_REQUEST_SET_UNSOL_CELL_INFO_LIST_RATE: return "SET_UNSOL_CELL_INFO_LIST_RATE";
	case RIL_REQUEST_SET_INITIAL_ATTACH_APN: return "SET_INITIAL_ATTACH_APN";
	case RIL_REQUEST_IMS_REGISTRATION_STATE: return "IMS_REGISTRATION_STATE";
	case RIL_REQUEST_IMS_SEND_SMS: return "IMS_SEND_SMS";
	case RIL_REQUEST_SIM_TRANSMIT_APDU_BASIC: return "SIM_TRANSMIT_APDU_BASIC";
	case RIL_REQUEST_SIM_OPEN_CHANNEL: return "SIM_OPEN_CHANNEL";
	case RIL_REQUEST_SIM_CLOSE_CHANNEL: return "SIM_CLOSE_CHANNEL";
	case RIL_REQUEST_SIM_TRANSMIT_APDU_CHANNEL: return "SIM_TRANSMIT_APDU_CHANNEL";
	case RIL_REQUEST_NV_READ_ITEM: return "NV_READ_ITEM";
	case RIL_REQUEST_NV_WRITE_ITEM: return "NV_WRITE_ITEM";
	case RIL_REQUEST_NV_WRITE_CDMA_PRL: return "NV_WRITE_CDMA_PRL";
	case RIL_REQUEST_NV_RESET_CONFIG: return "NV_RESET_CONFIG";
	case RIL_REQUEST_SET_UICC_SUBSCRIPTION: return "SET_UICC_SUBSCRIPTION";
	case RIL_REQUEST_ALLOW_DATA: return "ALLOW_DATA";
	case RIL_REQUEST_GET_HARDWARE_CONFIG: return "GET_HARDWARE_CONFIG";
	case RIL_REQUEST_SIM_AUTHENTICATION: return "SIM_AUTHENTICATION";
	case RIL_REQUEST_GET_DC_RT_INFO: return "GET_DC_RT_INFO";
	case RIL_REQUEST_SET_DC_RT_INFO_RATE: return "SET_DC_RT_INFO_RATE";
	case RIL_REQUEST_SET_DATA_PROFILE: return "SET_DATA_PROFILE";
	case RIL_REQUEST_SHUTDOWN: return "SHUTDOWN";
	case RIL_REQUEST_GET_RADIO_CAPABILITY: return "GET_RADIO_CAPABILITY";
	case RIL_REQUEST_SET_RADIO_CAPABILITY: return "SET_RADIO_CAPABILITY";
	case RIL_REQUEST_START_LCE: return "START_LCE";
	case RIL_REQUEST_STOP_LCE: return "STOP_LCE";
	case RIL_REQUEST_PULL_LCEDATA: return "PULL_LCEDATA";
	case RIL_REQUEST_GET_ACTIVITY_INFO: return "GET_ACTIVITY_INFO";
	case RIL_REQUEST_SET_CARRIER_RESTRICTIONS: return "SET_CARRIER_RESTRICTIONS";
	case RIL_REQUEST_GET_CARRIER_RESTRICTIONS: return "GET_CARRIER_RESTRICTIONS";
	case RIL_REQUEST_SET_CARRIER_INFO_IMSI_ENCRYPTION: return "SET_CARRIER_INFO_IMSI_ENCRYPTION";
	case RIL_RESPONSE_ACKNOWLEDGEMENT: return "RESPONSE_ACKNOWLEDGEMENT";
	case RIL_UNSOL_RESPONSE_RADIO_STATE_CHANGED: return "UNSOL_RESPONSE_RADIO_STATE_CHANGED";
	case RIL_UNSOL_RESPONSE_CALL_STATE_CHANGED: return "UNSOL_RESPONSE_CALL_STATE_CHANGED";
	case RIL_UNSOL_RESPONSE_VOICE_NETWORK_STATE_CHANGED: return "UNSOL_RESPONSE_VOICE_NETWORK_STATE_CHANGED";
	case RIL_UNSOL_RESPONSE_NEW_SMS: return "UNSOL_RESPONSE_NEW_SMS";
	case RIL_UNSOL_RESPONSE_NEW_SMS_STATUS_REPORT: return "UNSOL_RESPONSE_NEW_SMS_STATUS_REPORT";
	case RIL_UNSOL_RESPONSE_NEW_SMS_ON_SIM: return "UNSOL_RESPONSE_NEW_SMS_ON_SIM";
	case RIL_UNSOL_ON_USSD: return "UNSOL_ON_USSD";
	case RIL_UNSOL_ON_USSD_REQUEST: return "UNSOL_ON_USSD_REQUEST";
	case RIL_UNSOL_NITZ_TIME_RECEIVED: return "UNSOL_NITZ_TIME_RECEIVED";
	case RIL_UNSOL_SIGNAL_STRENGTH: return "UNSOL_SIGNAL_STRENGTH";
	case RIL_UNSOL_DATA_CALL_LIST_CHANGED: return "UNSOL_DATA_CALL_LIST_CHANGED";
	case RIL_UNSOL_SUPP_SVC_NOTIFICATION: return "UNSOL_SUPP_SVC_NOTIFICATION";
	case RIL_UNSOL_STK_SESSION_END: return "UNSOL_STK_SESSION_END";
	case RIL_UNSOL_STK_PROACTIVE_COMMAND: return "UNSOL_STK_PROACTIVE_COMMAND";
	case RIL_UNSOL_STK_EVENT_NOTIFY: return "UNSOL_STK_EVENT_NOTIFY";
	case RIL_UNSOL_STK_CALL_SETUP: return "UNSOL_STK_CALL_SETUP";
	case RIL_UNSOL_SIM_SMS_STORAGE_FULL: return "UNSOL_SIM_SMS_STORAGE_FULL";
	case RIL_UNSOL_SIM_REFRESH: return "UNSOL_SIM_REFRESH";
	case RIL_UNSOL_CALL_RING: return "UNSOL_CALL_RING";
	case RIL_UNSOL_RESPONSE_SIM_STATUS_CHANGED: return "UNSOL_RESPONSE_SIM_STATUS_CHANGED";
	case RIL_UNSOL_RESPONSE_CDMA_NEW_SMS: return "UNSOL_RESPONSE_CDMA_NEW_SMS";
	case RIL_UNSOL_RESPONSE_NEW_BROADCAST_SMS: return "UNSOL_RESPONSE_NEW_BROADCAST_SMS";
	case RIL_UNSOL_CDMA_RUIM_SMS_STORAGE_FULL: return "UNSOL_CDMA_RUIM_SMS_STORAGE_FULL";
	case RIL_UNSOL_RESTRICTED_STATE_CHANGED: return "UNSOL_RESTRICTED_STATE_CHANGED";
	case RIL_UNSOL_ENTER_EMERGENCY_CALLBACK_MODE: return "UNSOL_ENTER_EMERGENCY_CALLBACK_MODE";
	case RIL_UNSOL_CDMA_CALL_WAITING: return "UNSOL_CDMA_CALL_WAITING";
	case RIL_UNSOL_CDMA_OTA_PROVISION_STATUS: return "UNSOL_CDMA_OTA_PROVISION_STATUS";
	case RIL_UNSOL_CDMA_INFO_REC: return "UNSOL_CDMA_INFO_REC";
	case RIL_UNSOL_OEM_HOOK_RAW: return "UNSOL_OEM_HOOK_RAW";
	case RIL_UNSOL_RINGBACK_TONE: return "UNSOL_RINGBACK_TONE";
	case RIL_UNSOL_RESEND_INCALL_MUTE: return "UNSOL_RESEND_INCALL_MUTE";
	case RIL_UNSOL_CDMA_SUBSCRIPTION_SOURCE_CHANGED: return "UNSOL_CDMA_SUBSCRIPTION_SOURCE_CHANGED";
	case RIL_UNSOL_CDMA_PRL_CHANGED: return "UNSOL_CDMA_PRL_CHANGED";
	case RIL_UNSOL_EXIT_EMERGENCY_CALLBACK_MODE: return "UNSOL_EXIT_EMERGENCY_CALLBACK_MODE";
	case RIL_UNSOL_RIL_CONNECTED: return "UNSOL_RIL_CONNECTED";
	case RIL_UNSOL_VOICE_RADIO_TECH_CHANGED: return "UNSOL_VOICE_RADIO_TECH_CHANGED";
	case RIL_UNSOL_CELL_INFO_LIST: return "UNSOL_CELL_INFO_LIST";
	case RIL_UNSOL_RESPONSE_IMS_NETWORK_STATE_CHANGED: return "UNSOL_RESPONSE_IMS_NETWORK_STATE_CHANGED";
	case RIL_UNSOL_UICC_SUBSCRIPTION_STATUS_CHANGED: return "UNSOL_UICC_SUBSCRIPTION_STATUS_CHANGED";
	case RIL_UNSOL_SRVCC_STATE_NOTIFY: return "UNSOL_SRVCC_STATE_NOTIFY";
	case RIL_UNSOL_HARDWARE_CONFIG_CHANGED: return "UNSOL_HARDWARE_CONFIG_CHANGED";
	case RIL_UNSOL_DC_RT_INFO_CHANGED: return "UNSOL_DC_RT_INFO_CHANGED";
	case RIL_UNSOL_RADIO_CAPABILITY: return "UNSOL_RADIO_CAPABILITY";
	case RIL_UNSOL_ON_SS: return "UNSOL_ON_SS";
	case RIL_UNSOL_STK_CC_ALPHA_NOTIFY: return "UNSOL_STK_CC_ALPHA_NOTIFY";
	case RIL_UNSOL_LCEDATA_RECV: return "UNSOL_LCEDATA_RECV";
	case RIL_UNSOL_PCO_DATA: return "UNSOL_PCO_DATA";
	case RIL_UNSOL_MODEM_RESTART: return "UNSOL_MODEM_RESTART";
	case RIL_UNSOL_CARRIER_INFO_IMSI_ENCRYPTION: return "UNSOL_CARRIER_INFO_IMSI_ENCRYPTION";
// MTK start
	case RIL_REQUEST_GET_COLP: return "RIL_REQUEST_GET_COLP";		// 2000
	case RIL_REQUEST_SET_COLP: return "RIL_REQUEST_SET_COLP";
	case RIL_REQUEST_GET_COLR: return "RIL_REQUEST_GET_COLR";
	case RIL_REQUEST_GET_CCM: return "RIL_REQUEST_GET_CCM";
	case RIL_REQUEST_GET_ACM: return "RIL_REQUEST_GET_ACM";
	case RIL_REQUEST_GET_ACMMAX: return "RIL_REQUEST_GET_ACMMAX";
	case RIL_REQUEST_GET_PPU_AND_CURRENCY: return "RIL_REQUEST_GET_PPU_AND_CURRENCY";
	case RIL_REQUEST_SET_ACMMAX: return "RIL_REQUEST_SET_ACMMAX";
	case RIL_REQUEST_RESET_ACM: return "RIL_REQUEST_RESET_ACM";
	case RIL_REQUEST_SET_PPU_AND_CURRENCY: return "RIL_REQUEST_SET_PPU_AND_CURRENCY";
	case RIL_REQUEST_MODEM_POWEROFF: return "RIL_REQUEST_MODEM_POWEROFF";	//10
	case RIL_REQUEST_DUAL_SIM_MODE_SWITCH: return "RIL_REQUEST_DUAL_SIM_MODE_SWITCH";
	case RIL_REQUEST_QUERY_PHB_STORAGE_INFO: return "RIL_REQUEST_QUERY_PHB_STORAGE_INFO";
	case RIL_REQUEST_WRITE_PHB_ENTRY: return "RIL_REQUEST_WRITE_PHB_ENTRY";
	case RIL_REQUEST_READ_PHB_ENTRY: return "RIL_REQUEST_READ_PHB_ENTRY";
	case RIL_REQUEST_SET_GPRS_CONNECT_TYPE: return "RIL_REQUEST_SET_GPRS_CONNECT_TYPE";
	case RIL_REQUEST_SET_GPRS_TRANSFER_TYPE: return "RIL_REQUEST_SET_GPRS_TRANSFER_TYPE";
	case RIL_REQUEST_MOBILEREVISION_AND_IMEI: return "RIL_REQUEST_MOBILEREVISION_AND_IMEI";
	case RIL_REQUEST_QUERY_SIM_NETWORK_LOCK: return "RIL_REQUEST_QUERY_SIM_NETWORK_LOCK";
	case RIL_REQUEST_SET_SIM_NETWORK_LOCK: return "RIL_REQUEST_SET_SIM_NETWORK_LOCK";
	case RIL_REQUEST_SET_SCRI: return "RIL_REQUEST_SET_SCRI";		//20
	case RIL_REQUEST_BTSIM_CONNECT: return "RIL_REQUEST_BTSIM_CONNECT";
	case RIL_REQUEST_BTSIM_DISCONNECT_OR_POWEROFF: return "RIL_REQUEST_BTSIM_DISCONNECT_OR_POWEROFF";
	case RIL_REQUEST_BTSIM_POWERON_OR_RESETSIM: return "RIL_REQUEST_BTSIM_POWERON_OR_RESETSIM";
	case RIL_REQUEST_BTSIM_TRANSFERAPDU: return "RIL_REQUEST_BTSIM_TRANSFERAPDU";
	case RIL_REQUEST_SET_NETWORK_SELECTION_MANUAL_WITH_ACT: return "RIL_REQUEST_SET_NETWORK_SELECTION_MANUAL_WITH_ACT";
	case RIL_REQUEST_QUERY_ICCID: return "RIL_REQUEST_QUERY_ICCID";
	case RIL_REQUEST_USIM_AUTHENTICATION: return "RIL_REQUEST_USIM_AUTHENTICATION";
	case RIL_REQUEST_MODEM_POWERON: return "RIL_REQUEST_MODEM_POWERON";
	case RIL_REQUEST_GET_SMS_SIM_MEM_STATUS: return "RIL_REQUEST_GET_SMS_SIM_MEM_STATUS";
	case RIL_REQUEST_GET_PHONE_CAPABILITY: return "RIL_REQUEST_GET_PHONE_CAPABILITY";	//30
	case RIL_REQUEST_SET_PHONE_CAPABILITY: return "RIL_REQUEST_SET_PHONE_CAPABILITY";
	case RIL_REQUEST_GET_POL_CAPABILITY: return "RIL_REQUEST_GET_POL_CAPABILITY";
	case RIL_REQUEST_GET_POL_LIST: return "RIL_REQUEST_GET_POL_LIST";
	case RIL_REQUEST_SET_POL_ENTRY: return "RIL_REQUEST_SET_POL_ENTRY";
	case RIL_REQUEST_QUERY_UPB_CAPABILITY: return "RIL_REQUEST_QUERY_UPB_CAPABILITY";
	case RIL_REQUEST_EDIT_UPB_ENTRY: return "RIL_REQUEST_EDIT_UPB_ENTRY";
	case RIL_REQUEST_DELETE_UPB_ENTRY: return "RIL_REQUEST_DELETE_UPB_ENTRY";
	case RIL_REQUEST_READ_UPB_GAS_LIST: return "RIL_REQUEST_READ_UPB_GAS_LIST";
	case RIL_REQUEST_READ_UPB_GRP: return "RIL_REQUEST_READ_UPB_GRP";
	case RIL_REQUEST_WRITE_UPB_GRP: return "RIL_REQUEST_WRITE_UPB_GRP";
	case RIL_REQUEST_SET_SIM_RECOVERY_ON: return "RIL_REQUEST_SET_SIM_RECOVERY_ON";
	case RIL_REQUEST_GET_SIM_RECOVERY_ON: return "RIL_REQUEST_GET_SIM_RECOVERY_ON";
	case RIL_REQUEST_SET_TRM: return "RIL_REQUEST_SET_TRM";
	case RIL_REQUEST_DETECT_SIM_MISSING: return "RIL_REQUEST_DETECT_SIM_MISSING";
	case RIL_REQUEST_GET_CALIBRATION_DATA: return "RIL_REQUEST_GET_CALIBRATION_DATA";
	case RIL_REQUEST_GET_PHB_STRING_LENGTH: return "RIL_REQUEST_GET_PHB_STRING_LENGTH";
	case RIL_REQUEST_GET_PHB_MEM_STORAGE: return "RIL_REQUEST_GET_PHB_MEM_STORAGE";
	case RIL_REQUEST_SET_PHB_MEM_STORAGE: return "RIL_REQUEST_SET_PHB_MEM_STORAGE";
	case RIL_REQUEST_READ_PHB_ENTRY_EXT: return "RIL_REQUEST_READ_PHB_ENTRY_EXT";
	case RIL_REQUEST_WRITE_PHB_ENTRY_EXT: return "RIL_REQUEST_WRITE_PHB_ENTRY_EXT";
	case RIL_REQUEST_GET_SMS_PARAMS: return "RIL_REQUEST_GET_SMS_PARAMS";
	case RIL_REQUEST_SET_SMS_PARAMS: return "RIL_REQUEST_SET_SMS_PARAMS";
	case RIL_REQUEST_SIM_TRANSMIT_BASIC: return "RIL_REQUEST_SIM_TRANSMIT_BASIC";
	case RIL_REQUEST_SIM_TRANSMIT_CHANNEL: return "RIL_REQUEST_SIM_TRANSMIT_CHANNEL";
	case RIL_REQUEST_SIM_GET_ATR: return "RIL_REQUEST_SIM_GET_ATR";
	case RIL_REQUEST_SET_CB_CHANNEL_CONFIG_INFO: return "RIL_REQUEST_SET_CB_CHANNEL_CONFIG_INFO";
	case RIL_REQUEST_SET_CB_LANGUAGE_CONFIG_INFO: return "RIL_REQUEST_SET_CB_LANGUAGE_CONFIG_INFO";
	case RIL_REQUEST_GET_CB_CONFIG_INFO: return "RIL_REQUEST_GET_CB_CONFIG_INFO";
	case RIL_REQUEST_SET_ALL_CB_LANGUAGE_ON: return "RIL_REQUEST_SET_ALL_CB_LANGUAGE_ON";
	case RIL_REQUEST_SET_ETWS: return "RIL_REQUEST_SET_ETWS";
	case RIL_REQUEST_SET_FD_MODE: return "RIL_REQUEST_SET_FD_MODE";
	case RIL_REQUEST_DETACH_PS: return "RIL_REQUEST_DETACH_PS";
	case RIL_REQUEST_SIM_OPEN_CHANNEL_WITH_SW: return "RIL_REQUEST_SIM_OPEN_CHANNEL_WITH_SW";
	case RIL_REQUEST_SET_REG_SUSPEND_ENABLED: return "RIL_REQUEST_SET_REG_SUSPEND_ENABLED";
	case RIL_REQUEST_RESUME_REGISTRATION: return "RIL_REQUEST_RESUME_REGISTRATION";
	case RIL_REQUEST_STORE_MODEM_TYPE: return "RIL_REQUEST_STORE_MODEM_TYPE";
	case RIL_REQUEST_QUERY_MODEM_TYPE: return "RIL_REQUEST_QUERY_MODEM_TYPE";
	case RIL_REQUEST_SIM_INTERFACE_SWITCH: return "RIL_REQUEST_SIM_INTERFACE_SWITCH";
	case RIL_REQUEST_UICC_SELECT_APPLICATION: return "RIL_REQUEST_UICC_SELECT_APPLICATION";
	case RIL_REQUEST_UICC_DEACTIVATE_APPLICATION: return "RIL_REQUEST_UICC_DEACTIVATE_APPLICATION";
	case RIL_REQUEST_UICC_APPLICATION_IO: return "RIL_REQUEST_UICC_APPLICATION_IO";
	case RIL_REQUEST_UICC_AKA_AUTHENTICATE: return "RIL_REQUEST_UICC_AKA_AUTHENTICATE";
	case RIL_REQUEST_UICC_GBA_AUTHENTICATE_BOOTSTRAP: return "RIL_REQUEST_UICC_GBA_AUTHENTICATE_BOOTSTRAP";
	case RIL_REQUEST_UICC_GBA_AUTHENTICATE_NAF: return "RIL_REQUEST_UICC_GBA_AUTHENTICATE_NAF";
	case RIL_REQUEST_STK_EVDL_CALL_BY_AP: return "RIL_REQUEST_STK_EVDL_CALL_BY_AP";
	case RIL_REQUEST_GET_FEMTOCELL_LIST: return "RIL_REQUEST_GET_FEMTOCELL_LIST";
	case RIL_REQUEST_ABORT_FEMTOCELL_LIST: return "RIL_REQUEST_ABORT_FEMTOCELL_LIST";
	case RIL_REQUEST_SELECT_FEMTOCELL: return "RIL_REQUEST_SELECT_FEMTOCELL";
	case RIL_REQUEST_SEND_OPLMN: return "RIL_REQUEST_SEND_OPLMN";
	case RIL_REQUEST_GET_OPLMN_VERSION: return "RIL_REQUEST_GET_OPLMN_VERSION";
	case RIL_REQUEST_ABORT_QUERY_AVAILABLE_NETWORKS: return "RIL_REQUEST_ABORT_QUERY_AVAILABLE_NETWORKS";
	case RIL_REQUEST_DIAL_UP_CSD: return "RIL_REQUEST_DIAL_UP_CSD";
	case RIL_REQUEST_SET_TELEPHONY_MODE: return "RIL_REQUEST_SET_TELEPHONY_MODE";
	case RIL_REQUEST_HANGUP_ALL: return "RIL_REQUEST_HANGUP_ALL";
	case RIL_REQUEST_FORCE_RELEASE_CALL: return "RIL_REQUEST_FORCE_RELEASE_CALL";
	case RIL_REQUEST_SET_CALL_INDICATION: return "RIL_REQUEST_SET_CALL_INDICATION";
	case RIL_REQUEST_EMERGENCY_DIAL: return "RIL_REQUEST_EMERGENCY_DIAL";
	case RIL_REQUEST_SET_ECC_SERVICE_CATEGORY: return "RIL_REQUEST_SET_ECC_SERVICE_CATEGORY";
	case RIL_REQUEST_SET_ECC_LIST: return "RIL_REQUEST_SET_ECC_LIST";
	case RIL_UNSOL_NEIGHBORING_CELL_INFO: return "RIL_UNSOL_NEIGHBORING_CELL_INFO"; // 3000
	case RIL_UNSOL_NETWORK_INFO: return "RIL_UNSOL_NETWORK_INFO";
	case RIL_UNSOL_PHB_READY_NOTIFICATION: return "RIL_UNSOL_PHB_READY_NOTIFICATION";
	case RIL_UNSOL_SIM_INSERTED_STATUS: return "RIL_UNSOL_SIM_INSERTED_STATUS";
	case RIL_UNSOL_RADIO_TEMPORARILY_UNAVAILABLE: return "RIL_UNSOL_RADIO_TEMPORARILY_UNAVAILABLE";
	case RIL_UNSOL_ME_SMS_STORAGE_FULL: return "RIL_UNSOL_ME_SMS_STORAGE_FULL";
	case RIL_UNSOL_SMS_READY_NOTIFICATION: return "RIL_UNSOL_SMS_READY_NOTIFICATION";
	case RIL_UNSOL_SCRI_RESULT: return "RIL_UNSOL_SCRI_RESULT";
	case RIL_UNSOL_SIM_MISSING: return "RIL_UNSOL_SIM_MISSING";
	case RIL_UNSOL_GPRS_DETACH: return "RIL_UNSOL_GPRS_DETACH";
	case RIL_UNSOL_ATCI_RESPONSE: return "RIL_UNSOL_ATCI_RESPONSE";	//3010
	case RIL_UNSOL_SIM_RECOVERY: return "RIL_UNSOL_SIM_RECOVERY";
	case RIL_UNSOL_VIRTUAL_SIM_ON: return "RIL_UNSOL_VIRTUAL_SIM_ON";
	case RIL_UNSOL_VIRTUAL_SIM_OFF: return "RIL_UNSOL_VIRTUAL_SIM_OFF";
	case RIL_UNSOL_INVALID_SIM: return "RIL_UNSOL_INVALID_SIM";
	case RIL_UNSOL_RESPONSE_PS_NETWORK_STATE_CHANGED: return "RIL_UNSOL_RESPONSE_PS_NETWORK_STATE_CHANGED";
	case RIL_UNSOL_RESPONSE_ACMT: return "RIL_UNSOL_RESPONSE_ACMT";
	case RIL_UNSOL_EF_CSP_PLMN_MODE_BIT: return "RIL_UNSOL_EF_CSP_PLMN_MODE_BIT";
	case RIL_UNSOL_IMEI_LOCK: return "RIL_UNSOL_IMEI_LOCK";
	case RIL_UNSOL_RESPONSE_MMRR_STATUS_CHANGED: return "RIL_UNSOL_RESPONSE_MMRR_STATUS_CHANGED";
	case RIL_UNSOL_SIM_PLUG_OUT: return "RIL_UNSOL_SIM_PLUG_OUT";	//3020
	case RIL_UNSOL_SIM_PLUG_IN: return "RIL_UNSOL_SIM_PLUG_IN";
	case RIL_UNSOL_RESPONSE_ETWS_NOTIFICATION: return "RIL_UNSOL_RESPONSE_ETWS_NOTIFICATION";
	case RIL_UNSOL_RESPONSE_PLMN_CHANGED: return "RIL_UNSOL_RESPONSE_PLMN_CHANGED";
	case RIL_UNSOL_RESPONSE_REGISTRATION_SUSPENDED: return "RIL_UNSOL_RESPONSE_REGISTRATION_SUSPENDED";
	case RIL_UNSOL_STK_EVDL_CALL: return "RIL_UNSOL_STK_EVDL_CALL";
	case RIL_UNSOL_DATA_PACKETS_FLUSH: return "RIL_UNSOL_DATA_PACKETS_FLUSH";
	case RIL_UNSOL_FEMTOCELL_INFO: return "RIL_UNSOL_FEMTOCELL_INFO";
	case RIL_UNSOL_STK_SETUP_MENU_RESET: return "RIL_UNSOL_STK_SETUP_MENU_RESET";
	case RIL_UNSOL_APPLICATION_SESSION_ID_CHANGED: return "RIL_UNSOL_APPLICATION_SESSION_ID_CHANGED";
	case RIL_UNSOL_ECONF_SRVCC_INDICATION: return "RIL_UNSOL_ECONF_SRVCC_INDICATION";	//3030
	case RIL_UNSOL_IMS_ENABLE_DONE: return "RIL_UNSOL_IMS_ENABLE_DONE";
	case RIL_UNSOL_IMS_DISABLE_DONE: return "RIL_UNSOL_IMS_DISABLE_DONE";
	case RIL_UNSOL_IMS_REGISTRATION_INFO: return "RIL_UNSOL_IMS_REGISTRATION_INFO";
	case RIL_UNSOL_DEDICATE_BEARER_ACTIVATED: return "RIL_UNSOL_DEDICATE_BEARER_ACTIVATED";
	case RIL_UNSOL_DEDICATE_BEARER_MODIFIED: return "RIL_UNSOL_DEDICATE_BEARER_MODIFIED";
	case RIL_UNSOL_DEDICATE_BEARER_DEACTIVATED: return "RIL_UNSOL_DEDICATE_BEARER_DEACTIVATED";
	case RIL_UNSOL_RAC_UPDATE: return "RIL_UNSOL_RAC_UPDATE";
	case RIL_UNSOL_ECONF_RESULT_INDICATION: return "RIL_UNSOL_ECONF_RESULT_INDICATION";
	case RIL_UNSOL_MELOCK_NOTIFICATION: return "RIL_UNSOL_MELOCK_NOTIFICATION";
	case RIL_UNSOL_CALL_FORWARDING: return "RIL_UNSOL_CALL_FORWARDING";	//3040
	case RIL_UNSOL_CRSS_NOTIFICATION: return "RIL_UNSOL_CRSS_NOTIFICATION";
	case RIL_UNSOL_INCOMING_CALL_INDICATION: return "RIL_UNSOL_INCOMING_CALL_INDICATION";
	case RIL_UNSOL_CIPHER_INDICATION: return "RIL_UNSOL_CIPHER_INDICATION";
	case RIL_UNSOL_CNAP: return "RIL_UNSOL_CNAP"; //obsolete
	case RIL_UNSOL_SIM_COMMON_SLOT_NO_CHANGED: return "RIL_UNSOL_SIM_COMMON_SLOT_NO_CHANGED";
	case RIL_UNSOL_DATA_ALLOWED: return "RIL_UNSOL_DATA_ALLOWED";
	case RIL_UNSOL_STK_CALL_CTRL: return "RIL_UNSOL_STK_CALL_CTRL";
	case RIL_UNSOL_VOLTE_EPS_NETWORK_FEATURE_SUPPORT: return "RIL_UNSOL_VOLTE_EPS_NETWORK_FEATURE_SUPPORT";
	case RIL_UNSOL_CALL_INFO_INDICATION: return "RIL_UNSOL_CALL_INFO_INDICATION";
	case RIL_UNSOL_VOLTE_EPS_NETWORK_FEATURE_INFO: return "RIL_UNSOL_VOLTE_EPS_NETWORK_FEATURE_INFO";	//3050
	case RIL_UNSOL_SRVCC_HANDOVER_INFO_INDICATION: return "RIL_UNSOL_SRVCC_HANDOVER_INFO_INDICATION";
	case RIL_UNSOL_SPEECH_CODEC_INFO: return "RIL_UNSOL_SPEECH_CODEC_INFO";
	case RIL_UNSOL_MD_STATE_CHANGE: return "RIL_UNSOL_MD_STATE_CHANGE";
	case RIL_UNSOL_REMOVE_RESTRICT_EUTRAN: return "RIL_UNSOL_REMOVE_RESTRICT_EUTRAN";
	case RIL_UNSOL_MO_DATA_BARRING_INFO: return "RIL_UNSOL_MO_DATA_BARRING_INFO";
	case RIL_UNSOL_SSAC_BARRING_INFO: return "RIL_UNSOL_SSAC_BARRING_INFO";
	case RIL_UNSOL_SIP_CALL_PROGRESS_INDICATOR: return "RIL_UNSOL_SIP_CALL_PROGRESS_INDICATOR";
	case RIL_UNSOL_ABNORMAL_EVENT: return "RIL_UNSOL_ABNORMAL_EVENT";
	case RIL_UNSOL_EMERGENCY_BEARER_SUPPORT_NOTIFY: return "RIL_UNSOL_EMERGENCY_BEARER_SUPPORT_NOTIFY";
	case RIL_UNSOL_INTER_3GPP_IRAT_STATE_CHANGE: return "RIL_UNSOL_INTER_3GPP_IRAT_STATE_CHANGE";	//3060
	case RIL_UNSOL_LTE_BG_SEARCH_STATUS: return "RIL_UNSOL_LTE_BG_SEARCH_STATUS";
	case RIL_UNSOL_GMSS_RAT_CHANGED: return "RIL_UNSOL_GMSS_RAT_CHANGED";
	case RIL_UNSOL_CDMA_CARD_TYPE: return "RIL_UNSOL_CDMA_CARD_TYPE";
	case RIL_UNSOL_IMS_ENABLE_START: return "RIL_UNSOL_IMS_ENABLE_START";
	case RIL_UNSOL_IMS_DISABLE_START: return "RIL_UNSOL_IMS_DISABLE_START";
	case RIL_UNSOL_IMSI_REFRESH_DONE: return "RIL_UNSOL_IMSI_REFRESH_DONE";
	case RIL_UNSOL_EUSIM_READY: return "RIL_UNSOL_EUSIM_READY";
	case RIL_UNSOL_STK_BIP_PROACTIVE_COMMAND: return "RIL_UNSOL_STK_BIP_PROACTIVE_COMMAND";
	case RIL_UNSOL_WORLD_MODE_CHANGED: return "RIL_UNSOL_WORLD_MODE_CHANGED";
	case RIL_UNSOL_VT_STATUS_INFO: return "RIL_UNSOL_VT_STATUS_INFO";	//3070
	case RIL_UNSOL_VT_RING_INFO: return "RIL_UNSOL_VT_RING_INFO";
	case RIL_UNSOL_VSIM_OPERATION_INDICATION: return "RIL_UNSOL_VSIM_OPERATION_INDICATION";
	case RIL_UNSOL_SET_ATTACH_APN: return "RIL_UNSOL_SET_ATTACH_APN";
	case RIL_UNSOL_MAL_AT_INFO: return "RIL_UNSOL_MAL_AT_INFO";
	case RIL_UNSOL_MAIN_SIM_INFO: return "RIL_UNSOL_MAIN_SIM_INFO";

    default: return "<unknown request>";
    }
}

const char *
rilSocketIdToString(RIL_SOCKET_ID socket_id)
{
    switch(socket_id) {
	case RIL_SOCKET_1:
	    return "RIL_SOCKET_1";
#if (SIM_COUNT >= 2)
	case RIL_SOCKET_2:
	    return "RIL_SOCKET_2";
#endif
#if (SIM_COUNT >= 3)
	case RIL_SOCKET_3:
	    return "RIL_SOCKET_3";
#endif
#if (SIM_COUNT >= 4)
	case RIL_SOCKET_4:
	    return "RIL_SOCKET_4";
#endif
	default:
	    return "not a valid RIL";
    }
}

const char *
proxyString (RILChannelId cid)
{
    int id = (int)cid % 6;
    switch(id) {
	case RIL_URC:
	    return "PROXY_4";
	case RIL_CMD_1:
	    return "PROXY_3";
	case RIL_CMD_2:
	    return "PROXY_2";
	case RIL_CMD_3:
	    return "PROXY_1";
	case RIL_CMD_4:
	    return "PROXY_5";
	case RIL_ATCI:
	    return "PROXY_6";
    }
    return "ERROR";
}

} /* namespace android */
