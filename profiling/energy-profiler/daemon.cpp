// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// SPDX-FileCopyrightText: Copyright Contributors to the Kokkos project

#include "daemon.hpp"
#include <stdexcept>
#include <thread>

namespace KokkosTools::EnergyProfiler {
void Daemon::start() {
  if (!running_) {
    running_ = true;
    thread_  = std::thread(&Daemon::run, this);
  }
}

void Daemon::stop() {
  if (running_) {
    running_ = false;
    thread_.join();
  }
}

void Daemon::run() {
  while (running_) {
    auto next_run = std::chrono::high_resolution_clock::now() + interval_;
    func_();
    std::this_thread::sleep_until(next_run);
  }
}
}  // namespace KokkosTools::EnergyProfiler
