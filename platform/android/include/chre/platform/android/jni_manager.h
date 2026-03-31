/*
 * Copyright (C) 2025 The Android Open Source Project
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

#ifndef CHRE_PLATFORM_ANDROID_JNI_MANAGER_H_
#define CHRE_PLATFORM_ANDROID_JNI_MANAGER_H_

#include "chre/util/singleton.h"
#include "chre_api/chre/wwan.h"

#include "jni.h"

#include <cstdint>

namespace chre {

/**
 * A class that keeps track of the JNI related objects and methods.
 */
class JniManager {
 public:
  using HandleCellInfoCallbackFn = void (*)(JNIEnv *, jobjectArray);

  JniManager() = default;
  ~JniManager() = default;

  void init(JavaVM *vm);

  void registerHandleCellInfoCallbackFn(HandleCellInfoCallbackFn fn);

  void onCellInfoReceived(JNIEnv *env, jobjectArray cellInfoList);

  JavaVM *mJavaVm = nullptr;

  JNIEnv *getOrAttachJniEnv();

  struct WwanJniCache {
    // ContextHubAPNative class
    jclass contextHubNativeClass = nullptr;
    jmethodID getCapabilitiesMethod = nullptr;
    jmethodID requestCellInfoMethod = nullptr;

    // Classes
    jclass cellInfoLteClass = nullptr;
    jclass cellInfoGsmClass = nullptr;
    jclass cellInfoWcdmaClass = nullptr;
    jclass cellInfoTdscdmaClass = nullptr;
    jclass cellInfoNrClass = nullptr;

    // Base CellInfo methods
    jmethodID getTimeStamp = nullptr;
    jmethodID isRegistered = nullptr;

    // --- LTE Methods ---
    jmethodID lteGetIdentity = nullptr;
    jmethodID lteGetSignal = nullptr;
    // CellIdentityLte
    jmethodID lteIdGetMcc = nullptr;
    jmethodID lteIdGetMnc = nullptr;
    jmethodID lteIdGetCi = nullptr;
    jmethodID lteIdGetPci = nullptr;
    jmethodID lteIdGetTac = nullptr;
    // CellSignalStrengthLte
    jmethodID lteSigGetDbm = nullptr;
    jmethodID lteSigGetRsrp = nullptr;
    jmethodID lteSigGetRsrq = nullptr;
    jmethodID lteSigGetRssnr = nullptr;
    jmethodID lteSigGetTa = nullptr;

    // --- GSM Methods ---
    jmethodID gsmGetIdentity = nullptr;
    jmethodID gsmGetSignal = nullptr;
    // CellIdentityGsm
    jmethodID gsmIdGetMcc = nullptr;
    jmethodID gsmIdGetMnc = nullptr;
    jmethodID gsmIdGetLac = nullptr;
    jmethodID gsmIdGetCid = nullptr;
    jmethodID gsmIdGetArfcn = nullptr;
    jmethodID gsmIdGetBsic = nullptr;
    // CellSignalStrengthGsm
    jmethodID gsmSigGetDbm = nullptr;
    jmethodID gsmSigGetBitErrorRate = nullptr;

    // --- WCDMA Methods ---
    jmethodID wcdmaGetIdentity = nullptr;
    jmethodID wcdmaGetSignal = nullptr;
    // CellIdentityWcdma
    jmethodID wcdmaIdGetMcc = nullptr;
    jmethodID wcdmaIdGetMnc = nullptr;
    jmethodID wcdmaIdGetLac = nullptr;
    jmethodID wcdmaIdGetCid = nullptr;
    jmethodID wcdmaIdGetPsc = nullptr;
    jmethodID wcdmaIdGetUarfcn = nullptr;
    // CellSignalStrengthWcdma
    jmethodID wcdmaSigGetDbm = nullptr;

    // --- NR (5G) Methods ---
    jmethodID nrGetIdentity = nullptr;
    jmethodID nrGetSignal = nullptr;
    // CellIdentityNr
    jmethodID nrIdGetMcc = nullptr;
    jmethodID nrIdGetMnc = nullptr;
    jmethodID nrIdGetNci = nullptr;
    jmethodID nrIdGetPci = nullptr;
    jmethodID nrIdGetTac = nullptr;
    jmethodID nrIdGetNrarfcn = nullptr;
    // CellSignalStrengthNr
    jmethodID nrSigGetDbm = nullptr;
    jmethodID nrSigGetCsiRsrp = nullptr;
    jmethodID nrSigGetCsiRsrq = nullptr;
    jmethodID nrSigGetCsiSinr = nullptr;
    jmethodID nrSigGetSsRsrp = nullptr;
    jmethodID nrSigGetSsRsrq = nullptr;
    jmethodID nrSigGetSsSinr = nullptr;

    // --- CDMA Methods ---
    jclass cellInfoCdmaClass = nullptr;
    jmethodID cdmaGetIdentity = nullptr;
    jmethodID cdmaGetSignal = nullptr;
    // CellIdentityCdma
    jmethodID cdmaIdGetNetworkId = nullptr;
    jmethodID cdmaIdGetSystemId = nullptr;
    jmethodID cdmaIdGetBasestationId = nullptr;
    jmethodID cdmaIdGetLongitude = nullptr;
    jmethodID cdmaIdGetLatitude = nullptr;
    // CellSignalStrengthCdma
    jmethodID cdmaSigGetCdmaDbm = nullptr;
    jmethodID cdmaSigGetCdmaEcio = nullptr;

    // --- TD-SCDMA Methods ---
    jmethodID tdscdmaGetIdentity = nullptr;
    jmethodID tdscdmaGetSignal = nullptr;
    // CellIdentityTdscdma
    jmethodID tdscdmaIdGetMcc = nullptr;
    jmethodID tdscdmaIdGetMnc = nullptr;
    jmethodID tdscdmaIdGetLac = nullptr;
    jmethodID tdscdmaIdGetCid = nullptr;
    jmethodID tdscdmaIdGetCpid = nullptr;
    jmethodID tdscdmaSigGetDbm = nullptr;
    // CellSignalStrengthTdscdma
    jmethodID tdscdmaSigGetRscp = nullptr;
  };
  WwanJniCache mWwanJniCache;

  HandleCellInfoCallbackFn mHandleCellInfoCallbackFn;
};

//! Provide an alias to the JniManager singleton.
typedef Singleton<JniManager> JniManagerSingleton;

}  // namespace chre

#endif  // CHRE_PLATFORM_ANDROID_JNI_MANAGER_H_