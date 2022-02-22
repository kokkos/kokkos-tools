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

static uint64_t uniqID = 0;

void kokkosp_init_library(const int loadSeq,
	const uint64_t interfaceVer,
	const uint32_t devInfoCount,
	Kokkos_Profiling_KokkosPDeviceInfo* deviceInfo) {

	printf("KokkosP: Example Library Initialized (sequence is %d, version: %llu)\n", loadSeq, interfaceVer);
}

// darwin report rusage.ru_maxrss in bytes
#if defined(__APPLE__) || defined(__MACH__)
#    define RU_MAXRSS_UNITS 1024
#else
#    define RU_MAXRSS_UNITS 1
#endif

void kokkosp_finalize_library() {
	printf("\n");
	printf("KokkosP: Finalization of profiling library.\n");

	struct rusage sys_resources;
	getrusage(RUSAGE_SELF, &sys_resources);

	printf("KokkosP: High water mark memory consumption: %li kB\n",
		(long) sys_resources.ru_maxrss * RU_MAXRSS_UNITS);
	printf("\n");
}

Kokkos::Tools::Experimental::EventSet get_event_set() {
    Kokkos::Tools::Experimental::EventSet my_event_set;
    memset(&my_event_set, 0, sizeof(my_event_set)); // zero any pointers not set here
    my_event_set.init = kokkosp_init_library;
    my_event_set.finalize = kokkosp_finalize_library;
    return my_event_set;
}

// static auto event_set = get_event_set();

}} // namespace KokkosTools::HighwaterMark

extern "C" {

namespace impl = KokkosTools::HighwaterMark;

EXPOSE_INIT(impl::kokkosp_init_library) 
EXPOSE_FINALIZE(impl::kokkosp_finalize_library)

// EXPOSE_KOKKOS_INTERFACE(KokkosTools::HighwaterMark::event_set)

} // extern "C"