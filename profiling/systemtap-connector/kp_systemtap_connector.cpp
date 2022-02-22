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
#include <cstdlib>
#include <cstring>
#include <vector>
#include <unordered_set>
#include <string>
#include <regex>
#include <cxxabi.h>
#include <dlfcn.h>

#include "probes.h"
#include "kp_core.hpp"

namespace KokkosTools {
namespace SystemtapConnector {

static uint64_t next_kernid;
static uint32_t next_sec_id;

void kokkosp_begin_parallel_for(const char* name, const uint32_t devid, uint64_t* kernid)
{
  *kernid = next_kernid++;
  if ( KOKKOS_END_PARALLEL_FOR_ENABLED()) {
    KOKKOS_BEGIN_PARALLEL_FOR(name, devid, kernid);
  }
}

void kokkosp_begin_parallel_scan(const char* name, const uint32_t devid, uint64_t* kernid)
{
  *kernid = next_kernid++;
  if (KOKKOS_BEGIN_PARALLEL_SCAN_ENABLED()) {
    KOKKOS_BEGIN_PARALLEL_SCAN(name, devid, kernid);
  }
}

void kokkosp_begin_parallel_reduce(const char* name, const uint32_t devid, uint64_t* kernid)
{
  *kernid = next_kernid++;
  if (KOKKOS_BEGIN_PARALLEL_REDUCE_ENABLED()) {
    KOKKOS_BEGIN_PARALLEL_REDUCE(name, devid, kernid);
  }
}

void kokkosp_end_parallel_scan(uint64_t kernid)
{
  if (KOKKOS_END_PARALLEL_SCAN_ENABLED()) {
    KOKKOS_END_PARALLEL_SCAN(kernid);
  }
}

void kokkosp_end_parallel_for(uint64_t kernid)
{
  if (KOKKOS_END_PARALLEL_FOR_ENABLED()) {
    KOKKOS_END_PARALLEL_FOR(kernid);
  }
}

void kokkosp_end_parallel_reduce(uint64_t kernid)
{
  if (KOKKOS_END_PARALLEL_REDUCE_ENABLED()) {
    KOKKOS_END_PARALLEL_REDUCE(kernid);
  }
}

void kokkosp_init_library(const int loadseq,
	const uint64_t version,
	const uint32_t ndevinfos,
	Kokkos_Profiling_KokkosPDeviceInfo* deviceinfos)
{
  if (KOKKOS_INIT_LIBRARY_ENABLED()) {
    KOKKOS_INIT_LIBRARY(loadseq, version, ndevinfos, deviceinfos);
  }
}

void kokkosp_finalize_library()
{
  if (KOKKOS_FINALIZE_LIBRARY_ENABLED()) {
    KOKKOS_FINALIZE_LIBRARY();
  }
}

void kokkosp_push_profile_region(const char* name)
{
  if (KOKKOS_PUSH_PROFILE_REGION_ENABLED()) {
    KOKKOS_PUSH_PROFILE_REGION(name);
  }
}

void kokkosp_pop_profile_region()
{
  if (KOKKOS_POP_PROFILE_REGION_ENABLED()) {
    KOKKOS_POP_PROFILE_REGION();
  }
}

void kokkosp_allocate_data(SpaceHandle handle, const char* name, const void* ptr, uint64_t size)
{
  if (KOKKOS_ALLOCATE_DATA_ENABLED()) {
    KOKKOS_ALLOCATE_DATA(handle, name, ptr, size);
  }
}

void kokkosp_deallocate_data(SpaceHandle handle, const char* name, const void* ptr, uint64_t size)
{
  if (KOKKOS_DEALLOCATE_DATA_ENABLED()) {
    KOKKOS_DEALLOCATE_DATA(handle, name, ptr, size);
  }
}

void kokkosp_begin_deep_copy(
    SpaceHandle dst_handle, const char* dst_name, const void* dst_ptr,
    SpaceHandle src_handle, const char* src_name, const void* src_ptr,
    uint64_t size)
{
  if (KOKKOS_BEGIN_DEEP_COPY_ENABLED()) {
    KOKKOS_BEGIN_DEEP_COPY(dst_handle, dst_name, dst_ptr,
                      src_handle, src_name, src_ptr,
                      size);
  }
}

void kokkosp_end_deep_copy()
{
  if (KOKKOS_END_DEEP_COPY_ENABLED()) {
    KOKKOS_END_DEEP_COPY();
  }
}
      
void kokkosp_create_profile_section(const char* name, uint32_t* sec_id)
{
  *sec_id = next_sec_id++;
  if (KOKKOS_CREATE_PROFILE_SECTION_ENABLED()) {
    KOKKOS_CREATE_PROFILE_SECTION(name, sec_id);
  }
}

void kokkosp_start_profile_section(const uint32_t sec_id)
{
  if (KOKKOS_START_PROFILE_SECTION_ENABLED()) {
    KOKKOS_START_PROFILE_SECTION(sec_id);
  }
}

void kokkosp_stop_profile_section(const uint32_t sec_id)
{
  if (KOKKOS_STOP_PROFILE_SECTION_ENABLED()) {
    KOKKOS_STOP_PROFILE_SECTION(sec_id);
  }
}

void kokkosp_destroy_profile_section(const uint32_t sec_id)
{
  if (KOKKOS_DESTROY_PROFILE_SECTION_ENABLED()) {
    KOKKOS_DESTROY_PROFILE_SECTION(sec_id);
  }
}
      
void kokkosp_profile_event(const char* name)
{
  if (KOKKOS_PROFILE_EVENT_ENABLED()) {
    KOKKOS_PROFILE_EVENT(name);
  }
}

Kokkos::Tools::Experimental::EventSet get_event_set() {
    Kokkos::Tools::Experimental::EventSet my_event_set;
    memset(&my_event_set, 0, sizeof(my_event_set)); // zero any pointers not set here
    my_event_set.init = kokkosp_init_library;
    my_event_set.finalize = kokkosp_finalize_library;
    my_event_set.push_region = kokkosp_push_profile_region;
    my_event_set.pop_region = kokkosp_pop_profile_region;
    my_event_set.allocate_data = kokkosp_allocate_data;
    my_event_set.deallocate_data = kokkosp_deallocate_data;
    my_event_set.begin_deep_copy = kokkosp_begin_deep_copy;
    my_event_set.end_deep_copy = kokkosp_end_deep_copy;
    my_event_set.begin_parallel_for = kokkosp_begin_parallel_for;
    my_event_set.begin_parallel_reduce = kokkosp_begin_parallel_reduce;
    my_event_set.begin_parallel_scan = kokkosp_begin_parallel_scan;
    my_event_set.end_parallel_for = kokkosp_end_parallel_for;
    my_event_set.end_parallel_reduce = kokkosp_end_parallel_reduce;
    my_event_set.end_parallel_scan = kokkosp_end_parallel_scan;
    my_event_set.create_profile_section = kokkosp_create_profile_section;
    my_event_set.start_profile_section = kokkosp_start_profile_section;
    my_event_set.stop_profile_section = kokkosp_stop_profile_section;
    my_event_set.destroy_profile_section = kokkosp_destroy_profile_section;
    my_event_set.profile_event = kokkosp_profile_event;
    return my_event_set;
}

}} // namespace KokkosTools::SystemtapConnector


extern "C" {

namespace impl = KokkosTools::SystemtapConnector;

EXPOSE_INIT(impl::kokkosp_init_library)
EXPOSE_FINALIZE(impl::kokkosp_finalize_library)
EXPOSE_PUSH_REGION(impl::kokkosp_push_profile_region)
EXPOSE_POP_REGION(impl::kokkosp_pop_profile_region)
EXPOSE_ALLOCATE(impl::kokkosp_allocate_data)
EXPOSE_DEALLOCATE(impl::kokkosp_deallocate_data)
EXPOSE_BEGIN_PARALLEL_FOR(impl::kokkosp_begin_parallel_for)
EXPOSE_END_PARALLEL_FOR(impl::kokkosp_end_parallel_for)
EXPOSE_BEGIN_PARALLEL_SCAN(impl::kokkosp_begin_parallel_scan)
EXPOSE_END_PARALLEL_SCAN(impl::kokkosp_end_parallel_scan)
EXPOSE_BEGIN_PARALLEL_REDUCE(impl::kokkosp_begin_parallel_reduce)
EXPOSE_END_PARALLEL_REDUCE(impl::kokkosp_end_parallel_reduce)
EXPOSE_CREATE_PROFILE_SECTION(impl::kokkosp_create_profile_section)
EXPOSE_START_PROFILE_SECTION(impl::kokkosp_start_profile_section)
EXPOSE_STOP_PROFILE_SECTION(impl::kokkosp_stop_profile_section)
EXPOSE_DESTROY_PROFILE_SECTION(impl::kokkosp_destroy_profile_section)
EXPOSE_PROFILE_EVENT(impl::kokkosp_profile_event)

} // extern "C"