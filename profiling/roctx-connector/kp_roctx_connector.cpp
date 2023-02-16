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

namespace {
struct Section {
  std::string label;
  roctx_range_id_t id;
};
std::vector<Section> kokkosp_sections;
}  // namespace

struct Kokkos_Tools_ToolSettings {
  bool requires_global_fencing;
  bool padding[255];
};

extern "C" void kokkosp_request_tool_settings(
    const uint32_t, Kokkos_Tools_ToolSettings* settings) {
  settings->requires_global_fencing = false;
}

extern "C" void kokkosp_init_library(const int loadSeq,
                                     const uint64_t interfaceVer,
                                     const uint32_t /*devInfoCount*/,
                                     void* /*deviceInfo*/) {
  std::cout << "-----------------------------------------------------------\n"
            << "KokkosP: ROC Tracer Connector (sequence is " << loadSeq
            << ", version: " << interfaceVer << ")\n"
            << "-----------------------------------------------------------\n";

  roctxMark("Kokkos::Initialization Complete");
}

extern "C" void kokkosp_finalize_library() {
  std::cout << R"(
-----------------------------------------------------------
KokkosP: Finalization of ROC Tracer Connector. Complete.
-----------------------------------------------------------
)";

  roctxMark("Kokkos::Finalization Complete");
}

extern "C" void kokkosp_begin_parallel_for(const char* name,
                                           const uint32_t /*devID*/,
                                           uint64_t* /*kID*/) {
  roctxRangePush(name);
}

extern "C" void kokkosp_end_parallel_for(const uint64_t /*kID*/) {
  roctxRangePop();
}

extern "C" void kokkosp_begin_parallel_scan(const char* name,
                                            const uint32_t /*devID*/,
                                            uint64_t* /*kID*/) {
  roctxRangePush(name);
}

extern "C" void kokkosp_end_parallel_scan(const uint64_t /*kID*/) {
  roctxRangePop();
}

extern "C" void kokkosp_begin_parallel_reduce(const char* name,
                                              const uint32_t /*devID*/,
                                              uint64_t* /*kID*/) {
  roctxRangePush(name);
}

extern "C" void kokkosp_end_parallel_reduce(const uint64_t /*kID*/) {
  roctxRangePop();
}

extern "C" void kokkosp_push_profile_region(char* name) {
  roctxRangePush(name);
}

extern "C" void kokkosp_pop_profile_region() { roctxRangePop(); }

extern "C" void kokkosp_create_profile_section(const char* name,
                                               uint32_t* sID) {
  *sID = kokkosp_sections.size();
  kokkosp_sections.push_back(
      {std::string(name), static_cast<roctx_range_id_t>(-1)});
}

extern "C" void kokkosp_start_profile_section(const uint32_t sID) {
  auto& section = kokkosp_sections[sID];
  section.id    = roctxRangeStart(section.label.c_str());
}

extern "C" void kokkosp_stop_profile_section(const uint32_t sID) {
  auto const& section = kokkosp_sections[sID];
  roctxRangeStop(section.id);
}

extern "C" void kokkosp_destroy_profile_section(const uint32_t sID) {
  // do nothing
}

extern "C" void kokkosp_begin_fence(const char* name, const uint32_t /*devID*/,
                                    uint64_t* fID) {
  *fID = roctxRangeStart(name);
}

extern "C" void kokkosp_end_fence(const uint64_t fID) { roctxRangeStop(fID); }
