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

#include "chre/core/wakeup_stats_manager.h"

#include "chre/core/event_loop_manager.h"
#include "chre/platform/assert.h"
#include "chre/platform/log.h"

namespace chre {

void WakeupStatsManager::blameWakeup(Optional<Nanoapp *> nanoapp,
                                     WakeupReason reason) {
  if (reason == WakeupReason::NANOAPP_MESSAGE) {
    CHRE_ASSERT_LOG(nanoapp.has_value(),
                    "Nanoapp message wakeup triggered with null nanoapp");
  }

  if (!mHostWakeupBlamed) {
    mHostWakeupBlamed = true;
    if (nanoapp.has_value()) {
      nanoapp.value()->blameHostWakeup(reason);
    }
    // Framework-level attribution logic for 'reason' will be added later.
  }
}

bool WakeupStatsManager::willWakeupHost() const {
  return !EventLoopManagerSingleton::get()
              ->getPowerControlManager()
              .hostIsAwake() &&
         !mHostWakeupBlamed;
}

void WakeupStatsManager::resetBlameForHostWakeup() {
  mHostWakeupBlamed = false;
}

}  // namespace chre
