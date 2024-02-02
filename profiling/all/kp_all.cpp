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

#include <stdexcept>
#include <string>
#include <cstring>
#include <map>

#include "kp_all.hpp"

#define KOKKOSTOOLS_EXTERN_EVENT_SET(NAMESPACE)                 \
  namespace KokkosTools {                                       \
  namespace NAMESPACE {                                         \
  extern Kokkos::Tools::Experimental::EventSet get_event_set(); \
  }                                                             \
  }

#ifndef WIN32
KOKKOSTOOLS_EXTERN_EVENT_SET(KernelTimer)
KOKKOSTOOLS_EXTERN_EVENT_SET(MemoryEvents)
KOKKOSTOOLS_EXTERN_EVENT_SET(MemoryUsage)
KOKKOSTOOLS_EXTERN_EVENT_SET(HighwaterMark)
KOKKOSTOOLS_EXTERN_EVENT_SET(HighwaterMarkMPI)
KOKKOSTOOLS_EXTERN_EVENT_SET(ChromeTracing)
KOKKOSTOOLS_EXTERN_EVENT_SET(SpaceTimeStack)
#ifdef KOKKOSTOOLS_HAS_SYSTEMTAP
KOKKOSTOOLS_EXTERN_EVENT_SET(SystemtapConnector)
#endif
#endif
#ifdef KOKKOSTOOLS_HAS_VTUNE
KOKKOSTOOLS_EXTERN_EVENT_SET(VTuneConnector)
KOKKOSTOOLS_EXTERN_EVENT_SET(VTuneFocusedConnector)
#endif
#ifdef KOKKOSTOOLS_HAS_VARIORUM
KOKKOSTOOLS_EXTERN_EVENT_SET(VariorumConnector)
#endif
#ifdef KOKKOSTOOLS_HAS_NVTX
KOKKOSTOOLS_EXTERN_EVENT_SET(NVTXConnector)
KOKKOSTOOLS_EXTERN_EVENT_SET(NVTXFocusedConnector)
#endif
#ifdef KOKKOSTOOLS_HAS_ROCTX
KOKKOSTOOLS_EXTERN_EVENT_SET(ROCTXConnector)
#endif
#ifdef KOKKOSTOOLS_HAS_CALIPER
namespace cali {
extern Kokkos::Tools::Experimental::EventSet get_kokkos_event_set(
    const char* config_str);
}
#endif

using EventSet = Kokkos::Tools::Experimental::EventSet;

namespace KokkosTools {

EventSet get_event_set(const char* profiler, const char* config_str) {
  std::map<std::string, EventSet> handlers;
#ifndef WIN32
  handlers["kernel-timer"]      = KernelTimer::get_event_set();
  handlers["memory-events"]     = MemoryEvents::get_event_set();
  handlers["memory-usage"]      = MemoryUsage::get_event_set();
#if USE_MPI
  handlers["highwater-mark-mpi"] = HighwaterMarkMPI::get_event_set();
#endif
  handlers["highwater-mark"]   = HighwaterMark::get_event_set();
  handlers["chrome-tracing"]   = ChromeTracing::get_event_set();
  handlers["space-time-stack"] = SpaceTimeStack::get_event_set();
#ifdef KOKKOSTOOLS_HAS_SYSTEMTAP
  handlers["systemtap-connector"] = SystemtapConnector::get_event_set();
#endif
#endif
#ifdef KOKKOSTOOLS_HAS_VARIORUM
  handlers["variorum"] = VariorumConnector::get_event_set();
#endif
#ifdef KOKKOSTOOLS_HAS_VTUNE
  handlers["vtune-connector"]         = VTuneConnector::get_event_set();
  handlers["vtune-focused-connector"] = VTuneFocusedConnector::get_event_set();
#endif
#ifdef KOKKOSTOOLS_HAS_CALIPER
  handlers["caliper"] = cali::get_kokkos_event_set(config_str);
#endif
#ifdef KOKKOSTOOLS_HAS_NVTX
  handlers["nvtx-connector"]         = NVTXConnector::get_event_set();
  handlers["nvtx-focused-connector"] = NVTXFocusedConnector::get_event_set();
#endif
#ifdef KOKKOSTOOLS_HAS_ROCTX
  handlers["roctx-connector"] = ROCTXConnector::get_event_set();
#endif
  auto e = handlers.find(profiler);
  if (e != handlers.end()) return e->second;

  if (strlen(profiler) > 0) {
    const auto msg =
        std::string("Profiler not supported: ") + profiler + " (unknown tool)";
    throw std::runtime_error(msg);
  }

  // default = no profiling
  EventSet eventSet;
  memset(&eventSet, 0, sizeof(eventSet));
  return eventSet;
}

}  // namespace KokkosTools
