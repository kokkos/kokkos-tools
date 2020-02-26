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

#include "kp_vtune_connector_domain.h"

static KernelVTuneConnectorInfo* currentKernel;
static std::unordered_map<std::string, KernelVTuneConnectorInfo*> domain_map;
static uint64_t nextKernelID;

extern "C" void kokkosp_init_library(const int loadSeq,
	const uint64_t interfaceVer,
	const uint32_t devInfoCount,
	void* deviceInfo) {

	printf("-----------------------------------------------------------\n");
	printf("KokkosP: VTune Analyzer Connector (sequence is %d, version: %llu)\n", loadSeq, interfaceVer);
	printf("-----------------------------------------------------------\n");

	nextKernelID = 0;
	const char* kpStartEventName = "Kokkos Initialization Complete";
	__itt_event startEv = __itt_event_create( kpStartEventName, strlen(kpStartEventName) );
	__itt_event_start(startEv);
}

extern "C" void kokkosp_finalize_library() {
	printf("-----------------------------------------------------------\n");
	printf("KokkosP: Finalization of VTune Connector. Complete.\n");
	printf("-----------------------------------------------------------\n");

	const char* kpFinalizeEventName = "Kokkos Finalize Complete";
	__itt_event finalEv = __itt_event_create( kpFinalizeEventName, strlen(kpFinalizeEventName) );
	__itt_event_start(finalEv);
}

extern "C" void kokkosp_begin_parallel_for(const char* name, const uint32_t devID, uint64_t* kID) {
	*kID = nextKernelID++;

	std::string nameStr(name);
	auto kDomain = domain_map.find(nameStr);
	currentKernel = NULL;

	if(kDomain == domain_map.end()) {
		currentKernel = new KernelVTuneConnectorInfo(name, PARALLEL_FOR);
		domain_map.insert(std::pair<std::string, KernelVTuneConnectorInfo*>(nameStr,
			currentKernel));
	} else {
		currentKernel = kDomain->second;
	}

	__itt_frame_begin_v3(currentKernel->getDomain(), NULL);
}

extern "C" void kokkosp_end_parallel_for(const uint64_t kID) {
	__itt_frame_end_v3(currentKernel->getDomain(), NULL);
	currentKernel = NULL;
}

extern "C" void kokkosp_begin_parallel_scan(const char* name, const uint32_t devID, uint64_t* kID) {
	*kID = nextKernelID++;

	std::string nameStr(name);
	auto kDomain = domain_map.find(nameStr);
	currentKernel = NULL;

	if(kDomain == domain_map.end()) {
		currentKernel = new KernelVTuneConnectorInfo(name, PARALLEL_SCAN);
		domain_map.insert(std::pair<std::string, KernelVTuneConnectorInfo*>(nameStr,
			currentKernel));
	} else {
		currentKernel = kDomain->second;
	}

	__itt_frame_begin_v3(currentKernel->getDomain(), NULL);

}

extern "C" void kokkosp_end_parallel_scan(const uint64_t kID) {
	__itt_frame_end_v3(currentKernel->getDomain(), NULL);
	currentKernel = NULL;
}

extern "C" void kokkosp_begin_parallel_reduce(const char* name, const uint32_t devID, uint64_t* kID) {
	*kID = nextKernelID++;

	std::string nameStr(name);
	auto kDomain = domain_map.find(nameStr);
	currentKernel = NULL;

	if(kDomain == domain_map.end()) {
		currentKernel = new KernelVTuneConnectorInfo(name, PARALLEL_REDUCE);
		domain_map.insert(std::pair<std::string, KernelVTuneConnectorInfo*>(nameStr,
			currentKernel));
	} else {
		currentKernel = kDomain->second;
	}

	__itt_frame_begin_v3(currentKernel->getDomain(), NULL);

}

extern "C" void kokkosp_end_parallel_reduce(const uint64_t kID) {
	__itt_frame_end_v3(currentKernel->getDomain(), NULL);
	currentKernel = NULL;
}

