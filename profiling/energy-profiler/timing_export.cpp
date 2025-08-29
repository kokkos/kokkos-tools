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

#include "timing_export.hpp"
#include <fstream>
#include <iostream>
#include <iomanip>

namespace KokkosTools {
namespace EnergyProfiler {

// Constants for table formatting
const int COLUMN_WIDTH_CATEGORY = 10;
const int COLUMN_WIDTH_NAME     = 32;
const int COLUMN_WIDTH_TYPE     = 14;
const int COLUMN_WIDTH_TIME     = 17;
const int COLUMN_WIDTH_DURATION = 13;

void export_all_timings_csv(const std::vector<TimingInfo>& all_timings,
                            const std::string& filename) {
  std::ofstream file(filename);
  if (!file.is_open()) {
    std::cerr << "ERROR: Unable to open file " << filename << " for writing.\n";
    return;
  }
  file << "name,type,start_time_epoch_ms,end_time_epoch_ms,duration_ms\n";
  for (const auto& timing : all_timings) {
    auto start_ms        = get_epoch_ms(timing.start_time);
    auto end_ms          = get_epoch_ms(timing.end_time);
    auto duration_ms     = get_duration_ms(timing.start_time, timing.end_time);
    std::string type_str = region_type_to_string(timing.type);
    file << timing.name << "," << type_str << "," << start_ms << "," << end_ms
         << "," << duration_ms << "\n";
  }
  std::cout << "All timing data exported to " << filename << '\n';
}

std::string get_category_from_type(RegionType type) {
  switch (type) {
    case RegionType::UserRegion: return "REGION";
    case RegionType::DeepCopy: return "DEEPCOPY";
    case RegionType::ParallelFor:
    case RegionType::ParallelScan:
    case RegionType::ParallelReduce: return "KERNEL";
    default: return "OTHER";
  }
}

void print_all_timings_summary(const std::vector<TimingInfo>& all_timings) {
  std::cout << "\n==== TIMING SUMMARY ====\n";
  std::cout << "| Category   | Name                             | Type         "
               "  | Start (ms)        | End (ms)          | Duration (ms) |\n";
  std::cout << "|------------|----------------------------------|--------------"
               "--|-------------------|-------------------|---------------|\n";
  for (const auto& timing_info : all_timings) {
    auto start_ms = get_epoch_ms(timing_info.start_time);
    auto end_ms   = get_epoch_ms(timing_info.end_time);
    auto duration_ms =
        get_duration_ms(timing_info.start_time, timing_info.end_time);
    std::string type_str = region_type_to_string(timing_info.type);
    std::string category = get_category_from_type(timing_info.type);
    std::cout << "| " << std::setw(COLUMN_WIDTH_CATEGORY) << std::left
              << category << " | " << std::setw(COLUMN_WIDTH_NAME) << std::left
              << timing_info.name << " | " << std::setw(COLUMN_WIDTH_TYPE)
              << std::left << type_str << " | " << std::setw(COLUMN_WIDTH_TIME)
              << std::right << start_ms << " | " << std::setw(COLUMN_WIDTH_TIME)
              << std::right << end_ms << " | "
              << std::setw(COLUMN_WIDTH_DURATION) << std::right << duration_ms
              << " |\n";
  }
}

}  // namespace EnergyProfiler
}  // namespace KokkosTools
