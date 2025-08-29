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
 * @file power_sampler.cpp
 * @brief Power sampling implementation using NVML and daemon.
 */

#include "power_sampler.hpp"
#include <iostream>

namespace KokkosTools {
namespace EnergyProfiler {

PowerSampler::PowerSampler() : sampling_active_(false) {}

PowerSampler::~PowerSampler() {
  if (sampling_active_.load()) {
    stop_sampling();
  }
  finalize();
}

bool PowerSampler::initialize() {
  if (nvml_provider_) return true;

  nvml_provider_ = std::make_unique<NVMLProvider>();
  return nvml_provider_->initialize();
}

void PowerSampler::finalize() {
  if (nvml_provider_) {
    nvml_provider_->finalize();
    nvml_provider_.reset();
  }
}

void PowerSampler::start_sampling() {
  if (!nvml_provider_ || !nvml_provider_->is_initialized()) {
    std::cerr << "[KokkosPowerProfiler] ERROR: Cannot start sampling - NVML "
                 "provider not initialized"
              << std::endl;
    return;
  }

  if (sampling_active_.load()) {
    std::cerr << "[KokkosPowerProfiler] WARNING: Sampling already active"
              << std::endl;
    return;
  }

  sampling_active_.store(true);
  daemon_ = std::make_unique<Daemon>([this]() { this->sample_power(); },
                                     SAMPLING_INTERVAL_MS);
  daemon_->start();

  std::cout << "[KokkosPowerProfiler] INFO: Started power sampling"
            << std::endl;
}

void PowerSampler::stop_sampling() {
  if (!sampling_active_.load()) {
    return;
  }

  sampling_active_.store(false);
  if (daemon_) {
    daemon_->stop();
    daemon_.reset();
  }

  std::cout << "[KokkosPowerProfiler] INFO: Stopped power sampling"
            << std::endl;
}

std::vector<PowerSample> PowerSampler::get_samples() const {
  std::lock_guard<std::mutex> lock(samples_mutex_);
  return power_samples_;
}

void PowerSampler::clear_samples() {
  std::lock_guard<std::mutex> lock(samples_mutex_);
  power_samples_.clear();
}

void PowerSampler::sample_power() {
  if (!sampling_active_.load() || !nvml_provider_) {
    return;
  }

  double current_power = 0.0;
  bool success         = nvml_provider_->get_total_power_usage(current_power);
  if (!success) {
    std::cerr
        << "[KokkosPowerProfiler] WARNING: Failed to get total power usage"
        << std::endl;
    return;
  }

  auto current_timestamp = std::chrono::high_resolution_clock::now();

  // Store sample
  {
    std::lock_guard<std::mutex> lock(samples_mutex_);
    power_samples_.push_back({current_timestamp, current_power});
  }
}

}  // namespace EnergyProfiler
}  // namespace KokkosTools
