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
import android.os.AsyncResult;
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
import java.io.IOException;


/**
 * RIL customization for Galaxy J2 Prime/Grand Prime Plus (GSM)
 *
 * {@hide}
 */

public class grandpplteRIL extends RIL implements CommandsInterface {

    /**********************************************************
     * SAMSUNG REQUESTS
     **********************************************************/
    static final boolean RILJ_LOGD = true;
    static final boolean RILJ_LOGV = true;
    
    /*request*/
    private static final int RIL_REQUEST_DIAL_EMERGENCY_CALL = 10001;
    private static final int GET_CELL_BROADCAST_CONFIG = 10008;
    /*response*/
    //private static final int SAMSUNG_UNSOL_RESPONSE_BASE = 11000; // alz
    
    private static final int RIL_UNSOL_STK_SEND_SMS_RESULT = 11002;
    private static final int RIL_UNSOL_STK_CALL_CONTROL_RESULT = 11003;
    private static final int RIL_UNSOL_DEVICE_READY_NOTI = 11008;
    private static final int RIL_UNSOL_AM = 11010;
    private static final int RIL_UNSOL_GPS_NOTI = 11009;
    private static final int RIL_UNSOL_SIM_PB_READY = 11021;
    private static final int RIL_UNSOL_PB_INIT_COMPLETE = 11035;
    private static final int RIL_UNSOL_SIM_ICCID_NOTI = 11066;

    public grandpplteRIL(Context context, int preferredNetworkType, int cdmaSubscription) {
        this(context, preferredNetworkType, cdmaSubscription, null);
    }

    public grandpplteRIL(Context context, int preferredNetworkType, int cdmaSubscription, Integer instanceId) {
        super(context, preferredNetworkType, cdmaSubscription, instanceId);
    }

    @Override
    public void setDataAllowed(boolean allowed, Message result) {
    // from stock

    	int i = 1;
        RILRequest rr = RILRequest.obtain(RIL_REQUEST_ALLOW_DATA, result);
        if (RILJ_LOGD) {
            riljLog(rr.serialString() + "> " + requestToString(rr.mRequest) +
                    " allowed: " + allowed);
        }
        rr.mParcel.writeInt(1);
        Parcel parcel = rr.mParcel;
        
        // disable 
        // if (!allowed) {
        //    i = 0;
        // }
        parcel.writeInt(i);
        send(rr);
    }

    @Override
    public void writeSmsToSim(int status, String smsc, String pdu, Message response) {
        status = translateStatus(status);

        RILRequest rr = RILRequest.obtain(RIL_REQUEST_WRITE_SMS_TO_SIM,
                response);

        rr.mParcel.writeInt(status);
        rr.mParcel.writeString(pdu);
        rr.mParcel.writeString(smsc);

        if (RILJ_LOGV) {
            riljLog(rr.serialString() + "> "
                    + requestToString(rr.mRequest)
                    + " " + status);
        }

        send(rr);
    }

    public void
    acceptCall(int index, Message result) {
        RILRequest rr =
            RILRequest.obtain(RIL_REQUEST_ANSWER, result);

        if (RILJ_LOGD) {
            riljLog(rr.serialString() + "> " + requestToString(rr.mRequest));
        }
        rr.mParcel.writeInt(1);
        rr.mParcel.writeInt(index);

        send(rr);
    }

    @Override
    public void
    acceptCall(Message result) {
        acceptCall(0, result);
    }

    @Override
    public void dial(String address, int clirMode, Message result) {
        dial(address, clirMode, null, result);
    }
    
    @Override
    public void dial(String address, int clirMode, UUSInfo uusInfo, Message result) {
	
	// inherit from SlteRIL
        if (PhoneNumberUtils.isEmergencyNumber(address)) {
            dialEmergencyCall(address, clirMode, result);
            return;
        } // -

        RILRequest rr = RILRequest.obtain(RIL_REQUEST_DIAL, result);
        rr.mParcel.writeString(address);
        rr.mParcel.writeInt(clirMode);
        rr.mParcel.writeInt(0);     // CallDetails.call_type
        rr.mParcel.writeInt(1);     // CallDetails.call_domain
        rr.mParcel.writeString(""); // CallDetails.getCsvFromExtras

        if (uusInfo == null) {
            rr.mParcel.writeInt(0);
        } else {
            rr.mParcel.writeInt(1);
            rr.mParcel.writeInt(uusInfo.getType());
            rr.mParcel.writeInt(uusInfo.getDcs());
            rr.mParcel.writeByteArray(uusInfo.getUserData());
        }
        riljLog(rr.serialString() + "> " + requestToString(rr.mRequest) );
        send(rr);
    }

    public void dialEmergencyCall(String address, int clirMode, Message result) {
        RILRequest rr = RILRequest.obtain(RIL_REQUEST_DIAL_EMERGENCY_CALL, result);
        rr.mParcel.writeString(address);
        rr.mParcel.writeInt(clirMode);
        rr.mParcel.writeInt(0);        // CallDetails.call_type
        rr.mParcel.writeInt(3);        // CallDetails.call_domain
        rr.mParcel.writeString("");    // CallDetails.getCsvFromExtra
        rr.mParcel.writeInt(0);        // Unknown
        
        if (RILJ_LOGD) riljLog(rr.serialString() + "> " + requestToString(rr.mRequest));
        send(rr);
    }

    @Override
    protected Object responseIccCardStatus(Parcel p) {
        IccCardStatus cardStatus = new IccCardStatus();
        cardStatus.setCardState(p.readInt());
        cardStatus.setUniversalPinState(p.readInt());
        cardStatus.mGsmUmtsSubscriptionAppIndex = p.readInt();
        cardStatus.mCdmaSubscriptionAppIndex = p.readInt();
        cardStatus.mImsSubscriptionAppIndex = p.readInt();
        int numApplications = p.readInt();
        
        if (numApplications > 8) {
            numApplications = 8;
        }
        
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

            /* compilation error */
            p.readInt(); // pin1_num_retries
            p.readInt(); // puk1_num_retries
            p.readInt(); // pin2_num_retries
            p.readInt(); // puk2_num_retries
            p.readInt(); // perso_unblock_retries
            cardStatus.mApplications[i] = appStatus;
        }
        return cardStatus;
    }
    
    /* BAD SIGNAL STRENGTH, DISABLE FOR NOW. STOCK USES AOSP IMPL ANYWAYS
    @Override
    protected Object
    responseSignalStrength(Parcel p) {
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
    END DISABLE responseSignalStrength */

    @Override
    public Object responseCallList(Parcel p) {
        boolean z;
        int num = p.readInt();
        ArrayList<DriverCall> response = new ArrayList<>(num);
        for (int i = 0; i < num; i++) {
            DriverCall dc = new DriverCall();
            dc.state = DriverCall.stateFromCLCC(p.readInt());
            dc.index = p.readInt();
            // almond: line is only for logging + compilation error // dc.id = (dc.index >> 8) & 255;
            dc.index &= 255;
            dc.TOA = p.readInt();
            dc.isMpty = p.readInt() != 0;
            dc.isMT = p.readInt() != 0;
            dc.als = p.readInt();
            if (p.readInt() == 0) {
                z = false;
            } else {
                z = true;
            }
            dc.isVoice = z;
            int type = p.readInt();
            int domain = p.readInt();
            String extras = p.readString();
           /* almond: remove all references of call details 
            *
            *dc.callDetails = new CallDetails(type, domain, null);
            *dc.callDetails.setExtrasFromCsv(extras);
            *Rlog.d(RILJ_LOG_TAG, "dc.index " + dc.index + " dc.id " + dc.id + " dc.callDetails " + dc.callDetails);
            */ 
            dc.isVoicePrivacy = p.readInt() != 0;
            dc.number = p.readString();
            dc.numberPresentation = DriverCall.presentationFromCLIP(p.readInt());
            dc.name = p.readString();
            Rlog.d(RILJ_LOG_TAG, "responseCallList dc.name" + dc.name);
            dc.namePresentation = DriverCall.presentationFromCLIP(p.readInt());
            if (p.readInt() == 1) {
                dc.uusInfo = new UUSInfo();
                dc.uusInfo.setType(p.readInt());
                dc.uusInfo.setDcs(p.readInt());
                dc.uusInfo.setUserData(p.createByteArray());
                
                riljLogv(String.format("Incoming UUS : type=%d, dcs=%d, length=%d", new Object[]{Integer.valueOf(dc.uusInfo.getType()), Integer.valueOf(dc.uusInfo.getDcs()), Integer.valueOf(dc.uusInfo.getUserData().length)}));
                riljLogv("Incoming UUS : data (string)=" + new String(dc.uusInfo.getUserData()));
                riljLogv("Incoming UUS : data (hex): " + IccUtils.bytesToHexString(dc.uusInfo.getUserData()));
            } else {
                riljLogv("Incoming UUS : NOT present!");
            }
            dc.number = PhoneNumberUtils.stringFromStringAndTOA(dc.number, dc.TOA);
            response.add(dc);
            if (dc.isVoicePrivacy) {
                this.mVoicePrivacyOnRegistrants.notifyRegistrants();
                riljLog("InCall VoicePrivacy is enabled");
            } else {
                this.mVoicePrivacyOffRegistrants.notifyRegistrants();
                riljLog("InCall VoicePrivacy is disabled");
            }
        }
        Collections.sort(response);
        if (num == 0 && this.mTestingEmergencyCall.getAndSet(false) && this.mEmergencyCallbackModeRegistrant != null) {
            riljLog("responseCallList: call ended, testing emergency call, notify ECM Registrants");
            this.mEmergencyCallbackModeRegistrant.notifyRegistrant();
        }
        return response;
    }
    /* this phone is GSM only */
    private void constructGsmSendSmsRilRequest(RILRequest rr, String smscPDU, String pdu) {
        rr.mParcel.writeInt(2);
        rr.mParcel.writeString(smscPDU);
        rr.mParcel.writeString(pdu);
    }

    /**
     *  Translates EF_SMS status bits to a status value compatible with
     *  SMS AT commands.  See TS 27.005 3.1.
     */
    private int translateStatus(int status) {
        switch(status & 0x7) {
            case SmsManager.STATUS_ON_ICC_READ:
                return 1;
            case SmsManager.STATUS_ON_ICC_UNREAD:
                return 0;
            case SmsManager.STATUS_ON_ICC_SENT:
                return 3;
            case SmsManager.STATUS_ON_ICC_UNSENT:
                return 2;
        }

        // Default to READ.
        return 1;
    }

    // according to SlteRIL
    /**
     * The RIL can't handle the RIL_REQUEST_SEND_SMS_EXPECT_MORE
     * request properly, so we use RIL_REQUEST_SEND_SMS instead.
     */
    @Override
    public void sendSMSExpectMore(String smscPDU, String pdu, Message result) {
        Rlog.v(RILJ_LOG_TAG, "MT6737T: sendSMSExpectMore");
        RILRequest rr = RILRequest.obtain(RIL_REQUEST_SEND_SMS, result);
        constructGsmSendSmsRilRequest(rr, smscPDU, pdu);
        if (RILJ_LOGD) riljLog(rr.serialString() + "> " + requestToString(rr.mRequest));
        send(rr);
    }

    
    @Override
    protected Object responseOperatorInfos(Parcel p) {
        ArrayList<OperatorInfo> ret;
        String[] strings = (String[])responseStrings(p);
        if (strings.length % 6 != 0) {
            throw new RuntimeException("RIL_REQUEST_QUERY_AVAILABLE_NETWORKS: invalid response. Got " + strings.length + " strings, expected multiple of 6");
        }
	
	/* might not be necessary        
	String isRoaming = TelephonyManager.getTelephonyProperty(this.mInstanceId.intValue(), "gsm.operator.isroaming", "");
        */
	/* removed some kind of CSC things~
	// use SlteRIL coding, same work but much more elegant
	// mQANElements in grandpplte is 6
	*/        
	ret = new ArrayList<OperatorInfo>(strings.length / 6);
        for (int i = 0 ; i < strings.length ; i += 6) {
            String strOperatorLong = strings[i+0];
            String strOperatorNumeric = strings[i+2];
            String strState = strings[i+3].toLowerCase();

            Rlog.v(RILJ_LOG_TAG,
                   "MT6737T: Add OperatorInfo: " + strOperatorLong +
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

    // uses SlteRIL because stock one is waaayyy too lengthy and unusable
    // tons of bugs incoming (.......)
    @Override
    protected void processUnsolicited(Parcel p, int type) {
        Object ret;

        int dataPosition = p.dataPosition();
        int origResponse = p.readInt();
        int newResponse = origResponse;

        // Remap incorrect respones or ignore them
        switch (origResponse) {
            case RIL_UNSOL_STK_CALL_CONTROL_RESULT:
            case RIL_UNSOL_SIM_PB_READY: // Registrant notification 
            case RIL_UNSOL_GPS_NOTI:
            case RIL_UNSOL_DEVICE_READY_NOTI:
	    case RIL_UNSOL_SIM_ICCID_NOTI:
        	Rlog.v(RILJ_LOG_TAG,
                       "MT6737T: ignoring unsolicited response " +
                       origResponse);
                return;
        }

        if (newResponse != origResponse) {
            riljLog("MT6737T: grandpplteRIL: remap unsolicited response from " +
                    origResponse + " to " + newResponse);
            p.setDataPosition(dataPosition);
            p.writeInt(newResponse);
        }

        switch (newResponse) {
            case RIL_UNSOL_DEVICE_READY_NOTI:
                ret = responseVoid(p);
                break;
            case RIL_UNSOL_AM:
                ret = responseString(p);
                break;
            case RIL_UNSOL_STK_SEND_SMS_RESULT:
                ret = responseInts(p);
                break;
            case RIL_UNSOL_NITZ_TIME_RECEIVED:
                fixNitz(p);
                p.setDataPosition(dataPosition);
                super.processUnsolicited(p, type);
                return;
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
                Rlog.v(RILJ_LOG_TAG, "MT6737T: am=" + strAm);
                break;
	    case RIL_UNSOL_PB_INIT_COMPLETE:
		ret = responseVoid(p);
		break;
        }
    }

    private void
    fixNitz(Parcel p) {
        int dataPosition = p.dataPosition();
        String nitz = p.readString();
        long nitzReceiveTime = p.readLong();

        String[] nitzParts = nitz.split(",");
        if (nitzParts.length >= 4) {
            // 0=date, 1=time+zone, 2=dst, 3(+)=garbage that confuses ServiceStateTracker
            nitz = nitzParts[0] + "," + nitzParts[1] + "," + nitzParts[2];
            p.setDataPosition(dataPosition);
            p.writeString(nitz);
            p.writeLong(nitzReceiveTime);
            // The string is shorter now, drop the extra bytes
            p.setDataSize(p.dataPosition());
        }
    }
}
