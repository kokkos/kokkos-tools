//@HEADER
// ************************************************************************
//
//                        Kokkos Direct Power Profiler
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
 * @file kp_nvml_direct_power.cpp
 * @brief Kokkos Direct Power Profiler Tool using NVML.
 *
 * This tool leverages a background daemon to periodically sample GPU power
 * consumption using the NVML library's direct power measurement API. It starts
 * monitoring when the Kokkos library is initialized and prints a detailed power
 * profile upon finalization.
 */

#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <mutex>
#include <iomanip>
#include <cmath>
#include <fstream>
#include <memory>

#include "kp_core.hpp"
#include "../common/daemon.hpp"
#include "../provider/provider_nvml.hpp"
#include "../common/filename_prefix.hpp"
#include "../common/timer.hpp"
#include "../tools/kernel_timer_tool.hpp"

namespace KokkosTools {
namespace DirectPower {

// --- Configuration ---
// The interval in milliseconds for power sampling.
constexpr int SAMPLING_INTERVAL_MS = 20;

// --- Global State for the Profiler ---
static std::unique_ptr<Daemon> g_power_daemon;
static std::unique_ptr<NVMLProvider> g_nvml_provider;

// Timer tool for kernel and region timing
static KernelTimerTool g_timer;

// Structure to store a single power measurement with a timestamp per device.
struct DirectPowerSample {
  std::chrono::high_resolution_clock::time_point timestamp;
  std::vector<double> device_powers_watts;  // Power for each device
};

// Thread-safe storage for collected power samples.
static std::vector<DirectPowerSample> g_power_samples;
static std::mutex g_samples_mutex;
static std::chrono::high_resolution_clock::time_point g_start_time;
static size_t g_device_count = 0;

/**
 * @brief The function executed by the daemon thread to sample power.
 *
 * This function is called periodically. It fetches the current direct power
 * usage from each GPU device using the NVML provider and stores it with a
 * timestamp.
 */
void power_monitoring_tick() {
  if (!g_nvml_provider || !g_nvml_provider->is_initialized()) {
    return;
  }

  DirectPowerSample sample;
  sample.timestamp = std::chrono::high_resolution_clock::now();
  sample.device_powers_watts.reserve(g_device_count);

  // Collect power for each device
  for (size_t i = 0; i < g_device_count; ++i) {
    double power = g_nvml_provider->get_device_power_usage_direct(i);
    sample.device_powers_watts.push_back(power);
  }

  std::lock_guard<std::mutex> lock(g_samples_mutex);
  g_power_samples.push_back(std::move(sample));
}

/**
 * @brief Calculates statistics from the collected power samples.
 *
 * @param samples A constant reference to the vector of power samples.
 * @param device_index The device index to analyze.
 * @param[out] avg_power Average power consumption.
 * @param[out] min_power Minimum power consumption.
 * @param[out] max_power Maximum power consumption.
 * @param[out] total_energy Total energy consumed in Joules.
 */
void analyze_device_power_data(const std::vector<DirectPowerSample>& samples,
                               size_t device_index, double& avg_power,
                               double& min_power, double& max_power,
                               double& total_energy) {
  if (samples.empty() || device_index >= g_device_count) {
    avg_power = min_power = max_power = total_energy = 0.0;
    return;
  }

  // Find first valid sample for this device
  size_t first_valid = 0;
  while (first_valid < samples.size() &&
         (device_index >= samples[first_valid].device_powers_watts.size() ||
          samples[first_valid].device_powers_watts[device_index] < 0)) {
    first_valid++;
  }

  if (first_valid >= samples.size()) {
    avg_power = min_power = max_power = total_energy = 0.0;
    return;
  }

  min_power            = samples[first_valid].device_powers_watts[device_index];
  max_power            = samples[first_valid].device_powers_watts[device_index];
  double power_sum     = 0.0;
  size_t valid_samples = 0;
  total_energy         = 0.0;

  for (size_t i = first_valid; i < samples.size(); ++i) {
    if (device_index >= samples[i].device_powers_watts.size()) continue;

    const double power = samples[i].device_powers_watts[device_index];
    if (power < 0) continue;  // Skip invalid measurements

    power_sum += power;
    valid_samples++;
    if (power < min_power) min_power = power;
    if (power > max_power) max_power = power;

    // Energy = Power * Time. Time delta is from the previous sample.
    if (i > first_valid) {
      double time_delta_s = std::chrono::duration<double>(
                                samples[i].timestamp - samples[i - 1].timestamp)
                                .count();
      // Use previous sample's power for energy calculation
      if (device_index < samples[i - 1].device_powers_watts.size() &&
          samples[i - 1].device_powers_watts[device_index] >= 0) {
        total_energy +=
            samples[i - 1].device_powers_watts[device_index] * time_delta_s;
      }
    }
  }

  avg_power = valid_samples > 0 ? power_sum / valid_samples : 0.0;
}

void export_direct_power_data_csv(const std::string& filename) {
  std::ofstream file(filename);
  if (!file.is_open()) {
    std::cerr << "ERROR: Unable to open file " << filename << " for writing.\n";
    return;
  }

  // Write header
  file << "timestamp";
  for (size_t i = 0; i < g_device_count; ++i) {
    file << ",device_" << i << "_power_watts";
  }
  file << "\n";

  // Write data
  for (const auto& sample : g_power_samples) {
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                         sample.timestamp.time_since_epoch())
                         .count();
    file << timestamp;
    for (size_t i = 0; i < g_device_count; ++i) {
      if (i < sample.device_powers_watts.size()) {
        file << "," << sample.device_powers_watts[i];
      } else {
        file << ",-1";  // Invalid measurement
      }
    }
    file << "\n";
  }
  file.close();
  std::cout << "Direct power data exported to " << filename << std::endl;
}

// --- Kokkos Profiling Hooks ---

void kokkosp_init_library(const int loadSeq, const uint64_t interfaceVer,
                          const uint32_t devInfoCount,
                          Kokkos_Profiling_KokkosPDeviceInfo* deviceInfo) {
  std::cout << "Kokkos Direct Power Profiler: Initializing...\n";
  std::cout << "Sampling Interval: " << SAMPLING_INTERVAL_MS << " ms\n";

  // Initialize the timer tool
  g_timer.init_library(loadSeq, interfaceVer, devInfoCount, deviceInfo);

  g_nvml_provider = std::make_unique<NVMLProvider>();
  if (!g_nvml_provider->initialize()) {
    std::cerr << "ERROR: Failed to initialize NVML provider. Direct power "
                 "profiling disabled.\n";
    g_nvml_provider.reset();  // Release the provider
    return;
  }

  g_device_count = g_nvml_provider->get_device_count();
  std::cout << "SUCCESS: NVML provider initialized with " << g_device_count
            << " device(s).\n";

  // Print device information
  for (size_t i = 0; i < g_device_count; ++i) {
    std::cout << "  Device " << i << ": " << g_nvml_provider->get_device_name(i)
              << std::endl;
  }

  // Start the monitoring daemon
  g_power_daemon =
      std::make_unique<Daemon>(power_monitoring_tick, SAMPLING_INTERVAL_MS);
  g_start_time = std::chrono::high_resolution_clock::now();
  g_power_daemon->start();
  std::cout << "SUCCESS: Direct power monitoring daemon started.\n";
}

void kokkosp_finalize_library() {
  std::cout << "\nKokkos Direct Power Profiler: Finalizing...\n";

  if (g_power_daemon) {
    g_power_daemon->stop();
    std::cout << "SUCCESS: Direct power monitoring daemon stopped.\n";
  }

  // Finalize the timer
  g_timer.finalize_library();

  // Make a copy of the samples to avoid holding the lock during analysis
  std::vector<DirectPowerSample> samples_copy;
  {
    std::lock_guard<std::mutex> lock(g_samples_mutex);
    samples_copy = g_power_samples;
  }

  if (samples_copy.empty()) {
    std::cout << "No direct power samples collected.\n";
  } else {
    auto end_time = std::chrono::high_resolution_clock::now();
    auto total_duration_s =
        std::chrono::duration<double>(end_time - g_start_time).count();

    std::cout << "\n==== Direct Power Profile Summary ====\n";
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Total Monitoring Duration: " << total_duration_s << " s\n";
    std::cout << "Samples Collected:         " << samples_copy.size() << "\n";
    std::cout << "Number of Devices:         " << g_device_count << "\n";
    std::cout << "---------------------------------------\n";

    // Analyze each device separately
    for (size_t dev = 0; dev < g_device_count; ++dev) {
      double avg_power, min_power, max_power, total_energy;
      analyze_device_power_data(samples_copy, dev, avg_power, min_power,
                                max_power, total_energy);

      std::cout << "Device " << dev << " ("
                << g_nvml_provider->get_device_name(dev) << "):\n";
      std::cout << "  Average Power:           " << avg_power << " W\n";
      std::cout << "  Minimum Power:           " << min_power << " W\n";
      std::cout << "  Maximum Power:           " << max_power << " W\n";
      std::cout << "  Total Energy Consumed:   " << total_energy << " J\n";
      std::cout << "---------------------------------------\n";
    }

    std::string csv_filename =
        generate_prefix() + "_nvml_direct_power_samples.csv";
    std::cout << "Exporting direct power data to " << csv_filename << "...\n";
    export_direct_power_data_csv(csv_filename);
  }

  std::string prefix = generate_prefix();

  const auto& kernels = g_timer.get_kernel_timings();
  KokkosTools::Timer::print_kernels_summary(kernels);
  KokkosTools::Timer::export_kernels_csv(kernels, prefix + "_kernels.csv");

  const auto& regions = g_timer.get_region_timings();
  KokkosTools::Timer::print_regions_summary(regions);
  KokkosTools::Timer::export_regions_csv(regions, prefix + "_regions.csv");

  const auto& deepcopies = g_timer.get_deep_copy_timings();
  KokkosTools::Timer::print_deepcopies_summary(deepcopies);
  KokkosTools::Timer::export_deepcopies_csv(deepcopies,
                                            prefix + "_deepcopies.csv");

  if (g_nvml_provider) {
    g_nvml_provider->finalize();
    std::cout << "SUCCESS: NVML provider finalized.\n";
  }
}

// --- Hook Implementations with Timer Integration ---
void kokkosp_begin_parallel_for(const char* name, uint32_t devID,
                                uint64_t* kID) {
  g_timer.begin_parallel_for(name, devID, *kID);
}
void kokkosp_end_parallel_for(uint64_t kID) { g_timer.end_parallel_for(kID); }
void kokkosp_begin_parallel_scan(const char* name, uint32_t devID,
                                 uint64_t* kID) {
  g_timer.begin_parallel_scan(name, devID, kID);
}
void kokkosp_end_parallel_scan(uint64_t kID) { g_timer.end_parallel_scan(kID); }
void kokkosp_begin_parallel_reduce(const char* name, uint32_t devID,
                                   uint64_t* kID) {
  g_timer.begin_parallel_reduce(name, devID, kID);
}
void kokkosp_end_parallel_reduce(uint64_t kID) {
  g_timer.end_parallel_reduce(kID);
}
void kokkosp_push_profile_region(const char* regionName) {
  g_timer.push_profile_region(regionName);
}
void kokkosp_pop_profile_region() { g_timer.pop_profile_region(); }
void kokkosp_begin_deep_copy(Kokkos::Tools::SpaceHandle dst_handle,
                             const char* dst_name, const void* dst_ptr,
                             Kokkos::Tools::SpaceHandle src_handle,
                             const char* src_name, const void* src_ptr,
                             uint64_t size) {
  g_timer.begin_deep_copy(dst_handle, dst_name, dst_ptr, src_handle, src_name,
                          src_ptr, size);
}
void kokkosp_end_deep_copy() { g_timer.end_deep_copy(); }

}  // namespace DirectPower
}  // namespace KokkosTools

extern "C" {

namespace impl = KokkosTools::DirectPower;

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

}  // extern "C"
