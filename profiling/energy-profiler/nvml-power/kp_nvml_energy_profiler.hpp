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

#ifndef KP_NVML_ENERGY_PROFILER_HPP
#define KP_NVML_ENERGY_PROFILER_HPP

#include <nvml.h>
#include <string>
#include <vector>
#include <chrono>

namespace KokkosTools {
namespace NVMLEnergyProfiler {

enum class RegionType { ParallelFor, ParallelReduce, ParallelScan, UserRegion };

struct TimingEnergyInfo {
  std::string name;
  RegionType type;
  std::chrono::high_resolution_clock::time_point start_time;
  std::chrono::high_resolution_clock::time_point end_time;
  std::chrono::nanoseconds duration;
  unsigned long long start_energy_mj;  // millijoules at start
  unsigned long long end_energy_mj;    // millijoules at end
  unsigned long long delta_energy_mj;  // energy consumed during region
  double average_power_w;              // average power in Watts
};

class DataManager {
 private:
  std::vector<TimingEnergyInfo> completed_kernels;
  std::vector<TimingEnergyInfo> completed_regions;
  std::vector<TimingEnergyInfo> active_regions;
  nvmlDevice_t device;
  bool nvml_initialized;

  const char* region_type_to_string(RegionType type) const;
  unsigned long long get_current_energy_mj() const;

 public:
  DataManager();
  ~DataManager();

  bool initialize();
  void finalize();

  void start_region(const std::string& name, RegionType type);
  void end_region();

  void write_kernel_data(const std::string& filename) const;
  void write_region_data(const std::string& filename) const;
};

extern DataManager* g_data_manager;

}  // namespace NVMLEnergyProfiler
}  // namespace KokkosTools

#endif  // KP_NVML_ENERGY_PROFILER_HPP
