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

#include <algorithm>
#include <chrono>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <mutex>
#include <string>
#include <vector>

#include "kp_core.hpp"
#include "timing_utils.hpp"
#include "timing_export.hpp"

namespace KokkosTools {
namespace EnergyProfiler {

// Helper function to generate new region ID
uint64_t generate_new_region_id() {
  auto& state = EnergyProfilerState::get_instance();
  std::lock_guard<std::mutex> lock(state.get_mutex());
  uint64_t current_id = state.get_next_region_id();
  state.increment_next_region_id();
  return current_id;
}

// Helper function for verbose logging
void log_verbose(const char* format, ...) {
  if (EnergyProfilerState::get_instance().get_verbose_enabled()) {
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
  }
}

// Start a region
void start_region(const std::string& name, RegionType type, uint64_t id) {
  try {
    TimingInfo region;
    region.name       = name;
    region.type       = type;
    region.start_time = std::chrono::high_resolution_clock::now();
    region.id         = id;
    auto& state       = EnergyProfilerState::get_instance();
    std::lock_guard<std::mutex> lock(state.get_mutex());
    state.get_active_regions().push_back(region);
  } catch (const std::exception& e) {
    std::cerr << "Error in start_region: " << e.what() << std::endl;
  }
}

// End last region of given type
void end_region_by_type(RegionType type_to_end) {
  try {
    auto& state = EnergyProfilerState::get_instance();
    std::lock_guard<std::mutex> lock(state.get_mutex());
    auto& active_regions = state.get_active_regions();
    if (active_regions.empty()) return;
    auto it = std::find_if(active_regions.rbegin(), active_regions.rend(),
                           [type_to_end](const TimingInfo& region) {
                             return region.type == type_to_end;
                           });
    if (it != active_regions.rend()) {
      auto region = *it;
      active_regions.erase(std::next(it).base());
      region.end_time = std::chrono::high_resolution_clock::now();
      state.get_completed_timings().push_back(region);
    }
  } catch (const std::exception& e) {
    std::cerr << "Error in end_region_by_type: " << e.what() << std::endl;
  }
}

// End region by id
void end_region_with_id(uint64_t expected_id) {
  try {
    auto& state = EnergyProfilerState::get_instance();
    std::lock_guard<std::mutex> lock(state.get_mutex());
    auto& active_regions = state.get_active_regions();
    if (active_regions.empty()) {
      std::cerr << "Warning: Attempting to end region with ID " << expected_id
                << " but no active regions found.\n";
      return;
    }
    auto it = std::find_if(active_regions.begin(), active_regions.end(),
                           [expected_id](const TimingInfo& region) {
                             return region.id == expected_id;
                           });
    if (it != active_regions.end()) {
      auto region = *it;
      active_regions.erase(it);
      region.end_time = std::chrono::high_resolution_clock::now();
      state.get_completed_timings().push_back(region);
    } else {
      std::cerr << "Warning: No active region found with ID " << expected_id
                << "\n";
    }
  } catch (const std::exception& e) {
    std::cerr << "Error in end_region_with_id: " << e.what() << std::endl;
  }
}

// Get all completed timings
std::vector<TimingInfo> get_all_timings() {
  try {
    auto& state = EnergyProfilerState::get_instance();
    std::lock_guard<std::mutex> lock(state.get_mutex());
    std::vector<TimingInfo> all_timings = state.get_completed_timings();
    std::sort(all_timings.begin(), all_timings.end(),
              [](const TimingInfo& a, const TimingInfo& b) {
                return a.start_time < b.start_time;
              });
    return all_timings;
  } catch (const std::exception& e) {
    std::cerr << "Error in get_all_timings: " << e.what() << std::endl;
    return {};
  }
}

}  // namespace EnergyProfiler
}  // namespace KokkosTools

extern "C" {

// Tool settings
void kokkosp_request_tool_settings(const uint32_t,
                                   Kokkos_Tools_ToolSettings* settings) {
  settings->requires_global_fencing = false;
  settings->padding[0]              = 0;
}

// Library init
void kokkosp_init_library(const int loadSeq, const uint64_t interfaceVer,
                          const uint32_t devInfoCount,
                          Kokkos_Profiling_KokkosPDeviceInfo* deviceInfo) {
  (void)devInfoCount;
  (void)deviceInfo;
  if (std::getenv("KOKKOS_TOOLS_ENERGY_VERBOSE")) {
    KokkosTools::EnergyProfiler::EnergyProfilerState::get_instance()
        .set_verbose_enabled(true);
  }
  printf(
      "Kokkos Energy Profiler: Initializing with load sequence %d and "
      "interface version %lu\n",
      loadSeq, interfaceVer);
  printf("Kokkos Energy Profiler: Library initialized\n");
}

// Library finalize
void kokkosp_finalize_library() {
  printf("Kokkos Energy Profiler: Finalizing library\n");
  std::string prefix = KokkosTools::EnergyProfiler::generate_prefix();
  auto all_timings   = KokkosTools::EnergyProfiler::get_all_timings();
  KokkosTools::EnergyProfiler::print_all_timings_summary(all_timings);
  KokkosTools::EnergyProfiler::export_all_timings_csv(
      all_timings, prefix + "_timing_data.csv");
  printf("Kokkos Energy Profiler: Library finalized\n");
}

// Begin parallel_for
void kokkosp_begin_parallel_for(const char* name, const uint32_t devID,
                                uint64_t* kID) {
  if (!name || !kID) {
    std::cerr << "Error: Invalid parameters in kokkosp_begin_parallel_for\n";
    return;
  }
  (void)devID;
  uint64_t new_id = KokkosTools::EnergyProfiler::generate_new_region_id();
  *kID            = new_id;
  KokkosTools::EnergyProfiler::start_region(
      name, KokkosTools::EnergyProfiler::RegionType::ParallelFor, *kID);
  KokkosTools::EnergyProfiler::log_verbose(
      "Kokkos Energy Profiler: Started parallel_for '%s' on device %u with ID "
      "%lu\n",
      name, devID, *kID);
}

// End parallel_for
void kokkosp_end_parallel_for(const uint64_t kID) {
  if (kID == 0) {
    std::cerr << "Error: Invalid kernel ID in kokkosp_end_parallel_for\n";
    return;
  }
  KokkosTools::EnergyProfiler::end_region_with_id(kID);
  KokkosTools::EnergyProfiler::log_verbose(
      "Kokkos Energy Profiler: Ended parallel_for with ID %lu\n", kID);
}

// Begin parallel_scan
void kokkosp_begin_parallel_scan(const char* name, const uint32_t devID,
                                 uint64_t* kID) {
  if (!name || !kID) {
    std::cerr << "Error: Invalid parameters in kokkosp_begin_parallel_scan\n";
    return;
  }
  (void)devID;
  uint64_t new_id = KokkosTools::EnergyProfiler::generate_new_region_id();
  *kID            = new_id;
  KokkosTools::EnergyProfiler::start_region(
      name, KokkosTools::EnergyProfiler::RegionType::ParallelScan, *kID);
  KokkosTools::EnergyProfiler::log_verbose(
      "Kokkos Energy Profiler: Started parallel_scan '%s' on device %u with ID "
      "%lu\n",
      name, devID, *kID);
}

// End parallel_scan
void kokkosp_end_parallel_scan(const uint64_t kID) {
  if (kID == 0) {
    std::cerr << "Error: Invalid kernel ID in kokkosp_end_parallel_scan\n";
    return;
  }
  KokkosTools::EnergyProfiler::end_region_with_id(kID);
  KokkosTools::EnergyProfiler::log_verbose(
      "Kokkos Energy Profiler: Ended parallel_scan with ID %lu\n", kID);
}

// Begin parallel_reduce
void kokkosp_begin_parallel_reduce(const char* name, const uint32_t devID,
                                   uint64_t* kID) {
  if (!name || !kID) {
    std::cerr << "Error: Invalid parameters in kokkosp_begin_parallel_reduce\n";
    return;
  }
  (void)devID;
  uint64_t new_id = KokkosTools::EnergyProfiler::generate_new_region_id();
  *kID            = new_id;
  KokkosTools::EnergyProfiler::start_region(
      name, KokkosTools::EnergyProfiler::RegionType::ParallelReduce, *kID);
  KokkosTools::EnergyProfiler::log_verbose(
      "Kokkos Energy Profiler: Started parallel_reduce '%s' on device %u with "
      "ID %lu\n",
      name, devID, *kID);
}

// End parallel_reduce
void kokkosp_end_parallel_reduce(const uint64_t kID) {
  if (kID == 0) {
    std::cerr << "Error: Invalid kernel ID in kokkosp_end_parallel_reduce\n";
    return;
  }
  KokkosTools::EnergyProfiler::end_region_with_id(kID);
  KokkosTools::EnergyProfiler::log_verbose(
      "Kokkos Energy Profiler: Ended parallel_reduce with ID %lu\n", kID);
}

// Push user region
void kokkosp_push_profile_region(char const* regionName) {
  if (!regionName) {
    std::cerr << "Error: Invalid region name in kokkosp_push_profile_region\n";
    return;
  }
  uint64_t new_id = KokkosTools::EnergyProfiler::generate_new_region_id();
  KokkosTools::EnergyProfiler::start_region(
      regionName, KokkosTools::EnergyProfiler::RegionType::UserRegion, new_id);
  KokkosTools::EnergyProfiler::log_verbose(
      "Kokkos Energy Profiler: Pushed profile region '%s'\n", regionName);
}

// Pop user region
void kokkosp_pop_profile_region() {
  KokkosTools::EnergyProfiler::end_region_by_type(
      KokkosTools::EnergyProfiler::RegionType::UserRegion);
  KokkosTools::EnergyProfiler::log_verbose(
      "Kokkos Energy Profiler: Popped profile region\n");
}

// Begin deep copy
void kokkosp_begin_deep_copy(Kokkos::Tools::SpaceHandle, const char* dst_name,
                             const void*, Kokkos::Tools::SpaceHandle,
                             const char* src_name, const void*, uint64_t size) {
  if (!dst_name || !src_name) {
    std::cerr << "Error: Invalid names in kokkosp_begin_deep_copy\n";
    return;
  }
  uint64_t new_id  = KokkosTools::EnergyProfiler::generate_new_region_id();
  std::string name = std::string(src_name) + " -> " + std::string(dst_name);
  KokkosTools::EnergyProfiler::start_region(
      name, KokkosTools::EnergyProfiler::RegionType::DeepCopy, new_id);
  KokkosTools::EnergyProfiler::log_verbose(
      "Kokkos Energy Profiler: Started deep copy from '%s' to '%s' (size: %lu "
      "bytes)\n",
      src_name, dst_name, size);
}

// End deep copy
void kokkosp_end_deep_copy() {
  KokkosTools::EnergyProfiler::end_region_by_type(
      KokkosTools::EnergyProfiler::RegionType::DeepCopy);
  KokkosTools::EnergyProfiler::log_verbose(
      "Kokkos Energy Profiler: Ended deep copy\n");
}

}  // extern "C"
