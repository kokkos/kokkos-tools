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

#include "daemon.hpp"
#include <stdexcept>
#include <thread>

void Daemon::start() {
  if (!running_) {
    running_ = true;
    thread_  = std::thread(&Daemon::tick, this);
  } else {
    throw std::runtime_error("Daemon already started");
  }
}

void Daemon::tick() {
  while (running_) {
    std::chrono::high_resolution_clock::time_point start_time =
        std::chrono::high_resolution_clock::now();

    // Execute the function
    func_();

    std::chrono::high_resolution_clock::time_point end_time =
        std::chrono::high_resolution_clock::now();
    std::chrono::milliseconds execution_duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(end_time -
                                                              start_time);

    // Calculate how long to sleep to maintain the interval
    if (execution_duration < interval_) {
      std::chrono::milliseconds sleep_duration = interval_ - execution_duration;
      std::this_thread::sleep_for(sleep_duration);
    }
  }
}

void Daemon::stop() {
  if (running_) {
    running_ = false;
    thread_.join();
  } else {
    throw std::runtime_error("Daemon not started");
  }
}