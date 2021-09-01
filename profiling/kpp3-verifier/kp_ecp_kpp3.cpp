//@HEADER
// ************************************************************************
//
//                        Kokkos v. 3.0
//       Copyright (2020) National Technology & Engineering
//               Solutions of Sandia, LLC (NTESS).
//
// Under the terms of Contract DE-NA0003525 with NTESS,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY NTESS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL NTESS OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact David Poliakoff (dzpolia@sandia.gov)
//
// ************************************************************************
//@HEADER

#include <execinfo.h>
#include <inttypes.h>
#include <stdio.h>
#include <sys/time.h>
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <fstream>
#include <map>
#include <string>
#include <vector>
#include <sys/resource.h>
#include <unistd.h>
#include "kp_kernel_info.h"
#ifdef SYCL
#include<CL/sycl.hpp>
#endif

int num_spaces;
std::vector<std::tuple<double, uint64_t, double> > space_size_track[16];
uint64_t space_size[16];
struct SpaceHandle {
          char name[64];
};

char space_name[16][64];

double max_mem_usage() {
  struct rusage app_info;
  getrusage(RUSAGE_SELF, &app_info);
  const double max_rssKB = app_info.ru_maxrss;
  return max_rssKB * 1024;
}

bool compareKernelPerformanceInfo(KernelPerformanceInfo* left,
                                  KernelPerformanceInfo* right) {
  return left->getTime() > right->getTime();
};

static uint64_t uniqID = 0;
static KernelPerformanceInfo* currentEntry;
static std::map<std::string, KernelPerformanceInfo*> count_map;
static double initTime;
static char* outputDelimiter;
static int current_region_level = 0;
static KernelPerformanceInfo* regions[512];

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

  // initialize regions to 0s so we know if there is an object there
  memset(&regions[0], 0, 512 * sizeof(KernelPerformanceInfo*));

  printf(
      "============================\n"
      "Kokkos KPP-3 Verifier Loaded\n"
      "============================\n"
      "\n");

  num_spaces = 0;
  for (int i = 0; i < 16; i++) space_size[i] = 0;

  initTime = seconds();
}

void extract_cpuinfo() {
  std::ifstream input("/proc/cpuinfo");
  std::string line;
  int count = 0;
  while( std::getline( input, line ) ) {
//          printf("%s\n",line.c_str());
    if(line.find("model name")<line.size()) {
            //,line.c_str())>5) {
      count++;
      int pos = line.find(":");
      if(count==1) printf("   Processor Name : %s\n",line.substr(pos+2).c_str());
    }
  }
  printf("   Num Processors : %i\n", count);
}

void extract_gpuinfo() {
  #ifdef NVIDIA
  // Try nvidia-smi
  {
      std::array<char, 128> buffer;
      std::string result;

      auto pipe = popen("nvidia-smi -L | grep GPU", "r"); // get rid of shared_ptr

      if (!pipe) throw std::runtime_error("popen() failed!");

      while (!feof(pipe)) {
        if (fgets(buffer.data(), 128, pipe) != nullptr)
            result += buffer.data();
      }

      auto rc = pclose(pipe);
      if(rc == 0) {
        printf("Success\n");
        auto sub = result;
        int gpu_pos = sub.find("GPU ");
        while(gpu_pos<sub.length()) {
          sub = sub.substr(gpu_pos);
          std::cout<<"   "<<sub.substr(0,sub.find("(UUID")) << std::endl;
          sub = sub.substr(sub.find("UUID"));
          gpu_pos = sub.find("GPU ");
        }
      }
      
  }
  #endif
  #ifdef ROCM
  // Try rocm-smi
  {
      std::array<char, 128> buffer;
      std::string result;

      auto pipe = popen("rocm-smi --showproductname | grep \"Card series:\"", "r"); // get rid of shared_ptr

      if (!pipe) throw std::runtime_error("popen() failed!");

      while (!feof(pipe)) {
        if (fgets(buffer.data(), 128, pipe) != nullptr)
            result += buffer.data();
      }

      auto rc = pclose(pipe);
      if(rc == 0) {
        auto sub = result;
        int gpu_pos = sub.find("GPU");
        while(gpu_pos<sub.length()) {
          sub = sub.substr(gpu_pos);
          auto sub2 = sub;
          std::cout<<"   "<<sub2.substr(0,sub2.find(":")); 
          sub2 = sub2.substr(sub2.find(":")+1);
          sub2 = sub2.substr(sub2.find(":")+1);
          std::cout<<": "<<sub2.substr(0,sub2.find("\n"))<<std::endl;
          sub = sub2;
          gpu_pos = sub2.find("GPU");
        }
      }
   }
   #endif
   #ifdef SYCL
   int count = 0;
   for( const cl::sycl::platform& platform :
     cl::sycl::platform::get_platforms() ) {
     for( const cl::sycl::device& device : platform.get_devices() ) {
       printf("Device[%i]: %s %s\n",
         count, 
         device.get_info< cl::sycl::info::device::vendor >().c_str(),
         device.get_info< cl::sycl::info::device::name >().c_str());
       count++;
     }
   }
   #endif
}

extern "C" void kokkosp_finalize_library() {
  double finishTime  = seconds();
  double kernelTimes = 0;

  char* hostname = (char*)malloc(sizeof(char) * 256);
  gethostname(hostname, 256);

  const double totalExecuteTime = (finishTime - initTime);

  printf("\n\n");
  printf("======================================================================\n");
  printf("Kokkos ECP KPP-3 Verification Tool\n");
  printf("======================================================================\n");
  printf("\n");
  printf("SystemConfig :\n");
  printf("   Name           : %s\n",hostname);
  extract_cpuinfo();
  extract_gpuinfo();
  printf("\n");
  printf("======================================================================\n");
  printf("Parallel Execution Summary:\n");
  printf("---------------------------\n");
  printf("\n");
  printf("Kokkos executed a total of %lu kernels\n", uniqID);
  printf("\n");
  printf("20 kernels with the most aggregate time: \n\n");
  std::vector<KernelPerformanceInfo*> kernelList;

  for(auto kernel_itr = count_map.begin(); kernel_itr != count_map.end();
  kernel_itr++) { kernelList.push_back(kernel_itr->second); kernelTimes +=
  kernel_itr->second->getTime();
  }

  std::sort(kernelList.begin(), kernelList.end(), compareKernelPerformanceInfo);

  if(0 == strcmp(outputDelimiter, " ")) {
          printf("%-52s %10s %14s %6s %6s %14s %4s\n", "KernelName",
  "Calls", "s/Total", "\%/Ko", "\%/Tot", "s/Call", "Type"); } else {
          printf("%s%s%s%s%s%s%s%s%s%s%s%s%s\n",
                  "KernelName",
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


  int count = 0;
  for(auto kernel_itr = kernelList.begin(); (kernel_itr != kernelList.end() && count<20);
  kernel_itr++) {
          KernelPerformanceInfo* kernelInfo = *kernel_itr;

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
          case REGION:
                  kType = const_cast<char*>("REGI"); break;
          }

          if(kernelInfo->getKernelType()==REGION) break;

          int demangleStatus;
          char* finalDemangle = abi::__cxa_demangle(kName.c_str(), 0, 0,
  &demangleStatus);

          if(0 == strcmp(outputDelimiter, " ")) {
                  printf("%-52s%s%10lu%s%14.5f%s%6.2f%s%6.2f%s%14.5f%s%4s\n", (0 == demangleStatus) ?
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
                  printf("%s%s%lu%s%f%s%f%s%f%s%f%s%s\n",
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
  printf("Kokkos: Total Execution Time:              %15.6f seconds.\n", totalExecuteTime);
  printf("Kokkos: Time in Parallel Algorithms:       %15.6f seconds.\n", kernelTimes);
  printf("Kokkos: Time spent outside Kokkos:         %15.6f seconds.\n", (totalExecuteTime - kernelTimes));

  const double percentKokkos = (kernelTimes / totalExecuteTime) * 100.0;
  printf("Kokkos: Fraction in Parallel algorithms:   %15.6f \%\n", percentKokkos);
  printf("Kokkos: Number of Unique kernels:          %22lu \n", (uint64_t) count_map.size());
  printf("Kokkos: Parallel For Calls:                %22lu \n",
  uniqID);

  printf("\n\n");
  printf("======================================================================\n");
  printf("Memory Utilization Summary:\n");
  printf("---------------------------\n");
  printf("\n");
  printf("Max Rusage: %10.1lf MB\n", max_mem_usage()/1024/1024);
  printf("  (Rusage may not include device allocations.)\n");
  printf("  (Rusage does include non-kokkos allocations, such as internal MPI allocs.)\n");
  printf("\n");
  printf("%-10s %15s %15s %15s\n",
         "SpaceName","MaxConcAllocs","TotalAllocs","HighWater(MB)");
  for (int s = 0; s < num_spaces; s++) {
    double maxvalue = 0;
    uint64_t max_concurrent_allocations = 0;
    uint64_t concurrent_allocations = 0;
    uint64_t total_allocations = 0;
    uint64_t old_size = 0;
    for (int i = 0; i < space_size_track[s].size(); i++) {
      uint64_t size = std::get<1>(space_size_track[s][i]);
      if (size > maxvalue) maxvalue = size;
      if (size > old_size) { concurrent_allocations++; total_allocations++; }
      else concurrent_allocations--;
      if (concurrent_allocations > max_concurrent_allocations) max_concurrent_allocations = concurrent_allocations;

    }

    printf( "%-10s %15lu %15lu %15.1lf\n", space_name[s], max_concurrent_allocations, total_allocations, maxvalue/1024/1024);
  }

  printf("\n");
  printf("======================================================================\n");
  printf("End of Kokkos ECP KPP-3 Verification Tool Output\n");
  printf("======================================================================\n");
  printf("\n");

  if(NULL != outputDelimiter) {
          free(outputDelimiter);
  }
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

extern "C" void kokkosp_allocate_data(const SpaceHandle space,
                                      const char* label, const void* const ptr,
                                      const uint64_t size) {

  double time = seconds();

  int space_i = num_spaces;
  for (int s = 0; s < num_spaces; s++)
    if (strcmp(space_name[s], space.name) == 0) space_i = s;

  if (space_i == num_spaces) {
    strncpy(space_name[num_spaces], space.name, 64);
    num_spaces++;
  }
  space_size[space_i] += size;
  space_size_track[space_i].push_back(
      std::make_tuple(time, space_size[space_i], max_mem_usage()));
}

extern "C" void kokkosp_deallocate_data(const SpaceHandle space,
                                        const char* label,
                                        const void* const ptr,
                                        const uint64_t size) {

  double time = seconds();

  int space_i = num_spaces;
  for (int s = 0; s < num_spaces; s++)
    if (strcmp(space_name[s], space.name) == 0) space_i = s;

  if (space_i == num_spaces) {
    strncpy(space_name[num_spaces], space.name, 64);
    num_spaces++;
  }
  if (space_size[space_i] >= size) {
    space_size[space_i] -= size;
    space_size_track[space_i].push_back(
        std::make_tuple(time, space_size[space_i], max_mem_usage()));
  }
}
