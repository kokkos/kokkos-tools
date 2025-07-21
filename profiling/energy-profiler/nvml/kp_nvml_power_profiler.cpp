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

#include "kp_nvml_power_profiler.hpp"
#include <cstdio>
#include <unistd.h>
#include <inttypes.h>

namespace KokkosTools {
namespace NVMLPowerProfiler {

void DataManager::add_power_data_point(int64_t timestamp, double power) {
  power_data_points.push_back({timestamp, power});
}

void DataManager::start_region(const std::string& name, RegionType type) {
  TimingInfo region;
  region.name       = name;
  region.type       = type;
  region.start_time = std::chrono::high_resolution_clock::now();
  active_regions.push_back(region);
}

void DataManager::end_region() {
  if (!active_regions.empty()) {
    auto& region    = active_regions.back();
    region.end_time = std::chrono::high_resolution_clock::now();
    region.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(
        region.end_time - region.start_time);

    if (region.type == RegionType::UserRegion) {
      completed_regions.push_back(region);
    } else {
      completed_kernels.push_back(region);
    }
    active_regions.pop_back();
  }
}

const char* DataManager::region_type_to_string(RegionType type) const {
  switch (type) {
    case RegionType::ParallelFor: return "parallel_for";
    case RegionType::ParallelReduce: return "parallel_reduce";
    case RegionType::ParallelScan: return "parallel_scan";
    case RegionType::UserRegion: return "user_region";
    default: return "unknown";
  }
}

void DataManager::write_power_data(const std::string& filename) const {
  FILE* csv_file = fopen(filename.c_str(), "w");
  if (csv_file) {
    fprintf(csv_file, "time_epoch_ns,power_w\n");
    for (const auto& point : power_data_points) {
      fprintf(csv_file, "%" PRId64 ",%.6f\n", point.timestamp_ns,
              point.power_watts);
    }
    fclose(csv_file);
    char cwd[256];
    getcwd(cwd, 256);
    printf("KokkosP NVML Power: Power CSV data written to %s/%s (%" PRIu64
           " data points)\n",
           cwd, filename.c_str(),
           static_cast<uint64_t>(power_data_points.size()));
  }
}

void DataManager::write_kernel_data(const std::string& filename) const {
  if (completed_kernels.empty()) return;

  FILE* file = fopen(filename.c_str(), "w");
  if (file) {
    fprintf(file,
            "name,type,start_time_epoch_ns,end_time_epoch_ns,duration_ns\n");
    for (const auto& region : completed_kernels) {
      auto start_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                          region.start_time.time_since_epoch())
                          .count();
      auto end_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                        region.end_time.time_since_epoch())
                        .count();
      fprintf(file, "%s,%s,%" PRId64 ",%" PRId64 ",%" PRId64 "\n",
              region.name.c_str(), region_type_to_string(region.type), start_ns,
              end_ns, (int64_t)region.duration.count());
    }
    fclose(file);
    char cwd[256];
    getcwd(cwd, 256);
    printf("KokkosP NVML Power: Kernel timing CSV written to %s/%s\n", cwd,
           filename.c_str());
  }
}

void DataManager::write_region_data(const std::string& filename) const {
  if (completed_regions.empty()) return;

  FILE* file = fopen(filename.c_str(), "w");
  if (file) {
    fprintf(file,
            "name,type,start_time_epoch_ns,end_time_epoch_ns,duration_ns\n");
    for (const auto& region : completed_regions) {
      auto start_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                          region.start_time.time_since_epoch())
                          .count();
      auto end_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                        region.end_time.time_since_epoch())
                        .count();
      fprintf(file, "%s,%s,%" PRId64 ",%" PRId64 ",%" PRId64 "\n",
              region.name.c_str(), region_type_to_string(region.type), start_ns,
              end_ns, (int64_t)region.duration.count());
    }
    fclose(file);
    char cwd[256];
    getcwd(cwd, 256);
    printf("KokkosP NVML Power: Region timing CSV written to %s/%s\n", cwd,
           filename.c_str());
  }
}

}  // namespace NVMLPowerProfiler
}  // namespace KokkosTools
