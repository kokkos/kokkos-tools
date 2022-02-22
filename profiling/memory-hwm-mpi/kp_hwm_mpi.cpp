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
#include <sys/resource.h>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cinttypes>
#include <mpi.h>

#include "kp_core.hpp"

namespace KokkosTools {
namespace HighwaterMarkMPI {

static int world_rank = 0;
static int world_size = 1;

// darwin report rusage.ru_maxrss in bytes
#if defined(__APPLE__) || defined(__MACH__)
#    define RU_MAXRSS_UNITS 1024
#else
#    define RU_MAXRSS_UNITS 1
#endif

void kokkosp_init_library(const int loadSeq,
  const uint64_t interfaceVer,
  const uint32_t devInfoCount,
  Kokkos_Profiling_KokkosPDeviceInfo* deviceInfo) {
  int mpi_is_initialized;
  MPI_Initialized(&mpi_is_initialized);
  if (!mpi_is_initialized) {
    printf("KokkosP: you must initialize MPI first!\n");
    exit(-1);
  }

  MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
  MPI_Comm_size(MPI_COMM_WORLD, &world_size);

  if (world_rank == 0) {
    printf("KokkosP: Example Library Initialized (sequence is %d, version: %" PRIu64 ")\n", loadSeq, interfaceVer);
  }
}

void kokkosp_finalize_library() {
  if (world_rank == 0) {
    printf("\n");
    printf("KokkosP: Finalization of profiling library.\n");
  }

  struct rusage sys_resources;
  getrusage(RUSAGE_SELF, &sys_resources);
  long hwm = sys_resources.ru_maxrss * RU_MAXRSS_UNITS;

  // Max
  long hwm_max;
  MPI_Reduce(&hwm, &hwm_max, 1, MPI_LONG, MPI_MAX, 0, MPI_COMM_WORLD);

  // Min
  long hwm_min;
  MPI_Reduce(&hwm, &hwm_min, 1, MPI_LONG, MPI_MIN, 0, MPI_COMM_WORLD);

  // Average
  long hwm_ave;
  MPI_Reduce(&hwm, &hwm_ave, 1, MPI_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
  hwm_ave /= world_size;

  if (world_rank == 0) {
    printf("KokkosP: High water mark memory consumption: %ld kB\n",
      hwm_max);
    printf("  Max: %ld, Min: %ld, Ave: %ld kB\n",
      hwm_max,hwm_min,hwm_ave);
    printf("\n");
  }
}

Kokkos::Tools::Experimental::EventSet get_event_set() {
    Kokkos::Tools::Experimental::EventSet my_event_set;
    memset(&my_event_set, 0, sizeof(my_event_set)); // zero any pointers not set here
    my_event_set.init = kokkosp_init_library;
    my_event_set.finalize = kokkosp_finalize_library;
    return my_event_set;
}

}} // namespace KokkosTools::HighwaterMarkMPI


extern "C" {

namespace impl = KokkosTools::HighwaterMarkMPI; 

EXPOSE_INIT(impl::kokkosp_init_library) 
EXPOSE_FINALIZE(impl::kokkosp_finalize_library)


} // extern "C"