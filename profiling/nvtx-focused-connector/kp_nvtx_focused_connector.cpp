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
#include <unordered_map>
#include <string>
#include <cxxabi.h>
#include <cuda_profiler_api.h>

#include "kp_nvtx_focused_connector_domain.h"

#include "kp_core.hpp"

static bool tool_globfences;
namespace KokkosTools {
namespace NVTXFocusedConnector {

void kokkosp_request_tool_settings(const uint32_t,
                                   Kokkos_Tools_ToolSettings* settings) {
  settings->requires_global_fencing = true;
  if (tool_globfences) {
    settings->requires_global_fencing = true;
  } else {
    settings->requires_global_fencing = false;
  }
}  // end request tool settings

static KernelNVTXFocusedConnectorInfo* currentKernel;
static std::unordered_map<std::string, KernelNVTXFocusedConnectorInfo*>
    domain_map;
static uint64_t nextKernelID;

void kokkosp_init_library(
    const int loadSeq, const uint64_t interfaceVer,
    const uint32_t /*devInfoCount*/,
    struct Kokkos_Profiling_KokkosPDeviceInfo* /*deviceInfo*/) {
  printf("-----------------------------------------------------------\n");
  printf(
      "KokkosP: NVTX Analyzer Focused Connector (sequence is %d, version: "
      "%llu)\n",
      loadSeq, (unsigned long long)(interfaceVer));
  printf("-----------------------------------------------------------\n");
  const char* tool_global_fences = getenv("KOKKOS_TOOLS_GLOBALFENCES");
  if (NULL != tool_global_fences) {
    tool_globfences = (atoi(tool_global_fences) != 0); // if user sets to 0, no global fences
  } else {
    tool_globfences =
        true;  // default to true to be conservative for capturing state by tool
  }
  nvtxNameOsThread(pthread_self(), "Application Main Thread");
  nvtxMarkA("Kokkos::Initialization Complete");
  nextKernelID = 0;
}  // end kokkosp_init_library

KernelNVTXFocusedConnectorInfo* getFocusedConnectorInfo(
    const char* name, KernelExecutionType kType) {
  std::string nameStr(name);
  auto kDomain  = domain_map.find(nameStr);
  currentKernel = NULL;

  if (kDomain == domain_map.end()) {
    currentKernel = new KernelNVTXFocusedConnectorInfo(name, kType);
    domain_map.insert(std::pair<std::string, KernelNVTXFocusedConnectorInfo*>(
        nameStr, currentKernel));
  } else {
    currentKernel = kDomain->second;
  }

  return currentKernel;
}  // end getFocusedConnectorInfo

void focusedConnectorExecuteStart() {
  cudaProfilerStart();
  currentKernel->startRange();
}  // end focusedConnectorExecuteStart

void focusedConnectorExecuteEnd() {
  currentKernel->endRange();
  cudaProfilerStop();
  currentKernel = NULL;
}  // end focusedConnectorExecuteEnd

void kokkosp_finalize_library() {
  printf("-----------------------------------------------------------\n");
  printf(
      "KokkosP: Finalization of NVTX Analyzer Focused Connector. Complete.\n");
  printf("-----------------------------------------------------------\n");
}  // end kokkosp_finalize_library

void kokkosp_begin_parallel_for(const char* name, const uint32_t /*devID*/,
                                uint64_t* kID) {
  *kID = nextKernelID++;
  currentKernel = getFocusedConnectorInfo(name, PARALLEL_FOR);
  focusedConnectorExecuteStart();
}

void kokkosp_end_parallel_for(const uint64_t /*kID*/) {
  focusedConnectorExecuteEnd();
}

void kokkosp_begin_parallel_scan(const char* name, const uint32_t /*devID*/,
                                 uint64_t* kID) {
  *kID = nextKernelID++;
  currentKernel = getFocusedConnectorInfo(name, PARALLEL_SCAN);
  focusedConnectorExecuteStart();
}

void kokkosp_end_parallel_scan(const uint64_t /*kID*/) {
  focusedConnectorExecuteEnd();
}

void kokkosp_begin_parallel_reduce(const char* name, const uint32_t /*devID*/,
                                   uint64_t* kID) {
  *kID = nextKernelID++;
  currentKernel = getFocusedConnectorInfo(name, PARALLEL_REDUCE);
  focusedConnectorExecuteStart();
}

void kokkosp_end_parallel_reduce(const uint64_t /*kID*/) {
  focusedConnectorExecuteEnd();
}

Kokkos::Tools::Experimental::EventSet get_event_set() {
  Kokkos::Tools::Experimental::EventSet my_event_set;
  memset(&my_event_set, 0,
         sizeof(my_event_set));  // zero any pointers not set here
  my_event_set.request_tool_settings = kokkosp_request_tool_settings;
  my_event_set.init                  = kokkosp_init_library;
  my_event_set.finalize              = kokkosp_finalize_library;
  my_event_set.begin_parallel_for    = kokkosp_begin_parallel_for;
  my_event_set.begin_parallel_reduce = kokkosp_begin_parallel_reduce;
  my_event_set.begin_parallel_scan   = kokkosp_begin_parallel_scan;
  my_event_set.end_parallel_for      = kokkosp_end_parallel_for;
  my_event_set.end_parallel_reduce   = kokkosp_end_parallel_reduce;
  my_event_set.end_parallel_scan     = kokkosp_end_parallel_scan;
  return my_event_set;
}  // end get_event_set

}  // namespace NVTXFocusedConnector
}  // namespace KokkosTools

extern "C" {
namespace impl = KokkosTools::NVTXFocusedConnector;
EXPOSE_TOOL_SETTINGS(impl::kokkosp_request_tool_settings)
EXPOSE_INIT(impl::kokkosp_init_library)
EXPOSE_FINALIZE(impl::kokkosp_finalize_library)
EXPOSE_BEGIN_PARALLEL_FOR(impl::kokkosp_begin_parallel_for)
EXPOSE_END_PARALLEL_FOR(impl::kokkosp_end_parallel_for)
EXPOSE_BEGIN_PARALLEL_SCAN(impl::kokkosp_begin_parallel_scan)
EXPOSE_END_PARALLEL_SCAN(impl::kokkosp_end_parallel_scan)
EXPOSE_BEGIN_PARALLEL_REDUCE(impl::kokkosp_begin_parallel_reduce)
EXPOSE_END_PARALLEL_REDUCE(impl::kokkosp_end_parallel_reduce)
}  // extern "C"
