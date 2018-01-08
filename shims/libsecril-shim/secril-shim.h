#ifndef __SECRIL_SHIM_H__
#define __SECRIL_SHIM_H__

#define LOG_TAG "secril-shim"
#define RIL_SHLIB

#include <dlfcn.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <binder/Parcel.h>
#include <cutils/compiler.h>
#include <cutils/properties.h>
#include <sys/cdefs.h>
#include <telephony/ril.h>
#include <utils/Log.h>

#define RIL_LIB_PATH "/system/lib/libsec-ril.so"

extern "C" const char * requestToString(int request);

typedef struct
{
  RIL_AppType      app_type;
  RIL_AppState     app_state;
  RIL_PersoSubstate perso_substate; /* applicable only if app_state ==
                                       RIL_APPSTATE_SUBSCRIPTION_PERSO */
  char             *aid_ptr;        /* null terminated string, e.g., from 0xA0, 0x00 -> 0x41,
                                       0x30, 0x30, 0x30 */
  char             *app_label_ptr;  /* null terminated string */
  int              pin1_replaced;   /* applicable to USIM, CSIM & ISIM */
  RIL_PinState     pin1;
  RIL_PinState     pin2;
  int              foo1;            /* Samsung */
  int              foo2;            /* Samsung */
  int              foo3;            /* Samsung */
  int              foo4;            /* Samsung */
  int              foo5;            /* Samsung */
} RIL_AppStatus_samsung;

typedef struct
{
  RIL_CardState card_state;
  RIL_PinState  universal_pin_state;             /* applicable to USIM and CSIM: RIL_PINSTATE_xxx */
  int           gsm_umts_subscription_app_index; /* value < RIL_CARD_MAX_APPS, -1 if none */
  int           cdma_subscription_app_index;     /* value < RIL_CARD_MAX_APPS, -1 if none */
  int           num_applications;                /* value <= RIL_CARD_MAX_APPS */
  RIL_AppStatus_samsung applications[RIL_CARD_MAX_APPS];
} RIL_CardStatus_v5_samsung;

/* TODO: Do we really need to redefine these? They aren't in a header... */
typedef struct {
    int requestNumber;
    void (*dispatchFunction) (android::Parcel &p, struct RequestInfo *pRI);
    int(*responseFunction) (android::Parcel &p, void *response, size_t responselen);
} CommandInfo;

typedef struct RequestInfo {
    int32_t token;
    CommandInfo *pCI;
    struct RequestInfo *p_next;
    char cancelled;
    char local;
    RIL_SOCKET_ID socket_id;
    int wasAckSent;
} RequestInfo;

#endif /* __SECRIL_SHIM_H__ */

