// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// SPDX-FileCopyrightText: Copyright Contributors to the Kokkos project

#include "kp_shared.h"

#include <ostream>
#include <algorithm>

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

inline std::string to_string(KernelExecutionType t) {
  switch (t) {
    case PARALLEL_FOR: return "\"PARALLEL_FOR\"";
    case PARALLEL_REDUCE: return "\"PARALLEL_REDUCE\"";
    case PARALLEL_SCAN: return "\"PARALLEL_SCAN\"";
    case REGION: return "\"REGION\"";
    default: throw t;
  }
}

inline void write_json(std::ostream& os, KernelPerformanceInfo const& kp,
                       std::string indent = "") {
  const uint64_t callcount = kp.getCallCount();
  os << indent << "{\n";
  if (is_region(kp)) {
    os << indent << "  \"region-name\": \"" << kp.getName() << "\",\n";
  } else {
    os << indent << "  \"kernel-name\": \"" << kp.getName() << "\",\n";
  }

  os << indent << "  \"call-count\": " << callcount << ",\n";
  os << indent << "  \"total-time\": " << kp.getTime() << ",\n";
  os << indent << "  \"time-per-call\": "
     << kp.getTime() / std::max((uint64_t)1, callcount) << ",\n";
  os << indent << "  \"kernel-type\": " << to_string(kp.getKernelType())
     << '\n';
  os << indent << '}';
}

void json_format_kernel_list(double totalExecuteTime,
                             std::vector<KernelPerformanceInfo*>& kernelInfo,
                             std::ostream& fout) {
  std::sort(kernelInfo.begin(), kernelInfo.end(), compareKernelPerformanceInfo);
  uint64_t totalKernelsCalls = 0;
  double totalKernelsTime    = 0;
  for (unsigned int i = 0; i < kernelInfo.size(); i++) {
    if (kernelInfo[i]->getKernelType() != REGION) {
      totalKernelsTime += kernelInfo[i]->getTime();
      // totalKernelsCalls += kernelInfo[i]->getCallCount();
      totalKernelsCalls++;
    }
  }

  fout << "{\n";

  fout << "  \"total-app-time\" : " << totalExecuteTime << ",\n";
  fout << "  \"total-kernel-time\" : " << totalKernelsTime << ",\n";
  fout << "  \"total-non-kernel-time\" : "
       << totalExecuteTime - totalKernelsTime << ",\n";
  fout << "  \"percent-in-kernels\" : "
       << 100. * totalKernelsTime / totalExecuteTime << ",\n";
  fout << "  \"unique-kernel-calls\" : " << totalKernelsCalls << ",\n";

  fout << "  \"region-data\" : [\n";
  {
    bool add_comma = false;
    for (auto const& kp : kernelInfo) {
      if (!is_region(*kp)) continue;
      if (add_comma) fout << ",\n";
      add_comma = true;
      write_json(fout, *kp, "    ");
    }
    fout << '\n';
  }
  fout << "  ],\n";

  fout << "  \"kernel-data\" : [\n";
  {
    bool add_comma = false;
    for (auto const& kp : kernelInfo) {
      if (is_region(*kp)) continue;
      if (add_comma) fout << ",\n";
      add_comma = true;
      write_json(fout, *kp, "    ");
    }
    fout << '\n';
  }
  fout << "  ]\n";

  fout << "}\n";
}

}  // namespace KernelTimer
}  // namespace KokkosTools
