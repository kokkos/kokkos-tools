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

static uint64_t uniqID = 0;

extern "C" void kokkosp_init_library(const int loadSeq,
                                     const uint64_t interfaceVer,
                                     const uint32_t devInfoCount,
                                     void* deviceInfo) {
  printf(
      "KokkosP: High Water Mark Library Initialized (sequence is %d, version: "
      "%llu)\n",
      loadSeq, interfaceVer);
}

// darwin report rusage.ru_maxrss in bytes
#if defined(__APPLE__) || defined(__MACH__)
#define RU_MAXRSS_UNITS 1024
#else
#define RU_MAXRSS_UNITS 1
#endif

extern "C" void kokkosp_finalize_library() {
  printf("\n");
  printf("KokkosP: Finalization of profiling library.\n");

  struct rusage sys_resources;
  getrusage(RUSAGE_SELF, &sys_resources);

  printf("KokkosP: High water mark memory consumption: %li kB\n",
         (long)sys_resources.ru_maxrss * RU_MAXRSS_UNITS);
  printf("\n");
}
