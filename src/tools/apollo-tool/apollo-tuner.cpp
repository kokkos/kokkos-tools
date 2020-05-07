
#include <cassert>
#include <cinttypes>
#include <cstdio>
#include <algorithm>
#include <cstring>
#include <utility>
#include <functional>
#include <map>
#include <queue>
#include <set>
#include <string>
#include <vector>
#include <chrono>
#include <tuple>
#include <impl/Kokkos_Profiling_Interface.hpp>
#include <impl/Kokkos_Tuning_Interface.hpp>
#include <apollo/Apollo.h>
#include <apollo/Region.h>
#define TUNING_TOOL_SAVE_STATISTICS  // TODO DZP make build system option

std::map<std::string, Apollo::Region> regions;
std::set<size_t> untunables;
std::set<size_t> unlearnables;
std::string activeRegion = "";
constexpr const int max_choices = 2 << 20;

int choices[max_choices];

extern "C" void kokkosp_init_library(const int loadSeq,
                                     const uint64_t interfaceVer,
                                     const uint32_t devInfoCount,
                                     void* deviceInfo) {
  printf("Initializing Apollo Tuning adapter\n");
  for(int x =0 ;x<max_choices;++x){
    choices[x] = x; // TODO constexpr smart blah blah
  }
  Apollo* apollo = Apollo::instance(); 
}

extern "C" void kokkosp_finalize_library() {
  printf("Finalizing Apollo Tuning adapter\n");
}

extern "C" void kokkosp_begin_parallel_for(const char* name,
                                           const uint32_t devID,
                                           uint64_t* kID) {
  //printf("kokkosp_begin_parallel_for:%s:%u:%lu::", name, devID, *kID);
}

extern "C" void kokkosp_end_parallel_for(const uint64_t kID) {
  //printf("kokkosp_end_parallel_for:%lu::", kID);
}

extern "C" void kokkosp_begin_parallel_scan(const char* name,
                                            const uint32_t devID,
                                            uint64_t* kID) {
  //printf("kokkosp_begin_parallel_scan:%s:%u:%lu::", name, devID, *kID);
}

extern "C" void kokkosp_end_parallel_scan(const uint64_t kID) {
  //printf("kokkosp_end_parallel_scan:%lu::", kID);
}

extern "C" void kokkosp_begin_parallel_reduce(const char* name,
                                              const uint32_t devID,
                                              uint64_t* kID) {
  //printf("kokkosp_begin_parallel_reduce:%s:%u:%lu::", name, devID, *kID);
}

extern "C" void kokkosp_end_parallel_reduce(const uint64_t kID) {
  //printf("kokkosp_end_parallel_reduce:%lu::", kID);
}

extern "C" void kokkosp_push_profile_region(char* regionName) {
  //printf("kokkosp_push_profile_region:%s::", regionName);
}

extern "C" void kokkosp_pop_profile_region() {
  //printf("kokkosp_pop_profile_region::");
}

extern "C" void kokkosp_allocate_data(Kokkos::Tools::SpaceHandle handle, const char* name,
                                      void* ptr, uint64_t size) {
  //printf("kokkosp_allocate_data:%s:%s:%p:%lu::", handle.name, name, ptr, size);
}

extern "C" void kokkosp_deallocate_data(Kokkos::Tools::SpaceHandle handle, const char* name,
                                        void* ptr, uint64_t size) {
  //printf("kokkosp_deallocate_data:%s:%s:%p:%lu::", handle.name, name, ptr,
  //       size);
}

extern "C" void kokkosp_begin_deep_copy(Kokkos::Tools::SpaceHandle dst_handle,
                                        const char* dst_name,
                                        const void* dst_ptr,
                                        Kokkos::Tools::SpaceHandle src_handle,
                                        const char* src_name,
                                        const void* src_ptr, uint64_t size) {
  //printf("kokkosp_begin_deep_copy:%s:%s:%p:%s:%s:%p:%lu::", dst_handle.name,
  //       dst_name, dst_ptr, src_handle.name, src_name, src_ptr, size);
}

extern "C" void kokkosp_end_deep_copy() { //printf("kokkosp_end_deep_copy::"); 
}

uint32_t section_id = 3;
extern "C" void kokkosp_create_profile_section(const char* name,
                                               uint32_t* sec_id) {
}

extern "C" void kokkosp_start_profile_section(uint32_t sec_id) {
}

extern "C" void kokkosp_stop_profile_section(uint32_t sec_id) {
}
extern "C" void kokkosp_destroy_profile_section(uint32_t sec_id) {
}

extern "C" void kokkosp_profile_event(const char* name) {
}

std::map<size_t, Kokkos::Tools::ValueType> kokkos_type_info;


extern "C" void kokkosp_declare_tuning_variable(const char* name, const size_t id, Kokkos::Tools::VariableInfo info){
  if((info.valueQuantity != kokkos_value_set)){
    printf("Apollo Tuning Adaptor: won't learn %s because values are drawn from a range\n", name);
    untunables.insert(id);
  } 
  kokkos_type_info[id] = info.type;
}
extern "C" void kokkosp_declare_context_variable(const char* name, const size_t id, Kokkos::Tools::VariableInfo info, Kokkos::Tools::SetOrRange candidates){
  if( (info.type != kokkos_value_integer) && (info.type != kokkos_value_floating_point) && (info.type != kokkos_value_text) ){
    // TODO: error message
    unlearnables.insert(id);
    return;
  } 
  kokkos_type_info[id] = info.type;
}

/**
   (*tuningVariableValueCallback)(contextId, context_values.size(),
                                   context_values.data(), count, values,
                                   candidate_values);
                                   */



float variableToFloat(Kokkos::Tools::VariableValue value){
  switch(kokkos_type_info[value.id]){
    case kokkos_value_integer:
          return static_cast<float>(value.value.int_value);
    case kokkos_value_floating_point:
          return static_cast<float>(value.value.double_value);
    case kokkos_value_text:
          return static_cast<float>(std::hash<std::string>{}(value.value.string_value)); // terrible
    default:
          return 0.0; // break in some fashion instead
  }
}

static Apollo::Region* testing_region;

extern "C" void kokkosp_request_tuning_variable_values(size_t contextId, size_t numContextVariables, Kokkos::Tools::VariableValue* contextValues, size_t numTuningVariables, Kokkos::Tools::VariableValue* tuningValues, Kokkos::Tools::SetOrRange* candidateValues){
  int choiceSpaceSize = 1;
  for(int x = 0 ; x < numTuningVariables; ++x){
    if(unlearnables.find(tuningValues[x].id) == unlearnables.end()){
      if(choiceSpaceSize > max_choices / candidateValues[x].set.size){
        printf("Apollo Tuner: too many choices\n");
      }
      choiceSpaceSize *= candidateValues[x].set.size;
    }
  }

  if(testing_region == nullptr){
    testing_region = new Apollo::Region(numContextVariables, "testing_testing", choiceSpaceSize);
  }
  
  testing_region->begin();

  for(int x =0 ;x<numContextVariables;++x){
    if(untunables.find(contextValues[x].id) == untunables.end()){
      testing_region->setFeature(variableToFloat(contextValues[x]));
    }
  }

  int policyChoice = testing_region->getPolicyIndex();
  
  for(int x = 0 ; x < numTuningVariables; ++x){
    if(unlearnables.find(tuningValues[x].id) == unlearnables.end()){
      int set_size = candidateValues[x].set.size;
      int local_policy_choice = policyChoice % set_size; 
      tuningValues[x] = candidateValues[x].set.values[local_policy_choice];
      policyChoice /= set_size;
    }
  }
  std::cout << "Request received, returning "<<variableToFloat(tuningValues[0])<<"\n";

  //assert(policyChoice == 1);
}

extern "C" void kokkosp_end_context(size_t contextId){
  testing_region->end();
}
