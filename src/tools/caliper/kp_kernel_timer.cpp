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
#include <caliper/ChannelController.h>
#include <caliper/cali.h>
#include <caliper-config.h>
#include <map>

void declareConfigError(const std::string& message){
   std::cerr << message <<std::endl;
   // TODO: decide on errors exiting or falling back
}

extern "C" void kokkosp_init_library(const int loadSeq,
	const uint64_t interfaceVer,
	const uint32_t devInfoCount,
	void* deviceInfo) {

	char* hostname = (char*) malloc(sizeof(char) * 256);
	gethostname(hostname, 256);
  	
	char* fileOutput = (char*) malloc(sizeof(char) * 256);
	sprintf(fileOutput, "%s-%d.cali", hostname, (int) getpid());
  //cali_config_set("CALI_RECORDER_FILENAME",fileOutput);
  //cali_config_set("CALI_SERVICES_ENABLE","timestamp:event:aggregate:recorder");
  cali::config_map_t default_config {
   {"CALI_RECORDER_FILENAME",fileOutput},
   {"CALI_SERVICES_ENABLE","timestamp:event:aggregate:recorder"}
  };
  cali::config_map_t nvprof_config {
   {"CALI_RECORDER_FILENAME",fileOutput},
   {"CALI_SERVICES_ENABLE","event:trace:nvprof"}
  };
  char* chosenConfigEnvEntry = getenv("KOKKOS_CALIPER_CONFIG"); 
  std::string chosenConfig;
  if(chosenConfigEnvEntry == nullptr){
    chosenConfig = "DEFAULT";
  }
  else{
    chosenConfig = chosenConfigEnvEntry;
  }
  cali::config_map_t& config = default_config;
  if(chosenConfig == "DEFAULT") {
    config = default_config;
  }
  else if(chosenConfig=="NVPROF") {
    #ifndef CALIPER_HAVE_NVPROF
    declareConfigError("Requested nvprof caliper configuration, but Caliper wasn't built with NVPROF support.");
    #endif
    config = nvprof_config;
  }
  cali::ChannelController caliperChannel("kokkos", CALI_CHANNEL_ALLOW_READ_ENV, config);
  caliperChannel.start();
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

