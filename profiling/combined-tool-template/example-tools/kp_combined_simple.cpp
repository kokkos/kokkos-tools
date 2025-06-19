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

#include <cstdlib>
#include <iostream>

#include "kp_core.hpp"
#include "kp_universal.hpp"

namespace KokkosTools {
namespace CombinedSimple {

// --- Kokkos Profiling Hooks ---

void kokkosp_init_library(const int loadSeq, const uint64_t interfaceVer,
                          const uint32_t devInfoCount,
                          Kokkos_Profiling_KokkosPDeviceInfo* deviceInfo) {
  std::cout
      << "CombinedSimple: Kokkos Profiling Library Initialized (sequence: "
      << loadSeq << ", version: " << interfaceVer << ")\n";
}

void kokkosp_finalize_library() {
  std::cout << "CombinedSimple: Kokkos Profiling Library Finalized.\n";
}

// --- Event Set Configuration ---

Kokkos::Tools::Experimental::EventSet get_event_set() {
  Kokkos::Tools::Experimental::EventSet my_event_set;
  memset(&my_event_set, 0,
         sizeof(my_event_set));  // zero any pointers not set here
  my_event_set.init     = kokkosp_init_library;
  my_event_set.finalize = kokkosp_finalize_library;
  return my_event_set;
}

}  // namespace CombinedSimple
}  // namespace KokkosTools

extern "C" {

namespace impl = KokkosTools::CombinedSimple;

EXPOSE_INIT(impl::kokkosp_init_library)
EXPOSE_FINALIZE(impl::kokkosp_finalize_library)
}