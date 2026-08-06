#pragma once

#include <vector>
#include <string>
#include <nvml.h>

/**
 * NVML Power Provider
 * Simplified power monitoring using nvmlDeviceGetPowerUsage()
 */
class NVMLProvider {
 public:
  NVMLProvider();
  ~NVMLProvider();

  // Initialize NVML and discover devices
  bool initialize();

  // Cleanup NVML resources
  void finalize();

  // Get current power consumption in Watts for all devices
  double get_total_power_usage();

  // Get power usage for a specific device
  double get_device_power_usage(size_t device_index);  // unit: Watts

  double get_device_power_usage_direct(size_t device_index);  // unit: Watts

  double get_current_energy_consumption(size_t device_index);  // unit: Joules

  // Get number of available devices
  size_t get_device_count() const;

  // Get device name
  std::string get_device_name(size_t device_index) const;

  // Check if provider is initialized
  bool is_initialized() const { return initialized_; }

 private:
  bool initialized_;
  std::vector<nvmlDevice_t> devices_;
  std::vector<std::string> device_names_;

  // Helper methods
  bool discover_devices();
  void cleanup_devices();
};