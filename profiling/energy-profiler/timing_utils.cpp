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

#include "timing_utils.hpp"
#include <unistd.h>
#include <cstring>

namespace KokkosTools {
namespace EnergyProfiler {

std::string generate_prefix() {
  char hostname[HOSTNAME_BUFFER_SIZE];
  if (gethostname(hostname, sizeof(hostname)) != 0) {
    // Fallback to "unknown" if hostname fails
    std::strncpy(hostname, "unknown", sizeof(hostname));
  }
  int pid = (int)getpid();
  return std::string(hostname) + "-" + std::to_string(pid);
}

std::string region_type_to_string(RegionType type) {
  switch (type) {
    case RegionType::ParallelFor: return "parallel_for";
    case RegionType::ParallelScan: return "parallel_scan";
    case RegionType::ParallelReduce: return "parallel_reduce";
    case RegionType::DeepCopy: return "deep_copy";
    case RegionType::UserRegion: return "user_region";
    default: return "unknown";
  }
}

}  // namespace EnergyProfiler
}  // namespace KokkosTools
