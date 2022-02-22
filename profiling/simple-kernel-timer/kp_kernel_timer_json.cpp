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
#include <algorithm>
#include <string>
#include <unistd.h>

#include "kp_core.hpp"
#include "kp_shared.h"

using namespace KokkosTools::KernelTimer;

namespace KokkosTools {
namespace KernelTimerJSON {

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

	printf("KokkosP: LDMS JSON Connector Initialized (sequence is %d, version: %llu)\n",
		loadSeq, (long long unsigned int)interfaceVer);

	initTime = seconds();
}

void kokkosp_finalize_library() {
	double finishTime = seconds();
	double kernelTimes = 0;

	char* mpi_rank = getenv("OMPI_COMM_WORLD_RANK");

	char* hostname = (char*) malloc(sizeof(char) * 256);
	gethostname(hostname, 256);

	char* fileOutput = (char*) malloc(sizeof(char) * 256);
	sprintf(fileOutput, "%s-%d-%s.json", hostname, (int) getpid(),
		(NULL == mpi_rank) ? "0" : mpi_rank);

	free(hostname);
	FILE* output_data = fopen(fileOutput, "w");

	const double totalExecuteTime = (finishTime - initTime);
	std::vector<KernelPerformanceInfo*> kernelList;

	for(auto kernel_itr = count_map.begin(); kernel_itr != count_map.end(); kernel_itr++) {
		kernelList.push_back(kernel_itr->second);
		kernelTimes += kernel_itr->second->getTime();
	}

	std::sort(kernelList.begin(), kernelList.end(), compareKernelPerformanceInfo);

	fprintf(output_data, "{\n\"kokkos-kernel-data\" : {\n");
	fprintf(output_data, "    \"mpi-rank\"               : %s,\n",
		(NULL == mpi_rank) ? "0" : mpi_rank);
	fprintf(output_data, "    \"total-app-time\"         : %10.3f,\n", totalExecuteTime);
	fprintf(output_data, "    \"total-kernel-times\"     : %10.3f,\n", kernelTimes);
	fprintf(output_data, "    \"total-non-kernel-times\" : %10.3f,\n", (totalExecuteTime - kernelTimes));

	const double percentKokkos = (kernelTimes / totalExecuteTime) * 100.0;
	fprintf(output_data, "    \"percent-in-kernels\"     : %6.2f,\n", percentKokkos);
	fprintf(output_data, "    \"unique-kernel-calls\"    : %22lu,\n", (uint64_t) count_map.size());
	fprintf(output_data, "\n");

	fprintf(output_data, "    \"kernel-perf-info\"       : [\n");

	#define KERNEL_INFO_INDENT "       "

        bool print_comma = false;
	for(auto const& kernel : count_map) {
                if (print_comma)
			fprintf(output_data, ",\n");
		kernel.second->writeToJSONFile(output_data, KERNEL_INFO_INDENT);
		print_comma = true;
	}

	fprintf(output_data, "\n");
	fprintf(output_data, "    ]\n");
	fprintf(output_data, "}\n}");
	fclose(output_data);
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

}} // namespace KokkosTools::KernelTimerJSON

extern "C" {

namespace impl = KokkosTools::KernelTimerJSON;

EXPOSE_INIT(impl::kokkosp_init_library)
EXPOSE_FINALIZE(impl::kokkosp_finalize_library)
EXPOSE_BEGIN_PARALLEL_FOR(impl::kokkosp_begin_parallel_for)
EXPOSE_END_PARALLEL_FOR(impl::kokkosp_end_parallel_for)
EXPOSE_BEGIN_PARALLEL_SCAN(impl::kokkosp_begin_parallel_scan)
EXPOSE_END_PARALLEL_SCAN(impl::kokkosp_end_parallel_scan)
EXPOSE_BEGIN_PARALLEL_REDUCE(impl::kokkosp_begin_parallel_reduce)
EXPOSE_END_PARALLEL_REDUCE(impl::kokkosp_end_parallel_reduce)

} // extern "C"
