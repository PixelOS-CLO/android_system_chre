/*
 * Copyright (C) 2026 The Android Open Source Project
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

#ifndef _CHREX_ANDROID_H_
#define _CHREX_ANDROID_H_

#include <cstdint>

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

/**
 * @file
 * API related to Android platform information.
 */

/**
 * @brief Get the Android API Level (SDK version) of the AP.
 * @return The Android API Level, or -1 if the information is unavailable.
 */
int32_t chrexGetAndroidApiLevel();

#ifdef __cplusplus
}
#endif  // __cplusplus

#endif  // _CHREX_ANDROID_H_
