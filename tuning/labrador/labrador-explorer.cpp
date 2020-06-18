#include "labrador-exploration.hpp"
#include <impl/Kokkos_Profiling_Interface.hpp>
extern "C" void kokkosp_begin_parallel_for(const char* name,
                                           const uint32_t devID,
                                           uint64_t* kID) {
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

extern "C" void kokkosp_init_library(const int a1,
                                     const uint64_t a2,
                                     const uint32_t a3, void * a4) {
  labrador::explorer::kokkosp_init_library(a1,a2,a3,a4);
}

extern "C" void
kokkosp_declare_input_type(const char *name, const size_t id,
                           Kokkos::Tools::Experimental::VariableInfo &info) {
  labrador::explorer::kokkosp_declare_input_type(name,id,info);
}

extern "C" void
kokkosp_declare_output_type(const char *name, const size_t id,
                            Kokkos::Tools::Experimental::VariableInfo &info) {
  labrador::explorer::kokkosp_declare_output_type(name,id,info);
}
using Kokkos::Tools::Experimental::VariableValue;
extern "C" void kokkosp_request_values(size_t context_id,
                                       size_t num_context_variables,
                                       VariableValue *context_values,
                                       size_t num_tuning_variables,
                                       VariableValue *tuning_values) {
  labrador::explorer::kokkosp_request_values(context_id, num_context_variables, context_values, num_tuning_variables, tuning_values);
}
extern "C" void kokkosp_finalize_library() {
  labrador::explorer::kokkosp_finalize_library();
}
extern "C" void kokkosp_end_context(size_t context_id) {
  labrador::explorer::kokkosp_end_context(context_id);
}
