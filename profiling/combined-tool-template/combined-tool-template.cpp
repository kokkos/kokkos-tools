
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

#include "kp_core.hpp"
#include "kp_universal.hpp"
#include <cstdio>

namespace KokkosTools {
namespace CombinedToolTemplate {

void kokkosp_init_library(const int loadSeq, const uint64_t interfaceVer,
                          const uint32_t devInfoCount,
                          Kokkos_Profiling_KokkosPDeviceInfo* deviceInfo) {
  printf("-----------------------------------------------------------\n");
  printf("KokkosP: ExampleTool (sequence is %d, version: %lu)\n", loadSeq,
         interfaceVer);
  printf(
      "-----------------------------------------------------------\n");  // Optional

  // Propagate initialization to the combined tools
  KokkosTools::CombinedSimple::kokkosp_init_library(loadSeq, interfaceVer,
                                                    devInfoCount, deviceInfo);
  KokkosTools::CombinedDaemon::kokkosp_init_library(loadSeq, interfaceVer,
                                                    devInfoCount, deviceInfo);
}

void kokkosp_finalize_library() {
  printf("-----------------------------------------------------------\n");
  printf("KokkosP: Finalization of ExampleTool. Complete.\n");
  printf(
      "-----------------------------------------------------------\n");  // Optional

  // Propagate finalization to the combined tools
  KokkosTools::CombinedSimple::kokkosp_finalize_library();
  KokkosTools::CombinedDaemon::kokkosp_finalize_library();
}

void kokkosp_begin_parallel_for(const char* name, const uint32_t devID,
                                uint64_t* kID) {
  KokkosTools::CombinedDaemon::kokkosp_begin_parallel_for(name, devID, kID); // Explicitly call the daemon tool's begin_parallel_for
  }

void kokkosp_end_parallel_for(const uint64_t kID) {
  KokkosTools::CombinedDaemon::kokkosp_end_parallel_for(kID); // Explicitly call the daemon tool's end_parallel_for
}

void kokkosp_begin_parallel_scan(const char* name, const uint32_t devID,
                                 uint64_t* kID) {}

void kokkosp_end_parallel_scan(const uint64_t kID) {}

void kokkosp_begin_parallel_reduce(const char* name, const uint32_t devID,
                                   uint64_t* kID) {}

void kokkosp_end_parallel_reduce(const uint64_t kID) {}

void kokkosp_push_profile_region(char const* regionName) {
  printf("KokkosP: Entering profiling region: %s\n", regionName);  // Optional
}

void kokkosp_pop_profile_region() {
  printf("KokkosP: Exiting profiling region.\n");  // Optional
}

Kokkos::Tools::Experimental::EventSet get_event_set() {
  Kokkos::Tools::Experimental::EventSet my_event_set;
  memset(&my_event_set, 0, sizeof(my_event_set));
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

}  // namespace CombinedToolTemplate
}  // namespace KokkosTools

extern "C" {

namespace impl = KokkosTools::CombinedToolTemplate;

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
}