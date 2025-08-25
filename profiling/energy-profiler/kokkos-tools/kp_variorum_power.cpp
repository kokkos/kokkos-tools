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

/**
 * @file kp_variorum_power_tool.cpp
 * @brief Kokkos Power Profiler Tool using Variorum.
 *
 * This tool leverages a background daemon to periodically sample GPU power
 * consumption using the Variorum library via a provider interface. It starts
 * monitoring when the Kokkos library is initialized and writes detailed
 * power profiles to CSV files upon finalization.
 */

#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <mutex>
#include <iomanip>
#include <cmath>
#include <memory>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <inttypes.h>
#include <fstream>

#include "kp_core.hpp"
#include "../provider/provider_variorum.hpp"
#include "../common/daemon.hpp"
#include "../common/filename_prefix.hpp"
#include "../common/timer_system.hpp"
#include "../common/error_handling.hpp"

using namespace KokkosTools::EnergyProfiler;

namespace KokkosTools {
namespace VariorumPower {

using EnergyProfiler::log_message;
using EnergyProfiler::LogLevel;

EnergyProfiler::KernelTimerTool timer;

// --- Data Structures for Self-Contained Management ---

struct PowerDataPoint {
  int64_t timestamp_ns;
  double power_watts;
};

// --- Global State for the Profiler ---
static std::unique_ptr<Daemon> g_power_daemon;
static std::unique_ptr<VariorumProvider> g_variorum_provider;
static std::mutex g_data_mutex;  // Mutex for all data collections
static std::chrono::high_resolution_clock::time_point g_start_time;

// Data Collections
static std::vector<PowerDataPoint> g_power_data;

// --- Helper Functions ---

// Get current time in nanoseconds since epoch
int64_t get_current_epoch_ns() {
  return std::chrono::duration_cast<std::chrono::nanoseconds>(
             std::chrono::high_resolution_clock::now().time_since_epoch())
      .count();
}

void write_power_data_to_csv(const std::string& filename) {
  std::ofstream outfile(filename);
  if (!outfile.is_open()) {
    std::cerr << "KokkosP Variorum Power: Could not open file for writing: "
              << filename << "\n";
    return;
  }

  outfile << "timestamp_nanoseconds,power_watts\n";
  std::lock_guard<std::mutex> lock(g_data_mutex);
  for (const auto& point : g_power_data) {
    outfile << point.timestamp_ns << "," << std::fixed << std::setprecision(3)
            << point.power_watts << "\n";
  }
  printf("KokkosP Variorum Power: Wrote power data to %s\n", filename.c_str());
}

// --- Monitoring Function (for Daemon) ---

void variorum_power_monitoring_tick() {
  if (!g_variorum_provider || !g_variorum_provider->is_initialized()) {
    return;
  }

  double current_power_W = 0.0;
  bool power_success =
      g_variorum_provider->get_total_power_usage(current_power_W);
  if (!power_success) {
    log_message(LogLevel::WARNING, "VariorumPower",
                "Failed to get power reading");
    return;
  }

  int64_t timestamp_ns = get_current_epoch_ns();

  std::lock_guard<std::mutex> lock(g_data_mutex);
  g_power_data.push_back({timestamp_ns, current_power_W});
}

// --- Kokkos Profiling Hooks ---

void kokkosp_init_library(const int loadSeq, const uint64_t interfaceVer,
                          const uint32_t devInfoCount,
                          Kokkos_Profiling_KokkosPDeviceInfo* deviceInfo) {
  printf(
      "======================================================================"
      "\n");
  printf("KokkosP: Variorum Power Profiler Initialized\n");
  printf("KokkosP: Sequence: %d, Interface Version: %llu, Devices: %u\n",
         loadSeq, (unsigned long long)interfaceVer, devInfoCount);
  printf(
      "======================================================================"
      "\n");

  g_start_time = std::chrono::high_resolution_clock::now();

  g_variorum_provider = std::make_unique<VariorumProvider>();
  bool init_success   = g_variorum_provider->initialize();
  if (!init_success) {
    log_message(LogLevel::ERROR, "VariorumPower",
                "Failed to initialize Variorum. Power monitoring disabled");
    g_variorum_provider.reset();
    return;
  }

  int interval_ms = 20;
  if (const char* interval_env =
          std::getenv("KOKKOS_VARIORUM_POWER_INTERVAL")) {
    try {
      interval_ms = std::stoi(interval_env);
      if (interval_ms <= 0) {
        interval_ms = 20;
        throw std::invalid_argument("Interval must be positive");
      }
      printf("KokkosP Variorum Power: Using custom interval: %d ms\n",
             interval_ms);
    } catch (const std::exception& e) {
      printf(
          "KokkosP Variorum Power: Invalid interval value, using default "
          "20ms\n");
    }
  } else {
    printf("KokkosP Variorum Power: Using default interval: 20 ms\n");
  }

  g_power_daemon = std::make_unique<Daemon>(
      std::function<void()>(variorum_power_monitoring_tick), interval_ms);
  g_power_daemon->start();
  printf("KokkosP Variorum Power: Power monitoring started\n");

  timer.init_library(loadSeq, interfaceVer, devInfoCount, deviceInfo);
}

void kokkosp_finalize_library() {
  auto end_time = std::chrono::high_resolution_clock::now();

  printf(
      "======================================================================"
      "\n");
  printf("KokkosP: Variorum Power Profiler Finalization\n");

  if (g_power_daemon && g_power_daemon->is_running()) {
    g_power_daemon->stop();
    printf("KokkosP Variorum Power: Power monitoring stopped\n");
  }

  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
      end_time - g_start_time);
  double elapsed_seconds = duration.count() / 1000.0;

  printf("KokkosP Variorum Power: Total execution time: %.3f seconds\n",
         elapsed_seconds);

  auto power_filename = generate_prefix() + "_variorum_power_samples.csv";
  write_power_data_to_csv(power_filename);

  if (g_variorum_provider) {
    g_variorum_provider->finalize();
  }
  printf(
      "======================================================================"
      "\n");

  timer.finalize_library();

  std::string prefix = generate_prefix();

  const auto& kernels = timer.get_kernel_timings();
  KokkosTools::EnergyProfiler::print_timings_summary(
      kernels, KokkosTools::EnergyProfiler::DataCategory::Kernels);
  KokkosTools::EnergyProfiler::export_timings_csv(
      kernels, prefix + "_kernels.csv",
      KokkosTools::EnergyProfiler::DataCategory::Kernels);

  const auto& regions = timer.get_region_timings();
  KokkosTools::EnergyProfiler::print_timings_summary(
      regions, KokkosTools::EnergyProfiler::DataCategory::Regions);
  KokkosTools::EnergyProfiler::export_timings_csv(
      regions, prefix + "_regions.csv",
      KokkosTools::EnergyProfiler::DataCategory::Regions);

  const auto& deepcopies = timer.get_deep_copy_timings();
  KokkosTools::EnergyProfiler::print_timings_summary(
      deepcopies, KokkosTools::EnergyProfiler::DataCategory::DeepCopies);
  KokkosTools::EnergyProfiler::export_timings_csv(
      deepcopies, prefix + "_deepcopies.csv",
      KokkosTools::EnergyProfiler::DataCategory::DeepCopies);
}

void kokkosp_begin_parallel_for(const char* name, const uint32_t devID,
                                uint64_t* kID) {
  timer.begin_parallel_for(name, devID, *kID);
}

void kokkosp_end_parallel_for(const uint64_t kID) {
  timer.end_parallel_for(kID);
}

void kokkosp_begin_parallel_scan(const char* name, const uint32_t devID,
                                 uint64_t* kID) {
  timer.begin_parallel_scan(name, devID, kID);
}

void kokkosp_end_parallel_scan(const uint64_t kID) {
  timer.end_parallel_scan(kID);
}

void kokkosp_begin_parallel_reduce(const char* name, const uint32_t devID,
                                   uint64_t* kID) {
  timer.begin_parallel_reduce(name, devID, kID);
}

void kokkosp_end_parallel_reduce(const uint64_t kID) {
  timer.end_parallel_reduce(kID);
}

void kokkosp_push_profile_region(char const* regionName) {
  timer.push_profile_region(regionName);
}

void kokkosp_pop_profile_region() { timer.pop_profile_region(); }

void kokkosp_begin_deep_copy(Kokkos::Tools::SpaceHandle dst_handle,
                             const char* dst_name, const void* dst_ptr,
                             Kokkos::Tools::SpaceHandle src_handle,
                             const char* src_name, const void* src_ptr,
                             uint64_t size) {
  timer.begin_deep_copy(dst_handle, dst_name, dst_ptr, src_handle, src_name,
                        src_ptr, size);
}

void kokkosp_end_deep_copy() { timer.end_deep_copy(); }

// --- Event Set Configuration ---

Kokkos::Tools::Experimental::EventSet get_event_set() {
  Kokkos::Tools::Experimental::EventSet my_event_set;
  memset(&my_event_set, 0,
         sizeof(my_event_set));  // zero any pointers not set here
  my_event_set.init                  = kokkosp_init_library;
  my_event_set.finalize              = kokkosp_finalize_library;
  my_event_set.begin_deep_copy       = kokkosp_begin_deep_copy;
  my_event_set.end_deep_copy         = kokkosp_end_deep_copy;
  my_event_set.begin_parallel_for    = kokkosp_begin_parallel_for;
  my_event_set.begin_parallel_reduce = kokkosp_begin_parallel_reduce;
  my_event_set.begin_parallel_scan   = kokkosp_begin_parallel_scan;
  my_event_set.end_parallel_for      = kokkosp_end_parallel_for;
  my_event_set.end_parallel_reduce   = kokkosp_end_parallel_reduce;
  my_event_set.end_parallel_scan     = kokkosp_end_parallel_scan;
  my_event_set.push_region           = kokkosp_push_profile_region;
  my_event_set.pop_region            = kokkosp_pop_profile_region;
  return my_event_set;
}

}  // namespace VariorumPower
}  // namespace KokkosTools

extern "C" {

namespace impl = KokkosTools::VariorumPower;

EXPOSE_INIT(impl::kokkosp_init_library)
EXPOSE_FINALIZE(impl::kokkosp_finalize_library)
EXPOSE_BEGIN_PARALLEL_FOR(impl::kokkosp_begin_parallel_for)
EXPOSE_END_PARALLEL_FOR(impl::kokkosp_end_parallel_for)
EXPOSE_BEGIN_PARALLEL_SCAN(impl::kokkosp_begin_parallel_scan)
EXPOSE_END_PARALLEL_SCAN(impl::kokkosp_end_parallel_scan)
EXPOSE_BEGIN_PARALLEL_REDUCE(impl::kokkosp_begin_parallel_reduce)
EXPOSE_END_PARALLEL_REDUCE(impl::kokkosp_end_parallel_reduce)
EXPOSE_PUSH_REGION(impl::kokkosp_push_profile_region)
EXPOSE_POP_REGION(impl::kokkosp_pop_profile_region)
EXPOSE_BEGIN_DEEP_COPY(impl::kokkosp_begin_deep_copy)
EXPOSE_END_DEEP_COPY(impl::kokkosp_end_deep_copy)
}