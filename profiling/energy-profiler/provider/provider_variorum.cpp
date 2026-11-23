#include "provider_variorum.hpp"
#include <iostream>
#include <set>
#include <cstring>

using namespace KokkosTools::EnergyProfiler;

VariorumProvider::VariorumProvider() : is_initialized_(false) {}

VariorumProvider::~VariorumProvider() {
  if (is_initialized_) {
    finalize();
  }
}

bool VariorumProvider::initialize() {
  if (is_initialized_) {
    return true;
  }

  // Initialize Variorum (in the original code, this was a no-op)
  // For now, we'll assume Variorum is available and working

  // Discover devices
  bool discover_success = discover_devices();
  if (!discover_success) {
    return false;
  }

  is_initialized_ = true;
  log_message(LogLevel::INFO, COMPONENT_NAME,
              "Successfully initialized with " +
                  std::to_string(device_ids_.size()) + " device(s)");

  return true;
}

void VariorumProvider::finalize() {
  if (!is_initialized_) {
    return;
  }

  cleanup_devices();
  is_initialized_ = false;

  log_message(LogLevel::INFO, COMPONENT_NAME, "Finalized");
}

bool VariorumProvider::get_total_power_usage(double& power_watts) {
  if (!is_initialized_) {
    log_message(LogLevel::ERROR, COMPONENT_NAME, "Provider not initialized");
    return false;
  }

  double total_power_W                      = 0.0;
  std::map<uint32_t, double> power_readings = get_current_power_readings();

  for (const auto& [device_id, power] : power_readings) {
    if (power >= 0.0) {
      total_power_W += power;
    }
  }

  power_watts = total_power_W;
  return true;
}

bool VariorumProvider::get_device_power_usage(size_t device_index,
                                              double& power_watts) {
  if (!validate_device_index(device_index)) {
    return false;
  }

  uint32_t device_id                        = device_ids_[device_index];
  std::map<uint32_t, double> power_readings = get_current_power_readings();

  auto it = power_readings.find(device_id);
  if (it != power_readings.end()) {
    power_watts = it->second;
    return true;
  }

  log_message(
      LogLevel::ERROR, COMPONENT_NAME,
      "Failed to read power for device " + std::to_string(device_index));
  return false;
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
    log_message(LogLevel::ERROR, COMPONENT_NAME,
                "Failed to get JSON data from Variorum");
    return false;
  }

  // Parse JSON to find GPU devices
  json_t* host_obj = json_object_iter_value(json_object_iter(root.get()));
  if (!host_obj) {
    log_message(LogLevel::ERROR, COMPONENT_NAME,
                "No host object found in JSON");
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
            log_message(LogLevel::WARNING, COMPONENT_NAME,
                        "Could not parse GPU ID from key: " + s_key + " (" +
                            e.what() + ")");
          }
        }
      }
    }
  }

  if (found_device_ids.empty()) {
    log_message(LogLevel::ERROR, COMPONENT_NAME, "No GPU devices found");
    return false;
  }

  // Store device information
  device_ids_.clear();
  device_names_.clear();

  for (uint32_t device_id : found_device_ids) {
    device_ids_.push_back(device_id);
    device_names_.push_back("GPU_" + std::to_string(device_id));

    log_message(LogLevel::INFO, COMPONENT_NAME,
                "Found device " + std::to_string(device_ids_.size() - 1) +
                    ": GPU_" + std::to_string(device_id));
  }

  // Test initial power readings
  log_message(LogLevel::INFO, COMPONENT_NAME,
              "Testing initial power readings...");
  std::map<uint32_t, double> test_readings = get_current_power_readings();
  for (size_t i = 0; i < device_ids_.size(); ++i) {
    uint32_t device_id = device_ids_[i];
    auto it            = test_readings.find(device_id);
    if (it != test_readings.end()) {
      log_message(LogLevel::INFO, COMPONENT_NAME,
                  "Device " + std::to_string(i) + ": Current power usage: " +
                      std::to_string(it->second) + " W");
    } else {
      log_message(LogLevel::WARNING, COMPONENT_NAME,
                  "Device " + std::to_string(i) + ": Power reading failed");
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

bool VariorumProvider::validate_device_index(size_t device_index) const {
  if (!is_initialized_) {
    log_message(LogLevel::ERROR, COMPONENT_NAME, "Provider not initialized");
    return false;
  }

  if (device_index >= device_ids_.size()) {
    log_message(LogLevel::ERROR, COMPONENT_NAME,
                "Device index " + std::to_string(device_index) +
                    " is out of range (0-" +
                    std::to_string(device_ids_.size() - 1) + ")");
    return false;
  }

  return true;
}