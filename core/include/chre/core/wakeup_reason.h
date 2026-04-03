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

#ifndef CHRE_CORE_WAKEUP_REASON_H_
#define CHRE_CORE_WAKEUP_REASON_H_

#include <cstdint>

namespace chre {

/**
 * Categorization of host wakeup reasons.
 *
 * These values correspond to the messages in the ChreMessage union defined in
 * system/chre/platform/shared/idl/host_messages.fbs.
 */
enum class WakeupReason : uint8_t {
  UNSPECIFIED,
  NANOAPP_MESSAGE,
  METRIC_LOG,
};

}  // namespace chre

#endif  // CHRE_CORE_WAKEUP_REASON_H_
