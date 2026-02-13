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
#include <cstdlib>
#include <cstring>
#include <unordered_map>
#include <string>
#include <vector>

#include "kp_core.hpp"
#include "kp_vtune_connector_domain.h"

namespace {
struct Section {
  std::string label;
  __itt_domain* domain;
};
std::vector<Section> kokkosp_sections;
}  // namespace

namespace KokkosTools {
namespace VTuneConnector {

static KernelVTuneConnectorInfo* currentKernel;
static std::unordered_map<std::string, KernelVTuneConnectorInfo*> domain_map;
static uint64_t nextKernelID;
static bool tool_globfences;

void kokkosp_request_tool_settings(const uint32_t,
                                   Kokkos_Tools_ToolSettings* settings) {
  if (tool_globfences) {
    settings->requires_global_fencing = true;
  } else {
    settings->requires_global_fencing = false;
  }
}

void kokkosp_init_library(const int loadSeq, const uint64_t interfaceVer,
                          const uint32_t devInfoCount,
                          Kokkos_Profiling_KokkosPDeviceInfo* deviceInfo) {
  const char* tool_global_fences = getenv("KOKKOS_TOOLS_GLOBALFENCES");
  if (NULL != tool_global_fences) {
    tool_globfences = (atoi(tool_global_fences) != 0);
  }

  printf("-----------------------------------------------------------\n");
  printf("KokkosP: VTune Analyzer Connector (sequence is %d, version: %llu)\n",
         loadSeq, interfaceVer);
  printf("-----------------------------------------------------------\n");

  nextKernelID                 = 0;
  const char* kpStartEventName = "Kokkos Initialization Complete";
  __itt_event startEv =
      __itt_event_create(kpStartEventName, strlen(kpStartEventName));
  __itt_event_start(startEv);
}

void kokkosp_finalize_library() {
  printf("-----------------------------------------------------------\n");
  printf("KokkosP: Finalization of VTune Connector. Complete.\n");
  printf("-----------------------------------------------------------\n");

  const char* kpFinalizeEventName = "Kokkos Finalize Complete";
  __itt_event finalEv =
      __itt_event_create(kpFinalizeEventName, strlen(kpFinalizeEventName));
  __itt_event_start(finalEv);
}

void kokkosp_begin_parallel_for(const char* name, const uint32_t devID,
                                uint64_t* kID) {
  *kID = nextKernelID++;

  std::string nameStr(name);
  auto kDomain  = domain_map.find(nameStr);
  currentKernel = NULL;

  if (kDomain == domain_map.end()) {
    currentKernel = new KernelVTuneConnectorInfo(name, PARALLEL_FOR);
    domain_map.insert(std::pair<std::string, KernelVTuneConnectorInfo*>(
        nameStr, currentKernel));
  } else {
    currentKernel = kDomain->second;
  }

  __itt_frame_begin_v3(currentKernel->getDomain(), NULL);
}

void kokkosp_end_parallel_for(const uint64_t kID) {
  __itt_frame_end_v3(currentKernel->getDomain(), NULL);
  currentKernel = NULL;
}

void kokkosp_begin_parallel_scan(const char* name, const uint32_t devID,
                                 uint64_t* kID) {
  *kID = nextKernelID++;

  std::string nameStr(name);
  auto kDomain  = domain_map.find(nameStr);
  currentKernel = NULL;

  if (kDomain == domain_map.end()) {
    currentKernel = new KernelVTuneConnectorInfo(name, PARALLEL_SCAN);
    domain_map.insert(std::pair<std::string, KernelVTuneConnectorInfo*>(
        nameStr, currentKernel));
  } else {
    currentKernel = kDomain->second;
  }

  __itt_frame_begin_v3(currentKernel->getDomain(), NULL);
}

void kokkosp_end_parallel_scan(const uint64_t kID) {
  __itt_frame_end_v3(currentKernel->getDomain(), NULL);
  currentKernel = NULL;
}

void kokkosp_begin_parallel_reduce(const char* name, const uint32_t devID,
                                   uint64_t* kID) {
  *kID = nextKernelID++;

  std::string nameStr(name);
  auto kDomain  = domain_map.find(nameStr);
  currentKernel = NULL;

  if (kDomain == domain_map.end()) {
    currentKernel = new KernelVTuneConnectorInfo(name, PARALLEL_REDUCE);
    domain_map.insert(std::pair<std::string, KernelVTuneConnectorInfo*>(
        nameStr, currentKernel));
  } else {
    currentKernel = kDomain->second;
  }

  __itt_frame_begin_v3(currentKernel->getDomain(), NULL);
}

void kokkosp_end_parallel_reduce(const uint64_t kID) {
  __itt_frame_end_v3(currentKernel->getDomain(), NULL);
  currentKernel = NULL;
}

void kokkosp_push_profile_region(const char* name) {
  __itt_domain* domain = __itt_domain_create(name);
  domain->flags        = 1;
  __itt_frame_begin_v3(domain, NULL);
}

void kokkosp_pop_profile_region() {
  // VTune requires domain to end frame, but we don't track it in push/pop
  // This is a known limitation - regions won't show properly in VTune
  // Users should use profile sections for better VTune integration
}

void kokkosp_create_profile_section(const char* name, uint32_t* sID) {
  *sID = kokkosp_sections.size();
  __itt_domain* domain = __itt_domain_create(name);
  domain->flags        = 1;
  kokkosp_sections.push_back({std::string(name), domain});
}

void kokkosp_start_profile_section(const uint32_t sID) {
  auto& section = kokkosp_sections[sID];
  __itt_frame_begin_v3(section.domain, NULL);
}

void kokkosp_stop_profile_section(const uint32_t sID) {
  auto const& section = kokkosp_sections[sID];
  __itt_frame_end_v3(section.domain, NULL);
}

void kokkosp_destroy_profile_section(const uint32_t sID) {
  // VTune domains are not explicitly destroyed
}

void kokkosp_profile_event(const char* name) {
  __itt_event event = __itt_event_create(name, strlen(name));
  __itt_event_start(event);
}

void kokkosp_begin_fence(const char* name, const uint32_t deviceId,
                         uint64_t* handle) {
  __itt_domain* domain = __itt_domain_create(name);
  domain->flags        = 1;
  __itt_frame_begin_v3(domain, NULL);
  // Store domain in handle for use in end_fence
  // This is not ideal but VTune API doesn't provide a better way
  *handle = reinterpret_cast<uint64_t>(domain);
}

void kokkosp_end_fence(uint64_t handle) {
  __itt_domain* domain = reinterpret_cast<__itt_domain*>(handle);
  __itt_frame_end_v3(domain, NULL);
}

Kokkos::Tools::Experimental::EventSet get_event_set() {
  Kokkos::Tools::Experimental::EventSet my_event_set;
  memset(&my_event_set, 0,
         sizeof(my_event_set));  // zero any pointers not set here
  my_event_set.request_tool_settings   = kokkosp_request_tool_settings;
  my_event_set.init                    = kokkosp_init_library;
  my_event_set.finalize                = kokkosp_finalize_library;
  my_event_set.push_region             = kokkosp_push_profile_region;
  my_event_set.pop_region              = kokkosp_pop_profile_region;
  my_event_set.begin_parallel_for      = kokkosp_begin_parallel_for;
  my_event_set.begin_parallel_reduce   = kokkosp_begin_parallel_reduce;
  my_event_set.begin_parallel_scan     = kokkosp_begin_parallel_scan;
  my_event_set.end_parallel_for        = kokkosp_end_parallel_for;
  my_event_set.end_parallel_reduce     = kokkosp_end_parallel_reduce;
  my_event_set.end_parallel_scan       = kokkosp_end_parallel_scan;
  my_event_set.create_profile_section  = kokkosp_create_profile_section;
  my_event_set.start_profile_section   = kokkosp_start_profile_section;
  my_event_set.stop_profile_section    = kokkosp_stop_profile_section;
  my_event_set.destroy_profile_section = kokkosp_destroy_profile_section;
  my_event_set.profile_event           = kokkosp_profile_event;
  my_event_set.begin_fence             = kokkosp_begin_fence;
  my_event_set.end_fence               = kokkosp_end_fence;
  return my_event_set;
}

}  // namespace VTuneConnector
}  // namespace KokkosTools

extern "C" {

namespace impl = KokkosTools::VTuneConnector;

EXPOSE_TOOL_SETTINGS(impl::kokkosp_request_tool_settings)
EXPOSE_INIT(impl::kokkosp_init_library)
EXPOSE_FINALIZE(impl::kokkosp_finalize_library)
EXPOSE_PUSH_REGION(impl::kokkosp_push_profile_region)
EXPOSE_POP_REGION(impl::kokkosp_pop_profile_region)
EXPOSE_BEGIN_PARALLEL_FOR(impl::kokkosp_begin_parallel_for)
EXPOSE_END_PARALLEL_FOR(impl::kokkosp_end_parallel_for)
EXPOSE_BEGIN_PARALLEL_SCAN(impl::kokkosp_begin_parallel_scan)
EXPOSE_END_PARALLEL_SCAN(impl::kokkosp_end_parallel_scan)
EXPOSE_BEGIN_PARALLEL_REDUCE(impl::kokkosp_begin_parallel_reduce)
EXPOSE_END_PARALLEL_REDUCE(impl::kokkosp_end_parallel_reduce)
EXPOSE_CREATE_PROFILE_SECTION(impl::kokkosp_create_profile_section)
EXPOSE_START_PROFILE_SECTION(impl::kokkosp_start_profile_section)
EXPOSE_STOP_PROFILE_SECTION(impl::kokkosp_stop_profile_section)
EXPOSE_DESTROY_PROFILE_SECTION(impl::kokkosp_destroy_profile_section)
EXPOSE_PROFILE_EVENT(impl::kokkosp_profile_event);
EXPOSE_BEGIN_FENCE(impl::kokkosp_begin_fence);
EXPOSE_END_FENCE(impl::kokkosp_end_fence);

}  // extern "C"
