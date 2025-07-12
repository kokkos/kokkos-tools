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

#pragma once

#include <chrono>
#include <string>
#include <deque>
#include <unordered_map>
#include <memory>
#include <thread>
#include <atomic>
#include <map>

extern "C" {
#include <variorum.h>
#include <jansson.h>
}

namespace KokkosTools {
namespace PowerProfiler {

enum class KernelType { FOR, SCAN, REDUCE };

struct EnergyReading {
  std::chrono::system_clock::time_point epoch_timestamp;
  std::chrono::time_point<std::chrono::steady_clock> timestamp;
  std::map<uint32_t, double> gpu_power_watts;
};

struct KernelTiming {
  uint64_t kernel_id;
  std::string name;
  KernelType type;
  std::chrono::system_clock::time_point epoch_start_time;
  std::chrono::system_clock::time_point epoch_end_time;
  std::chrono::time_point<std::chrono::steady_clock> start_time;
  std::chrono::time_point<std::chrono::steady_clock> end_time;
  std::chrono::nanoseconds duration;
};

struct RegionTiming {
  std::string name;
  std::string type;
  std::chrono::system_clock::time_point epoch_start_time;
  std::chrono::system_clock::time_point epoch_end_time;
  std::chrono::time_point<std::chrono::steady_clock> start_time;
  std::chrono::time_point<std::chrono::steady_clock> end_time;
  std::chrono::nanoseconds duration;
};

class VariorumPowerProfiler {
 public:
  VariorumPowerProfiler();
  ~VariorumPowerProfiler();

  bool initialize();
  void finalize();

  void begin_kernel(uint64_t kernel_id, const std::string& name,
                    KernelType type);
  void end_kernel(uint64_t kernel_id);

  void push_region(const std::string& name, const std::string& type = "");
  void pop_region();

  bool is_initialized() const { return initialized_; }

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

  bool initialize_variorum();
  unique_json_ptr get_variorum_json_data() const;
  EnergyReading get_current_energy_reading() const;
  std::deque<uint32_t> get_available_devices() const;

  void monitoring_thread_function();
  void start_monitoring();
  void stop_monitoring();

  void generate_outputs();
  void output_to_csv() const;

  std::chrono::time_point<std::chrono::steady_clock> get_current_time() const;
  std::chrono::system_clock::time_point get_current_epoch_time() const;

  std::chrono::microseconds monitor_interval_{20000};
  std::string output_file_path_{"power_profile_output"};

  bool initialized_{false};
  std::deque<uint32_t> available_devices_;

  std::atomic<bool> monitoring_active_{false};
  std::unique_ptr<std::thread> monitoring_thread_;

  std::deque<EnergyReading> energy_readings_;
  std::deque<KernelTiming> completed_kernels_;
  std::deque<RegionTiming> completed_regions_;

  std::unordered_map<uint64_t, KernelTiming> active_kernels_;
  std::deque<RegionTiming> active_regions_;
};

// Utility function to convert KernelType to string
std::string kernel_type_to_string(KernelType type);

}  // namespace PowerProfiler
}  // namespace KokkosTools