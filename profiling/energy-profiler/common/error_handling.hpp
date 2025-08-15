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
 * @brief Simple error handling for the energy profiler
 *
 * This provides basic error reporting and status checking without
 * complex exception handling systems. Keeps it simple as requested.
 */

enum class ErrorCode {
  SUCCESS = 0,
  PROVIDER_INIT_FAILED,
  DEVICE_ACCESS_FAILED,
  FILE_WRITE_FAILED,
  INVALID_DEVICE_INDEX,
  MEASUREMENT_FAILED,
  UNKNOWN_ERROR
};

struct Result {
  ErrorCode code;
  std::string message;

  Result() : code(ErrorCode::SUCCESS) {}
  Result(ErrorCode c) : code(c) {}
  Result(ErrorCode c, const std::string& msg) : code(c), message(msg) {}

  bool is_success() const { return code == ErrorCode::SUCCESS; }
  bool is_error() const { return code != ErrorCode::SUCCESS; }

  operator bool() const { return is_success(); }
};

/**
 * @brief Simple logging utility
 */
class Logger {
 public:
  enum Level { INFO, WARNING, ERROR };

  static void log(Level level, const std::string& component,
                  const std::string& message) {
    const char* level_str = "";
    switch (level) {
      case INFO: level_str = "INFO"; break;
      case WARNING: level_str = "WARNING"; break;
      case ERROR: level_str = "ERROR"; break;
    }

    std::cerr << "[" << level_str << "] " << component << ": " << message
              << std::endl;
  }

  static void info(const std::string& component, const std::string& message) {
    log(INFO, component, message);
  }

  static void warning(const std::string& component,
                      const std::string& message) {
    log(WARNING, component, message);
  }

  static void error(const std::string& component, const std::string& message) {
    log(ERROR, component, message);
  }
};

/**
 * @brief Helper macros for consistent error reporting
 */
#define ENERGY_PROFILER_LOG_INFO(component, msg) \
  KokkosTools::EnergyProfiler::Logger::info(component, msg)

#define ENERGY_PROFILER_LOG_WARNING(component, msg) \
  KokkosTools::EnergyProfiler::Logger::warning(component, msg)

#define ENERGY_PROFILER_LOG_ERROR(component, msg) \
  KokkosTools::EnergyProfiler::Logger::error(component, msg)

}  // namespace EnergyProfiler
}  // namespace KokkosTools
