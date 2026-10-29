//@HEADER
// ************************************************************************
//
//                        Kokkos Energy Profiler Error Handling
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

#include <string>
#include <iostream>

namespace KokkosTools {
namespace EnergyProfiler {

/**
 * @brief Simple logging utility
 *
 * Usage example:
 *   log_message(LogLevel::INFO, "EnergyProfiler", "Initialization complete");
 *   log_message(LogLevel::ERROR, "DeviceManager", "Failed to access device");
 */
enum class LogLevel { INFO, WARNING, ERROR };

inline void log_message(LogLevel level, const std::string& component,
                        const std::string& message) {
  const char* level_str = "";
  switch (level) {
    case LogLevel::INFO: level_str = "INFO"; break;
    case LogLevel::WARNING: level_str = "WARNING"; break;
    case LogLevel::ERROR: level_str = "ERROR"; break;
  }

  std::cerr << "[" << level_str << "] " << component << ": " << message
            << std::endl;
}

}  // namespace EnergyProfiler
}  // namespace KokkosTools
