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

#ifndef KOKKOSTOOLS_KOKKOSINTERFACE_HPP
#define KOKKOSTOOLS_KOKKOSINTERFACE_HPP

#include <cstring>

#include "kp_config.hpp"
#include "impl/Kokkos_Profiling_Interface.hpp" // Note: impl/... is used inside the header

using Kokkos::Tools::SpaceHandle;

#ifdef WIN32

  #define EXPOSE_INIT(FUNC_NAME)
  #define EXPOSE_NOARGFUNCTION(HANDLER_NAME, FUNC_NAME)
  #define EXPOSE_FINALIZE(FUNC_NAME)
  #define EXPOSE_ALLOCATE(FUNC_NAME)
  #define EXPOSE_DEALLOCATE(FUNC_NAME)
  #define EXPOSE_PUSH_REGION(FUNC_NAME)
  #define EXPOSE_POP_REGION(FUNC_NAME)
  #define EXPOSE_BEGIN_PARALLEL_FOR(FUNC_NAME)
  #define EXPOSE_END_PARALLEL_FOR(FUNC_NAME)
  #define EXPOSE_BEGIN_PARALLEL_SCAN(FUNC_NAME)
  #define EXPOSE_END_PARALLEL_SCAN(FUNC_NAME)
  #define EXPOSE_BEGIN_PARALLEL_REDUCE(FUNC_NAME)
  #define EXPOSE_END_PARALLEL_REDUCE(FUNC_NAME)
  #define EXPOSE_BEGIN_DEEP_COPY(FUNC_NAME)
  #define EXPOSE_END_DEEP_COPY(FUNC_NAME)
  #define EXPOSE_CREATE_PROFILE_SECTION(FUNC_NAME)
  #define EXPOSE_START_PROFILE_SECTION(FUNC_NAME)
  #define EXPOSE_STOP_PROFILE_SECTION(FUNC_NAME)
  #define EXPOSE_DESTROY_PROFILE_SECTION(FUNC_NAME)
  #define EXPOSE_PROFILE_EVENT(FUNC_NAME)

#else

#define EXPOSE_TOOL_SETTINGS(FUNC_NAME) \
__attribute__((weak)) \
void kokkosp_request_tool_settings(const uint32_t num_actions, \
  Kokkos_Tools_ToolSettings* settings) { \
  FUNC_NAME(num_actions, settings); \
}

#define EXPOSE_INIT(FUNC_NAME) \
__attribute__((weak)) \
void kokkosp_init_library(const int loadSeq, \
	const uint64_t interfaceVer, \
	const uint32_t devInfoCount, \
	Kokkos_Profiling_KokkosPDeviceInfo* deviceInfo) { \
  FUNC_NAME(loadSeq, interfaceVer, devInfoCount, deviceInfo); \
}

#define EXPOSE_NOARGFUNCTION(HANDLER_NAME, FUNC_NAME) \
__attribute__((weak)) void HANDLER_NAME() { FUNC_NAME(); }

#define EXPOSE_FINALIZE(FUNC_NAME) EXPOSE_NOARGFUNCTION(kokkosp_finalize_library, FUNC_NAME)

#define EXPOSE_ALLOCATE(FUNC_NAME) \
__attribute__((weak)) \
void kokkosp_allocate_data(const SpaceHandle space, const char* label, const void* const ptr, const uint64_t size) { \
  FUNC_NAME(space, label, ptr, size); \
}

#define EXPOSE_DEALLOCATE(FUNC_NAME) \
__attribute__((weak)) \
void kokkosp_deallocate_data(const SpaceHandle space, const char* label, const void* const ptr, const uint64_t size) { \
  FUNC_NAME(space, label, ptr, size); \
}

#define EXPOSE_PUSH_REGION(FUNC_NAME) \
__attribute__((weak)) \
void kokkosp_push_profile_region(const char* name) { FUNC_NAME(name); }

#define EXPOSE_POP_REGION(FUNC_NAME) EXPOSE_NOARGFUNCTION(kokkosp_pop_profile_region, FUNC_NAME)

#define EXPOSE_BEGIN_PARALLEL_FOR(FUNC_NAME) \
__attribute__((weak)) \
void kokkosp_begin_parallel_for(const char* name, const uint32_t devID, uint64_t* kID) { \
	FUNC_NAME(name, devID, kID); \
}

#define EXPOSE_END_PARALLEL_FOR(FUNC_NAME) \
__attribute__((weak)) \
void kokkosp_end_parallel_for(const uint64_t kID) { \
	FUNC_NAME(kID); \
}

#define EXPOSE_BEGIN_PARALLEL_SCAN(FUNC_NAME) \
__attribute__((weak)) \
void kokkosp_begin_parallel_scan(const char* name, const uint32_t devID, uint64_t* kID) { \
	FUNC_NAME(name, devID, kID); \
}

#define EXPOSE_END_PARALLEL_SCAN(FUNC_NAME) \
__attribute__((weak)) \
void kokkosp_end_parallel_scan(const uint64_t kID) { \
	FUNC_NAME(kID); \
}

#define EXPOSE_BEGIN_PARALLEL_REDUCE(FUNC_NAME) \
__attribute__((weak)) \
void kokkosp_begin_parallel_reduce(const char* name, const uint32_t devID, uint64_t* kID) { \
	FUNC_NAME(name, devID, kID); \
}

#define EXPOSE_END_PARALLEL_REDUCE(FUNC_NAME) \
__attribute__((weak)) \
void kokkosp_end_parallel_reduce(const uint64_t kID) { \
	FUNC_NAME(kID); \
}

#define EXPOSE_BEGIN_DEEP_COPY(FUNC_NAME) \
__attribute__((weak)) \
void kokkosp_begin_deep_copy(SpaceHandle dst_handle, \
      const char *dst_name, const void *dst_ptr, \
      SpaceHandle src_handle, const char *src_name, \
      const void *src_ptr, uint64_t size) { \
  FUNC_NAME(dst_handle, dst_name, dst_ptr, src_handle, src_name, src_ptr, size); \
}

#define EXPOSE_END_DEEP_COPY(FUNC_NAME) EXPOSE_NOARGFUNCTION(kokkosp_end_deep_copy, FUNC_NAME)

#define EXPOSE_CREATE_PROFILE_SECTION(FUNC_NAME) \
__attribute__((weak)) \
void kokkosp_create_profile_section(const char* name, uint32_t* sec_id) { FUNC_NAME(name, sec_id); }

#define EXPOSE_START_PROFILE_SECTION(FUNC_NAME) \
__attribute__((weak)) \
void kokkosp_start_profile_section(const uint32_t sec_id) { FUNC_NAME(sec_id); }

#define EXPOSE_STOP_PROFILE_SECTION(FUNC_NAME) \
__attribute__((weak)) \
void kokkosp_stop_profile_section(const uint32_t sec_id) { FUNC_NAME(sec_id); }

#define EXPOSE_DESTROY_PROFILE_SECTION(FUNC_NAME) \
__attribute__((weak)) \
void kokkosp_destroy_profile_section(const uint32_t sec_id) { FUNC_NAME(sec_id); }

#define EXPOSE_PROFILE_EVENT(FUNC_NAME) \
__attribute__((weak)) \
void kokkosp_profile_event(const char* name) { FUNC_NAME(name); }
#endif

#endif // KOKKOSTOOLS_KOKKOSINTERFACE_HPP
