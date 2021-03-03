#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cxxabi.h>
#include <dlfcn.h>
#include <impl/Kokkos_Profiling_Interface.hpp>
#include <iostream>
#include <regex>
#include <string>
#include <tool_chain.hpp>
#include <unordered_set>
#include <vector>
static Kokkos::Tools::callback_chainer next;
using action_type = Kokkos::Tools::Experimental::ToolActions;
action_type actions;
static uint64_t sample_rate = 101;

extern "C" void kokkosp_begin_parallel_scan(const char *name,
                                            const uint32_t devID,
                                            uint64_t *kID) {
  *kID = 0;
  static uint64_t encounter;
  ++encounter;
  if ((encounter % sample_rate) == 0) {
    actions.fence(0);
    *kID = 1;
    next.kokkosp_begin_parallel_scan(name, devID, kID);
  }
}
extern "C" void kokkosp_begin_parallel_for(const char *name,
                                           const uint32_t devID,
                                           uint64_t *kID) {
  *kID = 0;
  static uint64_t encounter;
  ++encounter;
  if ((encounter % sample_rate) == 0) {
    actions.fence(0);
    *kID = 1;
    next.kokkosp_begin_parallel_for(name, devID, kID);
  }
}
extern "C" void kokkosp_begin_parallel_reduce(const char *name,
                                              const uint32_t devID,
                                              uint64_t *kID) {
  *kID = 0;
  static uint64_t encounter;
  ++encounter;
  if ((encounter % sample_rate) == 0) {
    actions.fence(0);
    *kID = 1;
    next.kokkosp_begin_parallel_reduce(name, devID, kID);
  }
}
extern "C" void kokkosp_end_parallel_scan(uint64_t kID) {
  if (kID > 0) {
    actions.fence(0);
    next.kokkosp_end_parallel_scan(kID);
  }
}
extern "C" void kokkosp_end_parallel_for(uint64_t kID) {
  if (kID > 0) {
    actions.fence(0);
    next.kokkosp_end_parallel_for(kID);
  }
}
extern "C" void kokkosp_end_parallel_reduce(uint64_t kID) {
  if (kID > 0) {
    actions.fence(0);
    next.kokkosp_end_parallel_reduce(kID);
  }
}
#include <iostream>
extern "C" void
kokkosp_init_library(int loadseq, uint64_t version, uint32_t ndevinfos,
                     Kokkos_Profiling_KokkosPDeviceInfo *devInfos) {
  next.setup(loadseq);
  next.kokkosp_init_library(loadseq, version, ndevinfos, devInfos);
}
extern "C" void kokkosp_finalize_library() { next.kokkosp_finalize_library(); }
extern "C" void kokkosp_push_profile_region(const char *name) {
  actions.fence(0);
  next.kokkosp_push_profile_region(name);
}
extern "C" void kokkosp_pop_profile_region() {
  actions.fence(0);
  next.kokkosp_pop_profile_region();
}
extern "C" void kokkosp_allocate_data(Kokkos::Profiling::SpaceHandle handle,
                                      const char *name, void *ptr,
                                      uint64_t size) {
  next.kokkosp_allocate_data(handle, name, ptr, size);
}
extern "C" void kokkosp_deallocate_data(Kokkos::Profiling::SpaceHandle handle,
                                        const char *name, void *ptr,
                                        uint64_t size) {
  next.kokkosp_deallocate_data(handle, name, ptr, size);
}
extern "C" void kokkosp_begin_deep_copy(Kokkos::Tools::SpaceHandle dst_handle,
                                        const char *dst_name,
                                        const void *dst_ptr,
                                        Kokkos::Tools::SpaceHandle src_handle,
                                        const char *src_name,
                                        const void *src_ptr, uint64_t size) {
  next.kokkosp_begin_deep_copy(dst_handle, dst_name, dst_ptr, src_handle,
                               src_name, src_ptr, size);
}
extern "C" void kokkosp_end_deep_copy() { next.kokkosp_end_deep_copy(); }
extern "C" void kokkosp_begin_fence(const char *name, const uint32_t devID,
                                    uint64_t *kID) {
  next.kokkosp_begin_fence(name, devID, kID);
}
extern "C" void kokkosp_end_fence(uint64_t kID) { next.kokkosp_end_fence(kID); }
extern "C" void kokkosp_dual_view_sync(const char *label, const void *const ptr,
                                       bool to_device) {
  next.kokkosp_dual_view_sync(label, ptr, to_device);
}
extern "C" void kokkosp_dual_view_modify(const char *label,
                                         const void *const ptr,
                                         bool to_device) {
  next.kokkosp_dual_view_modify(label, ptr, to_device);
}
extern "C" void kokkosp_declare_metadata(const char *key, const char *value) {
  next.kokkosp_declare_metadata(key, value);
}
extern "C" void kokkosp_create_profile_section(const char *name,
                                               uint32_t *sec_id) {
  next.kokkosp_create_profile_section(name, sec_id);
}
extern "C" void kokkosp_start_profile_section(uint32_t sec_id) {
  next.kokkosp_start_profile_section(sec_id);
}
extern "C" void kokkosp_stop_profile_section(uint32_t sec_id) {
  next.kokkosp_stop_profile_section(sec_id);
}
extern "C" void kokkosp_destroy_profile_section(uint32_t sec_id) {
  next.kokkosp_destroy_profile_section(sec_id);
}
extern "C" void kokkosp_profile_event(const char *name) {
  next.kokkosp_profile_event(name);
}
extern "C" void
kokkosp_declare_output_type(const char *name, size_t id,
                            Kokkos::Tools::Experimental::VariableInfo *info) {
  next.kokkosp_declare_output_type(name, id, info);
}
extern "C" void
kokkosp_declare_input_type(const char *name, size_t id,
                           Kokkos::Tools::Experimental::VariableInfo *info) {
  next.kokkosp_declare_input_type(name, id, info);
}
extern "C" void kokkosp_request_values(
    size_t context_id, size_t num_context_values,
    Kokkos::Tools::Experimental::VariableValue *context_values,
    size_t num_output_values,
    Kokkos::Tools::Experimental::VariableValue *output_values) {
  next.kokkosp_request_values(context_id, num_context_values, context_values,
                              num_output_values, output_values);
}
extern "C" void kokkosp_end_context(size_t context_id) {
  next.kokkosp_end_context(context_id);
}
extern "C" void kokkosp_begin_context(size_t context_id) {
  next.kokkosp_begin_context(context_id);
}
extern "C" void kokkosp_declare_optimization_goal(
    size_t context_id, Kokkos::Tools::Experimental::OptimizationGoal goal) {
  next.kokkosp_declare_optimization_goal(context_id, goal);
}
extern "C" void kokkosp_print_help(char *arg0) {
  next.kokkosp_print_help(arg0);
}
extern "C" void kokkosp_parse_args(int argc, char **argv) {

  next.kokkosp_parse_args(argc, argv);
}
extern "C" void
kokkosp_transmit_actions(Kokkos::Tools::Experimental::ToolActions nactions) {
  actions = nactions;
  next.kokkosp_transmit_actions(nactions);
}
extern "C" void kokkosp_request_responses(
    Kokkos::Tools::Experimental::ToolResponses *responses) {
  responses->requires_global_fencing = false;
  next.kokkosp_request_responses(responses);
}
