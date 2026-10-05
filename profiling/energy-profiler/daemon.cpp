// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// SPDX-FileCopyrightText: Copyright Contributors to the Kokkos project

#include "daemon.hpp"
#include <mutex>
#include <thread>

namespace KokkosTools::EnergyProfiler {
Daemon::~Daemon() { stop(); }

void Daemon::start() {
  if (!running_) {
    running_ = true;
    thread_  = std::thread(&Daemon::run, this);
  }
}

void Daemon::stop() {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!running_) return;
    running_ = false;
  }
  wake_.notify_one();
  thread_.join();
}

void Daemon::run() {
  std::unique_lock<std::mutex> lock(mutex_);
  while (running_) {
    auto next_run = std::chrono::steady_clock::now() + interval_;
    lock.unlock();
    func_();
    lock.lock();
    // Returns early when stop() clears the flag.
    wake_.wait_until(lock, next_run, [this] { return !running_; });
  }
}
}  // namespace KokkosTools::EnergyProfiler
