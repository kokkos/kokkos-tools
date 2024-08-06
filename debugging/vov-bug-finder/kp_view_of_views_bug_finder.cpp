//@HEADER
// ************************************************************************
//
//                        Kokkos v. 4.0
//       Copyright (2022) National Technology & Engineering
//               Solutions of Sandia, LLC (NTESS).
//
// Under the terms of Contract DE-NA0003525 with NTESS,
// the U.S. Government retains certain rights in this software.
//
// Part of Kokkos, under the Apache License v2.0 with LLVM Exceptions.
// See https://kokkos.org/LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//@HEADER

#include <kp_core.hpp>

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <mutex>
#include <optional>
#include <string>

namespace {

bool abort_on_error = true;

class {
  uint64_t count_;
  std::map<uint64_t, std::string> map_;

 public:
  std::mutex mutex;
  uint64_t push(std::string s) {
    auto it = map_.emplace_hint(map_.end(), count_, std::move(s));
    assert(++it == map_.end());
    return count_++;
  }
  void pop(uint64_t x) {
    auto it = map_.find(x);
    assert(it != map_.end());
    map_.erase(it);
  }
  std::string const &top() {
    assert(!map_.empty());
    return map_.begin()->second;
  }
  bool is_empty() noexcept { return map_.empty(); }
} current;

bool ignore_fence(std::string_view s) {
  return (s == "Kokkos::Impl::ViewValueFunctor: View init/destroy fence") ||
         (s == "Kokkos::ThreadsInternal::fence: Unnamed Instance Fence");
}

bool ignore_alloc(std::string_view s) { return (s.find("Kokkos::") == 0); }

std::optional<std::string> get_substr(std::string const &str,
                                      std::string_view prefix,
                                      std::string_view suffix) {
  if (auto found = str.find(prefix); found != std::string::npos) {
    found += prefix.length();
    return str.substr(found, str.rfind(suffix) - found);
  }
  return std::nullopt;
}

void vov_bug_finder_request_tool_settings(const uint32_t,
                                          Kokkos_Tools_ToolSettings *settings) {
  settings->requires_global_fencing = false;
}

void vov_bug_finder_begin_parallel_for(char const *kernelName,
                                       uint32_t /*deviceID*/,
                                       uint64_t *kernelID) {
  std::lock_guard lock(current.mutex);
  if (!current.is_empty()) {
    if (auto lbl =
            get_substr(kernelName, "Kokkos::View::initialization [", "]")) {
      std::cerr << "constructing view \"" << *lbl
                << "\" within a parallel region \"" << current.top() << "\"\n";
      if (abort_on_error) {
        std::abort();
      }
    }
  }
  *kernelID = current.push(kernelName);
}

void vov_bug_finder_end_parallel_for(uint64_t kernelID) {
  std::lock_guard lock(current.mutex);
  current.pop(kernelID);
}

void vov_bug_finder_begin_fence(char const *fenceName, uint32_t /*deviceID*/,
                                uint64_t * /*fenceID*/) {
  std::lock_guard lock(current.mutex);
  if (!current.is_empty() && !ignore_fence(fenceName)) {
    if (auto lbl =
            get_substr(current.top(), "Kokkos::View::destruction [", "]")) {
      std::cerr << "view of views \"" << *lbl
                << "\" not properly cleared this fence labelled \"" << fenceName
                << "\" will hang\n";
      if (abort_on_error) {
        std::abort();
      }
    }
  }
}

void vov_bug_finder_allocate_data(SpaceHandle handle, char const *name,
                                  void const * /*ptr*/, uint64_t /*size*/) {
  std::lock_guard lock(current.mutex);
  if (!current.is_empty() && !ignore_alloc(name)) {
    std::cerr << "allocating \"" << name << "\" within parallel region \""
              << current.top() << "\"\n";
    if (abort_on_error) {
      std::abort();
    }
  }
}

void vov_bug_finder_deallocate_data(SpaceHandle handle, char const *name,
                                    void const * /*ptr*/, uint64_t /*size*/) {
  std::lock_guard lock(current.mutex);
  if (!current.is_empty() && !ignore_alloc(name)) {
    std::cerr << "deallocating \"" << name << "\" within parallel region \""
              << current.top() << "\"\n";
    if (abort_on_error) {
      std::abort();
    }
  }
}

}  // namespace

extern "C" {
EXPOSE_TOOL_SETTINGS(vov_bug_finder_request_tool_settings)
EXPOSE_BEGIN_PARALLEL_FOR(vov_bug_finder_begin_parallel_for)
EXPOSE_END_PARALLEL_FOR(vov_bug_finder_end_parallel_for)
EXPOSE_BEGIN_FENCE(vov_bug_finder_begin_fence)
EXPOSE_ALLOCATE(vov_bug_finder_allocate_data)
EXPOSE_DEALLOCATE(vov_bug_finder_deallocate_data)
}
