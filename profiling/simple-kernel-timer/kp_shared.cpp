// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// SPDX-FileCopyrightText: Copyright Contributors to the Kokkos project

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

void increment_counter(const char* name, KernelExecutionType kType) {
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

void increment_counter_region(const char* name, KernelExecutionType kType) {
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

}  // namespace KernelTimer
}  // namespace KokkosTools
