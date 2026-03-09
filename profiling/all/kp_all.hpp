// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// SPDX-FileCopyrightText: Copyright Contributors to the Kokkos project

#ifndef KOKKOSTOOLS_ALL_HPP
#define KOKKOSTOOLS_ALL_HPP

#include "kp_config.hpp"
#include "impl/Kokkos_Profiling_Interface.hpp"  // Note: impl/... is used inside the header

namespace KokkosTools {

Kokkos::Tools::Experimental::EventSet get_event_set(const char *profiler,
                                                    const char *options);

}

#endif
