// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// SPDX-FileCopyrightText: Copyright Contributors to the Kokkos project

#ifndef _H_KOKKOSP_KERNEL_VTUNE_CONNECTOR_INFO
#define _H_KOKKOSP_KERNEL_VTUNE_CONNECTOR_INFO

#include <stdio.h>
#include <cstring>

#include "ittnotify.h"

namespace KokkosTools {
namespace VTuneFocusedConnector {

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
}  // namespace VTuneFocusedConnector
}  // namespace KokkosTools

#endif
