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

#include <cstdio>
#include <cstdint>
#include <vector>
#include <string>

#include "nvToolsExt.h"

#include "kp_core.hpp"

namespace KokkosTools {
namespace NVProfConnector {

void kokkosp_request_tool_settings(const uint32_t,
                                   Kokkos_Tools_ToolSettings* settings) {
  settings->requires_global_fencing = false;
}

void kokkosp_init_library(const int loadSeq, const uint64_t interfaceVer,
                          const uint32_t /*devInfoCount*/,
                          Kokkos_Profiling_KokkosPDeviceInfo* /*deviceInfo*/) {
  printf("-----------------------------------------------------------\n");
  printf("KokkosP: NVTX Analyzer Connector (sequence is %d, version: %llu)\n",
         loadSeq, (unsigned long long)(interfaceVer));
  printf("-----------------------------------------------------------\n");

  nvtxNameOsThread(pthread_self(), "Application Main Thread");
  nvtxMarkA("Kokkos::Initialization Complete");
}

void kokkosp_finalize_library() {
  printf("-----------------------------------------------------------\n");
  printf("KokkosP: Finalization of NVTX Connector. Complete.\n");
  printf("-----------------------------------------------------------\n");

  nvtxMarkA("Kokkos::Finalization Complete");
}

void kokkosp_begin_parallel_for(const char* name, const uint32_t /*devID*/,
                                uint64_t* /*kID*/) {
  nvtxRangePush(name);
}

void kokkosp_end_parallel_for(const uint64_t /*kID*/) { nvtxRangePop(); }

void kokkosp_begin_parallel_scan(const char* name, const uint32_t /*devID*/,
                                 uint64_t* /*kID*/) {
  nvtxRangePush(name);
}

void kokkosp_end_parallel_scan(const uint64_t /*kID*/) { nvtxRangePop(); }

void kokkosp_begin_parallel_reduce(const char* name, const uint32_t /*devID*/,
                                   uint64_t* /*kID*/) {
  nvtxRangePush(name);
}

void kokkosp_end_parallel_reduce(const uint64_t /*kID*/) { nvtxRangePop(); }

void kokkosp_push_profile_region(const char* regionName) {
  nvtxRangePush(regionName);
}

void kokkosp_pop_profile_region() { nvtxRangePop(); }

// TODO: move this to kp_core?
namespace {
struct Section {
  std::string label;
  nvtxRangeId_t id;
};
std::vector<Section> kokkosp_sections;
}  // namespace

Kokkos::Tools::Experimental::EventSet get_event_set() {
  Kokkos::Tools::Experimental::EventSet my_event_set;
  memset(&my_event_set, 0,
         sizeof(my_event_set));  // zero any pointers not set here
  my_event_set.request_tool_settings = kokkosp_request_tool_settings;
  my_event_set.init                  = kokkosp_init_library;
  my_event_set.finalize              = kokkosp_finalize_library;
  my_event_set.push_region           = kokkosp_push_profile_region;
  my_event_set.pop_region            = kokkosp_pop_profile_region;
  my_event_set.begin_parallel_for    = kokkosp_begin_parallel_for;
  my_event_set.begin_parallel_reduce = kokkosp_begin_parallel_reduce;
  my_event_set.begin_parallel_scan   = kokkosp_begin_parallel_scan;
  my_event_set.end_parallel_for      = kokkosp_end_parallel_for;
  my_event_set.end_parallel_reduce   = kokkosp_end_parallel_reduce;
  my_event_set.end_parallel_scan     = kokkosp_end_parallel_scan;
  return my_event_set;
}

void kokkosp_create_profile_section(const char* name, uint32_t* sID) {
  *sID = kokkosp_sections.size();
  kokkosp_sections.push_back(
      {std::string(name), static_cast<nvtxRangeId_t>(-1)});
}

void kokkosp_start_profile_section(const uint32_t sID) {
  auto& section = kokkosp_sections[sID];
  section.id    = nvtxRangeStartA(section.label.c_str());
}

void kokkosp_stop_profile_section(const uint32_t sID) {
  auto const& section = kokkosp_sections[sID];
  nvtxRangeEnd(section.id);
}

}  // namespace NVProfConnector
}  // namespace KokkosTools

extern "C" {

namespace impl = KokkosTools::NVProfConnector;

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
// TODO: expose section stuff
}  // extern "C"
