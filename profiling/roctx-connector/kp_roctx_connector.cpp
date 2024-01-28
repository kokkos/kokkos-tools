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

#include <roctx.h>

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "kp_core.hpp"

namespace {
struct Section {
  std::string label;
  roctx_range_id_t id;
};
std::vector<Section> kokkosp_sections;
}  // namespace

namespace KokkosTools {
namespace ROCTXConnector {

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
                          const uint32_t /*devInfoCount*/,
                          Kokkos_Profiling_KokkosPDeviceInfo* /*deviceInfo*/) {
  const char* tool_global_fences = std::getenv("KOKKOS_TOOLS_GLOBALFENCES");
  if (tool_global_fences) {
    tool_globfences = (atoi(tool_global_fences) != 0);
  }

  std::cout << "-----------------------------------------------------------\n"
            << "KokkosP: ROC Tracer Connector (sequence is " << loadSeq
            << ", version: " << interfaceVer << ")\n"
            << "-----------------------------------------------------------\n";

  roctxMark("Kokkos::Initialization Complete");
}

void kokkosp_finalize_library() {
  std::cout << R"(
-----------------------------------------------------------
KokkosP: Finalization of ROC Tracer Connector. Complete.
-----------------------------------------------------------
)";

  roctxMark("Kokkos::Finalization Complete");
}

void kokkosp_begin_parallel_for(const char* name, const uint32_t /*devID*/,
                                uint64_t* /*kID*/) {
  roctxRangePush(name);
}

void kokkosp_end_parallel_for(const uint64_t /*kID*/) { roctxRangePop(); }

void kokkosp_begin_parallel_scan(const char* name, const uint32_t /*devID*/,
                                 uint64_t* /*kID*/) {
  roctxRangePush(name);
}

void kokkosp_end_parallel_scan(const uint64_t /*kID*/) { roctxRangePop(); }

void kokkosp_begin_parallel_reduce(const char* name, const uint32_t /*devID*/,
                                   uint64_t* /*kID*/) {
  roctxRangePush(name);
}

void kokkosp_end_parallel_reduce(const uint64_t /*kID*/) { roctxRangePop(); }

void kokkosp_push_profile_region(const char* name) { roctxRangePush(name); }

void kokkosp_pop_profile_region() { roctxRangePop(); }

void kokkosp_create_profile_section(const char* name, uint32_t* sID) {
  *sID = kokkosp_sections.size();
  kokkosp_sections.push_back(
      {std::string(name), static_cast<roctx_range_id_t>(-1)});
}

void kokkosp_start_profile_section(const uint32_t sID) {
  auto& section = kokkosp_sections[sID];
  section.id    = roctxRangeStart(section.label.c_str());
}

void kokkosp_stop_profile_section(const uint32_t sID) {
  auto const& section = kokkosp_sections[sID];
  roctxRangeStop(section.id);
}

void kokkosp_destroy_profile_section(const uint32_t sID) {
  // do nothing
}

void kokkosp_profile_event(const char* name) { roctxMark(name); }

void kokkosp_begin_fence(const char* name, const uint32_t /*devID*/,
                         uint64_t* fID) {
  *fID = roctxRangeStart(name);
}

void kokkosp_end_fence(const uint64_t fID) { roctxRangeStop(fID); }

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

}  // namespace ROCTXConnector
}  // namespace KokkosTools

extern "C" {

namespace impl = KokkosTools::ROCTXConnector;

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
