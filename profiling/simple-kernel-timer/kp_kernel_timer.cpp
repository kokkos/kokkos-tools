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

#include <vector>
#include <string>
#include <iostream>
#include <unistd.h>

#include "kp_core.hpp"
#include "kp_shared.h"

namespace KokkosTools {
namespace KernelTimer {

void kokkosp_init_library(const int loadSeq,
	const uint64_t interfaceVer,
	const uint32_t devInfoCount,
	Kokkos_Profiling_KokkosPDeviceInfo* deviceInfo) {

	const char* output_delim_env = getenv("KOKKOSP_OUTPUT_DELIM");
	if(NULL == output_delim_env) {
		outputDelimiter = (char*) malloc(sizeof(char) * 2);
		sprintf(outputDelimiter, "%c", ' ');
	} else {
		outputDelimiter = (char*) malloc(sizeof(char) * (strlen(output_delim_env) + 1));
		sprintf(outputDelimiter, "%s", output_delim_env);
	}

	// initialize regions to 0s so we know if there is an object there
	memset(&regions[0], 0, 512 * sizeof(KernelPerformanceInfo*));

	printf("KokkosP: Example Library Initialized (sequence is %d, version: %llu)\n",
	    loadSeq, (long long unsigned int)interfaceVer);

	initTime = seconds();
}

void kokkosp_finalize_library() {
	double finishTime = seconds();
	double kernelTimes = 0;

	char* hostname = (char*) malloc(sizeof(char) * 256);
	gethostname(hostname, 256);

	char* fileOutput = (char*) malloc(sizeof(char) * 256);
	sprintf(fileOutput, "%s-%d.dat", hostname, (int) getpid());

	free(hostname);
	FILE* output_data = fopen(fileOutput, "wb");

	const double totalExecuteTime = (finishTime - initTime);
	fwrite(&totalExecuteTime, sizeof(totalExecuteTime), 1, output_data);

	std::vector<KernelPerformanceInfo*> kernelList;

	for(auto kernel_itr = count_map.begin(); kernel_itr != count_map.end(); kernel_itr++) {
		kernel_itr->second->writeToBinaryFile(output_data);
	}

	fclose(output_data);

	char currentwd[256];
  getcwd(currentwd, 256);
  printf("KokkosP: Kernel timing written to %s/%s \n", currentwd, fileOutput);

	/*printf("\n");
	printf("======================================================================\n");
	printf("KokkosP: Finalization of Profiling Library\n");
	printf("KokkosP: Executed a total of %llu kernels\n", uniqID);

	std::vector<KernelPerformanceInfo*> kernelList;

	for(auto kernel_itr = count_map.begin(); kernel_itr != count_map.end(); kernel_itr++) {
		kernelList.push_back(kernel_itr->second);
		kernelTimes += kernel_itr->second->getTime();
	}

	std::sort(kernelList.begin(), kernelList.end(), compareKernelPerformanceInfo);
	const double totalExecuteTime = (finishTime - initTime);

	if(0 == strcmp(outputDelimiter, " ")) {
		printf("KokkosP: %100s %14s %14s %6s %6s %14s %4s\n", "Kernel", "Calls",
			"s/Total", "\%/Ko", "\%/Tot", "s/Call", "Type");
	} else {
		printf("KokkosP: %s%s%s%s%s%s%s%s%s%s%s%s%s\n",
			"Kernel",
			outputDelimiter,
			"Calls",
			outputDelimiter,
			"s/Total",
			outputDelimiter,
			"\%/Ko",
			outputDelimiter,
			"\%/Tot",
			outputDelimiter,
			"s/Call",
			outputDelimiter,
			"Type");
	}

	for(auto kernel_itr = kernelList.begin(); kernel_itr != kernelList.end(); kernel_itr++) {
		KernelPerformanceInfo* kernelInfo = *kernel_itr;

		const uint64_t kCallCount = kernelInfo->getCallCount();
		const double   kTime      = kernelInfo->getTime();
		const double   kTimeMean  = kTime / (double) kCallCount;

		const std::string& kName   = kernelInfo->getName();
		char* kType = const_cast<char*>("");

		switch(kernelInfo->getKernelType()) {
		case PARALLEL_FOR:
			kType = const_cast<char*>("PFOR"); break;
		case PARALLEL_SCAN:
			kType = const_cast<char*>("SCAN"); break;
		case PARALLEL_REDUCE:
			kType = const_cast<char*>("RDCE"); break;
                case REGION
                        kType = const_cast<char*>("REGI"); break;
		}

		int demangleStatus;
		char* finalDemangle = abi::__cxa_demangle(kName.c_str(), 0, 0, &demangleStatus);

		if(0 == strcmp(outputDelimiter, " ")) {
			printf("KokkosP: %s%s%14llu%s%14.5f%s%6.2f%s%6.2f%s%14.5f%s%4s\n",
				(0 == demangleStatus) ? finalDemangle : kName.c_str(),
				outputDelimiter,
				kCallCount,
				outputDelimiter,
				kTime,
				outputDelimiter,
				(kTime / kernelTimes) * 100.0,
				outputDelimiter,
				(kTime / totalExecuteTime) * 100.0,
				outputDelimiter,
				kTimeMean,
				outputDelimiter,
				kType
				);
		} else {
			printf("KokkosP: %s%s%llu%s%f%s%f%s%f%s%f%s%s\n",
				(0 == demangleStatus) ? finalDemangle : kName.c_str(),
				outputDelimiter,
				kCallCount,
				outputDelimiter,
				kTime,
				outputDelimiter,
				(kTime / kernelTimes) * 100.0,
				outputDelimiter,
				(kTime / totalExecuteTime) * 100.0,
				outputDelimiter,
				kTimeMean,
				outputDelimiter,
				kType
				);
		}
	}

	printf("\n");
	printf("KokkosP: Total Execution Time:        %15.6f seconds.\n", totalExecuteTime);
	printf("KokkosP: Time in Kokkos Kernels:      %15.6f seconds.\n", kernelTimes);
	printf("KokkosP: Time spent outside Kokkos:   %15.6f seconds.\n", (totalExecuteTime - kernelTimes));

	const double percentKokkos = (kernelTimes / totalExecuteTime) * 100.0;
	printf("KokkosP: Runtime in Kokkos Kernels:   %15.6f \%\n", percentKokkos);
	printf("KokkosP: Unique kernels:              %22llu \n", (uint64_t) count_map.size());
	printf("KokkosP: Parallel For Calls:          %22llu \n", uniqID);

	printf("\n");
	printf("======================================================================\n");
	printf("\n");

	if(NULL != outputDelimiter) {
		free(outputDelimiter);
	}*/


}

void kokkosp_begin_parallel_for(const char* name, const uint32_t devID, uint64_t* kID) {
	*kID = uniqID++;

	if( (NULL == name) || (strcmp("", name) == 0) ) {
		fprintf(stderr, "Error: kernel is empty\n");
		exit(-1);
	}

	increment_counter(name, PARALLEL_FOR);
}

void kokkosp_end_parallel_for(const uint64_t kID) {
	currentEntry->addFromTimer();
}

void kokkosp_begin_parallel_scan(const char* name, const uint32_t devID, uint64_t* kID) {
	*kID = uniqID++;

	if( (NULL == name) || (strcmp("", name) == 0) ) {
		fprintf(stderr, "Error: kernel is empty\n");
		exit(-1);
	}

	increment_counter(name, PARALLEL_SCAN);
}

void kokkosp_end_parallel_scan(const uint64_t kID) {
	currentEntry->addFromTimer();
}

void kokkosp_begin_parallel_reduce(const char* name, const uint32_t devID, uint64_t* kID) {
	*kID = uniqID++;

	if( (NULL == name) || (strcmp("", name) == 0) ) {
		fprintf(stderr, "Error: kernel is empty\n");
		exit(-1);
	}

	increment_counter(name, PARALLEL_REDUCE);
}

void kokkosp_end_parallel_reduce(const uint64_t kID) {
	currentEntry->addFromTimer();
}

void kokkosp_push_profile_region(char* regionName) {
        increment_counter_region(regionName, REGION);
}

void kokkosp_pop_profile_region() {
        current_region_level--;

        // current_region_level is out of bounds, inform the user they
        // called popRegion too many times.
        if (current_region_level < 0) {
           current_region_level = 0;
           std::cerr << "WARNING:: Kokkos::Profiling::popRegion() called outside " <<
                   " of an actve region. Previous regions: ";

          /* This code block will walk back through the non-null regions
           * pointers and print the names.  This takes advantage of a slight
           * issue with regions logic: we never actually delete the
           * KernelPerformanceInfo objects.  If that ever changes this needs
           * to be updated.
           */
           for (int i = 0; i < 5; i++) {
              if (regions[i] != 0 ) {
                 std::cerr << (i == 0 ? " " : ";") << regions[i]->getName();
              } else {
                 break;
              }
           }
           std::cerr << "\n";
        } else {
           // don't call addFromTimer if we are outside an active region
           regions[current_region_level]->addFromTimer();
        }
}

Kokkos::Tools::Experimental::EventSet get_event_set() {
    Kokkos::Tools::Experimental::EventSet my_event_set;
    memset(&my_event_set, 0, sizeof(my_event_set)); // zero any pointers not set here
    my_event_set.init = kokkosp_init_library;
    my_event_set.finalize = kokkosp_finalize_library;
    my_event_set.begin_parallel_for = kokkosp_begin_parallel_for;
    my_event_set.begin_parallel_reduce = kokkosp_begin_parallel_reduce;
    my_event_set.begin_parallel_scan = kokkosp_begin_parallel_scan;
    my_event_set.end_parallel_for = kokkosp_end_parallel_for;
    my_event_set.end_parallel_reduce = kokkosp_end_parallel_reduce;
    my_event_set.end_parallel_scan = kokkosp_end_parallel_scan;
    return my_event_set;
}

}} // namespace KokkosTools::KernelTimer

extern "C" {

namespace impl = KokkosTools::KernelTimer;

EXPOSE_INIT(impl::kokkosp_init_library)
EXPOSE_FINALIZE(impl::kokkosp_finalize_library)
EXPOSE_BEGIN_PARALLEL_FOR(impl::kokkosp_begin_parallel_for)
EXPOSE_END_PARALLEL_FOR(impl::kokkosp_end_parallel_for)
EXPOSE_BEGIN_PARALLEL_SCAN(impl::kokkosp_begin_parallel_scan)
EXPOSE_END_PARALLEL_SCAN(impl::kokkosp_end_parallel_scan)
EXPOSE_BEGIN_PARALLEL_REDUCE(impl::kokkosp_begin_parallel_reduce)
EXPOSE_END_PARALLEL_REDUCE(impl::kokkosp_end_parallel_reduce)

} // extern "C"
