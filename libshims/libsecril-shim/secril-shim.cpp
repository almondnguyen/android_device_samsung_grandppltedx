#include "secril-shim.h"

#define ATOI_NULL_HANDLED(x) (x ? atoi(x) : 0)

/* A copy of the original RIL function table. */
static const RIL_RadioFunctions *origRilFunctions;

/* A copy of the ril environment passed to RIL_Init. */
static const struct RIL_Env *rilEnv;

/* Response data for RIL_REQUEST_VOICE_REGISTRATION_STATE */
static const int VOICE_REGSTATE_SIZE = 15 * sizeof(char *);
static char *voiceRegStateResponse[VOICE_REGSTATE_SIZE];

/* Store voice radio technology */
static int voiceRadioTechnology = -1;

static RIL_Dial dial;

static void onRequestAllowData(int request, void *data, size_t datalen, RIL_Token t) {
	RLOGI("%s: got request %s (data:%p datalen:%d)\n", __FUNCTION__,
			requestToString(request),
			data, datalen);

	const char rawHookCmd[] = { 0x09, 0x04 }; // RAW_HOOK_OEM_CMD_SWITCH_DATAPREFER
	bool allowed = *((int *)data) == 0 ? false : true;

	if (allowed) {
		RequestInfo *pRI = (RequestInfo *)t;
		pRI->pCI->requestNumber = RIL_REQUEST_OEM_HOOK_RAW;
		origRilFunctions->onRequest(pRI->pCI->requestNumber, (void *)rawHookCmd, sizeof(rawHookCmd), t);
	}

	rilEnv->OnRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
}

static void onRequestDial(int request, void *data, RIL_Token t) {
	RIL_UUS_Info uusInfo;

	dial.address = ((RIL_Dial *) data)->address;
	dial.clir = ((RIL_Dial *) data)->clir;
	dial.uusInfo = ((RIL_Dial *) data)->uusInfo;

	if (dial.uusInfo == NULL) {
		uusInfo.uusType = (RIL_UUS_Type) 0;
		uusInfo.uusDcs = (RIL_UUS_DCS) 0;
		uusInfo.uusData = NULL;
		uusInfo.uusLength = 0;
		dial.uusInfo = &uusInfo;
	}

	origRilFunctions->onRequest(request, &dial, sizeof(dial), t);
}

static int
decodeVoiceRadioTechnology (RIL_RadioState radioState) {
    switch (radioState) {
        case RADIO_STATE_SIM_NOT_READY:
        case RADIO_STATE_SIM_LOCKED_OR_ABSENT:
        case RADIO_STATE_SIM_READY:
            return RADIO_TECH_UMTS;

        case RADIO_STATE_RUIM_NOT_READY:
        case RADIO_STATE_RUIM_READY:
        case RADIO_STATE_RUIM_LOCKED_OR_ABSENT:
        case RADIO_STATE_NV_NOT_READY:
        case RADIO_STATE_NV_READY:
            return RADIO_TECH_1xRTT;

        default:
            RLOGD("decodeVoiceRadioTechnology: Invoked with incorrect RadioState");
            return -1;
    }
}

static void onRequestVoiceRadioTech(int request, void *data, size_t datalen, RIL_Token t) {
	RLOGI("%s: got request %s (data:%p datalen:%d)\n", __FUNCTION__,
		requestToString(request),
		data, datalen);
        RIL_RadioState radioState = origRilFunctions->onStateRequest();

	voiceRadioTechnology = decodeVoiceRadioTechnology(radioState);
	if (voiceRadioTechnology < 0) {
		rilEnv->OnRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
		return;
	}
	rilEnv->OnRequestComplete(t, RIL_E_SUCCESS, &voiceRadioTechnology, sizeof(voiceRadioTechnology));
}

static bool onRequestGetRadioCapability(RIL_Token t)
{
	RIL_RadioCapability rc[1] =
	{
		{ /* rc[0] */
			RIL_RADIO_CAPABILITY_VERSION, /* version */
			0, /* session */
			RC_PHASE_CONFIGURED, /* phase */
			RAF_GSM | RAF_GPRS | RAF_EDGE | RAF_HSUPA | RAF_HSDPA | RAF_HSPA | RAF_HSPAP | RAF_UMTS, /* rat */
			{ /* logicalModemUuid */
				0,
			},
			RC_STATUS_SUCCESS /* status */
		}
	};
	rilEnv->OnRequestComplete(t, RIL_E_SUCCESS, rc, sizeof(rc));
	return true;
}

static bool onCompleteGetActivityInfo(RIL_Token t)
{
	RIL_ActivityStatsInfo stats[1];
	stats[0].sleep_mode_time_ms = 0;
	stats[0].idle_mode_time_ms = 0;
	for(int i = 0; i < RIL_NUM_TX_POWER_LEVELS; i++) {
		stats[0].tx_mode_time_ms[i] = 0;
	}
	stats[0].rx_mode_time_ms = 0;

	rilEnv->OnRequestComplete(t, RIL_E_SUCCESS, stats, sizeof(stats));
	return true;
}

static void onRequestShim(int request, void *data, size_t datalen, RIL_Token t)
{
	switch (request) {
		/* Our RIL doesn't support this, so we implement this ourself */
		case RIL_REQUEST_VOICE_RADIO_TECH:
			onRequestVoiceRadioTech(request, data, datalen, t);
			RLOGI("%s: got request %s: replied with our implementation!\n", __FUNCTION__, requestToString(request));
			return;
		/* The Samsung RIL crashes if uusInfo is NULL... */
		case RIL_REQUEST_DIAL:
			if (datalen == sizeof(RIL_Dial) && data != NULL) {
				onRequestDial(request, data, t);
				RLOGI("%s: got request %s: replied with our implementation!\n", __FUNCTION__, requestToString(request));
				return;
			}
			break;

		/* Necessary; RILJ may fake this for us if we reply not supported, but we can just implement it. */
		case RIL_REQUEST_GET_RADIO_CAPABILITY:
			onRequestGetRadioCapability(t);
			RLOGI("%s: got request %s: replied with our implementation!\n", __FUNCTION__, requestToString(request));
			return;
		case RIL_REQUEST_ALLOW_DATA:
			onRequestAllowData(request, data, datalen, t);
			RLOGI("%s: got request %s: replied with our implementation!\n", __FUNCTION__, requestToString(request));
			return;
		/* The Samsung RIL doesn't support RIL_REQUEST_SEND_SMS_EXPECT_MORE, reply with RIL_REQUEST_SEND_SMS instead */
		case RIL_REQUEST_SEND_SMS_EXPECT_MORE:
			RLOGI("%s: got request %s: replied with %s!", __FUNCTION__,
				      requestToString(request), requestToString(RIL_REQUEST_SEND_SMS));
			origRilFunctions->onRequest(RIL_REQUEST_SEND_SMS, data, datalen, t);
			return;
		/* The following requests were introduced post-4.3. */
		case RIL_REQUEST_SIM_TRANSMIT_APDU_BASIC:
		case RIL_REQUEST_SIM_OPEN_CHANNEL: /* !!! */
		case RIL_REQUEST_SIM_CLOSE_CHANNEL:
		case RIL_REQUEST_SIM_TRANSMIT_APDU_CHANNEL:
		case RIL_REQUEST_NV_READ_ITEM:
		case RIL_REQUEST_NV_WRITE_ITEM:
		case RIL_REQUEST_NV_WRITE_CDMA_PRL:
		case RIL_REQUEST_NV_RESET_CONFIG:
		case RIL_REQUEST_SET_UICC_SUBSCRIPTION:
		case RIL_REQUEST_GET_HARDWARE_CONFIG:
		case RIL_REQUEST_SIM_AUTHENTICATION:
		case RIL_REQUEST_GET_DC_RT_INFO:
		case RIL_REQUEST_SET_DC_RT_INFO_RATE:
		case RIL_REQUEST_SET_DATA_PROFILE:
		case RIL_REQUEST_SHUTDOWN: /* TODO: Is there something we can do for RIL_REQUEST_SHUTDOWN ? */
		case RIL_REQUEST_SET_RADIO_CAPABILITY:
		case RIL_REQUEST_START_LCE:
		case RIL_REQUEST_STOP_LCE:
		case RIL_REQUEST_PULL_LCEDATA:
			RLOGW("%s: got request %s: replied with REQUEST_NOT_SUPPPORTED.\n", __FUNCTION__, requestToString(request));
			rilEnv->OnRequestComplete(t, RIL_E_REQUEST_NOT_SUPPORTED, NULL, 0);
			return;
	}

	RLOGD("%s: got request %s: forwarded to RIL.\n", __FUNCTION__, requestToString(request));
	origRilFunctions->onRequest(request, data, datalen, t);
}

static void onCompleteRequestGetSimStatus(RIL_Token t, RIL_Errno e, void *response) {
	/* While at it, upgrade the response to RIL_CardStatus_v6 */
	RIL_CardStatus_v6_samsung *p_cur = (RIL_CardStatus_v6_samsung *) response;
	RIL_CardStatus_v6 v6response;

	v6response.card_state = p_cur->card_state;
	v6response.universal_pin_state = p_cur->universal_pin_state;
	v6response.gsm_umts_subscription_app_index = p_cur->gsm_umts_subscription_app_index;
	v6response.cdma_subscription_app_index = p_cur->cdma_subscription_app_index;
	v6response.ims_subscription_app_index = -1;
	v6response.num_applications = p_cur->num_applications;

	int i;
	for (i = 0; i < RIL_CARD_MAX_APPS; ++i)
		memcpy(&v6response.applications[i], &p_cur->applications[i], sizeof(RIL_AppStatus));

	/* Send the fixed response to libril */
	rilEnv->OnRequestComplete(t, e, &v6response, sizeof(RIL_CardStatus_v6));
}

static void onRequestCompleteVoiceRegistrationState(RIL_Token t, RIL_Errno e, void *response, size_t responselen) {
	char **resp = (char **) response;
        char radioTechUmts = '3';
	memset(voiceRegStateResponse, 0, VOICE_REGSTATE_SIZE);
	for (int index = 0; index < (int)responselen; index++) {
		voiceRegStateResponse[index] = resp[index];
                // Add RADIO_TECH_UMTS because our RIL doesn't provide this here
		if (index == 3) {
			voiceRegStateResponse[index] = &radioTechUmts;
                }
	}
	rilEnv->OnRequestComplete(t, e, voiceRegStateResponse, VOICE_REGSTATE_SIZE);
}

static void fixupDataCallList(void *response, size_t responselen) {
	RIL_Data_Call_Response_v6 *p_cur = (RIL_Data_Call_Response_v6 *) response;
	int num = responselen / sizeof(RIL_Data_Call_Response_v6);

	int i;
	for (i = 0; i < num; ++i)
		p_cur[i].gateways = p_cur[i].addresses;
}

static void onCompleteQueryAvailableNetworks(RIL_Token t, RIL_Errno e, void *response, size_t responselen) {
	/* Response is a char **, pointing to an array of char *'s */
	size_t numStrings = responselen / sizeof(char *);
	size_t newResponseLen = (numStrings - (numStrings / 3)) * sizeof(char *);

	void *newResponse = malloc(newResponseLen);

	/* Remove every 5th and 6th strings (qan elements) */
	char **p_cur = (char **) response;
	char **p_new = (char **) newResponse;
	size_t i, j;
	for (i = 0, j = 0; i < numStrings; i += 6) {
		p_new[j++] = p_cur[i];
		p_new[j++] = p_cur[i + 1];
		p_new[j++] = p_cur[i + 2];
		p_new[j++] = p_cur[i + 3];
	}

	/* Send the fixed response to libril */
	rilEnv->OnRequestComplete(t, e, newResponse, newResponseLen);

	free(newResponse);
}

static void fixupSignalStrength(void *response) {
	int gsmSignalStrength;

	RIL_SignalStrength_v10 *p_cur = ((RIL_SignalStrength_v10 *) response);

	gsmSignalStrength = p_cur->GW_SignalStrength.signalStrength & 0xFF;

	if (gsmSignalStrength < 0 ||
		(gsmSignalStrength > 31 && p_cur->GW_SignalStrength.signalStrength != 99)) {
		gsmSignalStrength = p_cur->CDMA_SignalStrength.dbm;
	}

	/* Fix GSM signal strength */
	p_cur->GW_SignalStrength.signalStrength = gsmSignalStrength;

	/* We don't support LTE - values should be set to INT_MAX */
	p_cur->LTE_SignalStrength.cqi = INT_MAX;
	p_cur->LTE_SignalStrength.rsrp = INT_MAX;
	p_cur->LTE_SignalStrength.rsrq = INT_MAX;
	p_cur->LTE_SignalStrength.rssnr = INT_MAX;
}

static void onRequestCompleteShim(RIL_Token t, RIL_Errno e, void *response, size_t responselen) {
	int request;
	RequestInfo *pRI;

	pRI = (RequestInfo *)t;

	/* If pRI is null, this entire function is useless. */
	if (pRI == NULL)
		goto null_token_exit;

	/* If pCI is null or invalid pointer, this entire function is useless. */
	if (!pRI->pCI || (0 <(int)pRI->pCI && (int)pRI->pCI < 0xff)) {
		RLOGE("Invalid CommandInterface pointer: %p", pRI->pCI);
		goto null_token_exit;
	}

	request = pRI->pCI->requestNumber;
	switch (request) {
                case RIL_REQUEST_VOICE_REGISTRATION_STATE:
                        /* libsecril expects responselen of 60 (bytes) */
                        /* numstrings (15 * sizeof(char *) = 60) */
			if (response != NULL && responselen < VOICE_REGSTATE_SIZE) {
				RLOGD("%s: got request %s and shimming response!\n", __FUNCTION__, requestToString(request));
				onRequestCompleteVoiceRegistrationState(t, e, response, responselen);
				return;
			}
			break;
		case RIL_REQUEST_GET_SIM_STATUS:
			/* Remove unused extra elements from RIL_AppStatus */
			if (response != NULL && responselen == sizeof(RIL_CardStatus_v6_samsung)) {
				RLOGD("%s: got request %s and shimming response!\n", __FUNCTION__, requestToString(request));
				onCompleteRequestGetSimStatus(t, e, response);
				return;
			}
			break;
		case RIL_REQUEST_LAST_CALL_FAIL_CAUSE:
			/* Remove extra element (ignored on pre-M, now crashing the framework) */
			if (responselen > sizeof(int)) {
				RLOGD("%s: got request %s and shimming response!\n", __FUNCTION__, requestToString(request));
				rilEnv->OnRequestComplete(t, e, response, sizeof(int));
				return;
			}
			break;
		case RIL_REQUEST_DATA_CALL_LIST:
		case RIL_REQUEST_SETUP_DATA_CALL:
			/* According to the Samsung RIL, the addresses are the gateways?
			 * This fixes mobile data. */
			if (response != NULL && responselen != 0 && (responselen % sizeof(RIL_Data_Call_Response_v6) == 0)) {
				RLOGD("%s: got request %s and shimming response!\n", __FUNCTION__, requestToString(request));
				fixupDataCallList(response, responselen);
				rilEnv->OnRequestComplete(t, e, response, responselen);
				return;
			}
			break;
		case RIL_REQUEST_QUERY_AVAILABLE_NETWORKS:
			/* Remove the extra (unused) elements from the operator info, freaking out the framework.
			 * Formerly, this is know as the mQANElements override. */
			if (response != NULL && responselen != 0 && (responselen % sizeof(char *) == 0)) {
				RLOGD("%s: got request %s and shimming response!\n", __FUNCTION__, requestToString(request));
				onCompleteQueryAvailableNetworks(t, e, response, responselen);
				return;
			}
			break;
		case RIL_REQUEST_SIGNAL_STRENGTH:
			/* The Samsung RIL reports the signal strength in a strange way... */
			if (response != NULL && responselen >= sizeof(RIL_SignalStrength_v5)) {
				RLOGD("%s: got request %s and shimming response!\n", __FUNCTION__, requestToString(request));
				fixupSignalStrength(response);
				rilEnv->OnRequestComplete(t, e, response, responselen);
				return;
			}
			break;
		case RIL_REQUEST_GET_ACTIVITY_INFO:
			RLOGD("%s: got request %s and shimming response!\n", __FUNCTION__, requestToString(request));
			onCompleteGetActivityInfo(t);
			return;
	}
	RLOGD("%s: got request %s: forwarded to libril.\n", __FUNCTION__, requestToString(request));

null_token_exit:
	rilEnv->OnRequestComplete(t, e, response, responselen);
}

static void onUnsolicitedResponseShim(int unsolResponse, const void *data, size_t datalen)
{
	switch (unsolResponse) {
		case RIL_UNSOL_DATA_CALL_LIST_CHANGED:
			/* According to the Samsung RIL, the addresses are the gateways?
			 * This fixes mobile data. */
			if (data != NULL && datalen != 0 && (datalen % sizeof(RIL_Data_Call_Response_v6) == 0))
				fixupDataCallList((void*) data, datalen);
			break;
		case RIL_UNSOL_SIGNAL_STRENGTH:
			/* The Samsung RIL reports the signal strength in a strange way... */
			if (data != NULL && datalen >= sizeof(RIL_SignalStrength_v5))
				fixupSignalStrength((void*) data);
			break;
	}

	rilEnv->OnUnsolicitedResponse(unsolResponse, data, datalen);
}

static void patchMem(void *libHandle) {
	/*
	 * MAX_TIMEOUT is used for a call to pthread_cond_timedwait_relative_np.
	 * The issue is bionic has switched to using absolute timeouts instead of
	 * relative timeouts, and a maximum time value can cause an overflow in
	 * the function converting relative to absolute timespecs if unpatched.
	 *
	 * By patching this to 0x01FFFFFF from 0x7FFFFFFF, the timeout should
	 * expire in about a year rather than 68 years, and the RIL should be good
	 * up until the year 2036 or so.
	 */
	uint32_t *MAX_TIMEOUT;

	MAX_TIMEOUT = (uint32_t *)dlsym(libHandle, "MAX_TIMEOUT");
	if (CC_UNLIKELY(!MAX_TIMEOUT)) {
		RLOGE("%s: MAX_TIMEOUT could not be found!", __FUNCTION__);
		return;
	}
	RLOGD("%s: MAX_TIMEOUT found at %p!", __FUNCTION__, MAX_TIMEOUT);
	RLOGD("%s: MAX_TIMEOUT is currently 0x%" PRIX32, __FUNCTION__, *MAX_TIMEOUT);
	if (CC_LIKELY(*MAX_TIMEOUT == 0x7FFFFFFF)) {
		*MAX_TIMEOUT = 0x01FFFFFF;
		RLOGI("%s: MAX_TIMEOUT was changed to 0x0%" PRIX32, __FUNCTION__, *MAX_TIMEOUT);
	} else {
		RLOGW("%s: MAX_TIMEOUT was not 0x7FFFFFFF; leaving alone", __FUNCTION__);
	}

}

const RIL_RadioFunctions* RIL_Init(const struct RIL_Env *env, int argc, char **argv)
{
	RIL_RadioFunctions const* (*origRilInit)(const struct RIL_Env *env, int argc, char **argv);
	static RIL_RadioFunctions shimmedFunctions;
	static struct RIL_Env shimmedEnv;
	void *origRil;

	/* Shim the RIL_Env passed to the real RIL, saving a copy of the original */
	rilEnv = env;
	shimmedEnv = *env;
	shimmedEnv.OnRequestComplete = onRequestCompleteShim;
	shimmedEnv.OnUnsolicitedResponse = onUnsolicitedResponseShim;

	/* Open and Init the original RIL. */

	origRil = dlopen(RIL_LIB_PATH, RTLD_GLOBAL);
	if (CC_UNLIKELY(!origRil)) {
		RLOGE("%s: failed to load '" RIL_LIB_PATH  "': %s\n", __FUNCTION__, dlerror());
		return NULL;
	}

	origRilInit = (const RIL_RadioFunctions *(*)(const struct RIL_Env *, int, char **))(dlsym(origRil, "RIL_Init"));
	if (CC_UNLIKELY(!origRilInit)) {
		RLOGE("%s: couldn't find original RIL_Init!\n", __FUNCTION__);
		goto fail_after_dlopen;
	}

	// Fix RIL issues by patching memory
	patchMem(origRil);

	//remove "-c" command line as Samsung's RIL does not understand it - it just barfs instead
	for (int i = 0; i < argc; i++) {
		if (!strcmp(argv[i], "-c") && i != argc -1) {	//found it
			memcpy(argv + i, argv + i + 2, sizeof(char*[argc - i - 2]));
			argc -= 2;
		}
	}

	origRilFunctions = origRilInit(&shimmedEnv, argc, argv);
	if (CC_UNLIKELY(!origRilFunctions)) {
		RLOGE("%s: the original RIL_Init derped.\n", __FUNCTION__);
		goto fail_after_dlopen;
	}

	/* Shim functions as needed. */
	shimmedFunctions = *origRilFunctions;
	shimmedFunctions.onRequest = onRequestShim;

	return &shimmedFunctions;

fail_after_dlopen:
	dlclose(origRil);
	return NULL;
}
