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

#ifndef KP_POWER_PROFILER_HPP
#define KP_POWER_PROFILER_HPP

#include <string>
#include <chrono>
#include <deque>
#include <nvml.h>

namespace KokkosTools {
namespace NVMLPowerProfiler {

enum class RegionType {
  Unknown,
  ParallelFor,
  ParallelReduce,
  ParallelScan,
  UserRegion
};

struct PowerDataPoint {
  int64_t timestamp_ns;
  double power_watts;
};

struct TimingInfo {
  std::string name;
  RegionType type;
  std::chrono::high_resolution_clock::time_point start_time;
  std::chrono::high_resolution_clock::time_point end_time;
  std::chrono::nanoseconds duration;
};

class DataManager {
 public:
  void add_power_data_point(int64_t timestamp, double power);
  void start_region(const std::string& name, RegionType type);
  void end_region();
  void write_power_data(const std::string& filename) const;
  void write_kernel_data(const std::string& filename) const;
  void write_region_data(const std::string& filename) const;

 private:
  const char* region_type_to_string(RegionType type) const;

  std::deque<PowerDataPoint> power_data_points;
  std::deque<TimingInfo> completed_kernels;
  std::deque<TimingInfo> completed_regions;
  std::deque<TimingInfo> active_regions;
};

}  // namespace NVMLPowerProfiler
}  // namespace KokkosTools

#endif  // KP_POWER_PROFILER_HPP
