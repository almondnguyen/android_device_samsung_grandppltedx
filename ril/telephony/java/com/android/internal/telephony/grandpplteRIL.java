/*
 * Copyright (c) 2014-2016, The CyanogenMod Project. All rights reserved.
 * Copyright (c) 2017, The LineageOS Project. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.internal.telephony;

import static com.android.internal.telephony.RILConstants.*;

import android.content.Context;
import android.telephony.Rlog;
import android.os.Message;
import android.os.Parcel;
import android.os.SystemProperties;
import android.telephony.PhoneNumberUtils;
import android.telephony.SignalStrength;
import android.telephony.SmsManager;
import com.android.internal.telephony.uicc.IccCardApplicationStatus;
import com.android.internal.telephony.uicc.IccCardStatus;
import com.android.internal.telephony.uicc.IccRefreshResponse;
import com.android.internal.telephony.uicc.IccUtils;
import java.util.ArrayList;
import java.util.Collections;

/**
 * RIL customization for Galaxy J2 Prime/Grand Prime Plus (GSM)
 *
 * {@hide}
 */

public class grandpplteRIL extends RIL {

    /**********************************************************
     * SAMSUNG REQUESTS
     **********************************************************/
    static final boolean RILJ_LOGD = true;
    static final boolean RILJ_LOGV = false;

    private static final int RIL_REQUEST_DIAL_EMERGENCY_CALL = 10001;
    private static final int RIL_UNSOL_STK_SEND_SMS_RESULT = 11002;
    private static final int RIL_UNSOL_STK_CALL_CONTROL_RESULT = 11003;

    private static final int RIL_UNSOL_DEVICE_READY_NOTI = 11008;
    private static final int RIL_UNSOL_AM = 11010;
    private static final int RIL_UNSOL_SIM_PB_READY = 11021;

    public grandpplteRIL(Context context, int preferredNetworkType, int cdmaSubscription) {
        this(context, preferredNetworkType, cdmaSubscription, null);
    }

    public grandpplteRIL(Context context, int preferredNetworkType, int cdmaSubscription, Integer instanceId) {
        super(context);
    }

    @Override
    public void acceptCall(Message result) {
        acceptCall(0, result);
    }

    public void acceptCall(int type, Message result) {
        RILRequest rr = RILRequest.obtain(40, result);
        riljLog(rr.serialString() + "> " + requestToString(rr.mRequest) + " " + type);
        rr.mParcel.writeInt(1);
        rr.mParcel.writeInt(type);
        send(rr);
    }

    private int translateStatus(int status) {
        switch (status & 7) {
            case 3:
                return 0;
            case 5:
                return 3;
            case 7:
                return 2;
            default:
                return 1;
        }
    }

    @Override
    public void writeSmsToSim(int status, String smsc, String pdu, Message response) {
        int status2 = translateStatus(status);
        RILRequest rr = RILRequest.obtain(63, response);
        rr.mParcel.writeInt(status2);
        rr.mParcel.writeString(pdu);
        rr.mParcel.writeString(smsc);
        send(rr);
    }

    public void dial(String address, int clirMode, UUSInfo uusInfo, Message result) {
	
	// inherit from SlteRIL
        if (PhoneNumberUtils.isEmergencyNumber(address)) {
            dialEmergencyCall(address, clirMode, result);
            return;
        }

        RILRequest rr = RILRequest.obtain(10, result);
        rr.mParcel.writeString(address);
        rr.mParcel.writeInt(clirMode);
        rr.mParcel.writeInt(0);
        rr.mParcel.writeInt(1);
        rr.mParcel.writeString("");

        if (uusInfo == null) {
            rr.mParcel.writeInt(0);
        } else {
            rr.mParcel.writeInt(1);
            rr.mParcel.writeInt(uusInfo.getType());
            rr.mParcel.writeInt(uusInfo.getDcs());
            rr.mParcel.writeByteArray(uusInfo.getUserData());
        }
        riljLog(rr.serialString() + "> " + requestToString(rr.mRequest) + " " + callDetails);
        send(rr);
    }

    public void dialEmergencyCall(String address, int clirMode, CallDetails callDetails, Message result) {
        RILRequest rr = RILRequest.obtain(10001, result);
        rr.mParcel.writeString(address);
        rr.mParcel.writeInt(clirMode);
        
        rr.mParcel.writeInt(0);
        rr.mParcel.writeInt(3);
        rr.mParcel.writeString("");
       
        rr.mParcel.writeInt(0);
        send(rr);

    @Override
    private Object responseIccCardStatus(Parcel p) {
	// TODO \ Selfnote : opensource RIL class is somewhat different than stock one.
	
        IccCardStatus cardStatus = new IccCardStatus();
        cardStatus.setCardState(p.readInt());
        cardStatus.setUniversalPinState(p.readInt());
        cardStatus.mGsmUmtsSubscriptionAppIndex = p.readInt();
        cardStatus.mCdmaSubscriptionAppIndex = p.readInt();
        cardStatus.mImsSubscriptionAppIndex = p.readInt();
        int numApplications = p.readInt();
        if (numApplications > 8) {
            numApplications = 8;
        
        cardStatus.mApplications = new IccCardApplicationStatus[numApplications];
        for (int i = 0; i < numApplications; i++) {
            IccCardApplicationStatus appStatus = new IccCardApplicationStatus();
            appStatus.app_type = appStatus.AppTypeFromRILInt(p.readInt());
            appStatus.app_state = appStatus.AppStateFromRILInt(p.readInt());
            appStatus.perso_substate = appStatus.PersoSubstateFromRILInt(p.readInt());
            appStatus.aid = p.readString();
            appStatus.app_label = p.readString();
            appStatus.pin1_replaced = p.readInt();
            appStatus.pin1 = appStatus.PinStateFromRILInt(p.readInt());
            appStatus.pin2 = appStatus.PinStateFromRILInt(p.readInt());
            appStatus.pin1_num_retries = p.readInt();
            appStatus.puk1_num_retries = p.readInt();
            appStatus.pin2_num_retries = p.readInt();
            appStatus.puk2_num_retries = p.readInt();
            appStatus.perso_unblock_retries = p.readInt();
            cardStatus.mApplications[i] = appStatus;
        }
        return cardStatus;
    }

    // use opensource aka SlteRIL, Samsung used MM packages (that quite sure will not work on later versions)

    @Override
    protected Object responseSignalStrength(Parcel p) {
        int numInts = 12;
        int response[];

        // Get raw data
        response = new int[numInts];
        for (int i = 0; i < numInts; i++) {
            response[i] = p.readInt();
        }
        // gsm
        response[0] &= 0xff;
        // cdma
        response[2] %= 256;
        response[4] %= 256;
        // lte
        response[7] &= 0xff;

        return new SignalStrength(response[0],
                                  response[1],
                                  response[2],
                                  response[3],
                                  response[4],
                                  response[5],
                                  response[6],
                                  response[7],
                                  response[8],
                                  response[9],
                                  response[10],
                                  response[11],
                                  true);
    }

    // this phone is GSM only
    private void constructGsmSendSmsRilRequest(RILRequest rr, String smscPDU, String pdu) {
        rr.mParcel.writeInt(2);
        rr.mParcel.writeString(smscPDU);
        rr.mParcel.writeString(pdu);
    }

    // according to SlteRIL
    /**
     * The RIL can't handle the RIL_REQUEST_SEND_SMS_EXPECT_MORE
     * request properly, so we use RIL_REQUEST_SEND_SMS instead.
     */
    @Override
    public void sendSMSExpectMore(String smscPDU, String pdu, Message result) {

	// idk how to search for device modem lol
        Rlog.v(RILJ_LOG_TAG, "grandpplte-ril-modem: sendSMSExpectMore");

        RILRequest rr = RILRequest.obtain(RIL_REQUEST_SEND_SMS, result);
        constructGsmSendSmsRilRequest(rr, smscPDU, pdu);

        if (RILJ_LOGD) riljLog(rr.serialString() + "> " + requestToString(rr.mRequest));

        send(rr);
    }

    // TODO: strip down certain network stuffs~
    private Object responseOperatorInfos(Parcel p) {
        ArrayList<OperatorInfo> ret;
        String[] strings = (String[]) responseStrings(p);
        if (strings.length % 6 != 0) {
            throw new RuntimeException("RIL_REQUEST_QUERY_AVAILABLE_NETWORKS: invalid response. Got " + strings.length + " strings, expected multible of 6");
        }
	
	// might not be necessary        
	//String isRoaming = TelephonyManager.getTelephonyProperty(this.mInstanceId.intValue(), "gsm.operator.isroaming", "");
        
	// removed some kind of CSC things~

	// use SlteRIL coding, same work but much more elegant
	// mQANElements in grandpplte is 6
        ret = new ArrayList<OperatorInfo>(strings.length / 6);
        for (int i = 0 ; i < strings.length ; i += 6) {
            String strOperatorLong = strings[i+0];
            String strOperatorNumeric = strings[i+2];
            String strState = strings[i+3].toLowerCase();

            Rlog.v(RILJ_LOG_TAG,
                   "grandpplte-ril-modem: Add OperatorInfo: " + strOperatorLong +
                   ", " + strOperatorLong +
                   ", " + strOperatorNumeric +
                   ", " + strState);

            ret.add(new OperatorInfo(strOperatorLong, // operatorAlphaLong
                                     strOperatorLong, // operatorAlphaShort
                                     strOperatorNumeric,    // operatorNumeric
                                     strState));  // stateString
        }
        
        return ret;
    }

    // uses SlteRIL because stock one is waaayyy too lengthy
    // tons of bugs incoming (.......)
        @Override
    protected void processUnsolicited(Parcel p, int type) {
        Object ret;

        int dataPosition = p.dataPosition();
        int origResponse = p.readInt();
        int newResponse = origResponse;

        /* Remap incorrect respones or ignore them */
        switch (origResponse) {
            case RIL_UNSOL_STK_CALL_CONTROL_RESULT:
            case RIL_UNSOL_WB_AMR_STATE:
            case RIL_UNSOL_DEVICE_READY_NOTI: /* Registrant notification */
            case RIL_UNSOL_SIM_PB_READY: /* Registrant notification */
                Rlog.v(RILJ_LOG_TAG,
                       "grandpplte-ril-modem: ignoring unsolicited response " +
                       origResponse);
                return;
        }

        if (newResponse != origResponse) {
            riljLog("grandpplteRIL: remap unsolicited response from " +
                    origResponse + " to " + newResponse);
            p.setDataPosition(dataPosition);
            p.writeInt(newResponse);
        }

        switch (newResponse) {
            case RIL_UNSOL_AM:
                ret = responseString(p);
                break;
            case RIL_UNSOL_STK_SEND_SMS_RESULT:
                ret = responseInts(p);
                break;
            default:
                // Rewind the Parcel
                p.setDataPosition(dataPosition);

                // Forward responses that we are not overriding to the super class
                super.processUnsolicited(p, type);
                return;
        }

        switch (newResponse) {
            case RIL_UNSOL_AM:
                String strAm = (String)ret;
                // Add debug to check if this wants to execute any useful am command
                Rlog.v(RILJ_LOG_TAG, "grandpplte-ril-modem: am=" + strAm);
                break;
        }
    }
}

}
