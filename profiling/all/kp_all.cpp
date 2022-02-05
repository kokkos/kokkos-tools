//@HEADER
// ************************************************************************
//
//                        Kokkos v. 3.0
//       Copyright (2020) National Technology & Engineering
//               Solutions of Sandia, LLC (NTESS).
//
// Under the terms of Contract DE-NA0003525 with NTESS,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY NTESS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL NTESS OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact David Poliakoff (dzpolia@sandia.gov)
//
// ************************************************************************
//@HEADER

#include <stdexcept>
#include <string>
#include <cstring>
#include <map>

#include "kp_all.hpp"

#define KOKKOSTOOLS_EXTERN_EVENT_SET(NAMESPACE) \
namespace KokkosTools::NAMESPACE { \
  extern Kokkos::Tools::Experimental::EventSet get_event_set(); \
}

KOKKOSTOOLS_EXTERN_EVENT_SET(KernelTimer)
KOKKOSTOOLS_EXTERN_EVENT_SET(KernelTimerJSON)
KOKKOSTOOLS_EXTERN_EVENT_SET(MemoryEvents)
KOKKOSTOOLS_EXTERN_EVENT_SET(MemoryUsage)
KOKKOSTOOLS_EXTERN_EVENT_SET(HighwaterMark)
KOKKOSTOOLS_EXTERN_EVENT_SET(HighwaterMarkMPI)
KOKKOSTOOLS_EXTERN_EVENT_SET(ChromeTracing)
KOKKOSTOOLS_EXTERN_EVENT_SET(SpaceTimeStack)
KOKKOSTOOLS_EXTERN_EVENT_SET(SystemtapConnector)
#ifdef KOKKOSTOOLS_HAS_VARIORUM
  KOKKOSTOOLS_EXTERN_EVENT_SET(VariorumConnector)
#endif
#ifdef KOKKOSTOOLS_HAS_CALIPER
namespace cali {
  extern Kokkos::Tools::Experimental::EventSet get_event_set(const char* config_str);
}
#endif

using EventSet = Kokkos::Tools::Experimental::EventSet;

namespace KokkosTools {

EventSet get_event_set(const char* profiler, const char* config_str)
{
  std::map<std::string, EventSet> handlers = {
    {"kernel-timer", KernelTimer::get_event_set()},
    {"kernel-timer-json", KernelTimerJSON::get_event_set()},
    {"memory-events", MemoryEvents::get_event_set()},
    {"memory-usage", MemoryUsage::get_event_set()},
    {"highwater-mark-mpi", HighwaterMarkMPI::get_event_set()},
    {"highwater-mark", HighwaterMark::get_event_set()},
    {"chrome-tracing", ChromeTracing::get_event_set()},
    {"space-time-stack", SpaceTimeStack::get_event_set()},
    {"systemtap-connector", SystemtapConnector::get_event_set()},
#ifdef KOKKOSTOOLS_HAS_VARIORUM
    {"variorum", VariorumConnector::get_event_set()},
#endif
#ifdef KOKKOSTOOLS_HAS_CALIPER
    {"caliper", cali::get_event_set(config_str)},
#endif
  };
  auto e = handlers.find(profiler);
  if (e != handlers.end())
    return e->second;

  if (strlen(profiler) > 0) {
    const auto msg = std::string("Profiler not supported: ") + profiler + " (unknown tool)";
    throw std::runtime_error(msg);
	}

  // default = no profiling
  EventSet eventSet;
  memset(&eventSet, 0, sizeof(eventSet));
  return eventSet;
}

}
