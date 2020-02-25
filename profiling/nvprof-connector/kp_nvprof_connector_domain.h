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

#ifndef _H_KOKKOSP_KERNEL_NVPROF_CONNECTOR_INFO
#define _H_KOKKOSP_KERNEL_NVPROF_CONNECTOR_INFO

#include <stdio.h>
#include <sys/time.h>
#include <cstring>

#include "nvToolsExt.h"

enum KernelExecutionType {
	PARALLEL_FOR = 0,
	PARALLEL_REDUCE = 1,
	PARALLEL_SCAN = 2
};

class KernelNVProfConnectorInfo {
	public:
		KernelNVProfConnectorInfo(std::string kName, KernelExecutionType kernelType) {

		  domainNameHandle = kName;
			char* domainName = (char*) malloc( sizeof(char*) * (32 + kName.size()) );

			if(kernelType == PARALLEL_FOR) {
				sprintf(domainName, "ParallelFor.%s", kName.c_str());
			} else if(kernelType == PARALLEL_REDUCE) {
				sprintf(domainName, "ParallelReduce.%s", kName.c_str());
			} else if(kernelType == PARALLEL_SCAN) {
				sprintf(domainName, "ParallelScan.%s", kName.c_str());
			} else {
				sprintf(domainName, "Kernel.%s", kName.c_str());
			}

			domain = nvtxDomainCreateA(domainName);
			currentRange = 0;
		}

		nvtxRangeId_t startRange() {
		  nvtxEventAttributes_t eventAttrib = {0};
		  eventAttrib.version = NVTX_VERSION;
		  eventAttrib.size = NVTX_EVENT_ATTRIB_STRUCT_SIZE;
		  eventAttrib.messageType = NVTX_MESSAGE_TYPE_ASCII;
		  eventAttrib.message.ascii = "Kernel";
		  currentRange = nvtxDomainRangeStartEx(domain,&eventAttrib);
		  return currentRange;
		}

		nvtxRangeId_t getCurrentRange() {
		  return currentRange;
		}

		void endRange() {
		  nvtxDomainRangeEnd(domain,currentRange);
		}

		nvtxDomainHandle_t getDomain() {
			return domain;
		}

		std::string getDomainNameHandle() {
			return domainNameHandle;
		}

		~KernelNVProfConnectorInfo() {
		  nvtxDomainDestroy(domain);
		}

	private:
		std::string domainNameHandle;
		nvtxRangeId_t currentRange;
		nvtxDomainHandle_t domain;
};

#endif
