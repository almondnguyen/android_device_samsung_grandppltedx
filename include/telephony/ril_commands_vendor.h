/* //device/libs/telephony/ril_commands.h
**
** Copyright 2006, The Android Open Source Project
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
*/
    {RIL_OEM_REQUEST_BASE, NULL, NULL},
    {RIL_REQUEST_DIAL_EMERGENCY_CALL, dispatchDial, responseVoid}, // 10001
    {RIL_REQUEST_CALL_DEFLECTION, dispatchVoid, responseVoid}, // 10002
    {RIL_REQUEST_MODIFY_CALL_INITIATE, dispatchVoid, responseVoid}, // 10003
    {RIL_REQUEST_SET_IMS_CALL_LIST, dispatchVoid, responseVoid}, // 10004
    {RIL_REQUEST_SET_VOICE_DOMAIN_PREF, dispatchVoid, responseVoid}, // 10005
    {RIL_REQUEST_SAFE_MODE, dispatchVoid, responseVoid}, // 10006
    {RIL_REQUEST_SET_TRANSMIT_POWER, dispatchVoid, responseVoid}, // 10007
    {RIL_REQUEST_GET_CELL_BROADCAST_CONFIG, dispatchVoid, responseVoid}, // 10008
    {RIL_REQUEST_GET_PHONEBOOK_STORAGE_INFO, dispatchVoid, responseInts}, // 10009
    {RIL_REQUEST_GET_PHONEBOOK_ENTRY, dispatchVoid, responseVoid}, // 10010
    {RIL_REQUEST_ACCESS_PHONEBOOK_ENTRY, dispatchVoid, responseInts}, // 10011
    {RIL_REQUEST_USIM_PB_CAPA, dispatchVoid, responseInts}, // 10012
    {RIL_REQUEST_LOCK_INFO, dispatchVoid, responseVoid}, // 10013
    {RIL_REQUEST_STK_SIM_INIT_EVENT, dispatchVoid, responseVoid}, // 10014
    {RIL_REQUEST_SET_PREFERRED_NETWORK_LIST, dispatchVoid, responseVoid}, // 10015
    {RIL_REQUEST_GET_PREFERRED_NETWORK_LIST, dispatchDial, responseVoid}, // 10016
    {RIL_REQUEST_CHANGE_SIM_PERSO, dispatchVoid, responseInts}, // 10017
    {RIL_REQUEST_ENTER_SIM_PERSO, dispatchVoid, responseInts}, // 10018
    {RIL_REQUEST_SEND_ENCODED_USSD, dispatchVoid, responseVoid}, // 10019
    {RIL_REQUEST_CDMA_SEND_SMS_EXPECT_MORE, dispatchVoid, responseVoid}, // 10020
    {10021, NULL, NULL},
    {RIL_REQUEST_HOLD, dispatchVoid, responseVoid}, // 10022
    {RIL_REQUEST_SET_SIM_POWER, dispatchVoid, responseVoid}, // 10023
    {RIL_REQUEST_GET_ACB_INFO, dispatchVoid, responseInts}, // 11024
    {RIL_REQUEST_UICC_GBA_AUTHENTICATE_BOOTSTRAP, dispatchVoid, responseVoid}, // 10025
    {RIL_REQUEST_UICC_GBA_AUTHENTICATE_NAF, dispatchVoid, responseVoid}, // 10026
    {RIL_REQUEST_GET_INCOMING_COMMUNICATION_BARRING, dispatchVoid, responseString}, // 10027
    {RIL_REQUEST_SET_INCOMING_COMMUNICATION_BARRING, dispatchVoid, responseVoid}, // 10028
    {RIL_REQUEST_QUERY_CNAP, dispatchVoid, responseInts}, // 10029
    {RIL_REQUEST_SET_TRANSFER_CALL, dispatchVoid, responseVoid}, // 10030
    {RIL_REQUEST_GET_DISABLE_2G, dispatchVoid, responseInts}, // 11031
    {RIL_REQUEST_SET_DISABLE_2G, dispatchVoid, responseVoid}, // 11032
    {RIL_REQUEST_REFRESH_NITZ_TIME, dispatchVoid, responseVoid}, // 11033
    {RIL_REQUEST_ENABLE_UNSOL_RESPONSE, dispatchVoid, responseVoid}, // 11034
    {RIL_REQUEST_CANCEL_TRANSFER_CALL, dispatchVoid, responseVoid}, // 11035
    {RIL_REQUEST_SIM_OPEN_CHANNEL_WITH_P2, dispatchVoid, responseVoid}, // 11036
