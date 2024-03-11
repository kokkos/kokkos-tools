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

#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "kp_core.hpp"

namespace KokkosTools {
namespace FunctorSize {

bool show_warnings = true;
#define WARN(x)                                                          \
  {                                                                      \
    if (show_warnings) {                                                 \
      std::cerr << "KokkosP: Functor Size: WARNING: " << x << std::endl; \
    }                                                                    \
  }
#define ERROR(x) \
  { std::cerr << "KokkosP: Functor Size: ERROR: " << x << std::endl; }

std::unordered_map<uint64_t, uint64_t> anonCount;  // [size] = count
std::unordered_map<std::string, std::unordered_map<uint64_t, uint64_t>>
    nameCounts;  // [name][size] = count
std::vector<std::string> names;
uint64_t uniqueID = 0;

void kokkosp_init_library(const int loadSeq, const uint64_t interfaceVer,
                          const uint32_t /*devInfoCount*/,
                          Kokkos_Profiling_KokkosPDeviceInfo* /*deviceInfo*/) {
  std::cerr << "KokkosP: FunctorSize Library Initialized (sequence is "
            << loadSeq << ", interface version: " << interfaceVer << std::endl;
}

void dump_csv(std::ostream& os, const std::string_view delim = ",") {
  os << "size" << delim << "count" << delim << "name" << std::endl;

  for (const auto& [name, counts] : nameCounts) {
    for (const auto& [size, count] : counts) {
      os << size << delim << count << delim << name << std::endl;
    }
  }
  for (const auto& [size, count] : anonCount) {
    os << size << delim << count << delim
       << "KOKKOSP_FUNCTOR_SIZE_ANONYMOUS_FUNCTION" << std::endl;
  }
}

void kokkosp_finalize_library() {
  std::cout << std::endl
            << "KokkosP: Finalization Functor Size profiling library."
            << std::endl;

  const char* output_csv_path =
      std::getenv("KOKKOSP_FUNCTOR_SIZE_OUTPUT_CSV_PATH");

  if (output_csv_path && std::string_view(output_csv_path) != "") {
    std::ofstream os(output_csv_path);
    if (os) {
      dump_csv(os);
    } else {
      ERROR(output_csv_path << " counldn't be opened");
    }
  }
  dump_csv(std::cout, ",");
}

void begin_parallel(const char* name, uint64_t* kID) {
  *kID = uniqueID++;
  if (nullptr == name) {
    WARN("Ignording kernel ID "
         << *kID << " with null name. Results may be incomplete");
    return;
  }

  if (*kID < names.size()) {
    WARN("set new name \"" << name << "\" for previously-seen kernel ID "
                           << *kID);
  } else {
    names.resize((*kID) + 1);  // may have skipped if name was null previously
  }
  names[*kID] = name;
}

void kokkosp_begin_parallel_for(const char* name, const uint32_t /*devID*/,
                                uint64_t* kID) {
  begin_parallel(name, kID);
}

void kokkosp_begin_parallel_reduce(const char* name, const uint32_t /*devID*/,
                                   uint64_t* kID) {
  begin_parallel(name, kID);
}

void kokkosp_begin_parallel_scan(const char* name, const uint32_t /*devID*/,
                                 uint64_t* kID) {
  begin_parallel(name, kID);
}

void kokkosp_mark_kernel_static_info(
    const uint64_t kernelID, const Kokkos_Profiling_Kernel_Static_Info* info) {
  if (!info) {
    WARN("Kokkos provided null info");
    return;
  }
  const uint64_t size = info->functor_size;

  if (kernelID < names.size()) {
    const std::string& name = names[kernelID];
    if (0 == nameCounts.count(name)) {
      nameCounts[name] = {{size, 0}};
    }
    std::unordered_map<uint64_t, uint64_t>& nameCount = nameCounts[name];

    if (0 == nameCount.count(size)) {
      nameCount[size] = 0;
    }
    nameCount[size]++;
  } else {
    WARN("never-before seen kernel ID \"" << kernelID << "\".");

    if (0 == anonCount.count(size)) {
      anonCount[size] = 0;
    }
    anonCount[size]++;
  }
}

}  // namespace FunctorSize
}  // namespace KokkosTools

extern "C" {

namespace impl = KokkosTools::FunctorSize;

EXPOSE_INIT(impl::kokkosp_init_library)
EXPOSE_FINALIZE(impl::kokkosp_finalize_library)
EXPOSE_BEGIN_PARALLEL_FOR(impl::kokkosp_begin_parallel_for)
EXPOSE_BEGIN_PARALLEL_REDUCE(impl::kokkosp_begin_parallel_reduce)
EXPOSE_BEGIN_PARALLEL_SCAN(impl::kokkosp_begin_parallel_scan)
EXPOSE_MARK_KERNEL_STATIC_INFO(impl::kokkosp_mark_kernel_static_info)

}  // extern "C"
