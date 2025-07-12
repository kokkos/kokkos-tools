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
 * Kokkos NVML Power Profiler
 * Simple Kokkos profiling tool that monitors GPU power consumption using NVML
 * Polls nvmlDeviceGetPowerUsage() every 20ms in a background thread
 */

#include <iostream>
#include <deque>
#include <chrono>
#include <thread>
#include <mutex>
#include <atomic>
#include <memory>
#include <limits>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <condition_variable>
#include <inttypes.h>
#include <string>

#include <nvml.h>

#include "kp_core.hpp"
#include "kp_nvml_power_profiler.hpp"

namespace KokkosTools {
namespace NVMLPowerProfiler {

// State variables
std::atomic<bool> g_stop_requested(false);
std::deque<nvmlDevice_t> g_nvml_devices;
std::unique_ptr<std::thread> g_monitoring_thread;
std::condition_variable g_sleep_cv;
std::mutex g_sleep_mutex;
DataManager g_data_manager;
std::chrono::high_resolution_clock::time_point g_start_time;

// Get current time in nanoseconds since epoch
int64_t get_current_epoch_ns() {
  return std::chrono::duration_cast<std::chrono::nanoseconds>(
             std::chrono::high_resolution_clock::now().time_since_epoch())
      .count();
}

void nvml_power_monitoring_thread_func(std::chrono::milliseconds interval) {
  auto start_time           = std::chrono::high_resolution_clock::now();
  int64_t interval_count    = 0;
  int64_t delayed_intervals = 0;

  while (!g_stop_requested.load()) {
    auto next_check_time = start_time + ((interval_count + 1) * interval);

    {
      std::unique_lock<std::mutex> sleep_lock(g_sleep_mutex);
      if (g_sleep_cv.wait_until(sleep_lock, next_check_time,
                                [] { return g_stop_requested.load(); })) {
        break;
      }
    }

    auto current_time = std::chrono::high_resolution_clock::now();
    interval_count++;

    auto expected_time = start_time + (interval_count * interval);
    auto delay         = current_time - expected_time;

    if (delay > interval / 2) {
      delayed_intervals++;
    }

    double current_power_sum_W = 0.0;

    for (size_t i = 0; i < g_nvml_devices.size(); ++i) {
      if (g_nvml_devices[i] == nullptr) continue;

      unsigned int power_mW;
      nvmlReturn_t result =
          nvmlDeviceGetPowerUsage(g_nvml_devices[i], &power_mW);

      if (NVML_SUCCESS == result) {
        double current_power_W = static_cast<double>(power_mW) / 1000.0;
        current_power_sum_W += current_power_W;
      }
    }

    int64_t timestamp_ns = get_current_epoch_ns();
    g_data_manager.add_power_data_point(timestamp_ns, current_power_sum_W);

    if (interval_count % 100 == 0 && delayed_intervals > 0) {
      printf("KokkosP NVML Power: Timing info - %" PRId64 " intervals, %" PRId64
             " delayed (%.1f%%)\n",
             interval_count, delayed_intervals,
             (100.0 * delayed_intervals) / interval_count);
    }
  }

  if (interval_count > 0) {
    auto total_duration =
        std::chrono::high_resolution_clock::now() - start_time;
    auto actual_avg_interval = total_duration / interval_count;
    printf(
        "KokkosP NVML Power: Monitoring completed - %" PRId64
        " intervals, avg interval: %.1f ms (expected: %" PRId64 " ms)\n",
        interval_count,
        std::chrono::duration<double, std::milli>(actual_avg_interval).count(),
        static_cast<int64_t>(interval.count()));
  }
}

bool initialize_nvml() {
  nvmlReturn_t result = nvmlInit();
  if (NVML_SUCCESS != result) {
    std::cerr << "KokkosP NVML Power: Failed to initialize NVML: "
              << nvmlErrorString(result) << "\n";
    return false;
  }

  unsigned int device_count;
  result = nvmlDeviceGetCount(&device_count);
  if (NVML_SUCCESS != result) {
    std::cerr << "KokkosP NVML Power: Failed to get device count: "
              << nvmlErrorString(result) << "\n";
    nvmlShutdown();
    return false;
  }

  if (device_count == 0) {
    std::cerr << "KokkosP NVML Power: No NVIDIA devices found\n";
    nvmlShutdown();
    return false;
  }

  g_nvml_devices.resize(device_count);

  printf("KokkosP NVML Power: Found %u NVIDIA device(s)\n", device_count);

  for (unsigned int i = 0; i < device_count; ++i) {
    result = nvmlDeviceGetHandleByIndex(i, &g_nvml_devices[i]);
    if (NVML_SUCCESS != result) {
      std::cerr << "KokkosP NVML Power: Failed to get handle for device " << i
                << "\n";
      g_nvml_devices[i] = nullptr;
      continue;
    }

    char device_name[NVML_DEVICE_NAME_BUFFER_SIZE];
    result = nvmlDeviceGetName(g_nvml_devices[i], device_name,
                               NVML_DEVICE_NAME_BUFFER_SIZE);
    if (NVML_SUCCESS == result) {
      printf("KokkosP NVML Power: Device %u: %s\n", i, device_name);
    }

    nvmlEnableState_t pmmode;
    result = nvmlDeviceGetPowerManagementMode(g_nvml_devices[i], &pmmode);
    if (NVML_SUCCESS == result && pmmode == NVML_FEATURE_ENABLED) {
      printf("KokkosP NVML Power: Device %u: Power management enabled\n", i);
    } else {
      printf(
          "KokkosP NVML Power: Device %u: Power management disabled or not "
          "supported\n",
          i);
    }
  }

  return true;
}

void finalize_nvml() {
  if (!g_nvml_devices.empty()) {
    nvmlShutdown();
  }
  g_nvml_devices.clear();
}

// Kokkos profiler interface functions
void kokkosp_init_library(const int loadSeq, const uint64_t interfaceVer,
                          const uint32_t devInfoCount,
                          Kokkos_Profiling_KokkosPDeviceInfo* deviceInfo) {
  printf(
      "======================================================================"
      "\n");
  printf("KokkosP: NVML Power Profiler Initialized\n");
  printf("KokkosP: Sequence: %d, Interface Version: %llu, Devices: %u\n",
         loadSeq, (unsigned long long)interfaceVer, devInfoCount);
  printf(
      "======================================================================"
      "\n");

  g_start_time = std::chrono::high_resolution_clock::now();

  if (!initialize_nvml()) {
    printf(
        "KokkosP NVML Power: Failed to initialize NVML, power monitoring "
        "disabled\n");
    return;
  }

  int interval_ms = 20;
  if (const char* interval_env =
          std::getenv("KOKKOS_NVML_POWER_INTERVAL")) {
    try {
      interval_ms = std::stoi(interval_env);
      if (interval_ms <= 0) {
        interval_ms = 20;
        throw std::invalid_argument("Interval must be positive");
      }
      printf("KokkosP NVML Power: Using custom interval: %d ms\n", interval_ms);
    } catch (const std::exception& e) {
      printf("KokkosP NVML Power: Invalid interval value, using default 20ms\n");
    }
  } else {
    printf("KokkosP NVML Power: Using default interval: 20 ms\n");
  }

  g_stop_requested.store(false);
  
  g_monitoring_thread = std::make_unique<std::thread>(
      nvml_power_monitoring_thread_func,
      std::chrono::milliseconds(interval_ms));

  printf("KokkosP NVML Power: Power monitoring started\n");
}

void kokkosp_finalize_library() {
  auto end_time = std::chrono::high_resolution_clock::now();

  printf(
      "======================================================================"
      "\n");
  printf("KokkosP: NVML Power Profiler Finalization\n");

  if (g_monitoring_thread) {
    g_stop_requested.store(true);
    g_sleep_cv.notify_all();
    g_monitoring_thread->join();
    g_monitoring_thread.reset();
  }

  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
      end_time - g_start_time);
  double elapsed_seconds = duration.count() / 1000.0;

  printf("KokkosP NVML Power: Total execution time: %.3f seconds\n",
         elapsed_seconds);

  char hostname[256];
  gethostname(hostname, 256);
  int pid = (int)getpid();

  char power_filename[512];
  snprintf(power_filename, 512, "%s-%d-nvml-power.csv", hostname, pid);
  g_data_manager.write_power_data(power_filename);

  char kernels_filename[512];
  snprintf(kernels_filename, 512, "%s-%d-nvml-kernels.csv", hostname, pid);
  g_data_manager.write_kernel_data(kernels_filename);

  char regions_filename[512];
  snprintf(regions_filename, 512, "%s-%d-nvml-regions.csv", hostname, pid);
  g_data_manager.write_region_data(regions_filename);

  finalize_nvml();
  printf(
      "======================================================================"
      "\n");
}

void kokkosp_begin_parallel_for(const char* name, const uint32_t devID,
                                uint64_t* kID) {
  g_data_manager.start_region(name, RegionType::ParallelFor);
}

void kokkosp_end_parallel_for(const uint64_t kID) {
  g_data_manager.end_region();
}

void kokkosp_begin_parallel_scan(const char* name, const uint32_t devID,
                                 uint64_t* kID) {
  g_data_manager.start_region(name, RegionType::ParallelScan);
}

void kokkosp_end_parallel_scan(const uint64_t kID) {
  g_data_manager.end_region();
}

void kokkosp_begin_parallel_reduce(const char* name, const uint32_t devID,
                                   uint64_t* kID) {
  g_data_manager.start_region(name, RegionType::ParallelReduce);
}

void kokkosp_end_parallel_reduce(const uint64_t kID) {
  g_data_manager.end_region();
}

void kokkosp_push_profile_region(char const* regionName) {
  g_data_manager.start_region(regionName, RegionType::UserRegion);
}

void kokkosp_pop_profile_region() {
  g_data_manager.end_region();
}

Kokkos::Tools::Experimental::EventSet get_event_set() {
  Kokkos::Tools::Experimental::EventSet my_event_set;
  memset(&my_event_set, 0, sizeof(my_event_set));
  my_event_set.init                  = kokkosp_init_library;
  my_event_set.finalize              = kokkosp_finalize_library;
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

}  // namespace NVMLPowerProfiler
}  // namespace KokkosTools

extern "C" {

namespace impl = KokkosTools::NVMLPowerProfiler;

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

}  // extern "C"