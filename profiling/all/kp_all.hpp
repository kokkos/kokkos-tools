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

#ifndef KOKKOSTOOLS_ALL_HPP
#define KOKKOSTOOLS_ALL_HPP

#include "kp_config.hpp"
#include "impl/Kokkos_Profiling_Interface.hpp"  // Note: impl/... is used inside the header

namespace KokkosTools {

Kokkos::Tools::Experimental::EventSet get_event_set(const char *profiler,
                                                    const char *options);

}

#endif
