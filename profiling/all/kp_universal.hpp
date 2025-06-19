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

#include <cstdint>
#include "impl/Kokkos_Profiling_C_Interface.h"

#ifndef KOOKOSTOOLS_UNIVERSAL_HPP

namespace KokkosTools {
#define GENERATE_TOOL_HEADER(TOOL_NAME)                                      \
  namespace TOOL_NAME {                                                      \
  void kokkosp_init_library(const int loadSeq, const uint64_t interfaceVer,  \
                            const uint32_t devInfoCount,                     \
                            Kokkos_Profiling_KokkosPDeviceInfo* deviceInfo); \
  void kokkosp_finalize_library();                                           \
  void kokkosp_begin_parallel_for(const char* name, const uint32_t devID,    \
                                  uint64_t* kID);                            \
  void kokkosp_end_parallel_for(const uint64_t kID);                         \
  void kokkosp_begin_parallel_reduce(const char* name, const uint32_t devID, \
                                     uint64_t* kID);                         \
  void kokkosp_end_parallel_reduce(const uint64_t kID);                      \
  void kokkosp_begin_parallel_scan(const char* name, const uint32_t devID,   \
                                   uint64_t* kID);                           \
  void kokkosp_end_parallel_scan(const uint64_t kID);                        \
  void kokkosp_push_profile_region(char const* regionName);                  \
  void kokkosp_pop_profile_region();                                         \
  }  // namespace TOOL_NAME

// CombinedToolTemplate examples
GENERATE_TOOL_HEADER(CombinedSimple)
GENERATE_TOOL_HEADER(CombinedDaemon)
}  // namespace KokkosTools

#endif  // KOOKOSTOOLS_UNIVERSAL_HPP