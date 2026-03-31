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

#include "chre/platform/platform_wwan.h"

#include <cstdint>
#include "chre/core/event_loop_manager.h"
#include "chre/core/wwan_request_manager.h"
#include "chre/pal/wwan.h"
#include "chre/platform/android/jni_manager.h"
#include "chre/platform/log.h"
#include "chre_api/chre/wwan.h"
#include "jni.h"

/**
 * An implementation of the WWAN for the CHRE AP platform.
 */
namespace chre {
namespace {

JNIEnv *getEnv() {
  return JniManagerSingleton::get()->getOrAttachJniEnv();
}

JniManager::WwanJniCache &getWwanJniCache() {
  return JniManagerSingleton::get()->mWwanJniCache;
}

// Helper to convert Java String MCC/MNC (used in NR) to int
static int32_t parseStringToInt(JNIEnv *env, jstring str) {
  if (!str) return INT32_MAX;

  const char *chars = env->GetStringUTFChars(str, nullptr);
  int32_t val = INT32_MAX;

  if (chars) {
    char *endPtr;
    long parsedVal = strtol(chars, &endPtr, 10);

    if (endPtr != chars && parsedVal >= 0 && parsedVal <= INT32_MAX) {
      val = static_cast<int32_t>(parsedVal);
    }

    env->ReleaseStringUTFChars(str, chars);
  }
  return val;
}

// Common Fields Populator
static void populateCommonFields(JNIEnv *env, jobject cellInfoObj,
                                 chreWwanCellInfo *outInfo) {
  if (getWwanJniCache().getTimeStamp != nullptr) {
    outInfo->timeStamp =
        env->CallLongMethod(cellInfoObj, getWwanJniCache().getTimeStamp);
  } else {
    outInfo->timeStamp = 0;
  }
  outInfo->timeStampType = CHRE_WWAN_CELL_TIMESTAMP_TYPE_MODEM;

  if (getWwanJniCache().isRegistered != nullptr) {
    jboolean registered =
        env->CallBooleanMethod(cellInfoObj, getWwanJniCache().isRegistered);
    outInfo->registered = (registered == JNI_TRUE) ? 1 : 0;
  } else {
    outInfo->registered = 0;
  }

  outInfo->reserved = 0;
}

// GSM Parser
static void parseGsmInfo(JNIEnv *env, jobject cellInfo,
                         chreWwanCellInfo *outInfo) {
  outInfo->cellInfoType = CHRE_WWAN_CELL_INFO_TYPE_GSM;

  jobject id =
      (getWwanJniCache().gsmGetIdentity != nullptr)
          ? env->CallObjectMethod(cellInfo, getWwanJniCache().gsmGetIdentity)
          : nullptr;
  if (id) {
    outInfo->CellInfo.gsm.cellIdentityGsm.mcc =
        (getWwanJniCache().gsmIdGetMcc != nullptr)
            ? env->CallIntMethod(id, getWwanJniCache().gsmIdGetMcc)
            : INT32_MAX;
    outInfo->CellInfo.gsm.cellIdentityGsm.mnc =
        (getWwanJniCache().gsmIdGetMnc != nullptr)
            ? env->CallIntMethod(id, getWwanJniCache().gsmIdGetMnc)
            : INT32_MAX;
    outInfo->CellInfo.gsm.cellIdentityGsm.lac =
        (getWwanJniCache().gsmIdGetLac != nullptr)
            ? env->CallIntMethod(id, getWwanJniCache().gsmIdGetLac)
            : INT32_MAX;
    outInfo->CellInfo.gsm.cellIdentityGsm.cid =
        (getWwanJniCache().gsmIdGetCid != nullptr)
            ? env->CallIntMethod(id, getWwanJniCache().gsmIdGetCid)
            : INT32_MAX;
    outInfo->CellInfo.gsm.cellIdentityGsm.arfcn =
        (getWwanJniCache().gsmIdGetArfcn != nullptr)
            ? env->CallIntMethod(id, getWwanJniCache().gsmIdGetArfcn)
            : INT32_MAX;
    outInfo->CellInfo.gsm.cellIdentityGsm.bsic =
        (getWwanJniCache().gsmIdGetBsic != nullptr)
            ? env->CallIntMethod(id, getWwanJniCache().gsmIdGetBsic)
            : UINT8_MAX;
    env->DeleteLocalRef(id);
  } else {
    // Fill with invalid/defaults if identity is missing
    outInfo->CellInfo.gsm.cellIdentityGsm.mcc = INT32_MAX;
    outInfo->CellInfo.gsm.cellIdentityGsm.mnc = INT32_MAX;
    outInfo->CellInfo.gsm.cellIdentityGsm.lac = INT32_MAX;
    outInfo->CellInfo.gsm.cellIdentityGsm.cid = INT32_MAX;
    outInfo->CellInfo.gsm.cellIdentityGsm.arfcn = INT32_MAX;
    outInfo->CellInfo.gsm.cellIdentityGsm.bsic = UINT8_MAX;
  }

  jobject sig =
      (getWwanJniCache().gsmGetSignal != nullptr)
          ? env->CallObjectMethod(cellInfo, getWwanJniCache().gsmGetSignal)
          : nullptr;
  if (sig) {
    outInfo->CellInfo.gsm.signalStrengthGsm.signalStrength =
        (getWwanJniCache().gsmSigGetDbm != nullptr)
            ? env->CallIntMethod(sig, getWwanJniCache().gsmSigGetDbm)
            : INT32_MAX;
    outInfo->CellInfo.gsm.signalStrengthGsm.bitErrorRate =
        (getWwanJniCache().gsmSigGetBitErrorRate != nullptr)
            ? env->CallIntMethod(sig, getWwanJniCache().gsmSigGetBitErrorRate)
            : INT32_MAX;
    // GSM Timing Advance often not available in standard API
    outInfo->CellInfo.gsm.signalStrengthGsm.timingAdvance = INT32_MAX;
    env->DeleteLocalRef(sig);
  } else {
    outInfo->CellInfo.gsm.signalStrengthGsm.signalStrength = INT32_MAX;
    outInfo->CellInfo.gsm.signalStrengthGsm.bitErrorRate = INT32_MAX;
    outInfo->CellInfo.gsm.signalStrengthGsm.timingAdvance = INT32_MAX;
  }
}

// LTE Parser
static void parseLteInfo(JNIEnv *env, jobject cellInfo,
                         chreWwanCellInfo *outInfo) {
  outInfo->cellInfoType = CHRE_WWAN_CELL_INFO_TYPE_LTE;

  jobject id =
      (getWwanJniCache().lteGetIdentity != nullptr)
          ? env->CallObjectMethod(cellInfo, getWwanJniCache().lteGetIdentity)
          : nullptr;
  if (id) {
    outInfo->CellInfo.lte.cellIdentityLte.mcc =
        (getWwanJniCache().lteIdGetMcc != nullptr)
            ? env->CallIntMethod(id, getWwanJniCache().lteIdGetMcc)
            : INT32_MAX;
    outInfo->CellInfo.lte.cellIdentityLte.mnc =
        (getWwanJniCache().lteIdGetMnc != nullptr)
            ? env->CallIntMethod(id, getWwanJniCache().lteIdGetMnc)
            : INT32_MAX;
    outInfo->CellInfo.lte.cellIdentityLte.ci =
        (getWwanJniCache().lteIdGetCi != nullptr)
            ? env->CallIntMethod(id, getWwanJniCache().lteIdGetCi)
            : INT32_MAX;
    outInfo->CellInfo.lte.cellIdentityLte.pci =
        (getWwanJniCache().lteIdGetPci != nullptr)
            ? env->CallIntMethod(id, getWwanJniCache().lteIdGetPci)
            : INT32_MAX;
    outInfo->CellInfo.lte.cellIdentityLte.tac =
        (getWwanJniCache().lteIdGetTac != nullptr)
            ? env->CallIntMethod(id, getWwanJniCache().lteIdGetTac)
            : INT32_MAX;
    outInfo->CellInfo.lte.cellIdentityLte.earfcn = INT32_MAX;
    env->DeleteLocalRef(id);
  } else {
    outInfo->CellInfo.lte.cellIdentityLte.mcc = INT32_MAX;
    outInfo->CellInfo.lte.cellIdentityLte.mnc = INT32_MAX;
    outInfo->CellInfo.lte.cellIdentityLte.ci = INT32_MAX;
    outInfo->CellInfo.lte.cellIdentityLte.pci = INT32_MAX;
    outInfo->CellInfo.lte.cellIdentityLte.tac = INT32_MAX;
    outInfo->CellInfo.lte.cellIdentityLte.earfcn = INT32_MAX;
  }

  jobject sig =
      (getWwanJniCache().lteGetSignal != nullptr)
          ? env->CallObjectMethod(cellInfo, getWwanJniCache().lteGetSignal)
          : nullptr;
  if (sig) {
    outInfo->CellInfo.lte.signalStrengthLte.signalStrength =
        (getWwanJniCache().lteSigGetDbm != nullptr)
            ? env->CallIntMethod(sig, getWwanJniCache().lteSigGetDbm)
            : INT32_MAX;
    outInfo->CellInfo.lte.signalStrengthLte.rsrp =
        (getWwanJniCache().lteSigGetRsrp != nullptr)
            ? env->CallIntMethod(sig, getWwanJniCache().lteSigGetRsrp)
            : INT32_MAX;
    outInfo->CellInfo.lte.signalStrengthLte.rsrq =
        (getWwanJniCache().lteSigGetRsrq != nullptr)
            ? env->CallIntMethod(sig, getWwanJniCache().lteSigGetRsrq)
            : INT32_MAX;
    outInfo->CellInfo.lte.signalStrengthLte.rssnr =
        (getWwanJniCache().lteSigGetRssnr != nullptr)
            ? env->CallIntMethod(sig, getWwanJniCache().lteSigGetRssnr)
            : INT32_MAX;
    outInfo->CellInfo.lte.signalStrengthLte.timingAdvance =
        (getWwanJniCache().lteSigGetTa != nullptr)
            ? env->CallIntMethod(sig, getWwanJniCache().lteSigGetTa)
            : INT32_MAX;
    outInfo->CellInfo.lte.signalStrengthLte.cqi =
        INT32_MAX;  // Typically not available in cache
    env->DeleteLocalRef(sig);
  } else {
    outInfo->CellInfo.lte.signalStrengthLte.signalStrength = INT32_MAX;
    outInfo->CellInfo.lte.signalStrengthLte.rsrp = INT32_MAX;
    outInfo->CellInfo.lte.signalStrengthLte.rsrq = INT32_MAX;
    outInfo->CellInfo.lte.signalStrengthLte.rssnr = INT32_MAX;
    outInfo->CellInfo.lte.signalStrengthLte.timingAdvance = INT32_MAX;
    outInfo->CellInfo.lte.signalStrengthLte.cqi = INT32_MAX;
  }
}

// WCDMA Parser
static void parseWcdmaInfo(JNIEnv *env, jobject cellInfo,
                           chreWwanCellInfo *outInfo) {
  outInfo->cellInfoType = CHRE_WWAN_CELL_INFO_TYPE_WCDMA;

  jobject id =
      (getWwanJniCache().wcdmaGetIdentity != nullptr)
          ? env->CallObjectMethod(cellInfo, getWwanJniCache().wcdmaGetIdentity)
          : nullptr;
  if (id) {
    outInfo->CellInfo.wcdma.cellIdentityWcdma.mcc =
        (getWwanJniCache().wcdmaIdGetMcc != nullptr)
            ? env->CallIntMethod(id, getWwanJniCache().wcdmaIdGetMcc)
            : INT32_MAX;
    outInfo->CellInfo.wcdma.cellIdentityWcdma.mnc =
        (getWwanJniCache().wcdmaIdGetMnc != nullptr)
            ? env->CallIntMethod(id, getWwanJniCache().wcdmaIdGetMnc)
            : INT32_MAX;
    outInfo->CellInfo.wcdma.cellIdentityWcdma.lac =
        (getWwanJniCache().wcdmaIdGetLac != nullptr)
            ? env->CallIntMethod(id, getWwanJniCache().wcdmaIdGetLac)
            : INT32_MAX;
    outInfo->CellInfo.wcdma.cellIdentityWcdma.cid =
        (getWwanJniCache().wcdmaIdGetCid != nullptr)
            ? env->CallIntMethod(id, getWwanJniCache().wcdmaIdGetCid)
            : INT32_MAX;
    outInfo->CellInfo.wcdma.cellIdentityWcdma.psc =
        (getWwanJniCache().wcdmaIdGetPsc != nullptr)
            ? env->CallIntMethod(id, getWwanJniCache().wcdmaIdGetPsc)
            : INT32_MAX;
    outInfo->CellInfo.wcdma.cellIdentityWcdma.uarfcn =
        (getWwanJniCache().wcdmaIdGetUarfcn != nullptr)
            ? env->CallIntMethod(id, getWwanJniCache().wcdmaIdGetUarfcn)
            : INT32_MAX;
    env->DeleteLocalRef(id);
  } else {
    outInfo->CellInfo.wcdma.cellIdentityWcdma.mcc = INT32_MAX;
    outInfo->CellInfo.wcdma.cellIdentityWcdma.mnc = INT32_MAX;
    outInfo->CellInfo.wcdma.cellIdentityWcdma.lac = INT32_MAX;
    outInfo->CellInfo.wcdma.cellIdentityWcdma.cid = INT32_MAX;
    outInfo->CellInfo.wcdma.cellIdentityWcdma.psc = INT32_MAX;
    outInfo->CellInfo.wcdma.cellIdentityWcdma.uarfcn = INT32_MAX;
  }

  jobject sig =
      (getWwanJniCache().wcdmaGetSignal != nullptr)
          ? env->CallObjectMethod(cellInfo, getWwanJniCache().wcdmaGetSignal)
          : nullptr;
  if (sig) {
    outInfo->CellInfo.wcdma.signalStrengthWcdma.signalStrength =
        (getWwanJniCache().wcdmaSigGetDbm != nullptr)
            ? env->CallIntMethod(sig, getWwanJniCache().wcdmaSigGetDbm)
            : INT32_MAX;
    // WCDMA getBitErrorRate is not available in standard API
    outInfo->CellInfo.wcdma.signalStrengthWcdma.bitErrorRate = INT32_MAX;
    env->DeleteLocalRef(sig);
  } else {
    outInfo->CellInfo.wcdma.signalStrengthWcdma.signalStrength = INT32_MAX;
    outInfo->CellInfo.wcdma.signalStrengthWcdma.bitErrorRate = INT32_MAX;
  }
}

// NR (5G) Parser
static void parseNrInfo(JNIEnv *env, jobject cellInfo,
                        chreWwanCellInfo *outInfo) {
  outInfo->cellInfoType = CHRE_WWAN_CELL_INFO_TYPE_NR;

  if (!getWwanJniCache().cellInfoNrClass) return;  // Safety check

  jobject id =
      (getWwanJniCache().nrGetIdentity != nullptr)
          ? env->CallObjectMethod(cellInfo, getWwanJniCache().nrGetIdentity)
          : nullptr;
  if (id) {
    jstring mccStr =
        (getWwanJniCache().nrIdGetMcc != nullptr)
            ? (jstring)env->CallObjectMethod(id, getWwanJniCache().nrIdGetMcc)
            : nullptr;
    jstring mncStr =
        (getWwanJniCache().nrIdGetMnc != nullptr)
            ? (jstring)env->CallObjectMethod(id, getWwanJniCache().nrIdGetMnc)
            : nullptr;

    outInfo->CellInfo.nr.cellIdentityNr.mcc = parseStringToInt(env, mccStr);
    outInfo->CellInfo.nr.cellIdentityNr.mnc = parseStringToInt(env, mncStr);
    if (mccStr) env->DeleteLocalRef(mccStr);
    if (mncStr) env->DeleteLocalRef(mncStr);

    if (getWwanJniCache().nrIdGetNci != nullptr) {
      chreWwanPackNrNci(env->CallLongMethod(id, getWwanJniCache().nrIdGetNci),
                        &(outInfo->CellInfo.nr.cellIdentityNr));
    } else {
      chreWwanPackNrNci(INT64_MAX, &(outInfo->CellInfo.nr.cellIdentityNr));
    }

    outInfo->CellInfo.nr.cellIdentityNr.pci =
        (getWwanJniCache().nrIdGetPci != nullptr)
            ? env->CallIntMethod(id, getWwanJniCache().nrIdGetPci)
            : INT32_MAX;
    outInfo->CellInfo.nr.cellIdentityNr.tac =
        (getWwanJniCache().nrIdGetTac != nullptr)
            ? env->CallIntMethod(id, getWwanJniCache().nrIdGetTac)
            : INT32_MAX;
    outInfo->CellInfo.nr.cellIdentityNr.nrarfcn =
        (getWwanJniCache().nrIdGetNrarfcn != nullptr)
            ? env->CallIntMethod(id, getWwanJniCache().nrIdGetNrarfcn)
            : INT32_MAX;

    env->DeleteLocalRef(id);
  } else {
    outInfo->CellInfo.nr.cellIdentityNr.mcc = INT32_MAX;
    outInfo->CellInfo.nr.cellIdentityNr.mnc = INT32_MAX;
    outInfo->CellInfo.nr.cellIdentityNr.pci = INT32_MAX;
    outInfo->CellInfo.nr.cellIdentityNr.tac = INT32_MAX;
    outInfo->CellInfo.nr.cellIdentityNr.nrarfcn = INT32_MAX;
  }

  jobject sig =
      (getWwanJniCache().nrGetSignal != nullptr)
          ? env->CallObjectMethod(cellInfo, getWwanJniCache().nrGetSignal)
          : nullptr;
  if (sig) {
    outInfo->CellInfo.nr.signalStrengthNr.ssRsrp =
        (getWwanJniCache().nrSigGetSsRsrp != nullptr)
            ? env->CallIntMethod(sig, getWwanJniCache().nrSigGetSsRsrp)
            : INT32_MAX;
    outInfo->CellInfo.nr.signalStrengthNr.ssRsrq =
        (getWwanJniCache().nrSigGetSsRsrq != nullptr)
            ? env->CallIntMethod(sig, getWwanJniCache().nrSigGetSsRsrq)
            : INT32_MAX;
    outInfo->CellInfo.nr.signalStrengthNr.ssSinr =
        (getWwanJniCache().nrSigGetSsSinr != nullptr)
            ? env->CallIntMethod(sig, getWwanJniCache().nrSigGetSsSinr)
            : INT32_MAX;
    // CSI is often optional or not available in basic CellSignalStrengthNr,
    // depends on method availability
    outInfo->CellInfo.nr.signalStrengthNr.csiRsrp = INT32_MAX;
    outInfo->CellInfo.nr.signalStrengthNr.csiRsrq = INT32_MAX;
    outInfo->CellInfo.nr.signalStrengthNr.csiSinr = INT32_MAX;

    env->DeleteLocalRef(sig);
  } else {
    outInfo->CellInfo.nr.signalStrengthNr.ssRsrp = INT32_MAX;
    outInfo->CellInfo.nr.signalStrengthNr.ssRsrq = INT32_MAX;
    outInfo->CellInfo.nr.signalStrengthNr.ssSinr = INT32_MAX;
    outInfo->CellInfo.nr.signalStrengthNr.csiRsrp = INT32_MAX;
    outInfo->CellInfo.nr.signalStrengthNr.csiRsrq = INT32_MAX;
    outInfo->CellInfo.nr.signalStrengthNr.csiSinr = INT32_MAX;
  }
}

// CDMA Parser
static void parseCdmaInfo(JNIEnv *env, jobject cellInfo,
                          chreWwanCellInfo *outInfo) {
  outInfo->cellInfoType = CHRE_WWAN_CELL_INFO_TYPE_CDMA;

  jobject id =
      (getWwanJniCache().cdmaGetIdentity != nullptr)
          ? env->CallObjectMethod(cellInfo, getWwanJniCache().cdmaGetIdentity)
          : nullptr;
  if (id) {
    outInfo->CellInfo.cdma.cellIdentityCdma.networkId =
        (getWwanJniCache().cdmaIdGetNetworkId != nullptr)
            ? env->CallIntMethod(id, getWwanJniCache().cdmaIdGetNetworkId)
            : INT32_MAX;
    outInfo->CellInfo.cdma.cellIdentityCdma.systemId =
        (getWwanJniCache().cdmaIdGetSystemId != nullptr)
            ? env->CallIntMethod(id, getWwanJniCache().cdmaIdGetSystemId)
            : INT32_MAX;
    outInfo->CellInfo.cdma.cellIdentityCdma.basestationId =
        (getWwanJniCache().cdmaIdGetBasestationId != nullptr)
            ? env->CallIntMethod(id, getWwanJniCache().cdmaIdGetBasestationId)
            : INT32_MAX;
    outInfo->CellInfo.cdma.cellIdentityCdma.longitude =
        (getWwanJniCache().cdmaIdGetLongitude != nullptr)
            ? env->CallIntMethod(id, getWwanJniCache().cdmaIdGetLongitude)
            : INT32_MAX;
    outInfo->CellInfo.cdma.cellIdentityCdma.latitude =
        (getWwanJniCache().cdmaIdGetLatitude != nullptr)
            ? env->CallIntMethod(id, getWwanJniCache().cdmaIdGetLatitude)
            : INT32_MAX;
    env->DeleteLocalRef(id);
  } else {
    outInfo->CellInfo.cdma.cellIdentityCdma.networkId = INT32_MAX;
    outInfo->CellInfo.cdma.cellIdentityCdma.systemId = INT32_MAX;
    outInfo->CellInfo.cdma.cellIdentityCdma.basestationId = INT32_MAX;
    outInfo->CellInfo.cdma.cellIdentityCdma.longitude = INT32_MAX;
    outInfo->CellInfo.cdma.cellIdentityCdma.latitude = INT32_MAX;
  }

  jobject sig =
      (getWwanJniCache().cdmaGetSignal != nullptr)
          ? env->CallObjectMethod(cellInfo, getWwanJniCache().cdmaGetSignal)
          : nullptr;
  if (sig) {
    outInfo->CellInfo.cdma.signalStrengthCdma.dbm =
        (getWwanJniCache().cdmaSigGetCdmaDbm != nullptr)
            ? env->CallIntMethod(sig, getWwanJniCache().cdmaSigGetCdmaDbm)
            : INT32_MAX;
    outInfo->CellInfo.cdma.signalStrengthCdma.ecio =
        (getWwanJniCache().cdmaSigGetCdmaEcio != nullptr)
            ? env->CallIntMethod(sig, getWwanJniCache().cdmaSigGetCdmaEcio)
            : INT32_MAX;
    env->DeleteLocalRef(sig);
  } else {
    outInfo->CellInfo.cdma.signalStrengthCdma.dbm = INT32_MAX;
    outInfo->CellInfo.cdma.signalStrengthCdma.ecio = INT32_MAX;
  }
}

// TD-SCDMA Parser
static void parseTdscdmaInfo(JNIEnv *env, jobject cellInfo,
                             chreWwanCellInfo *outInfo) {
  outInfo->cellInfoType = CHRE_WWAN_CELL_INFO_TYPE_TD_SCDMA;

  jobject id = (getWwanJniCache().tdscdmaGetIdentity != nullptr)
                   ? env->CallObjectMethod(cellInfo,
                                           getWwanJniCache().tdscdmaGetIdentity)
                   : nullptr;
  if (id) {
    jstring mccStr = (getWwanJniCache().tdscdmaIdGetMcc != nullptr)
                         ? (jstring)env->CallObjectMethod(
                               id, getWwanJniCache().tdscdmaIdGetMcc)
                         : nullptr;
    jstring mncStr = (getWwanJniCache().tdscdmaIdGetMnc != nullptr)
                         ? (jstring)env->CallObjectMethod(
                               id, getWwanJniCache().tdscdmaIdGetMnc)
                         : nullptr;
    outInfo->CellInfo.tdscdma.cellIdentityTdscdma.mcc =
        parseStringToInt(env, mccStr);
    outInfo->CellInfo.tdscdma.cellIdentityTdscdma.mnc =
        parseStringToInt(env, mncStr);
    if (mccStr) env->DeleteLocalRef(mccStr);
    if (mncStr) env->DeleteLocalRef(mncStr);

    outInfo->CellInfo.tdscdma.cellIdentityTdscdma.lac =
        (getWwanJniCache().tdscdmaIdGetLac != nullptr)
            ? env->CallIntMethod(id, getWwanJniCache().tdscdmaIdGetLac)
            : INT32_MAX;
    outInfo->CellInfo.tdscdma.cellIdentityTdscdma.cid =
        (getWwanJniCache().tdscdmaIdGetCid != nullptr)
            ? env->CallIntMethod(id, getWwanJniCache().tdscdmaIdGetCid)
            : INT32_MAX;
    outInfo->CellInfo.tdscdma.cellIdentityTdscdma.cpid =
        (getWwanJniCache().tdscdmaIdGetCpid != nullptr)
            ? env->CallIntMethod(id, getWwanJniCache().tdscdmaIdGetCpid)
            : INT32_MAX;
    env->DeleteLocalRef(id);
  } else {
    outInfo->CellInfo.tdscdma.cellIdentityTdscdma.mcc = INT32_MAX;
    outInfo->CellInfo.tdscdma.cellIdentityTdscdma.mnc = INT32_MAX;
    outInfo->CellInfo.tdscdma.cellIdentityTdscdma.lac = INT32_MAX;
    outInfo->CellInfo.tdscdma.cellIdentityTdscdma.cid = INT32_MAX;
    outInfo->CellInfo.tdscdma.cellIdentityTdscdma.cpid = INT32_MAX;
  }

  jobject sig =
      (getWwanJniCache().tdscdmaGetSignal != nullptr)
          ? env->CallObjectMethod(cellInfo, getWwanJniCache().tdscdmaGetSignal)
          : nullptr;
  if (sig) {
    outInfo->CellInfo.tdscdma.signalStrengthTdscdma.rscp =
        (getWwanJniCache().tdscdmaSigGetDbm != nullptr)
            ? env->CallIntMethod(sig, getWwanJniCache().tdscdmaSigGetDbm)
            : INT32_MAX;
    env->DeleteLocalRef(sig);
  } else {
    outInfo->CellInfo.tdscdma.signalStrengthTdscdma.rscp = INT32_MAX;
  }
}
}  // namespace

PlatformWwan::~PlatformWwan() {}

void PlatformWwan::init() {
  JniManagerSingleton::get()->registerHandleCellInfoCallbackFn(
      [](JNIEnv *env, jobjectArray cellInfoList) {
        if (cellInfoList == nullptr) return;

        jsize count = env->GetArrayLength(cellInfoList);

        auto *result = static_cast<chreWwanCellInfoResult *>(
            memoryAlloc(sizeof(chreWwanCellInfoResult)));

        if (!result) {
          return;
        }

        result->errorCode = CHRE_ERROR_NONE;
        result->cellInfoCount = static_cast<uint8_t>(count);
        result->reserved = 0;
        result->cookie = nullptr;
        result->cells = nullptr;

        if (count > 0) {
          result->cells = static_cast<chreWwanCellInfo *>(
              memoryAlloc(sizeof(chreWwanCellInfo) * count));

          if (!result->cells) {
            memoryFree(result);
            return;
          }

          for (int i = 0; i < count; i++) {
            jobject infoObj = env->GetObjectArrayElement(cellInfoList, i);
            if (!infoObj) continue;

            chreWwanCellInfo *currCell =
                const_cast<chreWwanCellInfo *>(&result->cells[i]);

            // Populate common fields (timeStamp, registered, etc.)
            populateCommonFields(env, infoObj, currCell);

            // Dispatch based on type
            if (env->IsInstanceOf(infoObj,
                                  getWwanJniCache().cellInfoLteClass)) {
              parseLteInfo(env, infoObj, currCell);
            } else if (env->IsInstanceOf(infoObj,
                                         getWwanJniCache().cellInfoGsmClass)) {
              parseGsmInfo(env, infoObj, currCell);
            } else if (env->IsInstanceOf(
                           infoObj, getWwanJniCache().cellInfoWcdmaClass)) {
              parseWcdmaInfo(env, infoObj, currCell);
            } else if (getWwanJniCache().cellInfoNrClass &&
                       env->IsInstanceOf(infoObj,
                                         getWwanJniCache().cellInfoNrClass)) {
              parseNrInfo(env, infoObj, currCell);
            } else if (getWwanJniCache().cellInfoCdmaClass &&
                       env->IsInstanceOf(infoObj,
                                         getWwanJniCache().cellInfoCdmaClass)) {
              parseCdmaInfo(env, infoObj, currCell);
            } else if (getWwanJniCache().cellInfoTdscdmaClass &&
                       env->IsInstanceOf(
                           infoObj, getWwanJniCache().cellInfoTdscdmaClass)) {
              parseTdscdmaInfo(env, infoObj, currCell);
            } else {
              // Should not reach here and set 0 as invalid type.
              currCell->cellInfoType = 0;
            }

            env->DeleteLocalRef(infoObj);
          }
        }

        EventLoopManagerSingleton::get()
            ->getWwanRequestManager()
            .handleCellInfoResult(result);
      });
}

uint32_t PlatformWwan::getCapabilities() {
  JNIEnv *env = getEnv();
  if (!env || !getWwanJniCache().contextHubNativeClass ||
      !getWwanJniCache().getCapabilitiesMethod) {
    LOGE("JNI not ready for getCapabilities");
    return CHRE_WWAN_CAPABILITIES_NONE;
  }
  return env->CallStaticIntMethod(getWwanJniCache().contextHubNativeClass,
                                  getWwanJniCache().getCapabilitiesMethod);
}

bool PlatformWwan::requestCellInfo() {
  JNIEnv *env = getEnv();
  if (!env || !getWwanJniCache().contextHubNativeClass ||
      !getWwanJniCache().requestCellInfoMethod) {
    LOGE("JNI not ready for requestCellInfo");
    return false;
  }

  // Call Java. Java will check cache and call back native onCellInfoReceived ->
  // handleCellInfoCallback
  return env->CallStaticBooleanMethod(getWwanJniCache().contextHubNativeClass,
                                      getWwanJniCache().requestCellInfoMethod);
}

void PlatformWwan::releaseCellInfoResult(chreWwanCellInfoResult *result) {
  if (result) {
    if (result->cells) {
      memoryFree(const_cast<chreWwanCellInfo *>(result->cells));
    }
    memoryFree(result);
  }
}

}  // namespace chre
