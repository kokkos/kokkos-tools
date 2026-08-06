// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// SPDX-FileCopyrightText: Copyright Contributors to the Kokkos project

#include <cstdio>
#include <inttypes.h>
#include <vector>
#include <string>
#include <limits>
#include <climits>
#include <cstring>
#include "impl/Kokkos_Profiling_Interface.hpp"

std::vector<std::string> regions;
static uint64_t uniqID;
struct SpaceHandle {
  char name[64];
};

// Get a useful label from the deviceId
// NOTE: Relevant code is in:
// kokkos/core/src/impl/Kokkos_Profiling_Interface.hpp
std::string deviceIdToString(const uint32_t deviceId) {
  using namespace Kokkos::Tools::Experimental;
  std::string device_label("(");
  ExecutionSpaceIdentifier eid = identifier_from_devid(deviceId);
  if (eid.type == DeviceType::Serial)
    device_label += "Serial";
  else if (eid.type == DeviceType::OpenMP)
    device_label += "OpenMP";
  else if (eid.type == DeviceType::Cuda)
    device_label += "Cuda";
  else if (eid.type == DeviceType::HIP)
    device_label += "HIP";
  else if (eid.type == DeviceType::OpenMPTarget)
    device_label += "OpenMPTarget";
  else if (eid.type == DeviceType::HPX)
    device_label += "HPX";
  else if (eid.type == DeviceType::Threads)
    device_label += "Threads";
  else if (eid.type == DeviceType::SYCL)
    device_label += "SYCL";
  else if (eid.type == DeviceType::OpenACC)
    device_label += "OpenACC";
  else if (eid.type == DeviceType::Unknown)
    device_label += "Unknown";
  else
    device_label += "Unknown to KokkosTools";

  if (eid.instance_id ==
      int_for_synchronization_reason(
          SpecialSynchronizationCases::GlobalDeviceSynchronization))
    device_label += " All Instances)";
  else if (eid.instance_id ==
           int_for_synchronization_reason(
               SpecialSynchronizationCases::DeepCopyResourceSynchronization))
    device_label += " DeepCopyResource)";
  else
    device_label += " Instance " + std::to_string(eid.instance_id) + ")";

  return device_label;
}

bool suppressCounts() {
  static bool value       = [](){
    const char* varVal = std::getenv("KOKKOS_TOOLS_LOGGER_SUPPRESS_COUNTS");
    if (varVal) {
      std::string v = std::string(varVal);
      // default to false
      if (v == "1" || v == "ON" || v == "on" || v == "TRUE" || v == "true" ||
          v == "YES" || v == "yes")
        return true;
    }
    return false;
  }();
  return value;
}

void kokkosp_print_region_stack_indent(const int level) {
  printf("KokkosP: ");

  for (int i = 0; i < level; i++) {
    printf("  ");
  }
}

int kokkosp_print_region_stack() {
  int level = 0;

  for (auto regItr = regions.begin(); regItr != regions.end(); regItr++) {
    kokkosp_print_region_stack_indent(level);
    printf("%s\n", (*regItr).c_str());

    level++;
  }

  return level;
}

extern "C" void kokkosp_init_library(const int loadSeq,
                                     const uint64_t interfaceVer,
                                     const uint32_t /*devInfoCount*/,
                                     void* /*deviceInfo*/) {
  printf(
      "KokkosP: Kernel Logger Library Initialized (sequence is %d, version: "
      "%llu)\n",
      loadSeq, (unsigned long long)(interfaceVer));
  uniqID = 0;
}

extern "C" void kokkosp_finalize_library() {
  printf("KokkosP: Kokkos library finalization called.\n");
}

extern "C" void kokkosp_begin_parallel_for(const char* name,
                                           const uint32_t devID,
                                           uint64_t* kID) {
  *kID       = uniqID++;
  int output = *kID;
  if (suppressCounts()) output = 0;

  printf(
      "KokkosP: Executing parallel-for kernel on device %s with unique "
      "execution identifier %llu\n",
      deviceIdToString(devID).c_str(), (unsigned long long)(output));

  int level = kokkosp_print_region_stack();
  kokkosp_print_region_stack_indent(level);

  printf("    %s\n", name);
}

extern "C" void kokkosp_end_parallel_for(const uint64_t kID) {
  int output = kID;
  if (suppressCounts()) output = 0;
  printf("KokkosP: Execution of kernel %llu is completed.\n",
         (unsigned long long)output);
}

extern "C" void kokkosp_begin_parallel_scan(const char* name,
                                            const uint32_t devID,
                                            uint64_t* kID) {
  *kID       = uniqID++;
  int output = *kID;
  if (suppressCounts()) output = 0;

  printf(
      "KokkosP: Executing parallel-scan kernel on device %s with unique "
      "execution identifier %llu\n",
      printf(
      "KokkosP: Executing parallel-scan kernel on device %d (%s) with unique "
      "execution identifier %llu\n",
      devID, deviceIdToString(devID).c_str(), (unsigned long long)(output));

  int level = kokkosp_print_region_stack();
  kokkosp_print_region_stack_indent(level);

  printf("    %s\n", name);
}

extern "C" void kokkosp_end_parallel_scan(const uint64_t kID) {
  int output = kID;
  if (suppressCounts()) output = 0;
  printf("KokkosP: Execution of kernel %llu is completed.\n",
         (unsigned long long)(output));
}

extern "C" void kokkosp_begin_parallel_reduce(const char* name,
                                              const uint32_t devID,
                                              uint64_t* kID) {
  *kID       = uniqID++;
  int output = *kID;
  if (suppressCounts()) output = 0;

  printf(
      "KokkosP: Executing parallel-reduce kernel on device %s with unique "
      "execution identifier %llu\n",
      deviceIdToString(devID).c_str(), (unsigned long long)(output));

  int level = kokkosp_print_region_stack();
  kokkosp_print_region_stack_indent(level);

  printf("    %s\n", name);
}

extern "C" void kokkosp_end_parallel_reduce(const uint64_t kID) {
  int output = kID;
  if (suppressCounts()) output = 0;

  printf("KokkosP: Execution of kernel %llu is completed.\n",
         (unsigned long long)(output));
}

extern "C" void kokkosp_begin_fence(const char* name, const uint32_t devID,
                                    uint64_t* kID) {
  // filter out fence as this is a duplicate and unneeded (causing the tool to
  // hinder performance of application). We use strstr for checking if the
  // string contains the label of a fence (we assume the user will always have
  // the word fence in the label of the fence).
  if (std::strstr(name, "Kokkos Profile Tool Fence")) {
    // set the dereferenced execution identifier to be the maximum value of
    // uint64_t, which is assumed to never be assigned
    *kID = std::numeric_limits<uint64_t>::max();
  } else {
    *kID = uniqID++;

    int output = *kID;
    if (suppressCounts()) output = 0;

    printf(
        "KokkosP: Executing fence on device %s with unique execution "
        "identifier %llu\n",
        deviceIdToString(devID).c_str(), (unsigned long long)(output));

    int level = kokkosp_print_region_stack();
    kokkosp_print_region_stack_indent(level);

    printf("    %s\n", name);
  }
}

extern "C" void kokkosp_end_fence(const uint64_t kID) {
  // if we find the kID to be maximum value uint64_t, then the callback is
  // dealing with the application's fence, which we filtered out in the callback
  // for fences
  if (kID != std::numeric_limits<uint64_t>::max()) {
    int output = kID;
    if (suppressCounts()) output = 0;

    printf("KokkosP: Execution of fence %llu is completed.\n",
           (unsigned long long)(output));
  }
}

extern "C" void kokkosp_push_profile_region(char* regionName) {
  printf("KokkosP: Entering profiling region: %s\n", regionName);

  std::string regionNameStr(regionName);
  regions.push_back(regionNameStr);
}

extern "C" void kokkosp_pop_profile_region() {
  if (regions.size() > 0) {
    printf("KokkosP: Exiting profiling region: %s\n", regions.back().c_str());
    regions.pop_back();
  }
}

extern "C" void kokkosp_allocate_data(SpaceHandle handle, const char* name,
                                      void* ptr, uint64_t size) {
  printf("KokkosP: Allocate<%s> name: %s pointer: %p size: %llu\n", handle.name,
         name, ptr, (unsigned long long)(size));
}

extern "C" void kokkosp_deallocate_data(SpaceHandle handle, const char* name,
                                        void* ptr, uint64_t size) {
  printf("KokkosP: Deallocate<%s> name: %s pointer: %p size: %llu\n",
         handle.name, name, ptr, (unsigned long long)(size));
}

extern "C" void kokkosp_begin_deep_copy(SpaceHandle dst_handle,
                                        const char* dst_name,
                                        const void* dst_ptr,
                                        SpaceHandle src_handle,
                                        const char* src_name,
                                        const void* src_ptr, uint64_t size) {
  printf(
      "KokkosP: DeepCopy<%s,%s> DST(name: %s pointer: %p) SRC(name: %s pointer "
      "%p) Size: %llu\n",
      dst_handle.name, src_handle.name, dst_name, dst_ptr, src_name, src_ptr,
      (unsigned long long)(size));
}
