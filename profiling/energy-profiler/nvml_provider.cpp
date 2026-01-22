//@HEADER
// ************************************************************************
//
//                        Kokkos Power Profiler
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
 * @file nvml_provider.cpp
 * @brief NVML Provider implementation for GPU power monitoring.
 */

#include "nvml_provider.hpp"
#include <iostream>
#include <nvml.h>

namespace KokkosTools {
namespace EnergyProfiler {

NVMLProvider::NVMLProvider() : is_initialized_(false) {}

NVMLProvider::~NVMLProvider() {
  if (is_initialized_) {
    finalize();
  }
}

bool NVMLProvider::initialize() {
  if (is_initialized_) return true;

  nvmlReturn_t nvml_result = nvmlInit();
  if (NVML_SUCCESS != nvml_result) {
    std::cerr << "[KokkosPowerProfiler] ERROR: Failed to initialize NVML: "
              << nvmlErrorString(nvml_result) << std::endl;
    return false;
  }

  if (!discover_devices()) {
    nvmlShutdown();
    return false;
  }

  is_initialized_ = true;
  return true;
}

void NVMLProvider::finalize() {
  if (!is_initialized_) return;
  devices_.clear();
  device_names_.clear();
  nvmlShutdown();
  is_initialized_ = false;
}

bool NVMLProvider::get_total_power_usage(double& power_watts) const {
  if (!is_initialized_) {
    std::cerr << "[KokkosPowerProfiler] ERROR: Provider not initialized"
              << std::endl;
    return false;
  }

  power_watts = 0.0;
  for (size_t i = 0; i < devices_.size(); ++i) {
    double device_power = 0.0;
    if (get_device_power_usage(i, device_power)) {
      power_watts += device_power;
    } else {
      // If one device fails, we continue with the others but log an error
      std::cerr << "[KokkosPowerProfiler] WARNING: Failed to get power usage "
                   "for device "
                << i << ", skipping it." << std::endl;
    }
  }
  return true;
}

bool NVMLProvider::get_device_power_usage(size_t device_index,
                                          double& power_watts) const {
  if (!validate_device_index(device_index)) return false;

  unsigned int power_mW = 0;
  nvmlReturn_t nvml_result =
      nvmlDeviceGetPowerUsage(devices_[device_index], &power_mW);

  if (nvml_result == NVML_SUCCESS) {
    power_watts = static_cast<double>(power_mW) / 1000.0;
    return true;
  } else {
    std::cerr
        << "[KokkosPowerProfiler] ERROR: Failed to get power usage for device "
        << device_index << ": " << nvmlErrorString(nvml_result) << std::endl;
    return false;
  }
}

std::string NVMLProvider::get_device_name(size_t device_index) const {
  if (device_index >= device_names_.size()) return "Unknown Device";
  return device_names_[device_index];
}

bool NVMLProvider::discover_devices() {
  unsigned int device_count;
  nvmlReturn_t nvml_result = nvmlDeviceGetCount(&device_count);

  if (NVML_SUCCESS != nvml_result) {
    std::cerr << "[KokkosPowerProfiler] ERROR: Failed to get device count: "
              << nvmlErrorString(nvml_result) << std::endl;
    return false;
  }

  if (device_count == 0) {
    std::cerr << "[KokkosPowerProfiler] ERROR: No NVIDIA devices found"
              << std::endl;
    return false;
  }

  devices_.resize(device_count);
  device_names_.resize(device_count);

  for (unsigned int i = 0; i < device_count; ++i) {
    nvml_result = nvmlDeviceGetHandleByIndex(i, &devices_[i]);
    if (NVML_SUCCESS != nvml_result) {
      std::cerr
          << "[KokkosPowerProfiler] WARNING: Failed to get handle for device "
          << i << std::endl;
      devices_[i]      = nullptr;
      device_names_[i] = "Failed Device";
      continue;
    }

    char device_name[NVML_DEVICE_NAME_BUFFER_SIZE];
    nvml_result = nvmlDeviceGetName(devices_[i], device_name,
                                    NVML_DEVICE_NAME_BUFFER_SIZE);
    if (NVML_SUCCESS == nvml_result) {
      device_names_[i] = std::string(device_name);
    } else {
      device_names_[i] = "Unknown Device " + std::to_string(i);
    }

    // Test power usage reading
    unsigned int test_power_mW = 0;
    nvml_result = nvmlDeviceGetPowerUsage(devices_[i], &test_power_mW);
    if (NVML_SUCCESS != nvml_result) {
      std::cerr << "[KokkosPowerProfiler] WARNING: Device " << i
                << ": Power usage reading failed: "
                << nvmlErrorString(nvml_result) << std::endl;
    }
  }
  return true;
}

bool NVMLProvider::validate_device_index(size_t device_index) const {
  if (!is_initialized_) {
    std::cerr << "[KokkosPowerProfiler] ERROR: Provider not initialized"
              << std::endl;
    return false;
  }
  if (device_index >= devices_.size()) {
    std::cerr << "[KokkosPowerProfiler] ERROR: Device index " << device_index
              << " out of range" << std::endl;
    return false;
  }
  return true;
}

}  // namespace EnergyProfiler
}  // namespace KokkosTools
