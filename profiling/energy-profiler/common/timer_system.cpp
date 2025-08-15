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

#include "timer_system.hpp"

#include <cstdio>
#include <iostream>

namespace KokkosTools {
namespace Timer {

// === CSV Export Functions ===

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

// === Summary Printing Functions ===

namespace {
// Helper function to avoid code duplication in time calculations
std::pair<long, long> get_timing_ms(const TimingInfo& info) {
  auto start_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                      info.start_time.time_since_epoch())
                      .count();
  auto end_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                    info.end_time.time_since_epoch())
                    .count();
  return {start_ms, end_ms};
}

// Helper function to get duration in milliseconds
long get_duration_ms(const TimingInfo& info) {
  return info.duration.count() / 1000000;
}

// Helper function to format table columns with proper padding
std::string format_table_cell(const std::string& content, size_t width) {
  return content + std::string(width - std::min(content.size(), width), ' ');
}

}  // anonymous namespace

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

    auto [start_ms, end_ms] = get_timing_ms(info);
    auto duration_ms        = get_duration_ms(info);

    std::cout << "| " << format_table_cell(info.name, 38) << "| "
              << format_table_cell(type, 16) << "| "
              << format_table_cell(std::to_string(start_ms), 19) << "| "
              << format_table_cell(std::to_string(end_ms), 19) << "| "
              << format_table_cell(std::to_string(duration_ms), 13) << "|\n";
  }
}

void print_regions_summary(const std::deque<TimingInfo>& regions) {
  std::cout << "\n==== REGIONS ====\n";
  std::cout << "| Name                                 | Start(ms)         | "
               "End(ms)           | Duration (ms) |\n";
  std::cout << "|--------------------------------------|-------------------|---"
               "----------------|---------------|\n";

  for (const auto& info : regions) {
    auto [start_ms, end_ms] = get_timing_ms(info);
    auto duration_ms        = get_duration_ms(info);

    std::cout << "| " << format_table_cell(info.name, 38) << "| "
              << format_table_cell(std::to_string(start_ms), 19) << "| "
              << format_table_cell(std::to_string(end_ms), 19) << "| "
              << format_table_cell(std::to_string(duration_ms), 13) << "|\n";
  }
}

void print_deepcopies_summary(const std::deque<TimingInfo>& deepcopies) {
  std::cout << "\n==== DEEP COPIES ====\n";
  std::cout << "| Name                                 | Start(ms)         | "
               "End(ms)           | Duration (ms) |\n";
  std::cout << "|--------------------------------------|-------------------|---"
               "----------------|---------------|\n";

  for (const auto& info : deepcopies) {
    auto [start_ms, end_ms] = get_timing_ms(info);
    auto duration_ms        = get_duration_ms(info);

    std::cout << "| " << format_table_cell(info.name, 38) << "| "
              << format_table_cell(std::to_string(start_ms), 19) << "| "
              << format_table_cell(std::to_string(end_ms), 19) << "| "
              << format_table_cell(std::to_string(duration_ms), 13) << "|\n";
  }
}

// === KernelTimerTool Implementation ===

void KernelTimerTool::init_library(
    const int loadSeq, const uint64_t interfaceVer, const uint32_t devInfoCount,
    Kokkos_Profiling_KokkosPDeviceInfo* deviceInfo) {
  (void)devInfoCount;
  (void)deviceInfo;
  std::cout << "Kokkos Power Profiler: Initializing with load sequence "
            << loadSeq << " and interface version " << interfaceVer
            << std::endl;
  std::cout << "Kokkos Power Profiler: Library initialized" << std::endl;
}

void KernelTimerTool::finalize_library() {
  // Implementation is empty - resources cleaned up automatically by destructors
}

void KernelTimerTool::start_region(const std::string& name, RegionType type,
                                   uint64_t id) {
  TimingInfo region;
  region.name       = name;
  region.type       = type;
  region.start_time = std::chrono::high_resolution_clock::now();
  region.id         = id;
  active_regions_.push_back(region);
}

void KernelTimerTool::end_region() {
  if (!active_regions_.empty()) {
    auto region = active_regions_.back();
    active_regions_.pop_back();
    region.end_time = std::chrono::high_resolution_clock::now();
    region.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(
        region.end_time - region.start_time);

    // Categorize based on type
    switch (region.type) {
      case RegionType::UserRegion: completed_regions_.push_back(region); break;
      case RegionType::DeepCopy: completed_deepcopies_.push_back(region); break;
      default: completed_kernels_.push_back(region); break;
    }
  }
}

// Kokkos parallel operations
void KernelTimerTool::begin_parallel_for(const char* name, const uint32_t devID,
                                         uint64_t kID) {
  (void)devID;
  start_region(name, RegionType::ParallelFor, kID);
}

void KernelTimerTool::end_parallel_for(uint64_t kID) {
  (void)kID;
  end_region();
}

void KernelTimerTool::begin_parallel_scan(const char* name,
                                          const uint32_t devID, uint64_t* kID) {
  (void)devID;
  start_region(name, RegionType::ParallelScan, *kID);
}

void KernelTimerTool::end_parallel_scan(uint64_t kID) {
  (void)kID;
  end_region();
}

void KernelTimerTool::begin_parallel_reduce(const char* name,
                                            const uint32_t devID,
                                            uint64_t* kID) {
  (void)devID;
  start_region(name, RegionType::ParallelReduce, *kID);
}

void KernelTimerTool::end_parallel_reduce(uint64_t kID) {
  (void)kID;
  end_region();
}

void KernelTimerTool::begin_deep_copy(Kokkos::Tools::SpaceHandle dst_handle,
                                      const char* dst_name, const void* dst_ptr,
                                      Kokkos::Tools::SpaceHandle src_handle,
                                      const char* src_name, const void* src_ptr,
                                      uint64_t size) {
  (void)dst_handle;
  (void)src_handle;
  (void)src_name;
  (void)src_ptr;
  (void)size;
  start_region(dst_name, RegionType::DeepCopy,
               reinterpret_cast<uint64_t>(dst_ptr));
}

void KernelTimerTool::end_deep_copy() { end_region(); }

void KernelTimerTool::push_profile_region(const char* region_name) {
  start_region(region_name, RegionType::UserRegion, next_region_id_++);
}

void KernelTimerTool::pop_profile_region() { end_region(); }

// Getters
const std::deque<TimingInfo>& KernelTimerTool::get_kernel_timings() const {
  return completed_kernels_;
}

const std::deque<TimingInfo>& KernelTimerTool::get_region_timings() const {
  return completed_regions_;
}

const std::deque<TimingInfo>& KernelTimerTool::get_deep_copy_timings() const {
  return completed_deepcopies_;
}

}  // namespace Timer
}  // namespace KokkosTools
