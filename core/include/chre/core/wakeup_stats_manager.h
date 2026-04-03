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

#ifndef CHRE_CORE_WAKEUP_STATS_MANAGER_H_
#define CHRE_CORE_WAKEUP_STATS_MANAGER_H_

#include "chre/core/nanoapp.h"
#include "chre/core/wakeup_reason.h"
#include "chre/platform/atomic.h"
#include "chre/util/non_copyable.h"
#include "chre/util/optional.h"

namespace chre {

/**
 * Central manager for host wakeup attribution and framework-level statistics.
 */
class WakeupStatsManager : public NonCopyable {
 public:
  /**
   * Records a host wakeup and attributes it to a nanoapp or the framework.
   *
   * @param nanoapp Optional pointer to the nanoapp that triggered the wakeup.
   * @param reason The reason for the host wakeup.
   */
  void blameWakeup(Optional<Nanoapp *> nanoapp, WakeupReason reason);

  /**
   * Resets the host wakeup blame latch. This should be called when the host
   * is known to be suspended.
   */
  void resetBlameForHostWakeup();

  /**
   * @return true if the host wakeup has already been blamed in the current
   *         cycle.
   */
  bool isHostWakeupBlamed() const {
    return mHostWakeupBlamed;
  }

  /**
   * @return true if the current action will trigger a host wakeup that will be
   *         attributed (blamed).
   */
  bool willWakeupHost() const;

 private:
  //! Ensures that we do not blame more than once per host wakeup.
  AtomicBool mHostWakeupBlamed{false};
};

}  // namespace chre

#endif  // CHRE_CORE_WAKEUP_STATS_MANAGER_H_
