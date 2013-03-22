package com.android.internal.telephony;

import java.util.ArrayList;
import java.util.Collections;
import java.lang.Runtime;
import java.io.IOException;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.ConnectivityManager;
import android.os.Handler;
import android.os.Message;
import android.os.AsyncResult;
import android.os.Parcel;
import android.os.SystemProperties;
import android.telephony.PhoneNumberUtils;
import android.telephony.SignalStrength;
import static com.android.internal.telephony.RILConstants.*;

import com.android.internal.telephony.CommandException;
import com.android.internal.telephony.DataCallState;
import com.android.internal.telephony.DataConnection.FailCause;
import com.android.internal.telephony.gsm.SmsBroadcastConfigInfo;
import com.android.internal.telephony.gsm.SuppServiceNotification;
import com.android.internal.telephony.RILConstants;
import com.android.internal.telephony.cdma.CdmaCallWaitingNotification;
import com.android.internal.telephony.cdma.CdmaInformationRecords;
import com.android.internal.telephony.cdma.CdmaInformationRecords.CdmaSignalInfoRec;
import com.android.internal.telephony.cdma.SignalToneUtil;

import android.util.Log;

public class SamsungMSMRIL extends RIL implements CommandsInterface {

    private boolean mSignalbarCount = SystemProperties.getInt("ro.telephony.sends_barcount", 0) == 1 ? true : false;
    private boolean mIsSamsungCdma = SystemProperties.getBoolean("ro.ril.samsung_cdma", false);

    public SamsungMSMRIL(Context context, int networkMode, int cdmaSubscription) {
        super(context, networkMode, cdmaSubscription);
    }

    @Override
    protected Object
    responseSignalStrength(Parcel p) {
        int numInts = 12;
        int response[];

        /* TODO: Add SignalStrength class to match RIL_SignalStrength */
        response = new int[numInts];
        for (int i = 0 ; i < 7 ; i++) {
            response[i] = p.readInt();
        }
        // CooperRIL is a v3 RIL, fill the rest with -1
        for (int i = 7; i < numInts; i++) {
            response[i] = -1;
        }

        if (mIsSamsungCdma)
            // Framework takes care of the rest for us.
            return response;

        /* Matching Samsung signal strength to asu.
		   Method taken from Samsungs cdma/gsmSignalStateTracker */
        if(mSignalbarCount)
        {
            //Samsung sends the count of bars that should be displayed instead of
            //a real signal strength
            response[0] = ((response[0] & 0xFF00) >> 8) * 3; //gsmDbm
        } else {
            response[0] = response[0] & 0xFF; //gsmDbm
        }
        response[1] = -1; //gsmEcio
        response[2] = (response[2] < 0)?-120:-response[2]; //cdmaDbm
        response[3] = (response[3] < 0)?-160:-response[3]; //cdmaEcio
        response[4] = (response[4] < 0)?-120:-response[4]; //evdoRssi
        response[5] = (response[5] < 0)?-1:-response[5]; //evdoEcio
        if(response[6] < 0 || response[6] > 8)
            response[6] = -1;

	SignalStrength signalStrength = new SignalStrength(
	            response[0], response[1], response[2], response[3], response[4],
	            response[5], response[6], !mIsSamsungCdma);
        return signalStrength;
    }
}
