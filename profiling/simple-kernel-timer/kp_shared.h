// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// SPDX-FileCopyrightText: Copyright Contributors to the Kokkos project

#ifndef _H_KOKKOSP_KERNEL_SHARED
#define _H_KOKKOSP_KERNEL_SHARED

#include <cstdint>
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

inline std::string to_string(KernelExecutionType t) {
  switch (t) {
    case PARALLEL_FOR: return "\"PARALLEL_FOR\"";
    case PARALLEL_REDUCE: return "\"PARALLEL_REDUCE\"";
    case PARALLEL_SCAN: return "\"PARALLEL_SCAN\"";
    case SINGLE: return "\"SINGLE\"";
    case REGION: return "\"REGION\"";
    default: throw t;
  }
}

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

inline bool is_region(KernelPerformanceInfo const& kp) {
  return kp.getKernelType() == REGION;
}

void json_format_kernel_list(double totalExecuteTime,
                             std::vector<KernelPerformanceInfo*>& kernelInfo,
                             std::ostream& fout);

}  // namespace KokkosTools::KernelTimer

#endif  // _H_KOKKOSP_KERNEL_SHARED
