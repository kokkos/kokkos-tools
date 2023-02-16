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

#ifndef _H_KOKKOSP_KERNEL_NVPROF_CONNECTOR_INFO
#define _H_KOKKOSP_KERNEL_NVPROF_CONNECTOR_INFO

#include <stdio.h>
#include <sys/time.h>
#include <cstring>

#include "nvToolsExt.h"

enum KernelExecutionType {
  PARALLEL_FOR    = 0,
  PARALLEL_REDUCE = 1,
  PARALLEL_SCAN   = 2
};

class KernelNVProfConnectorInfo {
 public:
  KernelNVProfConnectorInfo(std::string kName, KernelExecutionType kernelType) {
    domainNameHandle = kName;
    char* domainName = (char*)malloc(sizeof(char*) * (32 + kName.size()));

    if (kernelType == PARALLEL_FOR) {
      sprintf(domainName, "ParallelFor.%s", kName.c_str());
    } else if (kernelType == PARALLEL_REDUCE) {
      sprintf(domainName, "ParallelReduce.%s", kName.c_str());
    } else if (kernelType == PARALLEL_SCAN) {
      sprintf(domainName, "ParallelScan.%s", kName.c_str());
    } else {
      sprintf(domainName, "Kernel.%s", kName.c_str());
    }

    domain       = nvtxDomainCreateA(domainName);
    currentRange = 0;
  }

  nvtxRangeId_t startRange() {
    nvtxEventAttributes_t eventAttrib = {0};
    eventAttrib.version               = NVTX_VERSION;
    eventAttrib.size                  = NVTX_EVENT_ATTRIB_STRUCT_SIZE;
    eventAttrib.messageType           = NVTX_MESSAGE_TYPE_ASCII;
    eventAttrib.message.ascii         = "Kernel";
    currentRange = nvtxDomainRangeStartEx(domain, &eventAttrib);
    return currentRange;
  }

  nvtxRangeId_t getCurrentRange() { return currentRange; }

  void endRange() { nvtxDomainRangeEnd(domain, currentRange); }

  nvtxDomainHandle_t getDomain() { return domain; }

  std::string getDomainNameHandle() { return domainNameHandle; }

  ~KernelNVProfConnectorInfo() { nvtxDomainDestroy(domain); }

 private:
  std::string domainNameHandle;
  nvtxRangeId_t currentRange;
  nvtxDomainHandle_t domain;
};

#endif
