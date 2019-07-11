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
#include <caliper/Annotation.h>
#include <caliper/cali.h>

extern "C" void kokkosp_init_library(const int loadSeq,
	const uint64_t interfaceVer,
	const uint32_t devInfoCount,
	void* deviceInfo) {

	char* hostname = (char*) malloc(sizeof(char) * 256);
	gethostname(hostname, 256);
	
	char* fileOutput = (char*) malloc(sizeof(char) * 256);
	sprintf(fileOutput, "%s-%d.cali", hostname, (int) getpid());

  cali_config_set("CALI_RECORDER_FILENAME",fileOutput);
  cali_config_set("CALI_SERVICES_ENABLE","timestamp:event:aggregate:recorder");

  free(hostname);
  free(fileOutput);
}

extern "C" void kokkosp_finalize_library() {
}

extern "C" void kokkosp_begin_parallel_for(const char* name, const uint32_t devID, uint64_t* kID) {
  cali::Annotation("kokkos#parallel_for").begin(name);
}

extern "C" void kokkosp_end_parallel_for(const uint64_t kID) {
	cali::Annotation("kokkos#parallel_for").end();
}
extern "C" void kokkosp_begin_parallel_scan(const char* name, const uint32_t devID, uint64_t* kID) {
  cali::Annotation("kokkos#parallel_scan").begin(name);
}

extern "C" void kokkosp_end_parallel_scan(const uint64_t kID) {
  cali::Annotation("kokkos#parallel_scan").end();
}

extern "C" void kokkosp_begin_parallel_reduce(const char* name, const uint32_t devID, uint64_t* kID) {
  cali::Annotation("kokkos#parallel_reduce").begin(name);
}

extern "C" void kokkosp_end_parallel_reduce(const uint64_t kID) {
  cali::Annotation("kokkos#parallel_reduce").end();
}

extern "C" void kokkosp_push_profile_region(char* regionName) {
  cali::Annotation("kokkos#region").begin(regionName); 
}

extern "C" void kokkosp_pop_profile_region() {
  cali::Annotation("kokkos#region").end(); 
}

