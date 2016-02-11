
#include <stdio.h>
#include <inttypes.h>
#include <execinfo.h>
#include <cstdlib>
#include <cstring>
#include <map>
#include <vector>
#include <algorithm>
#include <string>
#include <sys/time.h>
#include <cxxabi.h>
#include <unistd.h>
#include <kp_kernel_info_vtune.h>

static uint64_t uniqID = 0;
static KernelPerformanceInfoVtune* currentEntry;
static std::map<std::string, KernelPerformanceInfoVtune*> count_map;

#define MAX_STACK_SIZE 128

void find_kernel(const char* name, KernelExecutionType kType) {
        std::string nameStr(name);

        if(count_map.find(name) == count_map.end()) {
                KernelPerformanceInfoVtune* info = new KernelPerformanceInfoVtune(nameStr, kType);
                count_map.insert(std::pair<std::string, KernelPerformanceInfoVtune*>(nameStr, info));

                currentEntry = info;
        } else {
                currentEntry = count_map[nameStr];
        }
}

extern "C" void kokkosp_init_library(const int loadSeq,
	const uint64_t interfaceVer,
	const uint32_t devInfoCount,
	void* deviceInfo) {

	/*const char* output_delim_env = getenv("KOKKOSP_OUTPUT_DELIM");
	if(NULL == output_delim_env) {
		outputDelimiter = (char*) malloc(sizeof(char) * 2);
		sprintf(outputDelimiter, "%c", ' ');
	} else {
		outputDelimiter = (char*) malloc(sizeof(char) * (strlen(output_delim_env) + 1));
		sprintf(outputDelimiter, "%s", output_delim_env);
	}*/

	printf("KokkosP: Example Library Initialized (sequence is %d, version: %llu)\n", loadSeq, interfaceVer);
}

extern "C" void kokkosp_finalize_library() {
}

extern "C" void kokkosp_begin_parallel_for(const char* name, const uint32_t devID, uint64_t* kID) {
	*kID = uniqID++;

	if( (NULL == name) || (strcmp("", name) == 0) ) {
		fprintf(stderr, "Error: kernel is empty\n");
		exit(-1);
	}

        find_kernel(name, PARALLEL_FOR);
        currentEntry->start_frame();
}

extern "C" void kokkosp_end_parallel_for(const uint64_t kID) {
        currentEntry->end_frame();
}

extern "C" void kokkosp_begin_parallel_scan(const char* name, const uint32_t devID, uint64_t* kID) {
	*kID = uniqID++;

	if( (NULL == name) || (strcmp("", name) == 0) ) {
		fprintf(stderr, "Error: kernel is empty\n");
		exit(-1);
	}
        find_kernel(name, PARALLEL_SCAN);
        currentEntry->start_frame();
}

extern "C" void kokkosp_end_parallel_scan(const uint64_t kID) {
        currentEntry->end_frame();
}

extern "C" void kokkosp_begin_parallel_reduce(const char* name, const uint32_t devID, uint64_t* kID) {
	*kID = uniqID++;

	if( (NULL == name) || (strcmp("", name) == 0) ) {
		fprintf(stderr, "Error: kernel is empty\n");
		exit(-1);
	}
        find_kernel(name, PARALLEL_REDUCE);
        currentEntry->start_frame();
}

extern "C" void kokkosp_end_parallel_reduce(const uint64_t kID) {
        currentEntry->end_frame();
}

