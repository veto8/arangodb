////////////////////////////////////////////////////////////////////////////////
/// DISCLAIMER
///
/// Copyright 2016 ArangoDB GmbH, Cologne, Germany
///
/// Licensed under the Apache License, Version 2.0 (the "License");
/// you may not use this file except in compliance with the License.
/// You may obtain a copy of the License at
///
///     http://www.apache.org/licenses/LICENSE-2.0
///
/// Unless required by applicable law or agreed to in writing, software
/// distributed under the License is distributed on an "AS IS" BASIS,
/// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
/// See the License for the specific language governing permissions and
/// limitations under the License.
///
/// Copyright holder is ArangoDB GmbH, Cologne, Germany
///
/// @author Simon Grätzer
////////////////////////////////////////////////////////////////////////////////

#ifndef ARANGODB_PREGEL_STATISTICS_H
#define ARANGODB_PREGEL_STATISTICS_H 1

#include <velocypack/Slice.h>
#include <velocypack/Builder.h>
#include <velocypack/velocypack-aliases.h>
#include "Pregel/Utils.h"

namespace arangodb {
namespace pregel {


struct WorkerStats {
  size_t activeCount;
  size_t sendCount;
  size_t receivedCount;
  uint64_t superstepRuntimeMilli = 0;

  WorkerStats() : activeCount(0), sendCount(0), receivedCount(0) {}
  WorkerStats(size_t a, size_t s, size_t r) : activeCount(a), sendCount(s), receivedCount(r) {}

  void accumulate(VPackSlice statValues) {
    VPackSlice p = statValues.get(Utils::activeCountKey);
    if (p.isInteger()) {
      activeCount += p.getUInt();
    }
    p = statValues.get(Utils::sendCountKey);
    if (p.isInteger()) {
      sendCount += p.getUInt();
    }
    p = statValues.get(Utils::receivedCountKey);
    if (p.isInteger()) {
      receivedCount += p.getUInt();
    }
  }

  void serializeValues(VPackBuilder& b) const {
    b.add(Utils::activeCountKey, VPackValue(activeCount));
    b.add(Utils::sendCountKey, VPackValue(sendCount));
    b.add(Utils::receivedCountKey, VPackValue(receivedCount));
    b.add(Utils::superstepRuntimeMilliKey, VPackValue(superstepRuntimeMilli));
  }
  
  bool allZero() const {
    return activeCount == 0 && sendCount == 0 && receivedCount == 0;
  }
};
}
}
#endif
