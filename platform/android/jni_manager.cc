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

#include "chre/platform/android/jni_manager.h"
#include "chre/platform/log.h"

#include "jni.h"

namespace chre {
namespace {

jclass findClass(JNIEnv *env, const char *name) {
  jclass cls = env->FindClass(name);
  if (env->ExceptionCheck()) {
    env->ExceptionClear();
    return nullptr;
  }
  return cls;
}

jmethodID getMethodID(JNIEnv *env, jclass cls, const char *name,
                      const char *sig) {
  if (cls == nullptr) return nullptr;
  jmethodID mid = env->GetMethodID(cls, name, sig);
  if (env->ExceptionCheck()) {
    env->ExceptionClear();
    return nullptr;
  }
  return mid;
}

jmethodID getStaticMethodID(JNIEnv *env, jclass cls, const char *name,
                            const char *sig) {
  if (cls == nullptr) return nullptr;
  jmethodID mid = env->GetStaticMethodID(cls, name, sig);
  if (env->ExceptionCheck()) {
    env->ExceptionClear();
    return nullptr;
  }
  return mid;
}

}  // namespace

void JniManager::init(JavaVM *vm) {
  mJavaVm = vm;
  if (vm == nullptr) {
    LOGE("JniManager init failed: JavaVM is null");
    return;
  }

  JNIEnv *env = nullptr;
  if (vm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6) != JNI_OK) {
    LOGE("JniManager init failed: Could not get JNIEnv from JavaVM");
    return;
  }

  // --- WWAN Initialization ---
  // ContextHubAPNative
  jclass nativeCls =
      findClass(env, "com/google/android/chre/ap/ContextHubAPNative");
  if (nativeCls) {
    mWwanJniCache.contextHubNativeClass = (jclass)env->NewGlobalRef(nativeCls);
    mWwanJniCache.getCapabilitiesMethod =
        getStaticMethodID(env, nativeCls, "getWwanCapabilities", "()I");
    mWwanJniCache.requestCellInfoMethod =
        getStaticMethodID(env, nativeCls, "requestWwanCellInfo", "()Z");
  }

  // Base CellInfo
  jclass baseCi = findClass(env, "android/telephony/CellInfo");
  if (baseCi) {
    mWwanJniCache.getTimeStamp =
        getMethodID(env, baseCi, "getTimeStamp", "()J");
    mWwanJniCache.isRegistered =
        getMethodID(env, baseCi, "isRegistered", "()Z");
  }

  // LTE
  jclass ciLte = findClass(env, "android/telephony/CellInfoLte");
  jclass idLte = findClass(env, "android/telephony/CellIdentityLte");
  jclass sigLte = findClass(env, "android/telephony/CellSignalStrengthLte");
  if (ciLte && idLte && sigLte) {
    mWwanJniCache.cellInfoLteClass = (jclass)env->NewGlobalRef(ciLte);

    mWwanJniCache.lteGetIdentity = getMethodID(
        env, ciLte, "getCellIdentity", "()Landroid/telephony/CellIdentityLte;");
    mWwanJniCache.lteGetSignal =
        getMethodID(env, ciLte, "getCellSignalStrength",
                    "()Landroid/telephony/CellSignalStrengthLte;");

    mWwanJniCache.lteIdGetMcc = getMethodID(env, idLte, "getMcc", "()I");
    mWwanJniCache.lteIdGetMnc = getMethodID(env, idLte, "getMnc", "()I");
    mWwanJniCache.lteIdGetCi = getMethodID(env, idLte, "getCi", "()I");
    mWwanJniCache.lteIdGetPci = getMethodID(env, idLte, "getPci", "()I");
    mWwanJniCache.lteIdGetTac = getMethodID(env, idLte, "getTac", "()I");

    mWwanJniCache.lteSigGetDbm = getMethodID(env, sigLte, "getDbm", "()I");
    mWwanJniCache.lteSigGetRsrp = getMethodID(env, sigLte, "getRsrp", "()I");
    mWwanJniCache.lteSigGetRsrq = getMethodID(env, sigLte, "getRsrq", "()I");
    mWwanJniCache.lteSigGetRssnr = getMethodID(env, sigLte, "getRssnr", "()I");
    mWwanJniCache.lteSigGetTa =
        getMethodID(env, sigLte, "getTimingAdvance", "()I");
  }

  // GSM
  jclass ciGsm = findClass(env, "android/telephony/CellInfoGsm");
  jclass idGsm = findClass(env, "android/telephony/CellIdentityGsm");
  jclass sigGsm = findClass(env, "android/telephony/CellSignalStrengthGsm");
  if (ciGsm && idGsm && sigGsm) {
    mWwanJniCache.cellInfoGsmClass = (jclass)env->NewGlobalRef(ciGsm);
    mWwanJniCache.gsmGetIdentity = getMethodID(
        env, ciGsm, "getCellIdentity", "()Landroid/telephony/CellIdentityGsm;");
    mWwanJniCache.gsmGetSignal =
        getMethodID(env, ciGsm, "getCellSignalStrength",
                    "()Landroid/telephony/CellSignalStrengthGsm;");

    mWwanJniCache.gsmIdGetMcc = getMethodID(env, idGsm, "getMcc", "()I");
    mWwanJniCache.gsmIdGetMnc = getMethodID(env, idGsm, "getMnc", "()I");
    mWwanJniCache.gsmIdGetLac = getMethodID(env, idGsm, "getLac", "()I");
    mWwanJniCache.gsmIdGetCid = getMethodID(env, idGsm, "getCid", "()I");
    mWwanJniCache.gsmIdGetArfcn = getMethodID(env, idGsm, "getArfcn", "()I");
    mWwanJniCache.gsmIdGetBsic = getMethodID(env, idGsm, "getBsic", "()I");

    mWwanJniCache.gsmSigGetDbm = getMethodID(env, sigGsm, "getDbm", "()I");
    mWwanJniCache.gsmSigGetBitErrorRate =
        getMethodID(env, sigGsm, "getBitErrorRate", "()I");
  }

  // WCDMA
  jclass ciWcdma = findClass(env, "android/telephony/CellInfoWcdma");
  jclass idWcdma = findClass(env, "android/telephony/CellIdentityWcdma");
  jclass sigWcdma = findClass(env, "android/telephony/CellSignalStrengthWcdma");
  if (ciWcdma && idWcdma && sigWcdma) {
    mWwanJniCache.cellInfoWcdmaClass = (jclass)env->NewGlobalRef(ciWcdma);
    mWwanJniCache.wcdmaGetIdentity =
        getMethodID(env, ciWcdma, "getCellIdentity",
                    "()Landroid/telephony/CellIdentityWcdma;");
    mWwanJniCache.wcdmaGetSignal =
        getMethodID(env, ciWcdma, "getCellSignalStrength",
                    "()Landroid/telephony/CellSignalStrengthWcdma;");

    mWwanJniCache.wcdmaIdGetMcc = getMethodID(env, idWcdma, "getMcc", "()I");
    mWwanJniCache.wcdmaIdGetMnc = getMethodID(env, idWcdma, "getMnc", "()I");
    mWwanJniCache.wcdmaIdGetLac = getMethodID(env, idWcdma, "getLac", "()I");
    mWwanJniCache.wcdmaIdGetCid = getMethodID(env, idWcdma, "getCid", "()I");
    mWwanJniCache.wcdmaIdGetPsc = getMethodID(env, idWcdma, "getPsc", "()I");
    mWwanJniCache.wcdmaIdGetUarfcn =
        getMethodID(env, idWcdma, "getUarfcn", "()I");

    mWwanJniCache.wcdmaSigGetDbm = getMethodID(env, sigWcdma, "getDbm", "()I");
  }

  // NR (5G)
  jclass ciNr = findClass(env, "android/telephony/CellInfoNr");
  jclass idNr = findClass(env, "android/telephony/CellIdentityNr");
  jclass sigNr = findClass(env, "android/telephony/CellSignalStrengthNr");
  if (ciNr && idNr && sigNr) {
    mWwanJniCache.cellInfoNrClass = (jclass)env->NewGlobalRef(ciNr);
    mWwanJniCache.nrGetIdentity = getMethodID(
        env, ciNr, "getCellIdentity", "()Landroid/telephony/CellIdentity;");
    mWwanJniCache.nrGetSignal =
        getMethodID(env, ciNr, "getCellSignalStrength",
                    "()Landroid/telephony/CellSignalStrength;");

    mWwanJniCache.nrIdGetMcc =
        getMethodID(env, idNr, "getMccString", "()Ljava/lang/String;");
    mWwanJniCache.nrIdGetMnc =
        getMethodID(env, idNr, "getMncString", "()Ljava/lang/String;");
    mWwanJniCache.nrIdGetNci = getMethodID(env, idNr, "getNci", "()J");
    mWwanJniCache.nrIdGetPci = getMethodID(env, idNr, "getPci", "()I");
    mWwanJniCache.nrIdGetTac = getMethodID(env, idNr, "getTac", "()I");
    mWwanJniCache.nrIdGetNrarfcn = getMethodID(env, idNr, "getNrarfcn", "()I");

    mWwanJniCache.nrSigGetDbm = getMethodID(env, sigNr, "getDbm", "()I");
    mWwanJniCache.nrSigGetSsRsrp = getMethodID(env, sigNr, "getSsRsrp", "()I");
    mWwanJniCache.nrSigGetSsRsrq = getMethodID(env, sigNr, "getSsRsrq", "()I");
    mWwanJniCache.nrSigGetSsSinr = getMethodID(env, sigNr, "getSsSinr", "()I");
  }

  // CDMA
  jclass ciCdma = findClass(env, "android/telephony/CellInfoCdma");
  jclass idCdma = findClass(env, "android/telephony/CellIdentityCdma");
  jclass sigCdma = findClass(env, "android/telephony/CellSignalStrengthCdma");
  if (ciCdma && idCdma && sigCdma) {
    mWwanJniCache.cellInfoCdmaClass = (jclass)env->NewGlobalRef(ciCdma);
    mWwanJniCache.cdmaGetIdentity =
        getMethodID(env, ciCdma, "getCellIdentity",
                    "()Landroid/telephony/CellIdentityCdma;");
    mWwanJniCache.cdmaGetSignal =
        getMethodID(env, ciCdma, "getCellSignalStrength",
                    "()Landroid/telephony/CellSignalStrengthCdma;");

    mWwanJniCache.cdmaIdGetNetworkId =
        getMethodID(env, idCdma, "getNetworkId", "()I");
    mWwanJniCache.cdmaIdGetSystemId =
        getMethodID(env, idCdma, "getSystemId", "()I");
    mWwanJniCache.cdmaIdGetBasestationId =
        getMethodID(env, idCdma, "getBasestationId", "()I");
    mWwanJniCache.cdmaIdGetLongitude =
        getMethodID(env, idCdma, "getLongitude", "()I");
    mWwanJniCache.cdmaIdGetLatitude =
        getMethodID(env, idCdma, "getLatitude", "()I");

    mWwanJniCache.cdmaSigGetCdmaDbm =
        getMethodID(env, sigCdma, "getCdmaDbm", "()I");
    mWwanJniCache.cdmaSigGetCdmaEcio =
        getMethodID(env, sigCdma, "getCdmaEcio", "()I");
  }

  // TD-SCDMA
  jclass ciTdscdma = findClass(env, "android/telephony/CellInfoTdscdma");
  jclass idTdscdma = findClass(env, "android/telephony/CellIdentityTdscdma");
  jclass sigTdscdma =
      findClass(env, "android/telephony/CellSignalStrengthTdscdma");
  if (ciTdscdma && idTdscdma && sigTdscdma) {
    mWwanJniCache.cellInfoTdscdmaClass = (jclass)env->NewGlobalRef(ciTdscdma);
    mWwanJniCache.tdscdmaGetIdentity =
        getMethodID(env, ciTdscdma, "getCellIdentity",
                    "()Landroid/telephony/CellIdentityTdscdma;");
    mWwanJniCache.tdscdmaGetSignal =
        getMethodID(env, ciTdscdma, "getCellSignalStrength",
                    "()Landroid/telephony/CellSignalStrengthTdscdma;");

    mWwanJniCache.tdscdmaIdGetMcc =
        getMethodID(env, idTdscdma, "getMccString", "()Ljava/lang/String;");
    mWwanJniCache.tdscdmaIdGetMnc =
        getMethodID(env, idTdscdma, "getMncString", "()Ljava/lang/String;");
    mWwanJniCache.tdscdmaIdGetLac =
        getMethodID(env, idTdscdma, "getLac", "()I");
    mWwanJniCache.tdscdmaIdGetCid =
        getMethodID(env, idTdscdma, "getCid", "()I");
    mWwanJniCache.tdscdmaIdGetCpid =
        getMethodID(env, idTdscdma, "getCpid", "()I");

    mWwanJniCache.tdscdmaSigGetDbm =
        getMethodID(env, sigTdscdma, "getDbm", "()I");
  }
}

void JniManager::registerHandleCellInfoCallbackFn(HandleCellInfoCallbackFn fn) {
  mHandleCellInfoCallbackFn = fn;
}

void JniManager::onCellInfoReceived(JNIEnv *env, jobjectArray cellInfoList) {
  if (mHandleCellInfoCallbackFn != nullptr) {
    mHandleCellInfoCallbackFn(env, cellInfoList);
  } else {
    LOGW("CellInfo received but no callback registered in JniManager");
  }
}

JNIEnv *JniManager::getOrAttachJniEnv() {
  if (mJavaVm == nullptr) {
    LOGE("JavaVM not initialized");
    return nullptr;
  }

  JNIEnv *env = nullptr;
  jint res = mJavaVm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6);

  if (res == JNI_OK) {
    return env;
  } else if (res == JNI_EDETACHED) {
    if (mJavaVm->AttachCurrentThread(&env, nullptr) != JNI_OK) {
      LOGE("Failed to attach thread to JavaVM");
      return nullptr;
    }
    return env;
  } else {
    LOGE("Failed to get JNIEnv: %d", res);
    return nullptr;
  }
}

}  // namespace chre
