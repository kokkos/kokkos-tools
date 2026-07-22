// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// SPDX-FileCopyrightText: Copyright Contributors to the Kokkos project

#include <vector>
#include <string>
#include <iostream>
#include <unistd.h>
#include <filesystem>
#include <algorithm>
#include "kp_core.hpp"
#include "kp_shared.h"
#include <sstream>
namespace KokkosTools {
namespace KernelTimer {
void print_ascii(std::map<std::string, KernelPerformanceInfo*>& count_map,
                 double totalExecuteTime) {
  std::vector<KernelPerformanceInfo*> kernelInfo;
  double totalKernelsTime    = 0;
  uint64_t totalKernelsCalls = 0;

  for (auto const& [name, info] : count_map) {
    kernelInfo.push_back(info);
  }

  std::sort(kernelInfo.begin(), kernelInfo.end(), compareKernelPerformanceInfo);

  // Calculate total time in kernels and total calls to kernels for summary
  for (auto const& info : kernelInfo) {
    if (info->getKernelType() != REGION) {
      totalKernelsTime += info->getTime();
      totalKernelsCalls += info->getCallCount();
    }
  }

  // Header matching kp_reader.cpp
  printf(
      "\n (Type)   Total Time, Call Count, Avg. Time per Call, %%Total Time in "
      "Kernels, %%Total Program Time\n");
  printf(
      "------------------------------------------------------------------------"
      "-\n\n");

  char delimiter = ' ';
  // We check for the environment delimiter if set during init
  if (outputDelimiter != nullptr && strlen(outputDelimiter) > 0) {
    delimiter = outputDelimiter[0];
  }

  auto print_row = [&](KernelPerformanceInfo* info) {
    const double callCountDouble = (double)info->getCallCount();
    const char* typeStr          = " (Region)  ";
    switch (info->getKernelType()) {
      case PARALLEL_FOR: typeStr = " (ParFor)  "; break;
      case PARALLEL_REDUCE: typeStr = " (ParRed)  "; break;
      case PARALLEL_SCAN: typeStr = " (ParScan) "; break;
      default: break;
    }

    printf(
        "- %s\n%s%c%f%c%" PRIu64 "%c%f%c%f%c%f\n", info->getName().c_str(),
        typeStr, delimiter, info->getTime(), delimiter, info->getCallCount(),
        delimiter, info->getTime() / std::max(1.0, callCountDouble), delimiter,
        (info->getTime() / std::max(1e-9, totalKernelsTime)) * 100.0, delimiter,
        (info->getTime() / std::max(1e-9, totalExecuteTime)) * 100.0);
  };

  printf("Regions: \n\n");
  for (auto const& info : kernelInfo) {
    if (info->getKernelType() == REGION) print_row(info);
  }

  printf(
      "\n----------------------------------------------------------------------"
      "---\n");
  printf("Kernels: \n\n");
  for (auto const& info : kernelInfo) {
    if (info->getKernelType() != REGION) print_row(info);
  }

  printf(
      "\n----------------------------------------------------------------------"
      "---\n");
  printf("Summary:\n\n");
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
         (totalKernelsTime / std::max(1e-9, totalExecuteTime)) * 100.0);
  printf("Total Calls to Kokkos Kernels:                         %20" PRIu64
         "\n",
         totalKernelsCalls);
  printf(
      "------------------------------------------------------------------------"
      "-\n\n");
}

void kokkosp_init_library(const int loadSeq, const uint64_t interfaceVer,
                          const uint32_t /*devInfoCount*/,
                          Kokkos_Profiling_KokkosPDeviceInfo* /*deviceInfo*/) {
  const char* output_delim_env = getenv("KOKKOSP_OUTPUT_DELIM");
  if (NULL == output_delim_env) {
    outputDelimiter = (char*)malloc(sizeof(char) * 2);
    snprintf(outputDelimiter, 2, "%c", ' ');
  } else {
    outputDelimiter =
        (char*)malloc(sizeof(char) * (strlen(output_delim_env) + 1));
    strcpy(outputDelimiter, output_delim_env);
  }

  // initialize regions to 0s so we know if there is an object there
  memset(&regions[0], 0, 512 * sizeof(KernelPerformanceInfo*));

  printf(
      "KokkosP: Simple Kernel Timer Library Initialized (sequence is %d, "
      "version: %llu)\n",
      loadSeq, (unsigned long long)(interfaceVer));

  initTime = seconds();
}
#define KERNEL_INFO_INDENT "       "
void kokkosp_finalize_library() {
  double finishTime             = seconds();
  const double totalExecuteTime = (finishTime - initTime);

  auto is_enabled = [](const char* env_var) {
    const char* env_var_raw = getenv(env_var);
    return env_var_raw != nullptr &&
           (strcmp(env_var_raw, "1") == 0 || strcmp(env_var_raw, "true") == 0 ||
            strcmp(env_var_raw, "True") == 0);
  };

  const bool kokkos_tools_timer_json = is_enabled("KOKKOS_TOOLS_TIMER_JSON");
  const bool kokkos_tools_timer_binary =
      is_enabled("KOKKOS_TOOLS_TIMER_BINARY");

  // Quick return for ascii output (default)
  if (!kokkos_tools_timer_json && !kokkos_tools_timer_binary) {
    print_ascii(count_map, totalExecuteTime);
    return;
  }

  char* hostname = (char*)malloc(sizeof(char) * 256);
  gethostname(hostname, 256);

  char* fileOutput = (char*)malloc(sizeof(char) * 256);
  snprintf(fileOutput, 256, "%s-%d.%s", hostname, (int)getpid(),
           kokkos_tools_timer_json ? "json" : "dat");

  free(hostname);
  FILE* output_data = fopen(fileOutput, "wb");
  if (kokkos_tools_timer_binary) {
    fwrite(&totalExecuteTime, sizeof(totalExecuteTime), 1, output_data);

    for (auto kernel_itr = count_map.begin(); kernel_itr != count_map.end();
         kernel_itr++) {
      kernel_itr->second->writeToBinaryFile(output_data);
    }
  } else if (kokkos_tools_timer_json) {
    std::vector<KernelPerformanceInfo*> kernelList;
    const std::size_t unique_counts = count_map.size();
    kernelList.reserve(unique_counts);

    for (auto kernel_itr = count_map.begin(); kernel_itr != count_map.end();
         kernel_itr++) {
      kernelList.push_back(kernel_itr->second);
    }

    std::ostringstream buffer;

    json_format_kernel_list(totalExecuteTime, kernelList, buffer);

    const std::string& s = buffer.str();
    fwrite(s.data(), 1, s.size(), output_data);
  }

  fclose(output_data);

  auto cwd = std::filesystem::current_path();
  printf("KokkosP: Kernel timing written to %s/%s \n", cwd.c_str(), fileOutput);

  /*printf("\n");
  printf("======================================================================\n");
  printf("KokkosP: Finalization of Profiling Library\n");
  printf("KokkosP: Executed a total of %llu kernels\n", uniqID);

  std::vector<KernelPerformanceInfo*> kernelList;

  for(auto kernel_itr = count_map.begin(); kernel_itr != count_map.end();
  kernel_itr++) { kernelList.push_back(kernel_itr->second); kernelTimes +=
  kernel_itr->second->getTime();
  }

  std::sort(kernelList.begin(), kernelList.end(), compareKernelPerformanceInfo);
  const double totalExecuteTime = (finishTime - initTime);

  if(0 == strcmp(outputDelimiter, " ")) {
          printf("KokkosP: %100s %14s %14s %6s %6s %14s %4s\n", "Kernel",
  "Calls", "s/Total", "\%/Ko", "\%/Tot", "s/Call", "Type"); } else {
          printf("KokkosP: %s%s%s%s%s%s%s%s%s%s%s%s%s\n",
                  "Kernel",
                  outputDelimiter,
                  "Calls",
                  outputDelimiter,
                  "s/Total",
                  outputDelimiter,
                  "\%/Ko",
                  outputDelimiter,
                  "\%/Tot",
                  outputDelimiter,
                  "s/Call",
                  outputDelimiter,
                  "Type");
  }

  for(auto kernel_itr = kernelList.begin(); kernel_itr != kernelList.end();
  kernel_itr++) { KernelPerformanceInfo* kernelInfo = *kernel_itr;

          const uint64_t kCallCount = kernelInfo->getCallCount();
          const double   kTime      = kernelInfo->getTime();
          const double   kTimeMean  = kTime / (double) kCallCount;

          const std::string& kName   = kernelInfo->getName();
          char* kType = const_cast<char*>("");

          switch(kernelInfo->getKernelType()) {
          case PARALLEL_FOR:
                  kType = const_cast<char*>("PFOR"); break;
          case PARALLEL_SCAN:
                  kType = const_cast<char*>("SCAN"); break;
          case PARALLEL_REDUCE:
                  kType = const_cast<char*>("RDCE"); break;
          case REGION
                  kType = const_cast<char*>("REGI"); break;
          }

          int demangleStatus;
          char* finalDemangle = abi::__cxa_demangle(kName.c_str(), 0, 0,
  &demangleStatus);

          if(0 == strcmp(outputDelimiter, " ")) {
                  printf("KokkosP:
  %s%s%14llu%s%14.5f%s%6.2f%s%6.2f%s%14.5f%s%4s\n", (0 == demangleStatus) ?
  finalDemangle : kName.c_str(), outputDelimiter, kCallCount, outputDelimiter,
                          kTime,
                          outputDelimiter,
                          (kTime / kernelTimes) * 100.0,
                          outputDelimiter,
                          (kTime / totalExecuteTime) * 100.0,
                          outputDelimiter,
                          kTimeMean,
                          outputDelimiter,
                          kType
                          );
          } else {
                  printf("KokkosP: %s%s%llu%s%f%s%f%s%f%s%f%s%s\n",
                          (0 == demangleStatus) ? finalDemangle : kName.c_str(),
                          outputDelimiter,
                          kCallCount,
                          outputDelimiter,
                          kTime,
                          outputDelimiter,
                          (kTime / kernelTimes) * 100.0,
                          outputDelimiter,
                          (kTime / totalExecuteTime) * 100.0,
                          outputDelimiter,
                          kTimeMean,
                          outputDelimiter,
                          kType
                          );
          }
  }

  printf("\n");
  printf("KokkosP: Total Execution Time:        %15.6f seconds.\n",
  totalExecuteTime); printf("KokkosP: Time in Kokkos Kernels:      %15.6f
  seconds.\n", kernelTimes); printf("KokkosP: Time spent outside Kokkos: %15.6f
  seconds.\n", (totalExecuteTime - kernelTimes));

  const double percentKokkos = (kernelTimes / totalExecuteTime) * 100.0;
  printf("KokkosP: Runtime in Kokkos Kernels:   %15.6f \%\n", percentKokkos);
  printf("KokkosP: Unique kernels:              %22llu \n", (uint64_t)
  count_map.size()); printf("KokkosP: Parallel For Calls:          %22llu \n",
  uniqID);

  printf("\n");
  printf("======================================================================\n");
  printf("\n");

  if(NULL != outputDelimiter) {
          free(outputDelimiter);
  }*/
}

void kokkosp_begin_parallel_for(const char* name, const uint32_t /*devID*/,
                                uint64_t* kID) {
  *kID = uniqID++;

  if ((NULL == name) || (strcmp("", name) == 0)) {
    fprintf(stderr, "Error: kernel is empty\n");
    exit(-1);
  }

  increment_counter(name, PARALLEL_FOR);
}

void kokkosp_end_parallel_for(const uint64_t /*kID*/) {
  currentEntry->addFromTimer();
}

void kokkosp_begin_parallel_scan(const char* name, const uint32_t /*devID*/,
                                 uint64_t* kID) {
  *kID = uniqID++;

  if ((NULL == name) || (strcmp("", name) == 0)) {
    fprintf(stderr, "Error: kernel is empty\n");
    exit(-1);
  }

  increment_counter(name, PARALLEL_SCAN);
}

void kokkosp_end_parallel_scan(const uint64_t /*kID*/) {
  currentEntry->addFromTimer();
}

void kokkosp_begin_parallel_reduce(const char* name, const uint32_t /*devID*/,
                                   uint64_t* kID) {
  *kID = uniqID++;

  if ((NULL == name) || (strcmp("", name) == 0)) {
    fprintf(stderr, "Error: kernel is empty\n");
    exit(-1);
  }

  increment_counter(name, PARALLEL_REDUCE);
}

void kokkosp_end_parallel_reduce(const uint64_t /*kID*/) {
  currentEntry->addFromTimer();
}

void kokkosp_push_profile_region(char const* regionName) {
  increment_counter_region(regionName, REGION);
}

void kokkosp_pop_profile_region() {
  current_region_level--;

  // current_region_level is out of bounds, inform the user they
  // called popRegion too many times.
  if (current_region_level < 0) {
    current_region_level = 0;
    std::cerr << "WARNING:: Kokkos::Profiling::popRegion() called outside "
              << " of an actve region. Previous regions: ";

    /* This code block will walk back through the non-null regions
     * pointers and print the names.  This takes advantage of a slight
     * issue with regions logic: we never actually delete the
     * KernelPerformanceInfo objects.  If that ever changes this needs
     * to be updated.
     */
    for (int i = 0; i < 5; i++) {
      if (regions[i] != 0) {
        std::cerr << (i == 0 ? " " : ";") << regions[i]->getName();
      } else {
        break;
      }
    }
    std::cerr << "\n";
  } else {
    // don't call addFromTimer if we are outside an active region
    regions[current_region_level]->addFromTimer();
  }
}

Kokkos::Tools::Experimental::EventSet get_event_set() {
  Kokkos::Tools::Experimental::EventSet my_event_set;
  memset(&my_event_set, 0,
         sizeof(my_event_set));  // zero any pointers not set here
  my_event_set.init                  = kokkosp_init_library;
  my_event_set.finalize              = kokkosp_finalize_library;
  my_event_set.begin_parallel_for    = kokkosp_begin_parallel_for;
  my_event_set.begin_parallel_reduce = kokkosp_begin_parallel_reduce;
  my_event_set.begin_parallel_scan   = kokkosp_begin_parallel_scan;
  my_event_set.end_parallel_for      = kokkosp_end_parallel_for;
  my_event_set.end_parallel_reduce   = kokkosp_end_parallel_reduce;
  my_event_set.end_parallel_scan     = kokkosp_end_parallel_scan;
  my_event_set.push_region           = kokkosp_push_profile_region;
  my_event_set.pop_region            = kokkosp_pop_profile_region;
  return my_event_set;
}

}  // namespace KernelTimer
}  // namespace KokkosTools

extern "C" {

namespace impl = KokkosTools::KernelTimer;

EXPOSE_INIT(impl::kokkosp_init_library)
EXPOSE_FINALIZE(impl::kokkosp_finalize_library)
EXPOSE_BEGIN_PARALLEL_FOR(impl::kokkosp_begin_parallel_for)
EXPOSE_END_PARALLEL_FOR(impl::kokkosp_end_parallel_for)
EXPOSE_BEGIN_PARALLEL_SCAN(impl::kokkosp_begin_parallel_scan)
EXPOSE_END_PARALLEL_SCAN(impl::kokkosp_end_parallel_scan)
EXPOSE_BEGIN_PARALLEL_REDUCE(impl::kokkosp_begin_parallel_reduce)
EXPOSE_END_PARALLEL_REDUCE(impl::kokkosp_end_parallel_reduce)
EXPOSE_PUSH_REGION(impl::kokkosp_push_profile_region)
EXPOSE_POP_REGION(impl::kokkosp_pop_profile_region)

}  // extern "C"
