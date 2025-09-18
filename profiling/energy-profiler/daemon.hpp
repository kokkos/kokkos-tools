//@HEADER
// ************************************************************************
//
//                        Kokkos v. 4.0
//       Copyright (2022) National Technology & Engineering
//               Solutions of Sandia, LLC (NTESS).
//
// Under the terms of Contract DE-NA0003525 with NTESS,
// the U.S. Government retains certain rights in this software.
//
// Part of Kokkos, under the Apache License v2.0 with LLVM Exceptions.
// See https://kokkos.org/LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//@HEADER

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
  std::thread& get_thread() { return thread_; }

 private:
  void run();
  std::chrono::duration interval_;
  bool running_{false};
  std::function<void()> func_;
  std::thread thread_;
};
}  // namespace EnergyProfiler
}  // namespace KokkosTools
#endif
