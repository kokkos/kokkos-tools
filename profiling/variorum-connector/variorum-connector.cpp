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

#include "kp_core.hpp"

extern "C" {
#include <variorum.h>
#include <jansson.h>
}

#include <iostream>
#include <fstream>

namespace KokkosTools {
namespace VariorumConnector {

// Initial and final power values for a kernel
double global_power[2] = {0, 0};
// Initial and final time value for a kernel
long long global_time[2]    = {0, 0};
uint32_t global_device_id   = -1;
uint32_t global_instance_id = -1;
std::string global_filename;
std::string global_kernel_name;
std::string global_device_type;

using initFunction = void (*)(const int, const uint64_t, const uint32_t, void*);
using finalizeFunction = void (*)();
using beginFunction    = void (*)(const char*, const uint32_t, uint64_t*);
using endFunction      = void (*)(uint64_t);

static initFunction initProfileLibrary         = nullptr;
static finalizeFunction finalizeProfileLibrary = nullptr;
static beginFunction beginForCallee            = nullptr;
static beginFunction beginScanCallee           = nullptr;
static beginFunction beginReduceCallee         = nullptr;
static endFunction endForCallee                = nullptr;
static endFunction endScanCallee               = nullptr;
static endFunction endReduceCallee             = nullptr;

inline std::string get_file_name(const char* env_var_name) {
  char* parsed_output_file = getenv(env_var_name);
  if (!parsed_output_file) {
    std::cerr << "Couldn't parse " << env_var_name
              << " environment "
                 "variable! Printing to variorumoutput.txt\n";
    return "variorumoutput.txt";
  }
  return parsed_output_file;
}

void create_file() {
  global_filename = get_file_name("KOKKOS_TOOLS_VARIORUM_OUTPUT_FILE");

  std::ifstream infile(global_filename);
  if (infile.good()) {
    infile.close();
    std::ofstream file(global_filename, std::ios::trunc);
  } else {
    std::ofstream file(global_filename, std::ios::app);
  }
}

void variorum_call() {
  char* s            = nullptr;
  int variorum_error = variorum_get_power_json(&s);
  if (variorum_error != 0) {
    std::cerr << "JSON get node power failed!\n";
    abort();
  }

  json_error_t error;
  json_t* root            = nullptr;
  json_t* socket_0        = nullptr;
  json_t* timestamp_value = nullptr;
  json_t* power_gpu_watts = nullptr;
  json_t* gpu_0_value     = nullptr;

  // Parse JSON string into a json_t object
  root = json_loads(s, 0, &error);
  if (!root) {
    std::cerr << "Error parsing JSON: " << error.text << '\n';
  }

  const char* key;
  json_t* value;
  json_object_foreach(root, key, value) {
    if (json_is_object(value)) {
      json_t* socket_object = json_object_get(value, "socket_0");
      if (socket_object && json_is_object(socket_object)) {
        socket_0        = socket_object;
        timestamp_value = json_object_get(value, "timestamp");
        if (global_time[0] == 0) {
          global_time[0] = (long long)json_integer_value(timestamp_value);
        } else {
          global_time[1] = (long long)json_integer_value(timestamp_value);
        }
        break;
      }
    }
  }
  // FIXME We assume that all GPUs are on socket 0 for now.
  if (!socket_0 || !timestamp_value) {
    std::cerr << "Failed to find 'socket_0' object or 'timestamp'.\n";
  }

  // Access power_gpu_watts within socket_0
  power_gpu_watts = json_object_get(socket_0, "power_gpu_watts");
  if (!json_is_object(power_gpu_watts)) {
    std::cerr << "Expected 'power_gpu_watts' to be an object.\n";
  }

  std::string gpu_key = "GPU_" + std::to_string(global_device_id);
  json_t* power_value = json_object_get(power_gpu_watts, gpu_key.c_str());
  if (json_is_number(power_value)) {
    double power_val = json_number_value(power_value);
    if (global_power[0] == 0) {
      global_power[0] = power_val;
    } else {
      global_power[1] = power_val;
    }
  } else {
    std::cerr << "Error: GPU key " << gpu_key << " not found or not a number"
              << std::endl;
  }

  if (global_power[0] != 0 && global_power[1] != 0) {
    int average_power = global_power[1] + global_power[0];
    double energy =
        ((average_power / 2) * ((global_time[1] - global_time[0]) * .001));
    std::ofstream file(global_filename, std::ios::app);
    file << "name: \"" << global_kernel_name
         << "\", Device ID: " << global_device_id
         << ", Instance ID: " << global_instance_id
         << ", DeviceType: " << global_device_type
         << ", Energy Estimate: " << energy << " J\n";
    global_time[0]  = 0;
    global_power[0] = 0;
    global_power[1] = 0;
    global_time[1]  = 0;
  }
}

void kokkosp_init_library(const int loadSeq, const uint64_t interfaceVer,
                          const uint32_t devInfoCount,
                          Kokkos_Profiling_KokkosPDeviceInfo* deviceInfo) {
  create_file();
}

std::string device_type_to_string(
    const Kokkos::Tools::Experimental::DeviceType& deviceType) {
  switch (deviceType) {
    case Kokkos::Tools::Experimental::DeviceType::Serial:
      return "SERIAL";
      break;
    case Kokkos::Tools::Experimental::DeviceType::OpenMP:
      return "OPENMP";
      break;
    case Kokkos::Tools::Experimental::DeviceType::Cuda: return "CUDA"; break;
    case Kokkos::Tools::Experimental::DeviceType::HIP: return "HIP"; break;
    case Kokkos::Tools::Experimental::DeviceType::OpenMPTarget:
      return "OPENMPTARGET";
      break;
    case Kokkos::Tools::Experimental::DeviceType::HPX: return "HPX"; break;
    case Kokkos::Tools::Experimental::DeviceType::Threads:
      return "THREADS";
      break;
    case Kokkos::Tools::Experimental::DeviceType::SYCL: return "SYCL"; break;
    case Kokkos::Tools::Experimental::DeviceType::OpenACC:
      return "OPENACC";
      break;
    default: return "UNKOWN";
  }
}

void kokkosp_begin_parallel_for(const char* name, const uint32_t devID,
                                uint64_t* kID) {
  auto result = Kokkos::Tools::Experimental::identifier_from_devid(devID);
  global_kernel_name = name;
  global_instance_id = result.instance_id;
  global_device_type = device_type_to_string(result.type);
  global_device_id   = result.device_id;
  variorum_call();
}

void kokkosp_end_parallel_for(const uint64_t kID) { variorum_call(); }

void kokkosp_begin_parallel_scan(const char* name, const uint32_t devID,
                                 uint64_t* kID) {
  auto result = Kokkos::Tools::Experimental::identifier_from_devid(devID);
  global_kernel_name = name;
  global_instance_id = result.instance_id;
  global_device_type = device_type_to_string(result.type);
  global_device_id   = result.device_id;
  variorum_call();
}

void kokkosp_end_parallel_scan(const uint64_t kID) { variorum_call(); }

void kokkosp_begin_parallel_reduce(const char* name, const uint32_t devID,
                                   uint64_t* kID) {
  auto result = Kokkos::Tools::Experimental::identifier_from_devid(devID);
  global_kernel_name = name;
  global_instance_id = result.instance_id;
  global_device_type = device_type_to_string(result.type);
  global_device_id   = result.device_id;
  variorum_call();
}

void kokkosp_end_parallel_reduce(const uint64_t kID) { variorum_call(); }

Kokkos::Tools::Experimental::EventSet get_event_set() {
  Kokkos::Tools::Experimental::EventSet my_event_set;
  memset(&my_event_set, 0,
         sizeof(my_event_set));  // zero any pointers not set here
  my_event_set.begin_parallel_for    = kokkosp_begin_parallel_for;
  my_event_set.begin_parallel_reduce = kokkosp_begin_parallel_reduce;
  my_event_set.begin_parallel_scan   = kokkosp_begin_parallel_scan;
  my_event_set.end_parallel_for      = kokkosp_end_parallel_for;
  my_event_set.end_parallel_reduce   = kokkosp_end_parallel_reduce;
  my_event_set.end_parallel_scan     = kokkosp_end_parallel_scan;
  return my_event_set;
}

}  // namespace VariorumConnector
}  // namespace KokkosTools

extern "C" {

namespace impl = KokkosTools::VariorumConnector;

EXPOSE_INIT(impl::kokkosp_init_library)
EXPOSE_BEGIN_PARALLEL_FOR(impl::kokkosp_begin_parallel_for)
EXPOSE_END_PARALLEL_FOR(impl::kokkosp_end_parallel_for)
EXPOSE_BEGIN_PARALLEL_SCAN(impl::kokkosp_begin_parallel_scan)
EXPOSE_END_PARALLEL_SCAN(impl::kokkosp_end_parallel_scan)
EXPOSE_BEGIN_PARALLEL_REDUCE(impl::kokkosp_begin_parallel_reduce)
EXPOSE_END_PARALLEL_REDUCE(impl::kokkosp_end_parallel_reduce)

}  // extern "C"
