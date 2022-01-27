/* //device/system/rild/rild.c
**
** Copyright 2017 The Android Open Source Project
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
** 2017/7/29:
** Initial Oero port for MT6752 - by:daniel_hk(https://github.com/daniel_hk)
*/

#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <telephony/ril.h>
#define LOG_TAG "RILD"
#include <log/log.h>
#include <cutils/properties.h>
#include <cutils/sockets.h>
#include <sys/capability.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <libril/ril_ex.h>

#include <private/android_filesystem_config.h>

#define LIB_PATH_PROPERTY   "rild.libpath"
#define LIB_ARGS_PROPERTY   "rild.libargs"
#define MAX_LIB_ARGS	16
#define MAX_CAP_NUM	 (CAP_TO_INDEX(CAP_LAST_CAP) + 1)

static void usage(const char *argv0) {
    fprintf(stderr, "Usage: %s -l <ril impl library> [-- <args for impl library>]\n", argv0);
    exit(EXIT_FAILURE);
}

extern char ril_service_name_base[MAX_SERVICE_NAME_LENGTH];
extern char ril_service_name[MAX_SERVICE_NAME_LENGTH];
extern void RIL_register (const RIL_RadioFunctions *callbacks);
extern void RIL_registerSocket (const RIL_RadioFunctionsSocket *callbacks);
extern void rilc_thread_pool ();

//extern void RIL_register_socket (RIL_RadioFunctions *(*rilUimInit)
//	(const struct RIL_Env *, int, char **), RIL_SOCKET_TYPE socketType, int argc, char **argv);

extern void RIL_register_socket (RIL_RadioFunctionsSocket *(*rilUimInit)
        (const struct RIL_EnvSocket *, int, char **), RIL_SOCKET_TYPE socketType, int argc, char **argv);

extern void RIL_onRequestComplete(RIL_Token t, RIL_Errno e,
	void *response, size_t responselen);

extern void RIL_onRequestAck(RIL_Token t);

extern void RIL_setServiceName(char *);

extern void RIL_onUnsolicitedResponseSocket(int unsolResponse, const void *data,
                                size_t datalen, RIL_SOCKET_ID socket_id);

#if defined(ANDROID_MULTI_SIM)
extern void RIL_onUnsolicitedResponse(int unsolResponse, const void *data,
	size_t datalen, RIL_SOCKET_ID socket_id);
#else
extern void RIL_onUnsolicitedResponse(int unsolResponse, const void *data,
	size_t datalen);
#endif

extern void RIL_requestTimedCallback (RIL_TimedCallback callback,
	void *param, const struct timeval *relativeTime);

extern void RIL_myProxyTimedCallback (RIL_TimedCallback callback,
                               void *param, const struct timeval *relativeTime, int proxyId);
extern RILChannelId RIL_myChannelId(RIL_Token t);
extern int RIL_myProxyIdByThread();

// mtk proprietary
extern int mtkInit();

static struct RIL_Env s_rilEnv = {
    RIL_onRequestComplete,
    RIL_onUnsolicitedResponse,
    RIL_requestTimedCallback
    ,RIL_myProxyTimedCallback
    ,RIL_myChannelId
    ,RIL_myProxyIdByThread
};

static struct RIL_EnvSocket s_rilEnvSocket = {
    RIL_onRequestComplete,
    RIL_onUnsolicitedResponseSocket,
    RIL_requestTimedCallback
    ,RIL_myProxyTimedCallback
    ,RIL_myChannelId
    ,RIL_myProxyIdByThread
};

extern void RIL_startEventLoop();

static int make_argv(char * args, char ** argv) {
    // Note: reserve argv[0]
    int count = 1;
    char * tok;
    char * s = args;

    while ((tok = strtok(s, " \0"))) {
	argv[count] = tok;
	s = NULL;
	count++;
    }
    return count;
}

/*
 * switchUser - Switches UID to radio, preserving CAP_NET_ADMIN capabilities.
 * Our group, cache, was set by init.
 */
void switchUser() {
    char debuggable[PROP_VALUE_MAX];

    prctl(PR_SET_KEEPCAPS, 1, 0, 0, 0);
    if (setresuid(AID_RADIO, AID_RADIO, AID_RADIO) == -1) {
	RLOGE("setresuid failed: %s", strerror(errno));
	exit(EXIT_FAILURE);
    }

    struct __user_cap_header_struct header;
    memset(&header, 0, sizeof(header));
    header.version = _LINUX_CAPABILITY_VERSION_3;
    header.pid = 0;

    struct __user_cap_data_struct data[MAX_CAP_NUM];
    memset(&data, 0, sizeof(data));

    data[CAP_TO_INDEX(CAP_NET_ADMIN)].effective |= CAP_TO_MASK(CAP_NET_ADMIN);
    data[CAP_TO_INDEX(CAP_NET_ADMIN)].permitted |= CAP_TO_MASK(CAP_NET_ADMIN);

    data[CAP_TO_INDEX(CAP_NET_RAW)].effective |= CAP_TO_MASK(CAP_NET_RAW);
    data[CAP_TO_INDEX(CAP_NET_RAW)].permitted |= CAP_TO_MASK(CAP_NET_RAW);

    data[CAP_TO_INDEX(CAP_BLOCK_SUSPEND)].effective |= CAP_TO_MASK(CAP_BLOCK_SUSPEND);
    data[CAP_TO_INDEX(CAP_BLOCK_SUSPEND)].permitted |= CAP_TO_MASK(CAP_BLOCK_SUSPEND);

    if (capset(&header, &data[0]) == -1) {
	RLOGE("capset failed: %s", strerror(errno));
	exit(EXIT_FAILURE);
    }

    /*
     * Debuggable build only:
     * Set DUMPABLE that was cleared by setuid() to have tombstone on RIL crash
     */
    property_get("ro.debuggable", debuggable, "0");
    if (strcmp(debuggable, "1") == 0) {
	prctl(PR_SET_DUMPABLE, 1, 0, 0, 0);
    }
}

int main(int argc, char **argv) {
    // vendor ril lib path either passed in as -l parameter, or read from rild.libpath property
    const char * rilLibPath = NULL;
    // ril arguments either passed in as -- parameter, or read from rild.libargs property
    char **rilArgv;
    // handle for vendor ril lib
    void *dlHandle;
    // Pointer to ril init function in vendor ril
    const RIL_RadioFunctions *(*rilInit)(const struct RIL_Env *, int, char **);
    const RIL_RadioFunctions *(*rilInitSocket)(const struct RIL_EnvSocket *, int, char **);
    // Pointer to sap init function in vendor ril
    //RIL_RadioFunctions *(*rilUimInit)(const struct RIL_Env *, int, char **);
    RIL_RadioFunctionsSocket *(*rilUimInit)(const struct RIL_EnvSocket *, int, char **);
    const char *err_str = NULL;

    // functions returned by ril init function in vendor ril
    const RIL_RadioFunctions *funcs;
    const RIL_RadioFunctionsSocket *funcsSocket;
    // lib path from rild.libpath property (if it's read)
    char libPath[PROPERTY_VALUE_MAX];
    // flat to indicate if -- parameters are present
    unsigned char hasLibArgs = 0;

    int i;
    // ril/socket id received as -c parameter, otherwise set to 0
    const char *clientId = NULL;

    if (mtkInit() == -1) {
	RLOGE("**mtkInit() error**");
	goto done;
    }

    RLOGD("**RIL Daemon Started**");
    RLOGD("**RILd param count=%d**", argc);

    umask(S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IWOTH | S_IXOTH);
    for (i = 1; i < argc ;) {
	if (0 == strcmp(argv[i], "-l") && (argc - i > 1)) {
	    rilLibPath = argv[i + 1];
	    i += 2;
	} else if (0 == strcmp(argv[i], "--")) {
	    i++;
	    hasLibArgs = 1;
	    break;
	} else if (0 == strcmp(argv[i], "-c") &&  (argc - i > 1)) {
	    clientId = argv[i+1];
	    i += 2;
	} else {
	    usage(argv[0]);
	}
    }

    if (clientId == NULL) {
	clientId = "0";
    } else if (atoi(clientId) >= MAX_RILDS) {
	RLOGE("Max Number of rild's supported is: %d", MAX_RILDS);
	exit(0);
    }
    if (strncmp(clientId, "0", MAX_CLIENT_ID_LENGTH)) {
	strncpy(ril_service_name, ril_service_name_base, MAX_SERVICE_NAME_LENGTH);
	strncat(ril_service_name, clientId, MAX_SERVICE_NAME_LENGTH);
	RIL_setServiceName(ril_service_name);
    }

    if (rilLibPath == NULL) {
	if ( 0 == property_get(LIB_PATH_PROPERTY, libPath, "mtk-ril.so")) {
	    // No lib sepcified on the command line, and nothing set in props.
	    // Assume "no-ril" case.
	    goto done;
	} else {
	    rilLibPath = libPath;
	}
    }

    switchUser();

    RLOGD("open vendor lib path: %s", rilLibPath);

    dlHandle = dlopen(rilLibPath, RTLD_NOW);

    if (dlHandle == NULL) {
	RLOGE("dlopen failed: %s", dlerror());
	exit(EXIT_FAILURE);
    }

    RIL_startEventLoop();

    rilInitSocket = (const int(*)(void))dlsym(dlHandle, "RIL_InitSocket");

    if (rilInitSocket == NULL) {
	RLOGD("Vendor RIL do not need socket id!");
	rilInit =
	    (const RIL_RadioFunctions *(*)(const struct RIL_Env *, int, char **))
	    dlsym(dlHandle, "RIL_Init");

	if (rilInit == NULL) {
	    RLOGE("RIL_Init not defined or exported in %s\n", rilLibPath);
	    exit(EXIT_FAILURE);
	}
    } else {
	RLOGD("vendor RIL need socket id");
    }

    dlerror(); // Clear any previous dlerror
    rilUimInit =
        (const RIL_RadioFunctionsSocket *(*)(const struct RIL_EnvSocket *, int, char **))
        dlsym(dlHandle, "RIL_SAP_Init");
    err_str = dlerror();
    if (err_str) {
	RLOGW("RIL_SAP_Init not defined or exported in %s: %s\n", rilLibPath, err_str);
    } else if (!rilUimInit) {
	RLOGW("RIL_SAP_Init defined as null in %s. SAP Not usable\n", rilLibPath);
    }

    if (hasLibArgs) {
	rilArgv = argv + i - 1;
	argc = argc -i + 1;
    } else {
	static char * newArgv[MAX_LIB_ARGS];
	static char args[PROPERTY_VALUE_MAX];
	rilArgv = newArgv;
	property_get(LIB_ARGS_PROPERTY, args, "-d /dev/ttyC0");
	argc = make_argv(args, rilArgv);
    }
/*
    rilArgv[argc++] = "-c";
    rilArgv[argc++] = clientId;
    RLOGD("RIL_Init argc = %d clientId = %s", argc, rilArgv[argc-1]);
*/
    // Make sure there's a reasonable argv[0]
    rilArgv[0] = argv[0];

    if (rilInitSocket == NULL) {
        RLOGD("Old vendor ril! so RIL_register is called");
        funcs = rilInit(&s_rilEnv, argc, rilArgv);
        RIL_register(funcs);
	RLOGD("RIL_Init RIL_register completed");
    } else {
        RLOGD("New vendor ril! so RIL_registerSocket is called");
        funcsSocket = rilInitSocket(&s_rilEnvSocket, argc, rilArgv);
        RIL_registerSocket(funcsSocket);
	RLOGD("RIL_InitSocket RIL_registerSocket completed");
    }

    if (rilUimInit) {
	RLOGD("RIL_register_socket started");
	RIL_register_socket(rilUimInit, RIL_SAP_SOCKET, argc, rilArgv);
    }

    RLOGD("RIL_register_socket completed");
done:

    rilc_thread_pool();

    RLOGD("RIL_Init starting sleep loop");
    while (true) {
	sleep(UINT32_MAX);
    }
}
