// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// SPDX-FileCopyrightText: Copyright Contributors to the Kokkos project

#include "daemon.hpp"
#include <condition_variable>
#include <mutex>
#include <thread>

namespace KokkosTools::EnergyProfiler {
void Daemon::start() {
  if (!running_) {
    running_ = true;
    thread_  = std::jthread([this](std::stop_token stop) { run(stop); });
  }
}

void Daemon::stop() {
  if (running_) {
    thread_.request_stop();
    thread_.join();
    running_ = false;
  }
}

void Daemon::run(std::stop_token stop) {
  std::mutex mutex;
  std::condition_variable_any wake;
  std::unique_lock<std::mutex> lock(mutex);
  while (!stop.stop_requested()) {
    auto next_run = std::chrono::steady_clock::now() + interval_;
    func_();
    // Returns early when a stop is requested.
    wake.wait_until(lock, stop, next_run, [] { return false; });
  }
}
}  // namespace KokkosTools::EnergyProfiler
