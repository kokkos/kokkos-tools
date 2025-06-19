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

/*
 * Example of a timer launched as a daemon process that can be used
 * alongside other Kokkos profiling tools using a combined tool system.
 */

#include <cstdlib>
#include <iostream>
#include <vector>
#include <mutex>
#include <thread>
#include <chrono>
#include <atomic>

#include "kp_core.hpp"
#include "kp_universal.hpp"

namespace KokkosTools {
namespace CombinedDaemon {

// --- Timer globals ---
static std::vector<long long> s_timestamps;
static std::mutex s_mutex;
static std::thread s_timer_thread;
static std::atomic<bool> s_timer_stop_flag{false};
static constexpr int INTERVAL_MS = 100;

void timer_thread_func() {
  while (!s_timer_stop_flag.load(std::memory_order_relaxed)) {
    long long now = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch())
                        .count();
    {
      std::lock_guard<std::mutex> lock(s_mutex);
      s_timestamps.push_back(now);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(INTERVAL_MS));
  }
  // std::cout << "Timer thread stopped.\n";
}

// --- Kokkos Profiling Hooks ---

void kokkosp_init_library(const int loadSeq, const uint64_t interfaceVer,
                          const uint32_t devInfoCount,
                          Kokkos_Profiling_KokkosPDeviceInfo* deviceInfo) {
  std::cout
      << "CombinedDaemon: Kokkos Profiling Library Initialized (sequence: "
      << loadSeq << ", version: " << interfaceVer << ")\n";
  s_timer_stop_flag = false;
  s_timer_thread    = std::thread(timer_thread_func);
}

void kokkosp_finalize_library() {
  if (s_timer_thread.joinable()) {
    s_timer_stop_flag = true;
    s_timer_thread.join();
  }
  std::vector<long long> timestamps_copy;
  {
    std::lock_guard<std::mutex> lock(s_mutex);
    timestamps_copy = s_timestamps;
  }
  std::cout << "--- TIMER_TIMESTAMP_LOG ---\n";
  for (auto ts : timestamps_copy) {
    std::cout << ts << '\n';
  }
  std::cout << "--- END ---\n";
  std::cout << "CombinedDaemon: Kokkos Profiling Library Finalized.\n";
}

// --- Event Set Configuration ---

Kokkos::Tools::Experimental::EventSet get_event_set() {
  Kokkos::Tools::Experimental::EventSet my_event_set;
  memset(&my_event_set, 0,
         sizeof(my_event_set));  // zero any pointers not set here
  my_event_set.init     = kokkosp_init_library;
  my_event_set.finalize = kokkosp_finalize_library;
  return my_event_set;
}

}  // namespace CombinedDaemon
}  // namespace KokkosTools

extern "C" {

namespace impl = KokkosTools::CombinedDaemon;

EXPOSE_INIT(impl::kokkosp_init_library)
EXPOSE_FINALIZE(impl::kokkosp_finalize_library)
}