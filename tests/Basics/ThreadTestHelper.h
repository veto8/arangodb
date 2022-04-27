////////////////////////////////////////////////////////////////////////////////
/// DISCLAIMER
///
/// Copyright 2014-2022 ArangoDB GmbH, Cologne, Germany
/// Copyright 2004-2014 triAGENS GmbH, Cologne, Germany
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
/// @author Tobias Gödderz
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <gtest/gtest.h>

#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>

namespace arangodb::tests {

/**
 * @brief The constructor of WorkerThread starts a thread, which immediately
 * starts waiting on a condition variable. The execute() method takes a
 * callback, which is passed to the waiting thread and is then executed by it,
 * while execute() returns.
 */
struct WorkerThread : std::enable_shared_from_this<WorkerThread> {
  WorkerThread() = default;

  void run() {
    // This can't be done in the constructor (shared_from_this)
    _thread = std::thread([self = this->shared_from_this()] {
      auto guard = std::unique_lock(self->_mutex);

      auto wait = [&] {
        self->_cv.wait(guard, [&] {
          return self->_callback != nullptr || self->_stopped;
        });
      };

      for (wait(); !self->_stopped; wait()) {
        self->_callback.operator()();
        self->_callback = {};
      }
    });
  }

  void execute(std::function<void()> callback) {
    ASSERT_TRUE(_thread.joinable());
    ASSERT_TRUE(callback);
    {
      auto guard = std::unique_lock(_mutex);
      ASSERT_TRUE(!_stopped);
      ASSERT_FALSE(_callback);
      std::swap(_callback, callback);
    }

    _cv.notify_one();
  }

  void stop() {
    {
      auto guard = std::unique_lock(_mutex);
      _stopped = true;
    }

    _cv.notify_one();
  }

 private:
  std::thread _thread;
  std::mutex _mutex;
  std::condition_variable _cv;
  std::function<void()> _callback;
  bool _stopped = false;
};

auto operator<<(WorkerThread& workerThread, std::function<void()> callback) {
  return workerThread.execute(std::move(callback));
}

}  // namespace arangodb::tests
