#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include "../common/error_handling.hpp"

extern "C" {
#include <variorum.h>
#include <jansson.h>
}

namespace KokkosTools {
namespace EnergyProfiler {

/**
 * @brief Variorum Provider with improved error handling
 *
 * Provides power monitoring using Variorum APIs with
 * consistent error reporting and status checking.
 */
class VariorumProvider {
 public:
  VariorumProvider();
  ~VariorumProvider();

  // Core functionality
  bool initialize();
  void finalize();
  bool is_initialized() const { return is_initialized_; }

  // Power monitoring
  bool get_total_power_usage(double& power_watts);
  bool get_device_power_usage(size_t device_index, double& power_watts);

  // Device information
  size_t get_device_count() const;
  std::string get_device_name(size_t device_index) const;

 private:
  static constexpr const char* COMPONENT_NAME = "VariorumProvider";

  struct JsonDeleter {
    void operator()(json_t* json) const {
      if (json) json_decref(json);
    }
  };
  using unique_json_ptr = std::unique_ptr<json_t, JsonDeleter>;

  struct CFreeDeleter {
    void operator()(char* ptr) const {
      if (ptr) free(ptr);
    }
  };
  using unique_cstring = std::unique_ptr<char, CFreeDeleter>;

  // Internal methods
  bool discover_devices();
  void cleanup_devices();
  unique_json_ptr get_variorum_json_data() const;
  std::map<uint32_t, double> get_current_power_readings() const;
  bool validate_device_index(size_t device_index) const;

  // Member variables
  bool is_initialized_;
  std::vector<uint32_t> device_ids_;
  std::vector<std::string> device_names_;
};

}  // namespace EnergyProfiler
}  // namespace KokkosTools