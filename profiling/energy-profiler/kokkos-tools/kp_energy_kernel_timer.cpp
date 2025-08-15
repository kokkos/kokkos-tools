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
#include "../common/timer_system.hpp"
#include "../common/filename_prefix.hpp"

namespace KokkosTools {
namespace KernelTimer {

// --- Core Initialization ---
Timer::KernelTimerTool timer;

#ifdef ENABLE_VERBOSE_OUTPUT
constexpr bool VERBOSE = true;
#else
constexpr bool VERBOSE = false;
#endif

std::string KOKKOS_PROFILE_LIBRARY_NAME =
    "Kokkos Kernel Timer for Energy Profiler";

// --- Library Initialization/Finalization ---

void kokkosp_init_library(const int loadSeq, const uint64_t interfaceVer,
                          const uint32_t devInfoCount,
                          Kokkos_Profiling_KokkosPDeviceInfo* deviceInfo) {
  timer.init_library(loadSeq, interfaceVer, devInfoCount, deviceInfo);
}

void kokkosp_finalize_library() {
  std::cout << "Kokkos Power Profiler: Finalizing library\n";
  timer.finalize_library();
  std::cout << "Kokkos Power Profiler: Library finalized\n";

  std::string prefix = generate_prefix();

  const auto& kernels = timer.get_kernel_timings();
  KokkosTools::Timer::print_kernels_summary(kernels);
  KokkosTools::Timer::export_kernels_csv(kernels, prefix + "_kernels.csv");

  // Récapitulatif des régions
  const auto& regions = timer.get_region_timings();
  KokkosTools::Timer::print_regions_summary(regions);
  KokkosTools::Timer::export_regions_csv(regions, prefix + "_regions.csv");

  // Récapitulatif des deep copies
  const auto& deepcopies = timer.get_deep_copy_timings();
  KokkosTools::Timer::print_deepcopies_summary(deepcopies);
  KokkosTools::Timer::export_deepcopies_csv(deepcopies,
                                            prefix + "_deepcopies.csv");
}

// --- Kernels Launch/End ---

void kokkosp_begin_parallel_for(const char* name, const uint32_t devID,
                                uint64_t* kID) {
  timer.begin_parallel_for(name, devID, *kID);
  if (VERBOSE) {
    std::cout << "Kokkos Power Profiler: Started parallel_for '" << name
              << "' on device " << devID << " with ID " << *kID << "\n";
  }
}

void kokkosp_end_parallel_for(const uint64_t kID) {
  timer.end_parallel_for(kID);
  if (VERBOSE) {
    std::cout << "Kokkos Power Profiler: Ended parallel_for with ID " << kID
              << "\n";
  }
}

void kokkosp_begin_parallel_scan(const char* name, const uint32_t devID,
                                 uint64_t* kID) {
  timer.begin_parallel_scan(name, devID, kID);
  if (VERBOSE) {
    std::cout << "Kokkos Power Profiler: Started parallel_scan '" << name
              << "' on device " << devID << " with ID " << *kID << "\n";
  }
}

void kokkosp_end_parallel_scan(const uint64_t kID) {
  timer.end_parallel_scan(kID);
  if (VERBOSE) {
    std::cout << "Kokkos Power Profiler: Ended parallel_scan with ID " << kID
              << "\n";
  }
}

void kokkosp_begin_parallel_reduce(const char* name, const uint32_t devID,
                                   uint64_t* kID) {
  timer.begin_parallel_reduce(name, devID, kID);
  if (VERBOSE) {
    std::cout << "Kokkos Power Profiler: Started parallel_reduce '" << name
              << "' on device " << devID << " with ID " << *kID << "\n";
  }
}

void kokkosp_end_parallel_reduce(const uint64_t kID) {
  timer.end_parallel_reduce(kID);
  if (VERBOSE) {
    std::cout << "Kokkos Power Profiler: Ended parallel_reduce with ID " << kID
              << "\n";
  }
}

void kokkosp_push_profile_region(char const* regionName) {
  timer.push_profile_region(regionName);
  if (VERBOSE) {
    std::cout << "Kokkos Power Profiler: Pushed profile region '" << regionName
              << "'\n";
  }
}

void kokkosp_pop_profile_region() {
  timer.pop_profile_region();
  if (VERBOSE) {
    std::cout << "Kokkos Power Profiler: Popped profile region\n";
  }
}

void kokkosp_begin_deep_copy(Kokkos::Tools::SpaceHandle dst_handle,
                             const char* dst_name, const void* dst_ptr,
                             Kokkos::Tools::SpaceHandle src_handle,
                             const char* src_name, const void* src_ptr,
                             uint64_t size) {
  timer.begin_deep_copy(dst_handle, dst_name, dst_ptr, src_handle, src_name,
                        src_ptr, size);
  if (VERBOSE) {
    std::cout << "Kokkos Power Profiler: Started deep copy from '" << src_name
              << "' to '" << dst_name << "' of size " << size << " bytes\n";
  }
}

void kokkosp_end_deep_copy() {
  timer.end_deep_copy();
  if (VERBOSE) {
    std::cout << "Kokkos Power Profiler: Ended deep copy\n";
  }
}

// --- Event Set Configuration ---

Kokkos::Tools::Experimental::EventSet get_event_set() {
  Kokkos::Tools::Experimental::EventSet my_event_set;
  memset(&my_event_set, 0,
         sizeof(my_event_set));  // zero any pointers not set here
  my_event_set.init                  = kokkosp_init_library;
  my_event_set.finalize              = kokkosp_finalize_library;
  my_event_set.begin_deep_copy       = kokkosp_begin_deep_copy;
  my_event_set.end_deep_copy         = kokkosp_end_deep_copy;
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

}  // namespace KernelTimer
}  // namespace KokkosTools

extern "C" {

namespace impl = KokkosTools::KernelTimer;

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
EXPOSE_BEGIN_DEEP_COPY(impl::kokkosp_begin_deep_copy)
EXPOSE_END_DEEP_COPY(impl::kokkosp_end_deep_copy)
}
