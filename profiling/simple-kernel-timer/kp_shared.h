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

#ifndef _H_KOKKOSP_KERNEL_SHARED
#define _H_KOKKOSP_KERNEL_SHARED

#include <map>
#include <memory>
#include <vector>

#include "kp_kernel_info.h"

namespace KokkosTools::KernelTimer {

extern uint64_t uniqID;
extern KernelPerformanceInfo* currentEntry;
extern std::map<std::string, KernelPerformanceInfo*> count_map;
extern double initTime;
extern char* outputDelimiter;
extern int current_region_level;
extern KernelPerformanceInfo* regions[512];

void increment_counter(const char* name, KernelExecutionType kType);
void increment_counter_region(const char* name, KernelExecutionType kType);

inline bool compareKernelPerformanceInfo(KernelPerformanceInfo* left,
                                         KernelPerformanceInfo* right) {
  return left->getTime() > right->getTime();
};

inline int find_index(const std::vector<KernelPerformanceInfo*>& kernels,
                      const std::string& kernelName) {
  for (unsigned int i = 0; i < kernels.size(); ++i) {
    if (kernels[i]->getName() == kernelName) {
      return i;
    }
  }
  return -1;
}

}  // namespace KokkosTools::KernelTimer

#endif  // _H_KOKKOSP_KERNEL_SHARED
