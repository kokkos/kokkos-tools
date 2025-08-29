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

#pragma once

#include <functional>
#include <thread>
#include <chrono>

class Daemon {
 public:
  Daemon(std::function<void()> func, int interval_ms)
      : interval_(interval_ms), func_(func){};

  void start();
  void run();
  void stop();
  bool is_running() const { return running_; }
  std::thread& get_thread() { return thread_; }

 private:
  std::chrono::milliseconds interval_;
  bool running_{false};
  std::function<void()> func_;
  std::thread thread_;
};
