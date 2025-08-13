#include "provider_nvml.hpp"
#include <nvml.h>
#include <iostream>
#include <cstring>

NVMLProvider::NVMLProvider() : initialized_(false) {}

NVMLProvider::~NVMLProvider() {
  if (initialized_) {
    finalize();
  }
}

bool NVMLProvider::initialize() {
  if (initialized_) {
    return true;
  }

  // Initialize NVML
  nvmlReturn_t result = nvmlInit();
  if (NVML_SUCCESS != result) {
    std::cerr << "NVML Provider: Failed to initialize NVML: "
              << nvmlErrorString(result) << std::endl;
    return false;
  }

  // Discover devices
  if (!discover_devices()) {
    nvmlShutdown();
    return false;
  }

  initialized_ = true;
  std::cout << "NVML Provider: Successfully initialized with "
            << devices_.size() << " device(s)" << std::endl;

  return true;
}

void NVMLProvider::finalize() {
  if (!initialized_) {
    return;
  }

  cleanup_devices();
  nvmlShutdown();
  initialized_ = false;

  std::cout << "NVML Provider: Finalized" << std::endl;
}

double NVMLProvider::get_total_power_usage() {
  if (!initialized_) {
    return 0.0;
  }

  double total_power_W = 0.0;

  for (size_t i = 0; i < devices_.size(); ++i) {
    double device_power = get_device_power_usage(i);
    if (device_power >= 0.0) {
      total_power_W += device_power;
    }
  }

  return total_power_W;
}

double NVMLProvider::get_device_power_usage(size_t device_index) {
  if (!initialized_ || device_index >= devices_.size()) {
    return -1.0;
  }

  if (devices_[device_index] == nullptr) {
    return -1.0;
  }

  unsigned int power_mW = 0;
  nvmlReturn_t result =
      nvmlDeviceGetPowerUsage(devices_[device_index], &power_mW);

  if (result == NVML_SUCCESS) {
    // Convert from milliwatts to watts
    return static_cast<double>(power_mW) / 1000.0;
  } else {
    std::cerr << "NVML Provider: Failed to get power usage for device "
              << device_index << ": " << nvmlErrorString(result) << std::endl;
    return -1.0;
  }
}

double NVMLProvider::get_device_power_usage_direct(size_t device_index) {
  if (!initialized_ || device_index >= devices_.size()) {
    return -1.0;
  }

  if (devices_[device_index] == nullptr) {
    return -1.0;
  }

  nvmlFieldValue_t powerFieldNow;
  powerFieldNow.fieldId = NVML_FI_DEV_POWER_INSTANT;
  if (nvmlDeviceGetFieldValues(devices_[device_index], 1, &powerFieldNow) !=
      NVML_SUCCESS) {
    std::cerr << "NVML power read failed — stopping measurement.\n";
    return -1.0;
  }
  unsigned int pw = static_cast<unsigned int>(powerFieldNow.value.uiVal);
  // Convert from milliwatts to watts
  return static_cast<double>(pw) / 1000.0;
}

double NVMLProvider::get_current_energy_consumption(size_t device_index) {
  if (!initialized_ || device_index >= devices_.size()) {
    return -1.0;
  }

  if (devices_[device_index] == nullptr) {
    return -1.0;
  }

  unsigned long long energy_joules = 0;
  nvmlReturn_t result              = nvmlDeviceGetTotalEnergyConsumption(
      devices_[device_index], &energy_joules);

  if (result == NVML_SUCCESS) {
    // Convert from millijoules to joules
    return static_cast<double>(energy_joules) / 1000.0;
  } else {
    std::cerr << "NVML Provider: Failed to get energy consumption for device "
              << device_index << ": " << nvmlErrorString(result) << std::endl;
    return -1.0;
  }
}

size_t NVMLProvider::get_device_count() const { return devices_.size(); }

std::string NVMLProvider::get_device_name(size_t device_index) const {
  if (device_index >= device_names_.size()) {
    return "Unknown Device";
  }
  return device_names_[device_index];
}

bool NVMLProvider::discover_devices() {
  unsigned int device_count;
  nvmlReturn_t result = nvmlDeviceGetCount(&device_count);

  if (NVML_SUCCESS != result) {
    std::cerr << "NVML Provider: Failed to get device count: "
              << nvmlErrorString(result) << std::endl;
    return false;
  }

  if (device_count == 0) {
    std::cerr << "NVML Provider: No NVIDIA devices found" << std::endl;
    return false;
  }

  devices_.resize(device_count);
  device_names_.resize(device_count);

  std::cout << "NVML Provider: Found " << device_count << " NVIDIA device(s)"
            << std::endl;

  for (unsigned int i = 0; i < device_count; ++i) {
    result = nvmlDeviceGetHandleByIndex(i, &devices_[i]);
    if (NVML_SUCCESS != result) {
      std::cerr << "NVML Provider: Failed to get handle for device " << i
                << std::endl;
      devices_[i]      = nullptr;
      device_names_[i] = "Failed Device";
      continue;
    }

    // Get device name
    char device_name[NVML_DEVICE_NAME_BUFFER_SIZE];
    result = nvmlDeviceGetName(devices_[i], device_name,
                               NVML_DEVICE_NAME_BUFFER_SIZE);
    if (NVML_SUCCESS == result) {
      device_names_[i] = std::string(device_name);
      std::cout << "NVML Provider: Device " << i << ": " << device_name
                << std::endl;
    } else {
      device_names_[i] = "Unknown Device " + std::to_string(i);
    }

    // Check power management capability
    nvmlEnableState_t pmmode;
    result = nvmlDeviceGetPowerManagementMode(devices_[i], &pmmode);
    if (NVML_SUCCESS == result && pmmode == NVML_FEATURE_ENABLED) {
      std::cout << "NVML Provider: Device " << i << ": Power management enabled"
                << std::endl;
    } else {
      std::cout << "NVML Provider: Device " << i
                << ": Power management disabled or not supported" << std::endl;
    }

    // Test power usage reading
    unsigned int test_power_mW = 0;
    result = nvmlDeviceGetPowerUsage(devices_[i], &test_power_mW);
    if (NVML_SUCCESS == result) {
      std::cout << "NVML Provider: Device " << i
                << ": Current power usage: " << (test_power_mW / 1000.0) << " W"
                << std::endl;
    } else {
      std::cout << "NVML Provider: Device " << i
                << ": Power usage reading failed: " << nvmlErrorString(result)
                << std::endl;
    }
  }

  return true;
}

void NVMLProvider::cleanup_devices() {
  devices_.clear();
  device_names_.clear();
}