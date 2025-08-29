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

#include <string>
#include <chrono>
#include <vector>
#include <mutex>
#include "energy_profiler_constants.hpp"

namespace KokkosTools {
namespace EnergyProfiler {

// Helper functions for region type conversion
/// @brief Enumeration of region types
enum class RegionType {
  Unknown,
  ParallelFor,
  ParallelReduce,
  ParallelScan,
  DeepCopy,
  UserRegion
};

/// @brief Structure to hold timing information
struct TimingInfo {
  std::string name;
  RegionType type;
  std::chrono::high_resolution_clock::time_point start_time;
  std::chrono::high_resolution_clock::time_point end_time;
  uint64_t id = 0;
};

// Singleton class to manage global state
/// @brief Singleton class for managing profiler state
class EnergyProfilerState {
 public:
  static EnergyProfilerState& get_instance() {
    static EnergyProfilerState instance;
    return instance;
  }

  // Delete copy and move operations
  EnergyProfilerState(const EnergyProfilerState&)            = delete;
  EnergyProfilerState& operator=(const EnergyProfilerState&) = delete;
  EnergyProfilerState(EnergyProfilerState&&)                 = delete;
  EnergyProfilerState& operator=(EnergyProfilerState&&)      = delete;

  // Accessors for state
  std::mutex& get_mutex() { return mutex_; }
  std::vector<TimingInfo>& get_active_regions() { return active_regions_; }
  std::vector<TimingInfo>& get_completed_timings() {
    return completed_timings_;
  }
  uint64_t get_next_region_id() const { return next_region_id_; }
  bool get_verbose_enabled() const { return verbose_enabled_; }

  // Safe setters
  void increment_next_region_id() { next_region_id_++; }
  void set_verbose_enabled(bool enabled) { verbose_enabled_ = enabled; }

 private:
  EnergyProfilerState() : next_region_id_(1), verbose_enabled_(false) {}

  std::mutex mutex_;
  std::vector<TimingInfo> active_regions_;
  std::vector<TimingInfo> completed_timings_;
  uint64_t next_region_id_;
  bool verbose_enabled_;
};

// Internal functions for region management
void start_region(const std::string& name, RegionType type, uint64_t id);
void end_region_by_type(RegionType type_to_end);
void end_region_with_id(uint64_t expected_id);
uint64_t generate_new_region_id();
bool is_verbose_enabled();
void set_verbose_enabled(bool enabled);
void log_verbose(const char* format, ...);
std::vector<TimingInfo> get_all_timings();

// Filename prefix generation
/// @brief Generate a prefix for output files based on hostname and PID
std::string generate_prefix();

/// @brief Convert RegionType to string
std::string region_type_to_string(RegionType type);

// Helper functions for timing calculations
/// @brief Get epoch milliseconds from time point
template <typename TimePoint>
long get_epoch_ms(const TimePoint& time_point) {
  return std::chrono::duration_cast<std::chrono::milliseconds>(
             time_point.time_since_epoch())
      .count();
}

/// @brief Get duration in milliseconds between two time points
template <typename TimePoint>
long get_duration_ms(const TimePoint& start_time, const TimePoint& end_time) {
  auto duration = end_time - start_time;
  return std::chrono::duration_cast<std::chrono::milliseconds>(duration)
      .count();
}

}  // namespace EnergyProfiler
}  // namespace KokkosTools
