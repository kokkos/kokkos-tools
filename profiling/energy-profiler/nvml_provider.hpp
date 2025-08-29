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
 * @file nvml_provider.hpp
 * @brief NVML Provider for GPU power monitoring.
 */

#pragma once

#include <vector>
#include <string>
#include <nvml.h>

namespace KokkosTools {
namespace EnergyProfiler {

class NVMLProvider {
 public:
  NVMLProvider();
  ~NVMLProvider();

  bool initialize();
  void finalize();

  bool get_total_power_usage(double& power_watts) const;
  bool get_device_power_usage(size_t device_index, double& power_watts) const;

  size_t get_device_count() const { return devices_.size(); }
  std::string get_device_name(size_t device_index) const;
  bool is_initialized() const { return is_initialized_; }

 private:
  bool is_initialized_;
  std::vector<nvmlDevice_t> devices_;
  std::vector<std::string> device_names_;

  bool discover_devices();
  bool validate_device_index(size_t device_index) const;
};

}  // namespace EnergyProfiler
}  // namespace KokkosTools
