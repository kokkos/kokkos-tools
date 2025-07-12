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

#include "variorum_power_profiler.hpp"
#include <iostream>
#include <fstream>
#include <set>
#include <cstdlib>
#include <iomanip>
#include <unistd.h>
#include <inttypes.h>
#include <deque>

namespace KokkosTools {
namespace PowerProfiler {

std::string kernel_type_to_string(KernelType type) {
  switch (type) {
    case KernelType::FOR: return "FOR";
    case KernelType::SCAN: return "SCAN";
    case KernelType::REDUCE: return "REDUCE";
    default: return "UNKNOWN";
  }
}

VariorumPowerProfiler::VariorumPowerProfiler() {
  if (const char* interval =
          std::getenv("KOKKOS_TOOLS_POWER_MONITOR_INTERVAL")) {
    try {
      auto interval_us  = std::stoul(interval);
      monitor_interval_ = std::chrono::microseconds(interval_us);
    } catch (const std::exception& e) {
      std::cerr
          << "PowerProfiler: Invalid monitor interval, using default 20ms\n";
    }
  }

  if (const char* output_path = std::getenv("KOKKOS_TOOLS_POWER_OUTPUT_PATH")) {
    output_file_path_ = output_path;
  }
}

VariorumPowerProfiler::~VariorumPowerProfiler() {
  if (initialized_) {
    finalize();
  }
}

bool VariorumPowerProfiler::initialize() {
  if (initialized_) {
    return true;
  }

  if (!initialize_variorum()) {
    std::cerr << "PowerProfiler: Failed to initialize Variorum\n";
    return false;
  }

  available_devices_ = get_available_devices();
  if (available_devices_.empty()) {
    std::cerr << "PowerProfiler: No energy monitoring devices found\n";
    return false;
  }

  start_monitoring();
  initialized_ = true;

  std::cout << "PowerProfiler: Initialized with " << available_devices_.size()
            << " devices, monitoring interval: " << monitor_interval_.count()
            << "μs\n";

  return true;
}

void VariorumPowerProfiler::finalize() {
  if (!initialized_) {
    return;
  }

  stop_monitoring();
  generate_outputs();
  initialized_ = false;
}

bool VariorumPowerProfiler::initialize_variorum() { return true; }

VariorumPowerProfiler::unique_json_ptr
VariorumPowerProfiler::get_variorum_json_data() const {
  char* json_string_c_raw = nullptr;
  int variorum_error      = variorum_get_power_json(&json_string_c_raw);

  if (variorum_error != 0) {
    std::cerr << "PowerProfiler: variorum_get_power_json() failed. Error code: "
              << variorum_error << "\n";
    return unique_json_ptr(nullptr);
  }

  unique_cstring json_string_c(json_string_c_raw);

  if (!json_string_c) {
    std::cerr << "PowerProfiler: variorum_get_power_json() returned success "
                 "but a null pointer.\n";
    return unique_json_ptr(nullptr);
  }

  json_error_t error;
  json_t* root_ptr = json_loads(json_string_c.get(), 0, &error);

  if (!root_ptr) {
    std::cerr << "PowerProfiler: Failed to parse JSON: " << error.text << "\n";
    return unique_json_ptr(nullptr);
  }

  return unique_json_ptr(root_ptr);
}

std::deque<uint32_t> VariorumPowerProfiler::get_available_devices() const {
  std::set<uint32_t> found_device_ids;
  unique_json_ptr root = get_variorum_json_data();

  if (!root) {
    return {};
  }

  json_t* host_obj = json_object_iter_value(json_object_iter(root.get()));
  if (!host_obj) {
    return {};
  }

  json_t* socket_0 = json_object_get(host_obj, "socket_0");
  if (socket_0 && json_is_object(socket_0)) {
    json_t* power_gpu_watts = json_object_get(socket_0, "power_gpu_watts");
    if (power_gpu_watts && json_is_object(power_gpu_watts)) {
      const char* key;
      json_t* value;
      json_object_foreach(power_gpu_watts, key, value) {
        std::string s_key(key);
        if (s_key.length() > 4 && s_key.substr(0, 4) == "GPU_") {
          try {
            uint32_t device_id = std::stoul(s_key.substr(4));
            found_device_ids.insert(device_id);
          } catch (const std::invalid_argument& e) {
            std::cerr << "PowerProfiler: Could not parse GPU ID from key: "
                      << s_key << " (" << e.what() << ")\n";
          } catch (const std::out_of_range& e) {
            std::cerr << "PowerProfiler: GPU ID out of range from key: "
                      << s_key << " (" << e.what() << ")\n";
          }
        }
      }
    }
  }

  return std::deque<uint32_t>(found_device_ids.begin(), found_device_ids.end());
}

EnergyReading VariorumPowerProfiler::get_current_energy_reading() const {
  EnergyReading reading;
  reading.timestamp       = get_current_time();
  reading.epoch_timestamp = get_current_epoch_time();

  unique_json_ptr root = get_variorum_json_data();
  if (!root) {
    return reading;
  }

  json_t* host_obj = json_object_iter_value(json_object_iter(root.get()));
  if (!host_obj) {
    return reading;
  }

  json_t* socket_0 = json_object_get(host_obj, "socket_0");
  if (socket_0 && json_is_object(socket_0)) {
    json_t* power_gpu_watts = json_object_get(socket_0, "power_gpu_watts");
    if (power_gpu_watts && json_is_object(power_gpu_watts)) {
      for (uint32_t device_id : available_devices_) {
        std::string gpu_key = "GPU_" + std::to_string(device_id);
        json_t* power_value = json_object_get(power_gpu_watts, gpu_key.c_str());

        if (json_is_number(power_value)) {
          reading.gpu_power_watts[device_id] = json_number_value(power_value);
        }
      }
    }
  }

  return reading;
}

void VariorumPowerProfiler::start_monitoring() {
  monitoring_active_ = true;
  monitoring_thread_ = std::make_unique<std::thread>(
      &VariorumPowerProfiler::monitoring_thread_function, this);
}

void VariorumPowerProfiler::stop_monitoring() {
  monitoring_active_ = false;
  if (monitoring_thread_ && monitoring_thread_->joinable()) {
    monitoring_thread_->join();
  }
}

void VariorumPowerProfiler::monitoring_thread_function() {
  while (monitoring_active_) {
    EnergyReading reading = get_current_energy_reading();
    energy_readings_.push_back(reading);
    std::this_thread::sleep_for(monitor_interval_);
  }
}

void VariorumPowerProfiler::begin_kernel(uint64_t kernel_id,
                                         const std::string& name,
                                         KernelType type) {
  KernelTiming timing;
  timing.kernel_id        = kernel_id;
  timing.name             = name;
  timing.type             = type;
  timing.start_time       = get_current_time();
  timing.epoch_start_time = get_current_epoch_time();

  active_kernels_[kernel_id] = timing;
}

void VariorumPowerProfiler::end_kernel(uint64_t kernel_id) {
  auto it = active_kernels_.find(kernel_id);
  if (it != active_kernels_.end()) {
    it->second.end_time       = get_current_time();
    it->second.epoch_end_time = get_current_epoch_time();
    it->second.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(
        it->second.end_time - it->second.start_time);

    completed_kernels_.push_back(it->second);
    active_kernels_.erase(it);
  }
}

void VariorumPowerProfiler::push_region(const std::string& name,
                                        const std::string& type) {
  RegionTiming region;
  region.name             = name;
  region.type             = type.empty() ? "DEFAULT" : type;
  region.start_time       = get_current_time();
  region.epoch_start_time = get_current_epoch_time();

  active_regions_.push_back(region);
}

void VariorumPowerProfiler::pop_region() {
  if (!active_regions_.empty()) {
    auto& region          = active_regions_.back();
    region.end_time       = get_current_time();
    region.epoch_end_time = get_current_epoch_time();
    region.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(
        region.end_time - region.start_time);

    completed_regions_.push_back(region);
    active_regions_.pop_back();
  }
}

std::chrono::time_point<std::chrono::steady_clock>
VariorumPowerProfiler::get_current_time() const {
  return std::chrono::steady_clock::now();
}

std::chrono::system_clock::time_point
VariorumPowerProfiler::get_current_epoch_time() const {
  return std::chrono::system_clock::now();
}

void VariorumPowerProfiler::generate_outputs() { output_to_csv(); }

void VariorumPowerProfiler::output_to_csv() const {
  char hostname[256];
  gethostname(hostname, 256);
  int pid = (int)getpid();

  // Create power data CSV file
  char power_filename[512];
  snprintf(power_filename, 512, "%s-%d-power.csv", hostname, pid);
  std::ofstream power_csv(power_filename);
  if (power_csv.is_open()) {
    power_csv << "timestamp_epoch_ns,device_id,power_watts\n";
    for (const auto& reading : energy_readings_) {
      auto epoch_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                          reading.epoch_timestamp.time_since_epoch())
                          .count();

      for (const auto& [device_id, power] : reading.gpu_power_watts) {
        power_csv << epoch_ns << "," << device_id << "," << power << "\n";
      }
    }
    power_csv.close();
    std::cout << "Power measurements written to " << power_filename << " ("
              << energy_readings_.size() << " readings)\n";
  }

  // Create regions CSV file
  char regions_filename[512];
  snprintf(regions_filename, 512, "%s-%d-regions.csv", hostname, pid);
  std::ofstream regions_csv(regions_filename);
  if (regions_csv.is_open() && !completed_regions_.empty()) {
    regions_csv << "name,type,start_timestamp_epoch_ns,end_timestamp_epoch_ns,"
                   "duration_ns\n";
    for (const auto& region : completed_regions_) {
      auto start_epoch_ns =
          std::chrono::duration_cast<std::chrono::nanoseconds>(
              region.epoch_start_time.time_since_epoch())
              .count();
      auto end_epoch_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                              region.epoch_end_time.time_since_epoch())
                              .count();

      regions_csv << "\"" << region.name << "\"," << "\"" << region.type
                  << "\"," << start_epoch_ns << "," << end_epoch_ns << ","
                  << region.duration.count() << "\n";
    }
    regions_csv.close();
    std::cout << "Region timings written to " << regions_filename << " ("
              << completed_regions_.size() << " regions)\n";
  }

  // Create kernels CSV file (can be considered part of regions with specific
  // type)
  if (!completed_kernels_.empty()) {
    char kernels_filename[512];
    snprintf(kernels_filename, 512, "%s-%d-kernels.csv", hostname, pid);
    std::ofstream kernels_csv(kernels_filename);
    if (kernels_csv.is_open()) {
      kernels_csv << "name,type,start_timestamp_epoch_ns,end_timestamp_epoch_"
                     "ns,duration_ns,kernel_id\n";
      for (const auto& kernel : completed_kernels_) {
        auto start_epoch_ns =
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                kernel.epoch_start_time.time_since_epoch())
                .count();
        auto end_epoch_ns =
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                kernel.epoch_end_time.time_since_epoch())
                .count();

        kernels_csv << "\"" << kernel.name << "\"," << "\""
                    << kernel_type_to_string(kernel.type) << "\","
                    << start_epoch_ns << "," << end_epoch_ns << ","
                    << kernel.duration.count() << "," << kernel.kernel_id
                    << "\n";
      }
      kernels_csv.close();
      std::cout << "Kernel timings written to " << kernels_filename << " ("
                << completed_kernels_.size() << " kernels)\n";
    }
  }
}

}  // namespace PowerProfiler
}  // namespace KokkosTools