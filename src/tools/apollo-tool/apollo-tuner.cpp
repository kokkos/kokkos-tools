
#include <algorithm>
#include <apollo/Apollo.h>
#include <apollo/Region.h>
#include <cassert>
#include<iostream>
#include <chrono>
#include <cinttypes>
#include <cstdio>
#include <cstring>
#include <functional>
#include <impl/Kokkos_Profiling_Interface.hpp>
#include <map>
#include <queue>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>
#define TUNING_TOOL_SAVE_STATISTICS // TODO DZP make build system option

std::map<std::string, Apollo::Region> regions;
std::set<size_t> untunables;
std::set<size_t> unlearnables;
std::string activeRegion = "";
constexpr const int max_choices = 2 << 20;
constexpr const int max_variables = 50;

struct variableSet {
  Kokkos::Tools::Experimental::VariableValue variable_ids[max_variables];
  size_t num_variables;
};

int choices[max_choices];

namespace std {
template <> struct less<variableSet> {
  bool operator()(const variableSet &l, const variableSet &r) {
    if (l.num_variables != r.num_variables) {
      return l.num_variables < r.num_variables;
    }
    for (int x = 0; x < l.num_variables; ++x) {
      if (l.variable_ids[x].type_id != r.variable_ids[x].type_id) {
        return l.variable_ids[x].type_id < r.variable_ids[x].type_id;
      }
    }
    return false;
  }
};
} // namespace std

Apollo *apollo;
extern "C" void kokkosp_init_library(const int loadSeq,
                                     const uint64_t interfaceVer,
                                     const uint32_t devInfoCount,
                                     void *deviceInfo) {
  printf("Initializing Apollo Tuning adapter\n");
  for (int x = 0; x < max_choices; ++x) {
    choices[x] = x; // TODO constexpr smart blah blah
  }
  putenv("APOLLO_RETRAIN_ENABLE=0");
  putenv("APOLLO_LOCAL_TRAINING=1");
  putenv("APOLLO_INIT_MODEL=RoundRobin");
  putenv("APOLLO_COLLECTIVE_TRAINING=0");
  putenv("APOLLO_TRACE_BEST_POLICIES=1");
  apollo = Apollo::instance();
}

extern "C" void kokkosp_finalize_library() {
  printf("Finalizing Apollo Tuning adapter\n");
}

extern "C" void kokkosp_begin_parallel_for(const char *name,
                                           const uint32_t devID,
                                           uint64_t *kID) {
  // printf("kokkosp_begin_parallel_for:%s:%u:%lu::", name, devID, *kID);
}

extern "C" void kokkosp_end_parallel_for(const uint64_t kID) {
  // printf("kokkosp_end_parallel_for:%lu::", kID);
}

extern "C" void kokkosp_begin_parallel_scan(const char *name,
                                            const uint32_t devID,
                                            uint64_t *kID) {
  // printf("kokkosp_begin_parallel_scan:%s:%u:%lu::", name, devID, *kID);
}

extern "C" void kokkosp_end_parallel_scan(const uint64_t kID) {
  // printf("kokkosp_end_parallel_scan:%lu::", kID);
}

extern "C" void kokkosp_begin_parallel_reduce(const char *name,
                                              const uint32_t devID,
                                              uint64_t *kID) {
  // printf("kokkosp_begin_parallel_reduce:%s:%u:%lu::", name, devID, *kID);
}

extern "C" void kokkosp_end_parallel_reduce(const uint64_t kID) {
  // printf("kokkosp_end_parallel_reduce:%lu::", kID);
}

extern "C" void kokkosp_push_profile_region(char *regionName) {
  // printf("kokkosp_push_profile_region:%s::", regionName);
}

extern "C" void kokkosp_pop_profile_region() {
  // printf("kokkosp_pop_profile_region::");
}

extern "C" void kokkosp_allocate_data(Kokkos::Tools::SpaceHandle handle,
                                      const char *name, void *ptr,
                                      uint64_t size) {
  // printf("kokkosp_allocate_data:%s:%s:%p:%lu::", handle.name, name, ptr,
  // size);
}

extern "C" void kokkosp_deallocate_data(Kokkos::Tools::SpaceHandle handle,
                                        const char *name, void *ptr,
                                        uint64_t size) {
  // printf("kokkosp_deallocate_data:%s:%s:%p:%lu::", handle.name, name, ptr,
  //       size);
}

extern "C" void kokkosp_begin_deep_copy(Kokkos::Tools::SpaceHandle dst_handle,
                                        const char *dst_name,
                                        const void *dst_ptr,
                                        Kokkos::Tools::SpaceHandle src_handle,
                                        const char *src_name,
                                        const void *src_ptr, uint64_t size) {
  // printf("kokkosp_begin_deep_copy:%s:%s:%p:%s:%s:%p:%lu::", dst_handle.name,
  //       dst_name, dst_ptr, src_handle.name, src_name, src_ptr, size);
}

extern "C" void kokkosp_end_deep_copy() { // printf("kokkosp_end_deep_copy::");
}

uint32_t section_id = 3;
extern "C" void kokkosp_create_profile_section(const char *name,
                                               uint32_t *sec_id) {}

extern "C" void kokkosp_start_profile_section(uint32_t sec_id) {}

extern "C" void kokkosp_stop_profile_section(uint32_t sec_id) {}
extern "C" void kokkosp_destroy_profile_section(uint32_t sec_id) {}

extern "C" void kokkosp_profile_event(const char *name) {}

extern "C" void
kokkosp_declare_output_type(const char *name, const size_t id,
                                Kokkos::Tools::Experimental::VariableInfo& info) {
  info.toolProvidedInfo = new std::string(name);
  if ((info.valueQuantity != kokkos_value_set)) {
    printf("Apollo Tuning Adaptor: won't learn %s because values are drawn "
           "from a range\n",
           name);
    untunables.insert(id);
  }
}


extern "C" void
kokkosp_declare_input_type(const char *name, const size_t id,
                                 Kokkos::Tools::Experimental::VariableInfo& info
      ) {
  info.toolProvidedInfo = new std::string(name);
  if ((info.type != Kokkos::Tools::Experimental::ValueType::kokkos_value_int64) &&
      (info.type != Kokkos::Tools::Experimental::ValueType::kokkos_value_double) &&
      (info.type != Kokkos::Tools::Experimental::ValueType::kokkos_value_string)) {
    // TODO: error message
    unlearnables.insert(id);
    
    return;
  }
}

float variableToFloat(Kokkos::Tools::Experimental::VariableValue value) {
  switch (value.metadata->type) {
  case Kokkos::Tools::Experimental::ValueType::kokkos_value_int64:
    return static_cast<float>(value.value.int_value);
  case Kokkos::Tools::Experimental::ValueType::kokkos_value_double:
    return static_cast<float>(value.value.double_value);
  case Kokkos::Tools::Experimental::ValueType::kokkos_value_string:
    return static_cast<float>(
        std::hash<std::string>{}(value.value.string_value)); // terrible
  default:
    return 0.0; // break in some fashion instead
  }
}

Kokkos::Tools::Experimental::VariableValue mvv(size_t index, Kokkos::Tools::Experimental::VariableValue reference, Kokkos::Tools::Experimental::ValueSet& set){
  Kokkos::Tools::Experimental::VariableValue value;
  value.type_id = reference.type_id;
  value.metadata = reference.metadata;
  switch(reference.metadata->type){
  case Kokkos::Tools::Experimental::ValueType::kokkos_value_int64:
    value.value.int_value = set.values.int_value[index];  
  break;
  case Kokkos::Tools::Experimental::ValueType::kokkos_value_double:
    value.value.double_value = set.values.double_value[index];  
  break;
  case Kokkos::Tools::Experimental::ValueType::kokkos_value_string:
    strncpy(value.value.string_value,set.values.string_value[index],65);
  break;
  }
  return value;
}

//using RegionData = std::pair<std::string, Apollo::Region*>;
using RegionData = Apollo::Region*;

RegionData get_region_name(variableSet variables);

static std::map<size_t, Apollo::Region *> tuned_contexts;
static std::map<variableSet, RegionData> tuning_regions;
extern "C" void kokkosp_request_values(
    size_t contextId, size_t numContextVariables,
    Kokkos::Tools::Experimental::VariableValue *contextValues, size_t numTuningVariables,
    Kokkos::Tools::Experimental::VariableValue *tuningValues
    ) {
  if (numTuningVariables == 0) {
    return;
  }
  static int created_regions;
  // std::cout << "Have "<<numContextVariables<<" context,
  // "<<numTuningVariables<<" tuning\n";
  int choiceSpaceSize = 1;
  for (int x = 0; x < numTuningVariables; ++x) {
    if (unlearnables.find(tuningValues[x].type_id) == unlearnables.end()) {
      if (choiceSpaceSize > max_choices / tuningValues[x].metadata->candidates.set.size) {
        printf("Apollo Tuner: too many choices\n");
      }
      choiceSpaceSize *= tuningValues[x].metadata->candidates.set.size;
    }
  }
  variableSet tuningProblem;
  int numValidVariables = 0;
  for (int x = 0; x < numContextVariables; ++x) {
    if (untunables.find(contextValues[x].type_id) == untunables.end()) {
      tuningProblem.variable_ids[numValidVariables++] = contextValues[x];
    }
  }
  for (int x = 0; x < numTuningVariables; ++x) {
    if (unlearnables.find(tuningValues[x].type_id) == unlearnables.end()) {
      tuningProblem.variable_ids[numValidVariables++] = tuningValues[x];
    }
  }
  tuningProblem.num_variables = numValidVariables;
  if (tuning_regions.find(tuningProblem) == tuning_regions.end()) {
    std::cout << "Testing space size is " << choiceSpaceSize << std::endl;
    std::string name = "debug_name_"+std::to_string(created_regions);
    auto region =
        new Apollo::Region(numContextVariables, name.c_str(), choiceSpaceSize);
    tuning_regions[tuningProblem] = region;
  }
  auto region = tuning_regions[tuningProblem];
  tuned_contexts[contextId] = region;
  region->begin();
  //std::cout <<"Cont: "<<numContextVariables<<std::endl;
  for (int x = 0; x < numContextVariables; ++x) {
    if (untunables.find(contextValues[x].type_id) == untunables.end()) {
      region->setFeature(variableToFloat(contextValues[x]));
    }
  }

  int policyChoice = region->getPolicyIndex();

  for (int x = 0; x < numTuningVariables; ++x) {
    if (unlearnables.find(tuningValues[x].type_id) == unlearnables.end()) {
      int set_size = tuningValues[x].metadata->candidates.set.size;
      int local_policy_choice = policyChoice % set_size;
      tuningValues[x] = mvv(local_policy_choice, tuningValues[x], tuningValues[x].metadata->candidates.set);
      policyChoice /= set_size;
    }
  }
  // assert(policyChoice == 1);
}

extern "C" void kokkosp_end_context(size_t contextId) {
  auto search = tuned_contexts.find(contextId);
  if (search == tuned_contexts.end()) {
    return;
  }
  tuned_contexts.erase(contextId);
  auto region = search->second;
  region->end();
  static int encounter;
  if ((++encounter % 664) == 0) {
    apollo->flushAllRegionMeasurements(0);
  }
}
