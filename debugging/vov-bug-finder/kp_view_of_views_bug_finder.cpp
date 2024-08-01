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

bool verbose        = false;
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

std::optional<std::string> get_substr(std::string const &str,
                                      std::string_view prefix,
                                      std::string_view suffix) {
  if (auto found = str.find(prefix); found != std::string::npos) {
    found += prefix.length();
    return str.substr(found, str.rfind(suffix) - found);
  }
  return std::nullopt;
}

}  // namespace

extern "C" void kokkosp_request_tool_settings(
    const uint32_t, Kokkos_Tools_ToolSettings *settings) {
  settings->requires_global_fencing = false;
}

extern "C" void kokkosp_begin_parallel_for(char const *kernelName,
                                           uint32_t deviceID,
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

  if (verbose) {
    std::cout << "begin kernel " << *kernelID << " " << kernelName
              << " on device " << deviceID << '\n';
  }
}

extern "C" void kokkosp_end_parallel_for(uint64_t kernelID) {
  std::lock_guard lock(current.mutex);
  current.pop(kernelID);

  if (verbose) {
    std::cout << "end kernel " << kernelID << '\n';
  }
}

extern "C" void kokkosp_begin_fence(char const *fenceName, uint32_t deviceID,
                                    uint64_t *fenceID) {
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
  *fenceID = -1;

  if (verbose) {
    std::cout << "begin fence " << *fenceID << " " << fenceName << " on device "
              << deviceID << '\n';
  }
}

extern "C" void kokkosp_end_fence(uint64_t fenceID) {
  if (verbose) {
    std::cout << "end fence " << fenceID << '\n';
  }
}

extern "C" void kokkosp_allocate_data(SpaceHandle handle, const char *name,
                                      void *ptr, uint64_t size) {
  std::lock_guard lock(current.mutex);
  if (!current.is_empty()) {
    std::cerr << "allocating \"" << name << "\" within parallel region \""
              << current.top() << "\"\n";
    if (abort_on_error) {
      std::abort();
    }
  }

  if (verbose) {
    std::cout << "alloc (" << handle.name << ") " << name << " pointer " << ptr
              << "size " << size << '\n';
  }
}

extern "C" void kokkosp_deallocate_data(SpaceHandle handle, const char *name,
                                        void *ptr, uint64_t size) {
  std::lock_guard lock(current.mutex);
  if (!current.is_empty()) {
    std::cerr << "deallocating \"" << name << "\" within parallel region \""
              << current.top() << "\"\n";
    if (abort_on_error) {
      std::abort();
    }
  }

  if (verbose) {
    std::cout << "dealloc (" << handle.name << ") " << name << " pointer "
              << ptr << "size " << size << '\n';
  }
}
