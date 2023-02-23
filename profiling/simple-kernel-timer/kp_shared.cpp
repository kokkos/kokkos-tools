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

#include "kp_shared.h"

namespace KokkosTools {
namespace KernelTimer {

uint64_t uniqID = 0;
KernelPerformanceInfo* currentEntry;
std::map<std::string, KernelPerformanceInfo*> count_map;
double initTime;
char* outputDelimiter;
int current_region_level = 0;
KernelPerformanceInfo* regions[512];

}  // namespace KernelTimer
}  // namespace KokkosTools
