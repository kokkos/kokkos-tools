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
#include "kp_kernel_info.h"

namespace KokkosTools::KernelTimer {

extern uint64_t uniqID;
extern KernelPerformanceInfo* currentEntry;
extern std::map<std::string, KernelPerformanceInfo*> count_map;
extern double initTime;
extern char* outputDelimiter;
extern int current_region_level;
extern KernelPerformanceInfo* regions[512];

inline void increment_counter(const char* name, KernelExecutionType kType) {
  std::string nameStr(name);

  if (count_map.find(name) == count_map.end()) {
    KernelPerformanceInfo* info = new KernelPerformanceInfo(nameStr, kType);
    count_map.insert(
        std::pair<std::string, KernelPerformanceInfo*>(nameStr, info));

    currentEntry = info;
  } else {
    currentEntry = count_map[nameStr];
  }

  currentEntry->startTimer();
}

inline void increment_counter_region(const char* name,
                                     KernelExecutionType kType) {
  std::string nameStr(name);

  if (count_map.find(name) == count_map.end()) {
    KernelPerformanceInfo* info = new KernelPerformanceInfo(nameStr, kType);
    count_map.insert(
        std::pair<std::string, KernelPerformanceInfo*>(nameStr, info));

    regions[current_region_level] = info;
  } else {
    regions[current_region_level] = count_map[nameStr];
  }

  regions[current_region_level]->startTimer();
  current_region_level++;
}

inline bool compareKernelPerformanceInfo(KernelPerformanceInfo* left,
                                         KernelPerformanceInfo* right) {
  return left->getTime() > right->getTime();
};

}  // namespace KokkosTools::KernelTimer

#endif  // _H_KOKKOSP_KERNEL_SHARED
