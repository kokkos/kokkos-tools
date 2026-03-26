// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// SPDX-FileCopyrightText: Copyright Contributors to the Kokkos project

#include <stdio.h>
#include <cinttypes>
#include <cstdlib>
#include <vector>
#include <algorithm>
#include <map>

#include "kp_shared.h"

using namespace KokkosTools::KernelTimer;

inline const char* to_string(KernelExecutionType t) {
  switch (t) {
    case PARALLEL_FOR: return "\" (ParFor)  \"";
    case PARALLEL_REDUCE: return "\" (ParRed)  \"";
    case PARALLEL_SCAN: return "\" (ParScan) \"";
    case SINGLE: return "\" (Single)  \"";
    case REGION: return "\" (Region)  \"";
    default: throw t;
  }
}

int main(int argc, char* argv[]) {
  if (argc == 1) {
    fprintf(stderr, "Did you specify any data files on the command line!\n");
    fprintf(stderr, "Usage: ./reader file1.dat [fileX.dat]*\n");
    exit(-1);
  }

  char delimiter  = ' ';
  int fixed_width = 0;

  int commandline_args = 1;
  while ((commandline_args < argc) && (argv[commandline_args][0] == '-')) {
    if (strcmp(argv[commandline_args], "--delimiter") == 0) {
      delimiter = argv[++commandline_args][0];
    }
    if (strcmp(argv[commandline_args], "--fixed-width") == 0) {
      fixed_width = atoi(argv[++commandline_args]);
    }

    commandline_args++;
  }

  std::vector<KernelPerformanceInfo*> kernelInfo;
  double totalKernelsTime    = 0;
  double totalExecuteTime    = 0;
  uint64_t totalKernelsCalls = 0;

  for (int i = commandline_args; i < argc; i++) {
    FILE* the_file = fopen(argv[i], "rb");

    double fileExecuteTime = 0;
    [[maybe_unused]] auto ignore =
        fread(&fileExecuteTime, sizeof(fileExecuteTime), 1, the_file);

    totalExecuteTime += fileExecuteTime;

    while (!feof(the_file)) {
      KernelPerformanceInfo* new_kernel =
          new KernelPerformanceInfo("", PARALLEL_FOR);
      if (new_kernel->readFromFile(the_file)) {
        if (!new_kernel->getName().empty()) {
          int kernelIndex = find_index(kernelInfo, new_kernel->getName());

          if (kernelIndex > -1) {
            kernelInfo[kernelIndex]->addTime(new_kernel->getTime());
            kernelInfo[kernelIndex]->addCallCount(new_kernel->getCallCount());
          } else {
            kernelInfo.push_back(new_kernel);
          }
        }
      }
    }

    fclose(the_file);
  }

  std::sort(kernelInfo.begin(), kernelInfo.end(), compareKernelPerformanceInfo);

  for (unsigned int i = 0; i < kernelInfo.size(); i++) {
    if (kernelInfo[i]->getKernelType() != REGION) {
      totalKernelsTime += kernelInfo[i]->getTime();
      totalKernelsCalls += kernelInfo[i]->getCallCount();
    }
  }

  printf(
      " (Type)   Total Time, Call Count, Avg. Time per Call, %%Total Time in "
      "Kernels, %%Total Program Time\n");
  printf(
      "------------------------------------------------------------------------"
      "-\n\n");

  printf("Regions: \n\n");

  for (unsigned int i = 0; i < kernelInfo.size(); i++) {
    const double callCountDouble = (double)kernelInfo[i]->getCallCount();

    if (kernelInfo[i]->getKernelType() != REGION) continue;
    const double avgTime = (callCountDouble > 0)
                               ? kernelInfo[i]->getTime() / callCountDouble
                               : 0.0;
    const double pctKernels =
        (totalKernelsTime > 0)
            ? (kernelInfo[i]->getTime() / totalKernelsTime) * 100.0
            : 0.0;
    const double pctTotal =
        (totalExecuteTime > 0)
            ? (kernelInfo[i]->getTime() / totalExecuteTime) * 100.0
            : 0.0;
    if (fixed_width)
      printf("- %100s\n%11s%c%15.5f%c%12" PRIu64 "%c%15.5f%c%7.3f%c%7.3f\n",
             kernelInfo[i]->getName().c_str(),
             to_string(kernelInfo[i]->getKernelType()), delimiter,
             kernelInfo[i]->getTime(), delimiter, kernelInfo[i]->getCallCount(),
             delimiter, avgTime, delimiter, pctKernels, delimiter, pctTotal);
    else
      printf("- %s\n%s%c%f%c%" PRIu64 "%c%f%c%f%c%f\n",
             kernelInfo[i]->getName().c_str(),
             to_string(kernelInfo[i]->getKernelType()), delimiter,
             kernelInfo[i]->getTime(), delimiter, kernelInfo[i]->getCallCount(),
             delimiter, avgTime, delimiter, pctKernels, delimiter, pctTotal);
  }

  printf("\n");
  printf(
      "------------------------------------------------------------------------"
      "-\n");
  printf("Kernels: \n\n");

  for (unsigned int i = 0; i < kernelInfo.size(); i++) {
    const double callCountDouble = (double)kernelInfo[i]->getCallCount();

    if (kernelInfo[i]->getKernelType() == REGION) continue;
    const double avgTime = (callCountDouble > 0)
                               ? kernelInfo[i]->getTime() / callCountDouble
                               : 0.0;
    const double pctKernels =
        (totalKernelsTime > 0)
            ? (kernelInfo[i]->getTime() / totalKernelsTime) * 100.0
            : 0.0;
    const double pctTotal =
        (totalExecuteTime > 0)
            ? (kernelInfo[i]->getTime() / totalExecuteTime) * 100.0
            : 0.0;
    if (fixed_width)
      printf("- %100s\n%11s%c%15.5f%c%12" PRIu64 "%c%15.5f%c%7.3f%c%7.3f\n",
             kernelInfo[i]->getName().c_str(),
             to_string(kernelInfo[i]->getKernelType()), delimiter,
             kernelInfo[i]->getTime(), delimiter, kernelInfo[i]->getCallCount(),
             delimiter, avgTime, delimiter, pctKernels, delimiter, pctTotal);
    else
      printf("- %s\n%s%c%f%c%" PRIu64 "%c%f%c%f%c%f\n",
             kernelInfo[i]->getName().c_str(),
             to_string(kernelInfo[i]->getKernelType()), delimiter,
             kernelInfo[i]->getTime(), delimiter, kernelInfo[i]->getCallCount(),
             delimiter, avgTime, delimiter, pctKernels, delimiter, pctTotal);
  }

  printf("\n");
  printf(
      "------------------------------------------------------------------------"
      "-\n");
  printf("Summary:\n");
  printf("\n");
  printf(
      "Total Execution Time (incl. Kokkos + non-Kokkos):      %20.5f seconds\n",
      totalExecuteTime);
  printf(
      "Total Time in Kokkos kernels:                          %20.5f seconds\n",
      totalKernelsTime);
  printf(
      "   -> Time outside Kokkos kernels:                     %20.5f seconds\n",
      (totalExecuteTime - totalKernelsTime));
  printf("   -> Percentage in Kokkos kernels:                    %20.2f %%\n",
         (totalExecuteTime > 0) ? (totalKernelsTime / totalExecuteTime) * 100
                                : 0.0);
  printf("Total Calls to Kokkos Kernels:                         %20" PRIu64
         "\n",
         totalKernelsCalls);
  printf("\n");
  printf(
      "------------------------------------------------------------------------"
      "-\n");

  return 0;
}
