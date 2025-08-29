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
 * @file power_sampler.hpp
 * @brief Power sampling functionality using NVML and daemon.
 */

#pragma once

#include <vector>
#include <chrono>
#include <memory>
#include <mutex>
#include <atomic>
#include "nvml_provider.hpp"
#include "daemon.hpp"
#include "energy_profiler_constants.hpp"

namespace KokkosTools {
namespace EnergyProfiler {

struct PowerSample {
  std::chrono::high_resolution_clock::time_point timestamp;
  double power_watts;
};

class PowerSampler {
 public:
  PowerSampler();
  ~PowerSampler();

  bool initialize();
  void finalize();

  void start_sampling();
  void stop_sampling();

  std::vector<PowerSample> get_samples() const;
  void clear_samples();

  bool is_initialized() const {
    return nvml_provider_ && nvml_provider_->is_initialized();
  }
  bool is_sampling() const { return sampling_active_.load(); }

 private:
  std::unique_ptr<NVMLProvider> nvml_provider_;
  std::unique_ptr<Daemon> daemon_;
  mutable std::mutex samples_mutex_;
  std::vector<PowerSample> power_samples_;
  std::atomic<bool> sampling_active_;

  void sample_power();
};

}  // namespace EnergyProfiler
}  // namespace KokkosTools
