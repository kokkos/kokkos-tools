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
#include "energy_profiler_constants.hpp"

namespace KokkosTools {
namespace EnergyProfiler {

void export_all_timings_csv(const std::vector<TimingInfo>& all_timings,
                            const std::string& filename) {
  std::ofstream file(filename);
  if (!file.is_open()) {
    log_error("Unable to open file " + filename + " for writing.");
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

void print_all_timings_summary(std::ostream& os,
                               std::vector<TimingInfo>::const_iterator begin,
                               std::vector<TimingInfo>::const_iterator end) {
  os << "\n==== TIMING SUMMARY ====\n";
  os << "| Category   | Name                             | Type         "
        "  | Start (ms)        | End (ms)          | Duration (ms) |\n";
  os << "|------------|----------------------------------|--------------"
        "--|-------------------|-------------------|---------------|\n";
  for (auto it = begin; it != end; ++it) {
    const auto& timing_info = *it;
    auto start_ms           = get_epoch_ms(timing_info.start_time);
    auto end_ms             = get_epoch_ms(timing_info.end_time);
    auto duration_ms =
        get_duration_ms(timing_info.start_time, timing_info.end_time);
    std::string type_str = region_type_to_string(timing_info.type);
    std::string category = get_category_from_type(timing_info.type);
    os << "| " << std::setw(COLUMN_WIDTH_CATEGORY) << std::left << category
       << " | " << std::setw(COLUMN_WIDTH_NAME) << std::left << timing_info.name
       << " | " << std::setw(COLUMN_WIDTH_TYPE) << std::left << type_str
       << " | " << std::setw(COLUMN_WIDTH_TIME) << std::right << start_ms
       << " | " << std::setw(COLUMN_WIDTH_TIME) << std::right << end_ms << " | "
       << std::setw(COLUMN_WIDTH_DURATION) << std::right << duration_ms
       << " |\n";
  }
}

void export_power_data_csv(const std::vector<PowerSample>& samples,
                           const std::string& filename) {
  std::ofstream file(filename);
  if (!file.is_open()) {
    log_error("[KokkosPowerProfiler] Unable to open file " + filename +
              " for writing.");
    return;
  }
  file << "timestamp_epoch_ms,power_watts\n";
  for (const auto& sample : samples) {
    auto timestamp_ms = get_epoch_ms(sample.timestamp);
    file << timestamp_ms << "," << std::fixed << std::setprecision(3)
         << sample.power_watts << "\n";
  }
  file.close();
  std::cout << "[KokkosPowerProfiler] INFO: Power data exported to " << filename
            << std::endl;
}

void print_power_summary(const std::vector<PowerSample>& samples,
                         const std::string& device_name) {
  if (samples.empty()) {
    std::cout << "[KokkosPowerProfiler] INFO: No power samples collected.\n";
    return;
  }

  // Calculate statistics
  double min_power = samples[0].power_watts;
  double max_power = samples[0].power_watts;
  double power_sum = 0.0;

  for (const auto& sample : samples) {
    double power = sample.power_watts;
    power_sum += power;
    if (power < min_power) min_power = power;
    if (power > max_power) max_power = power;
  }

  double avg_power = power_sum / samples.size();

  // Calculate duration
  auto start_time = samples.front().timestamp;
  auto end_time   = samples.back().timestamp;
  auto duration_s =
      std::chrono::duration<double>(end_time - start_time).count();

  std::cout << "\n==== POWER PROFILE SUMMARY ====\n";
  std::cout << std::fixed << std::setprecision(2);
  std::cout << "Device:                    " << device_name << "\n";
  std::cout << "Total Monitoring Duration: " << duration_s << " s\n";
  std::cout << "Samples Collected:         " << samples.size() << "\n";
  std::cout << "---------------------------------\n";
  std::cout << "Average Power:             " << avg_power << " W\n";
  std::cout << "Minimum Power:             " << min_power << " W\n";
  std::cout << "Maximum Power:             " << max_power << " W\n";
  std::cout << "===============================\n";
}

}  // namespace EnergyProfiler
}  // namespace KokkosTools
