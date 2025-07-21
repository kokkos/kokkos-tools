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

#include <iostream>
#include <deque>
#include <chrono>
#include <thread>
#include <mutex>
#include <atomic>
#include <memory>
#include <limits>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <condition_variable>
#include <inttypes.h>
#include <cinttypes>
#include <string>

#include <nvml.h>

#include "kp_core.hpp"
#include "kp_nvml_energy_profiler.hpp"

using namespace KokkosTools::NVMLEnergyProfiler;

extern "C" {

void kokkosp_init_library(const int loadSeq, const uint64_t interfaceVer,
                          const uint32_t devInfoCount __attribute__((unused)),
                          void* deviceInfo __attribute__((unused))) {
  const char* tool_verbose = getenv("KOKKOS_TOOLS_LIBS_VERBOSE");
  if (tool_verbose != nullptr) {
    printf(
        "KokkosP NVML Energy: library loaded (sequence is %d, version: %" PRIu64
        ")\n",
        loadSeq, interfaceVer);
  }

  g_data_manager = new DataManager();
  if (!g_data_manager->initialize()) {
    printf("KokkosP NVML Energy: Failed to initialize, profiling disabled\n");
    delete g_data_manager;
    g_data_manager = nullptr;
  }
}

void kokkosp_finalize_library() {
  const char* tool_verbose = getenv("KOKKOS_TOOLS_LIBS_VERBOSE");
  if (tool_verbose != nullptr) {
    printf("KokkosP NVML Energy: finalizing library\n");
  }

  char hostname[256];
  gethostname(hostname, 256);
  int pid = (int)getpid();

  if (g_data_manager) {
    // Write output files
    auto kernel_filename = std::string(hostname) + "-" + std::to_string(pid) +
                            "-nvml-energy-kernels.csv";
    auto region_filename = std::string(hostname) + "-" + std::to_string(pid) +
                           "-nvml-energy-regions.csv";
    g_data_manager->write_kernel_data(kernel_filename);
    g_data_manager->write_region_data(region_filename);

    delete g_data_manager;
    g_data_manager = nullptr;
  }
}

void kokkosp_begin_parallel_for(const char* name,
                                uint32_t devid __attribute__((unused)),
                                uint64_t* kernid __attribute__((unused))) {
  if (g_data_manager) {
    g_data_manager->start_region(std::string(name), RegionType::ParallelFor);
  }
}

void kokkosp_end_parallel_for(uint64_t kernid __attribute__((unused))) {
  if (g_data_manager) {
    g_data_manager->end_region();
  }
}

void kokkosp_begin_parallel_reduce(const char* name,
                                   uint32_t devid __attribute__((unused)),
                                   uint64_t* kernid __attribute__((unused))) {
  if (g_data_manager) {
    g_data_manager->start_region(std::string(name), RegionType::ParallelReduce);
  }
}

void kokkosp_end_parallel_reduce(uint64_t kernid __attribute__((unused))) {
  if (g_data_manager) {
    g_data_manager->end_region();
  }
}

void kokkosp_begin_parallel_scan(const char* name,
                                 uint32_t devid __attribute__((unused)),
                                 uint64_t* kernid __attribute__((unused))) {
  if (g_data_manager) {
    g_data_manager->start_region(std::string(name), RegionType::ParallelScan);
  }
}

void kokkosp_end_parallel_scan(uint64_t kernid __attribute__((unused))) {
  if (g_data_manager) {
    g_data_manager->end_region();
  }
}

void kokkosp_push_profile_region(const char* name) {
  if (g_data_manager) {
    g_data_manager->start_region(std::string(name), RegionType::UserRegion);
  }
}

void kokkosp_pop_profile_region() {
  if (g_data_manager) {
    g_data_manager->end_region();
  }
}

}  // extern "C"
