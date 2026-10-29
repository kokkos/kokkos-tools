// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// SPDX-FileCopyrightText: Copyright Contributors to the Kokkos project

#ifndef KOKKOSP_ENERGY_PROFILER_DAEMON_HPP
#define KOKKOSP_ENERGY_PROFILER_DAEMON_HPP

#include <atomic>
#include <chrono>
#include <functional>
#include <stop_token>
#include <thread>

namespace KokkosTools::EnergyProfiler {
class Daemon {
 public:
  Daemon(std::function<void()> func, std::chrono::nanoseconds interval)
      : interval_(interval), func_(std::move(func)){};

  void start();
  void stop();
  bool is_running() const { return running_; }
  auto& get_thread() { return thread_; }

 private:
  void run(std::stop_token stop);
  std::chrono::nanoseconds interval_;
  std::atomic<bool> running_{false};
  std::function<void()> func_;
  // Declared last so that it is destroyed first: the thread is stopped and
  // joined before the members it uses go away.
  std::jthread thread_;
};
}  // namespace KokkosTools::EnergyProfiler
#endif
