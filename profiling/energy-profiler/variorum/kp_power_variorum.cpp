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

/**
 * Kokkos Power Profiler - Specialized for Variorum
 * Simplified version focused on Variorum energy monitoring with integrated
 * timing
 */

#include <cstring>
#include <iostream>

#include "kp_core.hpp"
#include "variorum_power_profiler.hpp"

namespace KokkosTools {
namespace PowerProfiler {

// --- Core Initialization ---
VariorumPowerProfiler power_profiler;

// --- Library Initialization/Finalization ---

void kokkosp_init_library(const int loadSeq, const uint64_t interfaceVer,
                          const uint32_t devInfoCount,
                          Kokkos_Profiling_KokkosPDeviceInfo* deviceInfo) {
  printf("-----------------------------------------------------------\n");
  printf(
      "KokkosP: Power Profiler (sequence is %d, version: %lu, devices: %u)\n",
      loadSeq, interfaceVer, devInfoCount);
  printf("-----------------------------------------------------------\n");
  power_profiler.initialize();
}

void kokkosp_finalize_library() {
  if (power_profiler.is_initialized()) {
    power_profiler.finalize();
  } else {
    std::cerr
        << "PowerProfiler: Core not initialized, skipping finalization.\n";
  }
  printf("-----------------------------------------------------------\n");
  printf("KokkosP: Finalization of Power Profiler. Complete.\n");
  printf("-----------------------------------------------------------\n");
}

// --- Kernels Launch/End ---

void kokkosp_begin_parallel_for(const char* name, const uint32_t devID,
                                uint64_t* kID) {
  if (power_profiler.is_initialized()) {
    power_profiler.begin_kernel(*kID, std::string(name), KernelType::FOR);
  } else {
    std::cerr
        << "PowerProfiler: Core not initialized, cannot begin parallel for.\n";
  }
}

void kokkosp_end_parallel_for(const uint64_t kID) {
  if (power_profiler.is_initialized()) {
    power_profiler.end_kernel(kID);
  } else {
    std::cerr
        << "PowerProfiler: Core not initialized, cannot end parallel for.\n";
  }
}

void kokkosp_begin_parallel_scan(const char* name, const uint32_t devID,
                                 uint64_t* kID) {
  if (power_profiler.is_initialized() && kID) {
    power_profiler.begin_kernel(*kID, std::string(name), KernelType::SCAN);
  } else {
    std::cerr << "PowerProfiler: Core not initialized or kID is null, "
                 "cannot begin parallel scan.\n";
  }
}

void kokkosp_end_parallel_scan(const uint64_t kID) {
  if (power_profiler.is_initialized()) {
    power_profiler.end_kernel(kID);
  } else {
    std::cerr
        << "PowerProfiler: Core not initialized, cannot end parallel scan.\n";
  }
}

void kokkosp_begin_parallel_reduce(const char* name, const uint32_t devID,
                                   uint64_t* kID) {
  if (power_profiler.is_initialized() && kID) {
    power_profiler.begin_kernel(*kID, std::string(name), KernelType::REDUCE);
  } else {
    std::cerr << "PowerProfiler: Core not initialized or kID is null, "
                 "cannot begin parallel reduce.\n";
  }
}

void kokkosp_end_parallel_reduce(const uint64_t kID) {
  if (power_profiler.is_initialized()) {
    power_profiler.end_kernel(kID);
  } else {
    std::cerr
        << "PowerProfiler: Core not initialized, cannot end parallel reduce.\n";
  }
}

void kokkosp_push_profile_region(char const* regionName) {
  if (power_profiler.is_initialized()) {
    power_profiler.push_region(std::string(regionName));
    // printf("KokkosP: Entering profiling region: %s\n", regionName);
    // Commented out to avoid excessive output
  } else {
    std::cerr
        << "PowerProfiler: Core not initialized, cannot push profile region.\n";
  }
}

void kokkosp_pop_profile_region() {
  if (power_profiler.is_initialized()) {
    power_profiler.pop_region();
  } else {
    std::cerr
        << "PowerProfiler: Core not initialized, cannot pop profile region.\n";
  }
}

// --- Event Set Configuration ---

Kokkos::Tools::Experimental::EventSet get_event_set() {
  Kokkos::Tools::Experimental::EventSet my_event_set;
  memset(&my_event_set, 0,
         sizeof(my_event_set));  // zero any pointers not set here
  my_event_set.init                  = kokkosp_init_library;
  my_event_set.finalize              = kokkosp_finalize_library;
  my_event_set.begin_parallel_for    = kokkosp_begin_parallel_for;
  my_event_set.begin_parallel_reduce = kokkosp_begin_parallel_reduce;
  my_event_set.begin_parallel_scan   = kokkosp_begin_parallel_scan;
  my_event_set.end_parallel_for      = kokkosp_end_parallel_for;
  my_event_set.end_parallel_reduce   = kokkosp_end_parallel_reduce;
  my_event_set.end_parallel_scan     = kokkosp_end_parallel_scan;
  my_event_set.push_region           = kokkosp_push_profile_region;
  my_event_set.pop_region            = kokkosp_pop_profile_region;
  return my_event_set;
}

}  // namespace PowerProfiler
}  // namespace KokkosTools

extern "C" {

namespace impl = KokkosTools::PowerProfiler;

EXPOSE_INIT(impl::kokkosp_init_library)
EXPOSE_FINALIZE(impl::kokkosp_finalize_library)
EXPOSE_BEGIN_PARALLEL_FOR(impl::kokkosp_begin_parallel_for)
EXPOSE_END_PARALLEL_FOR(impl::kokkosp_end_parallel_for)
EXPOSE_BEGIN_PARALLEL_SCAN(impl::kokkosp_begin_parallel_scan)
EXPOSE_END_PARALLEL_SCAN(impl::kokkosp_end_parallel_scan)
EXPOSE_BEGIN_PARALLEL_REDUCE(impl::kokkosp_begin_parallel_reduce)
EXPOSE_END_PARALLEL_REDUCE(impl::kokkosp_end_parallel_reduce)
EXPOSE_PUSH_REGION(impl::kokkosp_push_profile_region)
EXPOSE_POP_REGION(impl::kokkosp_pop_profile_region)
}
