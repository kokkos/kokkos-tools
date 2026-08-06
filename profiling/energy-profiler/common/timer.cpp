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

#include "timer.hpp"
#include <iostream>
#include <deque>
#include <cstdio>

// EnergyTiming implementations
EnergyTiming::EnergyTiming()
    : timing_id_(0), name_(""), region_type_(RegionType::Unknown) {
  start_time_ = std::chrono::high_resolution_clock::now();
}

EnergyTiming::EnergyTiming(uint64_t timing_id, RegionType type,
                           std::string name)
    : timing_id_(timing_id), name_(name), region_type_(type) {
  start_time_ = std::chrono::high_resolution_clock::now();
}

void EnergyTiming::end() {
  end_time_ = std::chrono::high_resolution_clock::now();
}

bool EnergyTiming::is_ended() const {
  return end_time_ !=
         std::chrono::time_point<std::chrono::high_resolution_clock>{};
}

uint64_t EnergyTiming::get_duration_ms() const {
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
      end_time_ - start_time_);
  return static_cast<uint64_t>(duration.count());
}

// EnergyTimer implementations
void EnergyTimer::start_timing(uint64_t timing_id, RegionType type,
                               std::string name) {
  timings_.emplace(timing_id, EnergyTiming(timing_id, type, name));
}

void EnergyTimer::end_timing(uint64_t timing_id) {
  auto it = timings_.find(timing_id);
  if (it != timings_.end()) {
    it->second.end();
  }
}

std::unordered_map<uint64_t, EnergyTiming>& EnergyTimer::get_timings() {
  return timings_;
}

namespace KokkosTools {
namespace Timer {

void export_kernels_csv(const std::deque<TimingInfo>& timings,
                        const std::string& filename) {
  if (timings.empty()) return;

  FILE* file = fopen(filename.c_str(), "w");
  if (file) {
    fprintf(file,
            "name,type,start_time_epoch_ms,end_time_epoch_ms,duration_ms\n");
    for (const auto& timing : timings) {
      auto start_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                          timing.start_time.time_since_epoch())
                          .count();
      auto end_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                        timing.end_time.time_since_epoch())
                        .count();
      auto duration_ms = timing.duration.count() / 1000000;

      std::string type;
      switch (timing.type) {
        case RegionType::ParallelFor: type = "parallel_for"; break;
        case RegionType::ParallelScan: type = "parallel_scan"; break;
        case RegionType::ParallelReduce: type = "parallel_reduce"; break;
        default: type = "unknown";
      }

      fprintf(file, "%s,%s,%ld,%ld,%ld\n", timing.name.c_str(), type.c_str(),
              start_ms, end_ms, duration_ms);
    }
    fclose(file);
    std::cout << "Timing data exported to " << filename << std::endl;
  } else {
    std::cerr << "ERROR: Unable to open file " << filename << " for writing.\n";
  }
}

void export_regions_csv(const std::deque<TimingInfo>& timings,
                        const std::string& filename) {
  if (timings.empty()) return;

  FILE* file = fopen(filename.c_str(), "w");
  if (file) {
    fprintf(file, "name,start_time_epoch_ms,end_time_epoch_ms,duration_ms\n");
    for (const auto& timing : timings) {
      auto start_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                          timing.start_time.time_since_epoch())
                          .count();
      auto end_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                        timing.end_time.time_since_epoch())
                        .count();
      auto duration_ms = timing.duration.count() / 1000000;

      fprintf(file, "%s,%ld,%ld,%ld\n", timing.name.c_str(), start_ms, end_ms,
              duration_ms);
    }
    fclose(file);
    std::cout << "Region data exported to " << filename << std::endl;
  } else {
    std::cerr << "ERROR: Unable to open file " << filename << " for writing.\n";
  }
}

void export_deepcopies_csv(const std::deque<TimingInfo>& timings,
                           const std::string& filename) {
  if (timings.empty()) return;

  FILE* file = fopen(filename.c_str(), "w");
  if (file) {
    fprintf(file, "name,start_time_epoch_ms,end_time_epoch_ms,duration_ms\n");
    for (const auto& timing : timings) {
      auto start_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                          timing.start_time.time_since_epoch())
                          .count();
      auto end_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                        timing.end_time.time_since_epoch())
                        .count();
      auto duration_ms = timing.duration.count() / 1000000;

      fprintf(file, "%s,%ld,%ld,%ld\n", timing.name.c_str(), start_ms, end_ms,
              duration_ms);
    }
    fclose(file);
    std::cout << "Deep copy data exported to " << filename << std::endl;
  } else {
    std::cerr << "ERROR: Unable to open file " << filename << " for writing.\n";
  }
}

void print_kernels_summary(const std::deque<TimingInfo>& kernels) {
  std::cout << "\n==== KERNELS ====\n";
  std::cout << "| Name                                 | Type           | "
               "Start(ms)         | End(ms)           | Duration (ms) |\n";
  std::cout << "|--------------------------------------|----------------|------"
               "-------------|-------------------|---------------|\n";
  for (const auto& info : kernels) {
    std::string type;
    switch (info.type) {
      case RegionType::ParallelFor: type = "parallel_for"; break;
      case RegionType::ParallelScan: type = "parallel_scan"; break;
      case RegionType::ParallelReduce: type = "parallel_reduce"; break;
      default: type = "unknown";
    }
    auto start_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                        info.start_time.time_since_epoch())
                        .count();
    auto end_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                      info.end_time.time_since_epoch())
                      .count();
    std::cout
        << "| " << info.name
        << std::string(38 - std::min<size_t>(info.name.size(), 38), ' ') << "| "
        << type << std::string(16 - type.size(), ' ') << "| " << start_ms
        << std::string(19 - std::to_string(start_ms).size(), ' ') << "| "
        << end_ms << std::string(19 - std::to_string(end_ms).size(), ' ')
        << "| " << (info.duration.count() / 1000000)
        << std::string(
               13 - std::to_string(info.duration.count() / 1000000).size(), ' ')
        << "|\n";
  }
}

void print_regions_summary(const std::deque<TimingInfo>& regions) {
  std::cout << "\n==== REGIONS ====\n";
  std::cout << "| Name                                 | Start(ms)         | "
               "End(ms)           | Duration (ms) |\n";
  std::cout << "|--------------------------------------|-------------------|---"
               "----------------|---------------|\n";
  for (const auto& info : regions) {
    auto start_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                        info.start_time.time_since_epoch())
                        .count();
    auto end_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                      info.end_time.time_since_epoch())
                      .count();
    std::cout << "| " << info.name
              << std::string(38 - std::min<size_t>(info.name.size(), 38), ' ')
              << "| " << start_ms
              << std::string(19 - std::to_string(start_ms).size(), ' ') << "| "
              << end_ms << std::string(19 - std::to_string(end_ms).size(), ' ')
              << "| " << (info.duration.count() / 1000000)
              << std::string(
                     13 -
                         std::to_string(info.duration.count() / 1000000).size(),
                     ' ')
              << "|\n";
  }
}

void print_deepcopies_summary(const std::deque<TimingInfo>& deepcopies) {
  std::cout << "\n==== DEEP COPIES ====\n";
  std::cout << "| Name                                 | Start(ms)         | "
               "End(ms)           | Duration (ms) |\n";
  std::cout << "|--------------------------------------|-------------------|---"
               "----------------|---------------|\n";
  for (const auto& info : deepcopies) {
    auto start_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                        info.start_time.time_since_epoch())
                        .count();
    auto end_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                      info.end_time.time_since_epoch())
                      .count();
    std::cout << "| " << info.name
              << std::string(38 - std::min<size_t>(info.name.size(), 38), ' ')
              << "| " << start_ms
              << std::string(19 - std::to_string(start_ms).size(), ' ') << "| "
              << end_ms << std::string(19 - std::to_string(end_ms).size(), ' ')
              << "| " << (info.duration.count() / 1000000)
              << std::string(
                     13 -
                         std::to_string(info.duration.count() / 1000000).size(),
                     ' ')
              << "|\n";
  }
}

}  // namespace Timer
}  // namespace KokkosTools
