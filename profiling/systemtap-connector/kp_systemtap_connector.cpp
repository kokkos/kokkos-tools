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

struct SpaceHandle {
  char name[64];
};

static uint64_t next_kernid;
static uint32_t next_sec_id;

extern "C" void kokkosp_begin_parallel_for(const char* name,
                                           const uint32_t devid,
                                           uint64_t* kernid) {
  *kernid = next_kernid++;
  if (KOKKOS_END_PARALLEL_FOR_ENABLED()) {
    KOKKOS_BEGIN_PARALLEL_FOR(name, devid, kernid);
  }
}

extern "C" void kokkosp_begin_parallel_scan(const char* name,
                                            const uint32_t devid,
                                            uint64_t* kernid) {
  *kernid = next_kernid++;
  if (KOKKOS_BEGIN_PARALLEL_SCAN_ENABLED()) {
    KOKKOS_BEGIN_PARALLEL_SCAN(name, devid, kernid);
  }
}

extern "C" void kokkosp_begin_parallel_reduce(const char* name,
                                              const uint32_t devid,
                                              uint64_t* kernid) {
  *kernid = next_kernid++;
  if (KOKKOS_BEGIN_PARALLEL_REDUCE_ENABLED()) {
    KOKKOS_BEGIN_PARALLEL_REDUCE(name, devid, kernid);
  }
}

extern "C" void kokkosp_end_parallel_scan(uint64_t kernid) {
  if (KOKKOS_END_PARALLEL_SCAN_ENABLED()) {
    KOKKOS_END_PARALLEL_SCAN(kernid);
  }
}

extern "C" void kokkosp_end_parallel_for(uint64_t kernid) {
  if (KOKKOS_END_PARALLEL_FOR_ENABLED()) {
    KOKKOS_END_PARALLEL_FOR(kernid);
  }
}

extern "C" void kokkosp_end_parallel_reduce(uint64_t kernid) {
  if (KOKKOS_END_PARALLEL_REDUCE_ENABLED()) {
    KOKKOS_END_PARALLEL_REDUCE(kernid);
  }
}

extern "C" void kokkosp_init_library(const int loadseq, const uint64_t version,
                                     const uint32_t ndevinfos,
                                     void* deviceinfos) {
  if (KOKKOS_INIT_LIBRARY_ENABLED()) {
    KOKKOS_INIT_LIBRARY(loadseq, version, ndevinfos, deviceinfos);
  }
}

extern "C" void kokkosp_finalize_library() {
  if (KOKKOS_FINALIZE_LIBRARY_ENABLED()) {
    KOKKOS_FINALIZE_LIBRARY();
  }
}

extern "C" void kokkosp_push_profile_region(const char* name) {
  if (KOKKOS_PUSH_PROFILE_REGION_ENABLED()) {
    KOKKOS_PUSH_PROFILE_REGION(name);
  }
}

extern "C" void kokkosp_pop_profile_region() {
  if (KOKKOS_POP_PROFILE_REGION_ENABLED()) {
    KOKKOS_POP_PROFILE_REGION();
  }
}

extern "C" void kokkosp_allocate_data(SpaceHandle handle, const char* name,
                                      void* ptr, uint64_t size) {
  if (KOKKOS_ALLOCATE_DATA_ENABLED()) {
    KOKKOS_ALLOCATE_DATA(handle, name, ptr, size);
  }
}

extern "C" void kokkosp_deallocate_data(SpaceHandle handle, const char* name,
                                        void* ptr, uint64_t size) {
  if (KOKKOS_DEALLOCATE_DATA_ENABLED()) {
    KOKKOS_DEALLOCATE_DATA(handle, name, ptr, size);
  }
}

extern "C" void kokkosp_begin_deep_copy(SpaceHandle dst_handle,
                                        const char* dst_name,
                                        const void* dst_ptr,
                                        SpaceHandle src_handle,
                                        const char* src_name,
                                        const void* src_ptr, uint64_t size) {
  if (KOKKOS_BEGIN_DEEP_COPY_ENABLED()) {
    KOKKOS_BEGIN_DEEP_COPY(dst_handle, dst_name, dst_ptr, src_handle, src_name,
                           src_ptr, size);
  }
}

extern "C" void kokkosp_end_deep_copy() {
  if (KOKKOS_END_DEEP_COPY_ENABLED()) {
    KOKKOS_END_DEEP_COPY();
  }
}

extern "C" void kokkosp_create_profile_section(const char* name,
                                               uint32_t* sec_id) {
  *sec_id = next_sec_id++;
  if (KOKKOS_CREATE_PROFILE_SECTION_ENABLED()) {
    KOKKOS_CREATE_PROFILE_SECTION(name, sec_id);
  }
}

extern "C" void kokkosp_start_profile_section(const uint32_t sec_id) {
  if (KOKKOS_START_PROFILE_SECTION_ENABLED()) {
    KOKKOS_START_PROFILE_SECTION(sec_id);
  }
}

extern "C" void kokkosp_stop_profile_section(const uint32_t sec_id) {
  if (KOKKOS_STOP_PROFILE_SECTION_ENABLED()) {
    KOKKOS_STOP_PROFILE_SECTION(sec_id);
  }
}

extern "C" void kokkosp_destroy_profile_section(const uint32_t sec_id) {
  if (KOKKOS_DESTROY_PROFILE_SECTION_ENABLED()) {
    KOKKOS_DESTROY_PROFILE_SECTION(sec_id);
  }
}

extern "C" void kokkosp_profile_event(const char* name) {
  if (KOKKOS_PROFILE_EVENT_ENABLED()) {
    KOKKOS_PROFILE_EVENT(name);
  }
}
