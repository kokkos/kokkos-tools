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

#include <stdio.h>
#include <inttypes.h>
#include <execinfo.h>
#include <cstdlib>
#include <cstring>
#include <map>
#include <vector>
#include <algorithm>
#include <string>
#include <sys/time.h>

#include <unistd.h>
#include "kp_kernel_info.h"

bool compareKernelPerformanceInfo(KernelPerformanceInfo* left,
                                  KernelPerformanceInfo* right) {
  return left->getTime() > right->getTime();
};

static uint64_t uniqID = 0;
static KernelPerformanceInfo* currentEntry;
static std::map<std::string, KernelPerformanceInfo*> count_map;
static double initTime;
static char* outputDelimiter;

#define MAX_STACK_SIZE 128

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

extern "C" void kokkosp_init_library(const int loadSeq,
                                     const uint64_t interfaceVer,
                                     const uint32_t devInfoCount,
                                     void* deviceInfo) {
  const char* output_delim_env = getenv("KOKKOSP_OUTPUT_DELIM");
  if (NULL == output_delim_env) {
    outputDelimiter = (char*)malloc(sizeof(char) * 2);
    sprintf(outputDelimiter, "%c", ' ');
  } else {
    outputDelimiter =
        (char*)malloc(sizeof(char) * (strlen(output_delim_env) + 1));
    sprintf(outputDelimiter, "%s", output_delim_env);
  }

  printf(
      "KokkosP: LDMS JSON Connector Initialized (sequence is %d, version: "
      "%llu)\n",
      loadSeq, interfaceVer);

  initTime = seconds();
}

extern "C" void kokkosp_finalize_library() {
  double finishTime  = seconds();
  double kernelTimes = 0;

  char* mpi_rank = getenv("OMPI_COMM_WORLD_RANK");

  char* hostname = (char*)malloc(sizeof(char) * 256);
  gethostname(hostname, 256);

  char* fileOutput = (char*)malloc(sizeof(char) * 256);
  sprintf(fileOutput, "%s-%d-%s.json", hostname, (int)getpid(),
          (NULL == mpi_rank) ? "0" : mpi_rank);

  free(hostname);
  FILE* output_data = fopen(fileOutput, "w");

  const double totalExecuteTime = (finishTime - initTime);
  std::vector<KernelPerformanceInfo*> kernelList;

  for (auto kernel_itr = count_map.begin(); kernel_itr != count_map.end();
       kernel_itr++) {
    kernelList.push_back(kernel_itr->second);
    kernelTimes += kernel_itr->second->getTime();
  }

  std::sort(kernelList.begin(), kernelList.end(), compareKernelPerformanceInfo);

  fprintf(output_data, "{\n\"kokkos-kernel-data\" : {\n");
  fprintf(output_data, "    \"mpi-rank\"               : %s,\n",
          (NULL == mpi_rank) ? "0" : mpi_rank);
  fprintf(output_data, "    \"total-app-time\"         : %10.3f,\n",
          totalExecuteTime);
  fprintf(output_data, "    \"total-kernel-times\"     : %10.3f,\n",
          kernelTimes);
  fprintf(output_data, "    \"total-non-kernel-times\" : %10.3f,\n",
          (totalExecuteTime - kernelTimes));

  const double percentKokkos = (kernelTimes / totalExecuteTime) * 100.0;
  fprintf(output_data, "    \"percent-in-kernels\"     : %6.2f,\n",
          percentKokkos);
  fprintf(output_data, "    \"unique-kernel-calls\"    : %22lu,\n",
          (uint64_t)count_map.size());
  fprintf(output_data, "\n");

  fprintf(output_data, "    \"kernel-perf-info\"       : [\n");

#define KERNEL_INFO_INDENT "       "

  bool print_comma = false;
  for (auto const& kernel : count_map) {
    if (print_comma) fprintf(output_data, ",\n");
    kernel.second->writeToFile(output_data, KERNEL_INFO_INDENT);
    print_comma = true;
  }

  fprintf(output_data, "\n");
  fprintf(output_data, "    ]\n");
  fprintf(output_data, "}\n}");
  fclose(output_data);
}

extern "C" void kokkosp_begin_parallel_for(const char* name,
                                           const uint32_t devID,
                                           uint64_t* kID) {
  *kID = uniqID++;

  if ((NULL == name) || (strcmp("", name) == 0)) {
    fprintf(stderr, "Error: kernel is empty\n");
    exit(-1);
  }

  increment_counter(name, PARALLEL_FOR);
}

extern "C" void kokkosp_end_parallel_for(const uint64_t kID) {
  currentEntry->addFromTimer();
}

extern "C" void kokkosp_begin_parallel_scan(const char* name,
                                            const uint32_t devID,
                                            uint64_t* kID) {
  *kID = uniqID++;

  if ((NULL == name) || (strcmp("", name) == 0)) {
    fprintf(stderr, "Error: kernel is empty\n");
    exit(-1);
  }

  increment_counter(name, PARALLEL_SCAN);
}

extern "C" void kokkosp_end_parallel_scan(const uint64_t kID) {
  currentEntry->addFromTimer();
}

extern "C" void kokkosp_begin_parallel_reduce(const char* name,
                                              const uint32_t devID,
                                              uint64_t* kID) {
  *kID = uniqID++;

  if ((NULL == name) || (strcmp("", name) == 0)) {
    fprintf(stderr, "Error: kernel is empty\n");
    exit(-1);
  }

  increment_counter(name, PARALLEL_REDUCE);
}

extern "C" void kokkosp_end_parallel_reduce(const uint64_t kID) {
  currentEntry->addFromTimer();
}
