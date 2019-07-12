#include <stdio.h>
#include <inttypes.h>
#include <execinfo.h>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <algorithm>
#include <string>
#include <sys/time.h>
#include <cxxabi.h>
#include <unistd.h>
#include <caliper/Annotation.h>
#include <caliper/ChannelController.h>
#include <caliper/cali.h>
#include <caliper/common/Variant.h>
#include <caliper/caliper-config.h>

#include "kp_memory_events.hpp"


bool caliper_kokkos_track_memory;
bool caliper_kokkos_tracing;
cali::ChannelController* caliperChannel = nullptr;
int num_spaces;

cali::Annotation::MetadataListType aggregatable_metadata {
  {"class.aggregatable", cali::Variant(true) }
};

void declareConfigError(const std::string& message){
   std::cerr << message <<std::endl;
   // TODO: decide on errors exiting or falling back
}

AnnotationsForSpace& getMemoryAnnotations(const char name[64]){
  int space_i = num_spaces;
  for(int s = 0; s<num_spaces; s++)
    if(strcmp(space_data[s].name,name)==0)
      space_i = s;

  if(space_i == num_spaces) {
    strncpy(space_data[num_spaces].name,name,64);
    std::string allocationName = std::string(name) + "#bytes_allocated";  
    std::string deallocationName = std::string(name) + "#bytes_deallocated";  
    space_data[num_spaces].allocationAnnotation = new cali::Annotation(allocationName.c_str(), aggregatable_metadata, CALI_ATTR_ASVALUE);
    space_data[num_spaces].deallocationAnnotation = new cali::Annotation(deallocationName.c_str(), aggregatable_metadata, CALI_ATTR_ASVALUE);
    num_spaces++;
  }
  return space_data[space_i];
}

extern "C" void kokkosp_init_library(const int loadSeq,
	const uint64_t interfaceVer,
	const uint32_t devInfoCount,
	void* deviceInfo) {

	char* hostname = (char*) malloc(sizeof(char) * 256);
	gethostname(hostname, 256);
  	
	char* fileOutput = (char*) malloc(sizeof(char) * 256);
	sprintf(fileOutput, "%s-%d.cali", hostname, (int) getpid());

  char* memoryEvents = getenv("KOKKOS_CALIPER_TRACK_MEMORY");
  if(memoryEvents){
    caliper_kokkos_track_memory = true;
  }
  cali::config_map_t default_config {
   {"CALI_RECORDER_FILENAME",fileOutput},
   {"CALI_SERVICES_ENABLE","timestamp:event:aggregate:recorder"}
  };
  cali::config_map_t nvprof_config {
   {"CALI_RECORDER_FILENAME",fileOutput},
   {"CALI_SERVICES_ENABLE","nvprof"}
  };
  cali::config_map_t trace_config {
   {"CALI_RECORDER_FILENAME",fileOutput},
   {"CALI_SERVICES_ENABLE","timestamp:event:trace:recorder"}
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
  else if(chosenConfig=="TRACE") {
    caliper_kokkos_tracing = true;
    config = trace_config;
  }
  else if(chosenConfig=="ENV"){
    /** this branch intentionally left blank, it lets users configure Caliper through the envioronment */
    /** TODO: if trace in services, set trace mode. */
    goto caliper_kokkos_tool_cleanup;
  }
  else{
    declareConfigError("Invalid configuration "+chosenConfig+", options are (DEFAULT,NVPROF,ENV)");
  }
  caliperChannel = new  cali::ChannelController("kokkos", 0, config);
  caliperChannel->start();
  caliper_kokkos_tool_cleanup:
  free(hostname);
  free(fileOutput);
}

extern "C" void kokkosp_finalize_library() {
  for(int x =0; x<num_spaces;++x){
    free(space_data[x].allocationAnnotation);
    free(space_data[x].deallocationAnnotation);
  }
}
extern "C" void kokkosp_deallocate_data(const SpaceHandle space, const char* label, const void* const ptr, const uint64_t size) {
  if(!caliper_kokkos_track_memory){
    return;
  }
  AnnotationsForSpace& spaceAnnotations = getMemoryAnnotations(space.name);
  spaceAnnotations.deallocationAnnotation->set((double)size);
  if(caliper_kokkos_tracing){
    cali::Annotation(label).end();
  }
}
extern "C" void kokkosp_allocate_data(const SpaceHandle space, const char* label, const void* const ptr, const uint64_t size) {
  if(!caliper_kokkos_track_memory){
    return;
  }
  AnnotationsForSpace& spaceAnnotations = getMemoryAnnotations(space.name);
  spaceAnnotations.allocationAnnotation->set((double)size);
  if(caliper_kokkos_tracing){
    cali::Annotation(label).begin();
  }
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

