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
#include <stdlib.h>
#include <string.h>
#include <map>
#include <vector>
#include <algorithm>
#include <string>
#include <sys/time.h>
#include <cxxabi.h>
#include <sys/time.h>
#include <sys/resource.h>

#include "kp_core.hpp"

namespace KokkosTools {
namespace HighwaterMark {

void kokkosp_init_library(const int loadSeq, const uint64_t interfaceVer,
                          const uint32_t /*devInfoCount*/,
                          Kokkos_Profiling_KokkosPDeviceInfo* /*deviceInfo*/) {
  printf(
      "KokkosP: High Water Mark Library Initialized (sequence is %d, version: "
      "%llu)\n",
      loadSeq, (unsigned long long)(interfaceVer));
}

// darwin report rusage.ru_maxrss in bytes
#if defined(__APPLE__) || defined(__MACH__)
#define RU_MAXRSS_UNITS 1024
#else
#define RU_MAXRSS_UNITS 1
#endif

void kokkosp_finalize_library() {
  printf("\n");
  printf("KokkosP: Finalization of profiling library.\n");

  struct rusage sys_resources;
  getrusage(RUSAGE_SELF, &sys_resources);

  printf("KokkosP: High water mark memory consumption: %li kB\n",
         (long)sys_resources.ru_maxrss * RU_MAXRSS_UNITS);
  printf("\n");
}

Kokkos::Tools::Experimental::EventSet get_event_set() {
  Kokkos::Tools::Experimental::EventSet my_event_set;
  memset(&my_event_set, 0,
         sizeof(my_event_set));  // zero any pointers not set here
  my_event_set.init     = kokkosp_init_library;
  my_event_set.finalize = kokkosp_finalize_library;
  return my_event_set;
}

// static auto event_set = get_event_set();

}  // namespace HighwaterMark
}  // namespace KokkosTools

extern "C" {

namespace impl = KokkosTools::HighwaterMark;

EXPOSE_INIT(impl::kokkosp_init_library)
EXPOSE_FINALIZE(impl::kokkosp_finalize_library)

// EXPOSE_KOKKOS_INTERFACE(KokkosTools::HighwaterMark::event_set)

}  // extern "C"
