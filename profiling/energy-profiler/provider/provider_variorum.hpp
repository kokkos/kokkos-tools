#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>

extern "C" {
#include <variorum.h>
#include <jansson.h>
}

class VariorumProvider {
 public:
  VariorumProvider();
  ~VariorumProvider();

  // Core functionality
  bool initialize();
  void finalize();
  bool is_initialized() const { return initialized_; }

  // Power monitoring
  double get_total_power_usage();
  double get_device_power_usage(size_t device_index);

  // Device information
  size_t get_device_count() const;
  std::string get_device_name(size_t device_index) const;

 private:
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

  // Member variables
  bool initialized_;
  std::vector<uint32_t> device_ids_;
  std::vector<std::string> device_names_;
};