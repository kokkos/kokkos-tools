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

#include "impl/Kokkos_Profiling_Interface.hpp"
#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cxxabi.h>
#include <dlfcn.h>
#include <iostream>
#include <regex>
#include <string>
#include <unordered_set>
#include <vector>
namespace Kokkos {

namespace Tools {
template <typename Callback>
void lookup_function(void *dlopen_handle, const std::string &basename,
                     Callback &callback) {
  void *p = dlsym(dlopen_handle, basename.c_str());
  callback = *reinterpret_cast<Callback *>(&p);
}

struct callback_chainer {
  Kokkos::Tools::Experimental::EventSet next_events;
  using responses = Kokkos::Tools::Experimental::ToolResponses;
  using actions = Kokkos::Tools::Experimental::ToolActions;
  responses tool_requirements;
  actions tool_actions;
  int load_seq;
  template <typename Callback, typename BooleanConstant, typename... Args>
  void call_kokkos_callback(const Callback callback, BooleanConstant,
                            Args... args) {
    if (callback != nullptr) {
      if ((BooleanConstant::value) &&
          (tool_requirements.requires_global_fencing) && (load_seq == 0)) {
        tool_actions.fence(0);
      }
      (*callback)(args...);
    }
  }
  void setup(int loadSeq) {
    void *handle;
    char *profileLibrary = getenv("KOKKOS_PROFILE_LIBRARY");
    char *envBuffer =
        (char *)malloc(sizeof(char) * (strlen(profileLibrary) + 1));
    sprintf(envBuffer, "%s", profileLibrary);

    char *nextLibrary = strtok(envBuffer, ";");

    for (int i = 0; i < loadSeq; i++) {
      nextLibrary = strtok(NULL, ";");
    }

    nextLibrary = strtok(NULL, ";");

    if (NULL == nextLibrary) {
      printf("KokkosP: No child library to call in %s\n", profileLibrary);
    } else {
      printf("KokkosP: Next library to call: %s\n", nextLibrary);
      printf("KokkosP: Loading child library ..\n");

      handle = dlopen(nextLibrary, RTLD_NOW | RTLD_GLOBAL);

      if (handle == nullptr) {
        fprintf(stderr, "KokkosP: Error: Unable to load: %s (Error=%s)\n",
                nextLibrary, dlerror());
      }
    }

    free(envBuffer);

    lookup_function(handle, "kokkosp_begin_parallel_scan",
                    next_events.begin_parallel_scan);
    lookup_function(handle, "kokkosp_begin_parallel_for",
                    next_events.begin_parallel_for);
    lookup_function(handle, "kokkosp_begin_parallel_reduce",
                    next_events.begin_parallel_reduce);
    lookup_function(handle, "kokkosp_end_parallel_scan",
                    next_events.end_parallel_scan);
    lookup_function(handle, "kokkosp_end_parallel_for",
                    next_events.end_parallel_for);
    lookup_function(handle, "kokkosp_end_parallel_reduce",
                    next_events.end_parallel_reduce);

    lookup_function(handle, "kokkosp_init_library", next_events.init);
    lookup_function(handle, "kokkosp_finalize_library", next_events.finalize);

    lookup_function(handle, "kokkosp_push_profile_region",
                    next_events.push_region);
    lookup_function(handle, "kokkosp_pop_profile_region",
                    next_events.pop_region);
    lookup_function(handle, "kokkosp_allocate_data", next_events.allocate_data);
    lookup_function(handle, "kokkosp_deallocate_data",
                    next_events.deallocate_data);

    lookup_function(handle, "kokkosp_begin_deep_copy",
                    next_events.begin_deep_copy);
    lookup_function(handle, "kokkosp_end_deep_copy", next_events.end_deep_copy);
    lookup_function(handle, "kokkosp_begin_fence", next_events.begin_fence);
    lookup_function(handle, "kokkosp_end_fence", next_events.end_fence);
    lookup_function(handle, "kokkosp_dual_view_sync",
                    next_events.sync_dual_view);
    lookup_function(handle, "kokkosp_dual_view_modify",
                    next_events.modify_dual_view);

    lookup_function(handle, "kokkosp_declare_metadata",
                    next_events.declare_metadata);
    lookup_function(handle, "kokkosp_create_profile_section",
                    next_events.create_profile_section);
    lookup_function(handle, "kokkosp_start_profile_section",
                    next_events.start_profile_section);
    lookup_function(handle, "kokkosp_stop_profile_section",
                    next_events.stop_profile_section);
    lookup_function(handle, "kokkosp_destroy_profile_section",
                    next_events.destroy_profile_section);

    lookup_function(handle, "kokkosp_profile_event", next_events.profile_event);

    lookup_function(handle, "kokkosp_declare_output_type",
                    next_events.declare_output_type);

    lookup_function(handle, "kokkosp_declare_input_type",
                    next_events.declare_input_type);
    lookup_function(handle, "kokkosp_request_values",
                    next_events.request_output_values);
    lookup_function(handle, "kokkosp_end_context",
                    next_events.end_tuning_context);
    lookup_function(handle, "kokkosp_begin_context",
                    next_events.begin_tuning_context);
    lookup_function(handle, "kokkosp_declare_optimization_goal",
                    next_events.declare_optimization_goal);

    lookup_function(handle, "kokkosp_print_help", next_events.print_help);
    lookup_function(handle, "kokkosp_parse_args", next_events.parse_args);
    lookup_function(handle, "kokkosp_transmit_actions",
                    next_events.transmit_actions);
    lookup_function(handle, "kokkosp_request_responses",
                    next_events.request_responses);
  }
  void kokkosp_begin_parallel_scan(const char *name, const uint32_t devID,
                                   uint64_t *kID) {
    call_kokkos_callback(next_events.begin_parallel_scan, std::true_type{},
                         name, devID, kID);
  }
  void kokkosp_begin_parallel_for(const char *name, const uint32_t devID,
                                  uint64_t *kID) {
    call_kokkos_callback(next_events.begin_parallel_for, std::true_type{}, name,
                         devID, kID);
  }
  void kokkosp_begin_parallel_reduce(const char *name, const uint32_t devID,
                                     uint64_t *kID) {
    call_kokkos_callback(next_events.begin_parallel_reduce, std::true_type{},
                         name, devID, kID);
  }
  void kokkosp_end_parallel_scan(uint64_t kID) {
    call_kokkos_callback(next_events.end_parallel_scan, std::true_type{}, kID);
  }
  void kokkosp_end_parallel_for(uint64_t kID) {
    call_kokkos_callback(next_events.end_parallel_for, std::true_type{}, kID);
  }
  void kokkosp_end_parallel_reduce(uint64_t kID) {
    call_kokkos_callback(next_events.end_parallel_reduce, std::true_type{},
                         kID);
  }
  void kokkosp_init_library(int loadseq, uint64_t version, uint32_t ndevinfos,
                            Kokkos_Profiling_KokkosPDeviceInfo *devInfos) {
    load_seq = loadseq;
    call_kokkos_callback(next_events.init, std::false_type{}, loadseq + 1,
                         version, ndevinfos, devInfos);
  }
  void kokkosp_finalize_library() {
    call_kokkos_callback(next_events.finalize, std::false_type{});
  }
  void kokkosp_push_profile_region(const char *name) {
    call_kokkos_callback(next_events.push_region, std::false_type{}, name);
  }
  void kokkosp_pop_profile_region() {
    call_kokkos_callback(next_events.pop_region, std::false_type{});
  }
  void kokkosp_allocate_data(Kokkos::Tools::SpaceHandle handle,
                             const char *name, void *ptr, uint64_t size) {
    call_kokkos_callback(next_events.allocate_data, std::false_type{}, handle,
                         name, ptr, size);
  }
  void kokkosp_deallocate_data(Kokkos::Tools::SpaceHandle handle,
                               const char *name, void *ptr, uint64_t size) {
    call_kokkos_callback(next_events.deallocate_data, std::false_type{}, handle,
                         name, ptr, size);
  }
  void kokkosp_begin_deep_copy(Kokkos::Tools::SpaceHandle dst_handle,
                               const char *dst_name, const void *dst_ptr,
                               Kokkos::Tools::SpaceHandle src_handle,
                               const char *src_name, const void *src_ptr,
                               uint64_t size) {
    call_kokkos_callback(next_events.begin_deep_copy, std::false_type{},
                         dst_handle, dst_name, dst_ptr, src_handle, src_name,
                         src_ptr, size);
  }
  void kokkosp_end_deep_copy() {
    call_kokkos_callback(next_events.end_deep_copy, std::false_type{});
  }
  void kokkosp_begin_fence(const char *name, const uint32_t devID,
                           uint64_t *kID) {
    call_kokkos_callback(next_events.begin_fence, std::false_type{}, name,
                         devID, kID);
  }
  void kokkosp_end_fence(uint64_t kID) {
    call_kokkos_callback(next_events.end_fence, std::false_type{}, kID);
  }
  void kokkosp_dual_view_sync(const char *label, const void *const ptr,
                              bool to_device) {
    call_kokkos_callback(next_events.sync_dual_view, std::false_type{}, label,
                         ptr, to_device);
  }
  void kokkosp_dual_view_modify(const char *label, const void *const ptr,
                                bool to_device) {
    call_kokkos_callback(next_events.modify_dual_view, std::false_type{}, label,
                         ptr, to_device);
  }
  void kokkosp_declare_metadata(const char *key, const char *value) {
    call_kokkos_callback(next_events.declare_metadata, std::false_type{}, key,
                         value);
  }
  void kokkosp_create_profile_section(const char *name, uint32_t *sec_id) {
    call_kokkos_callback(next_events.create_profile_section, std::false_type{},
                         name, sec_id);
  }
  void kokkosp_start_profile_section(uint32_t sec_id) {
    call_kokkos_callback(next_events.start_profile_section, std::false_type{},
                         sec_id);
  }
  void kokkosp_stop_profile_section(uint32_t sec_id) {
    call_kokkos_callback(next_events.stop_profile_section, std::false_type{},
                         sec_id);
  }
  void kokkosp_destroy_profile_section(uint32_t sec_id) {
    call_kokkos_callback(next_events.destroy_profile_section, std::false_type{},
                         sec_id);
  }
  void kokkosp_profile_event(const char *name) {
    call_kokkos_callback(next_events.profile_event, std::false_type{}, name);
  }
  void
  kokkosp_declare_output_type(const char *name, size_t id,
                              Kokkos::Tools::Experimental::VariableInfo *info) {
    call_kokkos_callback(next_events.declare_output_type, std::false_type{},
                         name, id, info);
  }
  void
  kokkosp_declare_input_type(const char *name, size_t id,
                             Kokkos::Tools::Experimental::VariableInfo *info) {
    call_kokkos_callback(next_events.declare_input_type, std::false_type{},
                         name, id, info);
  }
  void kokkosp_request_values(
      size_t context_id, size_t num_context_values,
      Kokkos::Tools::Experimental::VariableValue *context_values,
      size_t num_output_values,
      Kokkos::Tools::Experimental::VariableValue *output_values) {
    call_kokkos_callback(next_events.request_output_values, std::false_type{},
                         context_id, num_context_values, context_values,
                         num_output_values, output_values);
  }
  void kokkosp_end_context(size_t context_id) {
    call_kokkos_callback(
        next_events.end_tuning_context, std::false_type{}, context_id,
        Kokkos::Tools::Experimental::VariableValue{}); // TODO: fix
  }
  void kokkosp_begin_context(size_t context_id) {
    call_kokkos_callback(next_events.begin_tuning_context, std::false_type{},
                         context_id);
  }
  void kokkosp_declare_optimization_goal(
      size_t context_id, Kokkos::Tools::Experimental::OptimizationGoal goal) {
    call_kokkos_callback(next_events.declare_optimization_goal,
                         std::false_type{}, context_id, goal);
  }
  void kokkosp_print_help(char *arg0) {
    call_kokkos_callback(next_events.print_help, std::false_type{}, arg0);
  }
  void kokkosp_parse_args(int argc, char **argv) {
    call_kokkos_callback(next_events.parse_args, std::false_type{}, argc, argv);
  }
  void
  kokkosp_transmit_actions(Kokkos::Tools::Experimental::ToolActions actions) {
    call_kokkos_callback(next_events.transmit_actions, std::false_type{},
                         actions);
  }
  void kokkosp_request_responses(
      Kokkos::Tools::Experimental::ToolResponses *responses) {
    call_kokkos_callback(next_events.request_responses, std::false_type{},
                         responses);
  }
};
} // namespace Tools
} // namespace Kokkos
