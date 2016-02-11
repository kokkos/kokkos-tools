
#include <stdio.h>
#include <inttypes.h>
#include <execinfo.h>
#include <stdlib.h>
#include <string.h>
#include <map>
#include <vector>
#include <algorithm>
#include <string>
#include <sys/time.h>
#include <cxxabi.h>
#include <sys/time.h>
#include <sys/resource.h>

static uint64_t uniqID = 0;

extern "C" void kokkosp_init_library(const int loadSeq,
	const uint64_t interfaceVer,
	const uint32_t devInfoCount,
	void* deviceInfo) {

	printf("KokkosP: Example Library Initialized (sequence is %d, version: %llu)\n", loadSeq, interfaceVer);
}

extern "C" void kokkosp_finalize_library() {
	printf("\n");
	printf("KokkosP: Finalization of profiling library.\n");

	struct rusage sys_resources;
	getrusage(RUSAGE_SELF, &sys_resources);

	printf("KokkosP: High water mark memory consumption: %d kB\n",
		sys_resources.ru_maxrss);
	printf("\n");
}
