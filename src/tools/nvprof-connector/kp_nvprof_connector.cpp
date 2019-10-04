
#include <stdio.h>
#include <inttypes.h>
#include <cstdlib>
#include <cstring>
#include <unordered_map>
#include <stack>
#include <string>
#include <iostream>

#include "nvToolsExt.h"

static std::unordered_map<uint64_t, nvtxRangeId_t> range_map;
static std::stack<nvtxRangeId_t> region_range_stack;
static uint64_t nextKernelID;

extern "C" void kokkosp_init_library(const int loadSeq,
	const uint64_t interfaceVer,
	const uint32_t devInfoCount,
	void* deviceInfo) {

	printf("-----------------------------------------------------------\n");
	printf("KokkosP: NVTX Analyzer Connector (sequence is %d, version: %llu)\n", loadSeq, interfaceVer);
	printf("-----------------------------------------------------------\n");

	nextKernelID = 0;
	nvtxNameOsThread(pthread_self(), "Application Main Thread");
	nvtxMarkA("Kokkos::Initialization Complete");
}

extern "C" void kokkosp_finalize_library() {
	printf("-----------------------------------------------------------\n");
	printf("KokkosP: Finalization of NVTX Connector. Complete.\n");
	printf("-----------------------------------------------------------\n");

	nvtxMarkA("Kokkos::Finalization Complete");
}

extern "C" void kokkosp_begin_parallel_for(const char* name, const uint32_t devID, uint64_t* kID) {
	*kID = nextKernelID++;

	nvtxRangeId_t kernelRangeMarker = nvtxRangeStartA(name);
	range_map.insert( std::pair<uint64_t, nvtxRangeId_t>(*kID, kernelRangeMarker) );
}

extern "C" void kokkosp_end_parallel_for(const uint64_t kID) {
	auto findKernel = range_map.find(kID);

	if(findKernel != range_map.end()) {
		nvtxRangeEnd(findKernel->second);
		range_map.erase(findKernel);
	} else {
		std::cerr << "KokkosP: Error - unable to find kernel " << kID << std::endl;
	}
}

extern "C" void kokkosp_begin_parallel_scan(const char* name, const uint32_t devID, uint64_t* kID) {
	*kID = nextKernelID++;

	nvtxRangeId_t kernelRangeMarker = nvtxRangeStartA(name);
	range_map.insert( std::pair<uint64_t, nvtxRangeId_t>(*kID, kernelRangeMarker) );
}

extern "C" void kokkosp_end_parallel_scan(const uint64_t kID) {
	auto findKernel = range_map.find(kID);

	if(findKernel != range_map.end()) {
		nvtxRangeEnd(findKernel->second);
		range_map.erase(findKernel);
	} else {
		std::cerr << "KokkosP: Error - unable to find kernel " << kID << std::endl;
	}
}

extern "C" void kokkosp_begin_parallel_reduce(const char* name, const uint32_t devID, uint64_t* kID) {
	*kID = nextKernelID++;

	nvtxRangeId_t kernelRangeMarker = nvtxRangeStartA(name);
	range_map.insert( std::pair<uint64_t, nvtxRangeId_t>(*kID, kernelRangeMarker) );
}

extern "C" void kokkosp_end_parallel_reduce(const uint64_t kID) {
	auto findKernel = range_map.find(kID);

	if(findKernel != range_map.end()) {
		nvtxRangeEnd(findKernel->second);
		range_map.erase(findKernel);
	} else {
		std::cerr << "KokkosP: Error - unable to find kernel " << kID << std::endl;
	}
}

extern "C" void kokkosp_push_profile_region(char* regionName) {
	nvtxRangeId_t kernelRangeMarker = nvtxRangeStartA(regionName);
	region_range_stack.push( kernelRangeMarker );
}

extern "C" void kokkosp_pop_profile_region() {
    if(region_range_stack.empty()){
		std::cerr << "KokkosP: Error - popped region with no active regions pushed. " << std::endl;
    }
    else{
      auto stack_top = region_range_stack.top();
      nvtxRangeEnd(stack_top);
      region_range_stack.pop();
    }
}
