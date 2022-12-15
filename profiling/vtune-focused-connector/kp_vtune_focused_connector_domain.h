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

#ifndef _H_KOKKOSP_KERNEL_VTUNE_CONNECTOR_INFO
#define _H_KOKKOSP_KERNEL_VTUNE_CONNECTOR_INFO

#include <stdio.h>
#include <sys/time.h>
#include <cstring>

#include "ittnotify.h"

enum KernelExecutionType {
  PARALLEL_FOR    = 0,
  PARALLEL_REDUCE = 1,
  PARALLEL_SCAN   = 2
};

class KernelVTuneFocusedConnectorInfo {
 public:
  KernelVTuneFocusedConnectorInfo(std::string kName,
                                  KernelExecutionType kernelType) {
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

    domain           = __itt_domain_create(domainName);
    domainNameHandle = __itt_string_handle_create(domainName);

    // Enable the domain for profile capture
    domain->flags = 1;
  }

  __itt_domain* getDomain() { return domain; }

  __itt_string_handle* getDomainNameHandle() { return domainNameHandle; }

  ~KernelVTuneFocusedConnectorInfo() {}

 private:
  __itt_domain* domain;
  __itt_string_handle* domainNameHandle;
};

#endif
