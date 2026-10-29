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

#include <chrono>
#include <cstdint>
#include <deque>
#include <string>

#include "kp_core.hpp"

namespace KokkosTools {
namespace EnergyProfiler {

// Forward declarations
enum class RegionType {
  Unknown,
  ParallelFor,
  ParallelReduce,
  ParallelScan,
  DeepCopy,
  UserRegion
};

// Data category for export and display
enum class DataCategory { Kernels, Regions, DeepCopies };

struct TimingInfo {
  std::string name;
  RegionType type;
  std::chrono::high_resolution_clock::time_point start_time;
  std::chrono::high_resolution_clock::time_point end_time;
  std::chrono::nanoseconds duration;
  uint64_t id = 0;
};

// CSV Export functions
void export_timings_csv(const std::deque<TimingInfo>& timings,
                        const std::string& filename, DataCategory category);

// Summary printing functions
void print_timings_summary(const std::deque<TimingInfo>& timings,
                           DataCategory category);

// Unified Timer Tool Class
class KernelTimerTool {
 public:
  KernelTimerTool()  = default;
  ~KernelTimerTool() = default;

  // Kokkos profiling interface implementation
  void init_library(const int loadSeq, const uint64_t interfaceVer,
                    const uint32_t devInfoCount,
                    Kokkos_Profiling_KokkosPDeviceInfo* deviceInfo);
  void finalize_library();

  void begin_parallel_for(const char* name, const uint32_t devID, uint64_t kID);
  void end_parallel_for(uint64_t kID);

  void begin_parallel_scan(const char* name, const uint32_t devID,
                           uint64_t* kID);
  void end_parallel_scan(uint64_t kID);

  void begin_parallel_reduce(const char* name, const uint32_t devID,
                             uint64_t* kID);
  void end_parallel_reduce(uint64_t kID);

  void begin_deep_copy(Kokkos::Tools::SpaceHandle dst_handle,
                       const char* dst_name, const void* dst_ptr,
                       Kokkos::Tools::SpaceHandle src_handle,
                       const char* src_name, const void* src_ptr,
                       uint64_t size);
  void end_deep_copy();

  void push_profile_region(const char* region_name);
  void pop_profile_region();

  // Stack-based timing for robust region/kernel tracking
  void start_region(const std::string& name, RegionType type, uint64_t id = 0);
  void end_region();

  // Getters for completed timings
  const std::deque<TimingInfo>& get_kernel_timings() const;
  const std::deque<TimingInfo>& get_region_timings() const;
  const std::deque<TimingInfo>& get_deep_copy_timings() const;

 private:
  std::deque<TimingInfo> active_regions_;
  std::deque<TimingInfo> completed_kernels_;
  std::deque<TimingInfo> completed_regions_;
  std::deque<TimingInfo> completed_deepcopies_;
  uint64_t next_region_id_ = 1;
};

}  // namespace EnergyProfiler
}  // namespace KokkosTools
