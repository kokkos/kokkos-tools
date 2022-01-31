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

#include "kp_all.hpp"
#include "kp_event_sets.hpp"

using EventSet = Kokkos::Tools::Experimental::EventSet;

namespace KokkosTools {

EventSet get_event_set(const char* profiler, const char* config_str)
{
  // default = no profiling
  EventSet eventSet;
  memset(&eventSet, 0, sizeof(eventSet));

  std::string name = profiler;
  if (name == "kernel-timer") {
    eventSet = KernelTimer::get_event_set();
  } else if (name == "kernel-timer-json") {
    eventSet = KernelTimerJSON::get_event_set();
  } else if (name == "memory-usage") {
    eventSet = MemoryUsage::get_event_set();
  } else if (name == "caliper") {
#ifdef KOKKOSTOOLS_HAS_CALIPER
    eventSet = cali::get_event_set(config_str);
#else
    throw std::runtime_error("Profiler not supported: caliper (KokkosTools library was built without Caliper)");
#endif
  } else if (name != "") {
    throw std::runtime_error(std::string("Profiler not supported: ") + name + std::string(" (unknown tool)"));
	}

  return eventSet;
}

}
