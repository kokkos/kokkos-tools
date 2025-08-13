#include "provider_variorum.hpp"
#include <iostream>
#include <set>
#include <cstring>

VariorumProvider::VariorumProvider() : initialized_(false) {}

VariorumProvider::~VariorumProvider() {
  if (initialized_) {
    finalize();
  }
}

bool VariorumProvider::initialize() {
  if (initialized_) {
    return true;
  }

  // Initialize Variorum (in the original code, this was a no-op)
  // For now, we'll assume Variorum is available and working

  // Discover devices
  if (!discover_devices()) {
    return false;
  }

  initialized_ = true;
  std::cout << "Variorum Provider: Successfully initialized with "
            << device_ids_.size() << " device(s)" << std::endl;

  return true;
}

void VariorumProvider::finalize() {
  if (!initialized_) {
    return;
  }

  cleanup_devices();
  initialized_ = false;

  std::cout << "Variorum Provider: Finalized" << std::endl;
}

double VariorumProvider::get_total_power_usage() {
  if (!initialized_) {
    return 0.0;
  }

  double total_power_W                      = 0.0;
  std::map<uint32_t, double> power_readings = get_current_power_readings();

  for (const auto& [device_id, power] : power_readings) {
    if (power >= 0.0) {
      total_power_W += power;
    }
  }

  return total_power_W;
}

double VariorumProvider::get_device_power_usage(size_t device_index) {
  if (!initialized_ || device_index >= device_ids_.size()) {
    return -1.0;
  }

  uint32_t device_id                        = device_ids_[device_index];
  std::map<uint32_t, double> power_readings = get_current_power_readings();

  auto it = power_readings.find(device_id);
  if (it != power_readings.end()) {
    return it->second;
  }

  return -1.0;
}

size_t VariorumProvider::get_device_count() const { return device_ids_.size(); }

std::string VariorumProvider::get_device_name(size_t device_index) const {
  if (device_index >= device_names_.size()) {
    return "Unknown Device";
  }
  return device_names_[device_index];
}

bool VariorumProvider::discover_devices() {
  std::set<uint32_t> found_device_ids;
  unique_json_ptr root = get_variorum_json_data();

  if (!root) {
    std::cerr << "Variorum Provider: Failed to get JSON data from Variorum"
              << std::endl;
    return false;
  }

  // Parse JSON to find GPU devices
  json_t* host_obj = json_object_iter_value(json_object_iter(root.get()));
  if (!host_obj) {
    std::cerr << "Variorum Provider: No host object found in JSON" << std::endl;
    return false;
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
          } catch (const std::exception& e) {
            std::cerr << "Variorum Provider: Could not parse GPU ID from key: "
                      << s_key << " (" << e.what() << ")" << std::endl;
          }
        }
      }
    }
  }

  if (found_device_ids.empty()) {
    std::cerr << "Variorum Provider: No GPU devices found" << std::endl;
    return false;
  }

  // Store device information
  device_ids_.clear();
  device_names_.clear();

  for (uint32_t device_id : found_device_ids) {
    device_ids_.push_back(device_id);
    device_names_.push_back("GPU_" + std::to_string(device_id));

    std::cout << "Variorum Provider: Found device " << device_ids_.size() - 1
              << ": GPU_" << device_id << std::endl;
  }

  // Test initial power readings
  std::cout << "Variorum Provider: Testing initial power readings..."
            << std::endl;
  std::map<uint32_t, double> test_readings = get_current_power_readings();
  for (size_t i = 0; i < device_ids_.size(); ++i) {
    uint32_t device_id = device_ids_[i];
    auto it            = test_readings.find(device_id);
    if (it != test_readings.end()) {
      std::cout << "Variorum Provider: Device " << i
                << ": Current power usage: " << it->second << " W" << std::endl;
    } else {
      std::cout << "Variorum Provider: Device " << i << ": Power reading failed"
                << std::endl;
    }
  }

  return true;
}

void VariorumProvider::cleanup_devices() {
  device_ids_.clear();
  device_names_.clear();
}

VariorumProvider::unique_json_ptr VariorumProvider::get_variorum_json_data()
    const {
  char* json_string_c_raw = nullptr;
  int variorum_error      = variorum_get_power_json(&json_string_c_raw);

  if (variorum_error != 0) {
    std::cerr
        << "Variorum Provider: variorum_get_power_json() failed. Error code: "
        << variorum_error << std::endl;
    return unique_json_ptr(nullptr);
  }

  unique_cstring json_string_c(json_string_c_raw);

  if (!json_string_c) {
    std::cerr
        << "Variorum Provider: variorum_get_power_json() returned success "
           "but a null pointer."
        << std::endl;
    return unique_json_ptr(nullptr);
  }

  json_error_t error;
  json_t* root_ptr = json_loads(json_string_c.get(), 0, &error);

  if (!root_ptr) {
    std::cerr << "Variorum Provider: Failed to parse JSON: " << error.text
              << std::endl;
    return unique_json_ptr(nullptr);
  }

  return unique_json_ptr(root_ptr);
}

std::map<uint32_t, double> VariorumProvider::get_current_power_readings()
    const {
  std::map<uint32_t, double> readings;

  unique_json_ptr root = get_variorum_json_data();
  if (!root) {
    return readings;
  }

  json_t* host_obj = json_object_iter_value(json_object_iter(root.get()));
  if (!host_obj) {
    return readings;
  }

  json_t* socket_0 = json_object_get(host_obj, "socket_0");
  if (socket_0 && json_is_object(socket_0)) {
    json_t* power_gpu_watts = json_object_get(socket_0, "power_gpu_watts");
    if (power_gpu_watts && json_is_object(power_gpu_watts)) {
      for (uint32_t device_id : device_ids_) {
        std::string gpu_key = "GPU_" + std::to_string(device_id);
        json_t* power_value = json_object_get(power_gpu_watts, gpu_key.c_str());

        if (json_is_number(power_value)) {
          readings[device_id] = json_number_value(power_value);
        }
      }
    }
  }

  return readings;
}