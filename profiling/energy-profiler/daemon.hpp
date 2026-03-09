// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// SPDX-FileCopyrightText: Copyright Contributors to the Kokkos project

#ifndef KOKKOSP_ENERGY_PROFILER_DAEMON_HPP
#define KOKKOSP_ENERGY_PROFILER_DAEMON_HPP

#include <chrono>
#include <functional>
#include <thread>

namespace KokkosTools::EnergyProfiler {
class Daemon {
 public:
  Daemon(std::function<void()> func, const std::chrono::duration& interval)
      : interval_(interval), func_(std::move(func)){};

  void start();
  void stop();
  bool is_running() const { return running_; }
  auto& get_thread() { return thread_; }

 private:
  void run();
  std::chrono::duration interval_;
  bool running_{false};
  std::function<void()> func_;
  std::thread thread_;
};
}  // namespace KokkosTools::EnergyProfiler
#endif
