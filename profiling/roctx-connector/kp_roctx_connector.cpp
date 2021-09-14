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

#include <roctx.h>

#include <cstdint>
#include <iostream>
#include <stack>

static std::stack<roctx_range_id_t> kokkosp_regions;

extern "C" void kokkosp_init_library(const int loadSeq,
                                     const uint64_t interfaceVer,
                                     const uint32_t /*devInfoCount*/,
                                     void* /*deviceInfo*/) {
        std::cout
            << "-----------------------------------------------------------\n"
            << "KokkosP: ROC Tracer Connector (sequence is " << loadSeq
            << ", version: " << interfaceVer << ")\n"
            << "-----------------------------------------------------------\n";

        roctxMark("Kokkos::Initialization Complete");
}

extern "C" void kokkosp_finalize_library() {
        std::cout << R"(
-----------------------------------------------------------
KokkosP: Finalization of ROC Tracer Connector. Complete.
-----------------------------------------------------------
)";

        roctxMark("Kokkos::Finalization Complete");
}

extern "C" void kokkosp_begin_parallel_for(const char* name,
                                           const uint32_t /*devID*/,
                                           uint64_t* /*kID*/) {
        roctxRangePush(name);
}

extern "C" void kokkosp_end_parallel_for(const uint64_t /*kID*/) {
        roctxRangePop();
}

extern "C" void kokkosp_begin_parallel_scan(const char* name,
                                            const uint32_t /*devID*/,
                                            uint64_t* /*kID*/) {
        roctxRangePush(name);
}

extern "C" void kokkosp_end_parallel_scan(const uint64_t /*kID*/) {
        roctxRangePop();
}

extern "C" void kokkosp_begin_parallel_reduce(const char* name,
                                              const uint32_t /*devID*/,
                                              uint64_t* /*kID*/) {
        roctxRangePush(name);
}

extern "C" void kokkosp_end_parallel_reduce(const uint64_t /*kID*/) {
        roctxRangePop();
}

extern "C" void kokkosp_push_profile_region(char* name) {
        kokkosp_regions.emplace(roctxRangeStartA(name));
}

extern "C" void kokkosp_pop_profile_region() {
        if (kokkosp_regions.empty()) {
                std::cerr << "KokkosP: Error - popped region with no active "
                             "regions pushed.\n";
        } else {
                roctxRangeStop(kokkosp_regions.top());
                kokkosp_regions.pop();
        }
}
