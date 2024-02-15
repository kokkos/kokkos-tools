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

#ifndef KOKKOSTOOLS_KOKKOSINTERFACE_HPP
#define KOKKOSTOOLS_KOKKOSINTERFACE_HPP

#include <cstring>

#include "kp_config.hpp"
#include "impl/Kokkos_Profiling_Interface.hpp"  // Note: impl/... is used inside the header

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
#define EXPOSE_BEGIN_FENCE(FUNC_NAME)
#define EXPOSE_END_FENCE(FUNC_NAME)
#define EXPOSE_PROVIDE_TOOL_PROGRAMMING_INTERFACE(FUNC_NAME)

#else

#define EXPOSE_PROVIDE_TOOL_PROGRAMMING_INTERFACE(FUNC_NAME)             \
  __attribute__((weak)) void kokkosp_provide_tool_programming_interface( \
      const uint32_t num_actions,                                        \
      Kokkos_Tools_ToolProgrammingInterface ptpi) {                     \
    FUNC_NAME(num_actions, ptpi);                                        \
  }

#define EXPOSE_TOOL_SETTINGS(FUNC_NAME)                                  \
  __attribute__((weak)) void kokkosp_request_tool_settings(              \
      const uint32_t num_actions, Kokkos_Tools_ToolSettings* settings) { \
    FUNC_NAME(num_actions, settings);                                    \
  }

#define EXPOSE_INIT(FUNC_NAME)                                  \
  __attribute__((weak)) void kokkosp_init_library(              \
      const int loadSeq, const uint64_t interfaceVer,           \
      const uint32_t devInfoCount,                              \
      Kokkos_Profiling_KokkosPDeviceInfo* deviceInfo) {         \
    FUNC_NAME(loadSeq, interfaceVer, devInfoCount, deviceInfo); \
  }

#define EXPOSE_NOARGFUNCTION(HANDLER_NAME, FUNC_NAME) \
  __attribute__((weak)) void HANDLER_NAME() { FUNC_NAME(); }

#define EXPOSE_FINALIZE(FUNC_NAME) \
  EXPOSE_NOARGFUNCTION(kokkosp_finalize_library, FUNC_NAME)

#define EXPOSE_ALLOCATE(FUNC_NAME)                                       \
  __attribute__((weak)) void kokkosp_allocate_data(                      \
      const SpaceHandle space, const char* label, const void* const ptr, \
      const uint64_t size) {                                             \
    FUNC_NAME(space, label, ptr, size);                                  \
  }

#define EXPOSE_DEALLOCATE(FUNC_NAME)                                     \
  __attribute__((weak)) void kokkosp_deallocate_data(                    \
      const SpaceHandle space, const char* label, const void* const ptr, \
      const uint64_t size) {                                             \
    FUNC_NAME(space, label, ptr, size);                                  \
  }

#define EXPOSE_PUSH_REGION(FUNC_NAME)                                        \
  __attribute__((weak)) void kokkosp_push_profile_region(const char* name) { \
    FUNC_NAME(name);                                                         \
  }

#define EXPOSE_POP_REGION(FUNC_NAME) \
  EXPOSE_NOARGFUNCTION(kokkosp_pop_profile_region, FUNC_NAME)

#define EXPOSE_BEGIN_PARALLEL_FOR(FUNC_NAME)                   \
  __attribute__((weak)) void kokkosp_begin_parallel_for(       \
      const char* name, const uint32_t devID, uint64_t* kID) { \
    FUNC_NAME(name, devID, kID);                               \
  }

#define EXPOSE_END_PARALLEL_FOR(FUNC_NAME)                                  \
  __attribute__((weak)) void kokkosp_end_parallel_for(const uint64_t kID) { \
    FUNC_NAME(kID);                                                         \
  }

#define EXPOSE_BEGIN_PARALLEL_SCAN(FUNC_NAME)                  \
  __attribute__((weak)) void kokkosp_begin_parallel_scan(      \
      const char* name, const uint32_t devID, uint64_t* kID) { \
    FUNC_NAME(name, devID, kID);                               \
  }

#define EXPOSE_END_PARALLEL_SCAN(FUNC_NAME)                                  \
  __attribute__((weak)) void kokkosp_end_parallel_scan(const uint64_t kID) { \
    FUNC_NAME(kID);                                                          \
  }

#define EXPOSE_BEGIN_PARALLEL_REDUCE(FUNC_NAME)                \
  __attribute__((weak)) void kokkosp_begin_parallel_reduce(    \
      const char* name, const uint32_t devID, uint64_t* kID) { \
    FUNC_NAME(name, devID, kID);                               \
  }

#define EXPOSE_END_PARALLEL_REDUCE(FUNC_NAME)                                  \
  __attribute__((weak)) void kokkosp_end_parallel_reduce(const uint64_t kID) { \
    FUNC_NAME(kID);                                                            \
  }

#define EXPOSE_BEGIN_DEEP_COPY(FUNC_NAME)                                   \
  __attribute__((weak)) void kokkosp_begin_deep_copy(                       \
      SpaceHandle dst_handle, const char* dst_name, const void* dst_ptr,    \
      SpaceHandle src_handle, const char* src_name, const void* src_ptr,    \
      uint64_t size) {                                                      \
    FUNC_NAME(dst_handle, dst_name, dst_ptr, src_handle, src_name, src_ptr, \
              size);                                                        \
  }

#define EXPOSE_END_DEEP_COPY(FUNC_NAME) \
  EXPOSE_NOARGFUNCTION(kokkosp_end_deep_copy, FUNC_NAME)

#define EXPOSE_CREATE_PROFILE_SECTION(FUNC_NAME)             \
  __attribute__((weak)) void kokkosp_create_profile_section( \
      const char* name, uint32_t* sec_id) {                  \
    FUNC_NAME(name, sec_id);                                 \
  }

#define EXPOSE_START_PROFILE_SECTION(FUNC_NAME)             \
  __attribute__((weak)) void kokkosp_start_profile_section( \
      const uint32_t sec_id) {                              \
    FUNC_NAME(sec_id);                                      \
  }

#define EXPOSE_STOP_PROFILE_SECTION(FUNC_NAME)             \
  __attribute__((weak)) void kokkosp_stop_profile_section( \
      const uint32_t sec_id) {                             \
    FUNC_NAME(sec_id);                                     \
  }

#define EXPOSE_DESTROY_PROFILE_SECTION(FUNC_NAME)             \
  __attribute__((weak)) void kokkosp_destroy_profile_section( \
      const uint32_t sec_id) {                                \
    FUNC_NAME(sec_id);                                        \
  }

#define EXPOSE_PROFILE_EVENT(FUNC_NAME)                                \
  __attribute__((weak)) void kokkosp_profile_event(const char* name) { \
    FUNC_NAME(name);                                                   \
  }

#define EXPOSE_BEGIN_FENCE(FUNC_NAME)                                \
  __attribute__((weak)) void kokkosp_begin_fence(                    \
      const char* name, const uint32_t deviceId, uint64_t* handle) { \
    FUNC_NAME(name, deviceId, handle);                               \
  }

#define EXPOSE_END_FENCE(FUNC_NAME)                               \
  __attribute__((weak)) void kokkosp_end_fence(uint64_t handle) { \
    FUNC_NAME(handle);                                            \
  }

#define EXPOSE_DUAL_VIEW_SYNC(FUNC_NAME)                         \
  __attribute__((weak)) void kokkosp_dual_view_sync(             \
      const char* name, const void* const ptr, bool is_device) { \
    FUNC_NAME(name, ptr, is_device);                             \
  }

#define EXPOSE_DUAL_VIEW_MODIFY(FUNC_NAME)                       \
  __attribute__((weak)) void kokkosp_dual_view_modify(           \
      const char* name, const void* const ptr, bool is_device) { \
    FUNC_NAME(name, ptr, is_device);                             \
  }
#endif
#endif  // KOKKOSTOOLS_KOKKOSINTERFACE_HPP
