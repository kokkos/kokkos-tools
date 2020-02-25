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
#include <unordered_map>
#include <string>
#include <cxxabi.h>
#include <cuda_profiler_api.h>
#include "kp_nvprof_focused_connector_domain.h"

static KernelNVProfFocusedConnectorInfo* currentKernel;
static std::unordered_map<std::string, KernelNVProfFocusedConnectorInfo*> domain_map;
static uint64_t nextKernelID;

extern "C" void kokkosp_init_library(const int loadSeq,
	const uint64_t interfaceVer,
	const uint32_t devInfoCount,
	void* deviceInfo) {

	printf("-----------------------------------------------------------\n");
	printf("KokkosP: NVProf Analyzer Focused Connector (sequence is %d, version: %llu)\n", loadSeq, interfaceVer);
	printf("-----------------------------------------------------------\n");

	nextKernelID = 0;
}

KernelNVProfFocusedConnectorInfo* getFocusedConnectorInfo(const char* name,
		KernelExecutionType kType) {

	std::string nameStr(name);
	auto kDomain = domain_map.find(nameStr);
	currentKernel = NULL;

	if(kDomain == domain_map.end()) {
		currentKernel = new KernelNVProfFocusedConnectorInfo(name, kType);
		domain_map.insert(std::pair<std::string, KernelNVProfFocusedConnectorInfo*>(nameStr,
			currentKernel));
	} else {
		currentKernel = kDomain->second;
	}

	return currentKernel;
}

void focusedConnectorExecuteStart() {
  cudaProfilerStart();
  currentKernel->startRange();
}

void focusedConnectorExecuteEnd() {
  currentKernel->endRange();
  cudaProfilerStop();
	currentKernel = NULL;
}

extern "C" void kokkosp_finalize_library() {
	printf("-----------------------------------------------------------\n");
	printf("KokkosP: Finalization of NVProf Connector. Complete.\n");
	printf("-----------------------------------------------------------\n");

}

extern "C" void kokkosp_begin_parallel_for(const char* name, const uint32_t devID, uint64_t* kID) {
	*kID = nextKernelID++;

	currentKernel = getFocusedConnectorInfo(name, PARALLEL_FOR);
	focusedConnectorExecuteStart();
}

extern "C" void kokkosp_end_parallel_for(const uint64_t kID) {
	focusedConnectorExecuteEnd();
}

extern "C" void kokkosp_begin_parallel_scan(const char* name, const uint32_t devID, uint64_t* kID) {
	*kID = nextKernelID++;

	currentKernel = getFocusedConnectorInfo(name, PARALLEL_SCAN);
	focusedConnectorExecuteStart();
}

extern "C" void kokkosp_end_parallel_scan(const uint64_t kID) {
	focusedConnectorExecuteEnd();
}

extern "C" void kokkosp_begin_parallel_reduce(const char* name, const uint32_t devID, uint64_t* kID) {
	*kID = nextKernelID++;

	currentKernel = getFocusedConnectorInfo(name, PARALLEL_REDUCE);
 	focusedConnectorExecuteStart();
}

extern "C" void kokkosp_end_parallel_reduce(const uint64_t kID) {
	focusedConnectorExecuteEnd();
}
