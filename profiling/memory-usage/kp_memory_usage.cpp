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

#include <cstdio>
#include <inttypes.h>
#include <vector>
#include <unordered_map>
#include <atomic>
#include <mutex>

#include <sys/resource.h>
#include <unistd.h>

#include "kp_core.hpp"
#include "kp_timer.hpp"

namespace KokkosTools {
namespace MemoryUsage {

char space_name[16][64];

int num_spaces;
std::vector<std::tuple<double, uint64_t, double> > space_size_track[16];
uint64_t space_size[16];
uint64_t totalMemoryTransferred[16][16];
static std::mutex m;

Kokkos::Timer timer;

double max_mem_usage() {
  struct rusage app_info;
  getrusage(RUSAGE_SELF, &app_info);
  const double max_rssKB = app_info.ru_maxrss;
  return max_rssKB * 1024;
}

void kokkosp_init_library(const int /*loadSeq*/,
                          const uint64_t /*interfaceVer*/,
                          const uint32_t /*devInfoCount*/,
                          Kokkos_Profiling_KokkosPDeviceInfo* /*deviceInfo*/) {
  num_spaces = 0;
  for (int i = 0; i < 16; i++) {
    space_size[i] = 0;
    for (int j = 0; j < 16; j++) {
      totalMemoryTransferred[i][j] = 0;
    }
  }
  timer.reset();
}

void kokkosp_finalize_library() {
  char* hostname = (char*)malloc(sizeof(char) * 256);
  gethostname(hostname, 256);
  int pid = getpid();

  for (int s = 0; s < num_spaces; s++) {
    char* fileOutput = (char*)malloc(sizeof(char) * 256);
    snprintf(fileOutput, 256, "%s-%d-%s.memspace_usage", hostname, pid,
             space_name[s]);

    FILE* ofile = fopen(fileOutput, "wb");
    free(fileOutput);

    fprintf(ofile, "# Space %s\n", space_name[s]);
    fprintf(ofile,
            "# Time(s)  Size(MB)   HighWater(MB)   HighWater-Process(MB)\n");
    uint64_t maxvalue = 0;
    for (unsigned int i = 0; i < space_size_track[s].size(); i++) {
      if (std::get<1>(space_size_track[s][i]) > maxvalue)
        maxvalue = std::get<1>(space_size_track[s][i]);
      fprintf(ofile, "%lf %.1lf %.1lf %.1lf\n",
              std::get<0>(space_size_track[s][i]),
              1.0 * std::get<1>(space_size_track[s][i]) / 1024 / 1024,
              1.0 * maxvalue / 1024 / 1024,
              1.0 * std::get<2>(space_size_track[s][i]) / 1024 / 1024);
    }

    fprintf(ofile, "# Data transferred between Kokkos Memory Spaces --- \n");
    for (int dst = 0; dst < num_spaces; dst++) {
      for (int src = 0; src < num_spaces; src++) {
        fprintf(ofile, "# DstSpace     SrcSpace    Data-Transferred(MB)\n");
        fprintf(ofile, "%s %s %.1lf \n", space_name[dst], space_name[src],
                1.0 * totalMemoryTransferred[dst][src] / 1024 / 1024);
      }
    }

    fclose(ofile);
  }
  free(hostname);
}

void kokkosp_allocate_data(const SpaceHandle space, const char* /*label*/,
                           const void* const /*ptr*/, const uint64_t size) {
  std::lock_guard<std::mutex> lock(m);

  double time = timer.seconds();

  int space_i = num_spaces;
  for (int s = 0; s < num_spaces; s++)
    if (strcmp(space_name[s], space.name) == 0) space_i = s;

  if (space_i == num_spaces) {
    strncpy(space_name[num_spaces], space.name, 64);
    num_spaces++;
  }
  space_size[space_i] += size;
  space_size_track[space_i].push_back(
      std::make_tuple(time, space_size[space_i], max_mem_usage()));
}

void kokkosp_deallocate_data(const SpaceHandle space, const char* /*label*/,
                             const void* const /*ptr*/, const uint64_t size) {
  std::lock_guard<std::mutex> lock(m);

  double time = timer.seconds();

  int space_i = num_spaces;
  for (int s = 0; s < num_spaces; s++)
    if (strcmp(space_name[s], space.name) == 0) space_i = s;

  if (space_i == num_spaces) {
    strncpy(space_name[num_spaces], space.name, 64);
    num_spaces++;
  }
  if (space_size[space_i] >= size) {
    space_size[space_i] -= size;
    space_size_track[space_i].push_back(
        std::make_tuple(time, space_size[space_i], max_mem_usage()));
  }
}

void kokkosp_begin_deep_copy(SpaceHandle dst_handle, const char* /* dst_name */,
                             const void* /* dst_ptr */, SpaceHandle src_handle,
                             const char* /* src_name */,
                             const void* /* src_ptr */, uint64_t size) {
  std::lock_guard<std::mutex> lock(m);

  int space_dst = num_spaces;
  for (int s = 0; s < num_spaces; s++)
    if (strcmp(space_name[s], dst_handle.name) == 0) space_dst = s;

  if (space_dst == num_spaces) {
    strncpy(space_name[num_spaces], dst_handle.name, 64);
    num_spaces++;
  }

  int space_src = num_spaces;
  for (int s = 0; s < num_spaces; s++)
    if (strcmp(space_name[s], src_handle.name) == 0) space_src = s;

  if (space_src == num_spaces) {
    strncpy(space_name[num_spaces], src_handle.name, 64);
    num_spaces++;
  }
  totalMemoryTransferred[space_dst][space_src] += size;
}

void kokkosp_end_deep_copy() { std::lock_guard<std::mutex> lock(m); }

Kokkos::Tools::Experimental::EventSet get_event_set() {
  Kokkos::Tools::Experimental::EventSet my_event_set;
  memset(&my_event_set, 0,
         sizeof(my_event_set));  // zero any pointers not set here
  my_event_set.init            = kokkosp_init_library;
  my_event_set.finalize        = kokkosp_finalize_library;
  my_event_set.allocate_data   = kokkosp_allocate_data;
  my_event_set.deallocate_data = kokkosp_deallocate_data;
  my_event_set.begin_deep_copy = kokkosp_begin_deep_copy;
  my_event_set.end_deep_copy   = kokkosp_end_deep_copy;
  return my_event_set;
}

}  // namespace MemoryUsage
}  // namespace KokkosTools

extern "C" {

namespace impl = KokkosTools::MemoryUsage;

EXPOSE_INIT(impl::kokkosp_init_library)
EXPOSE_FINALIZE(impl::kokkosp_finalize_library)
EXPOSE_ALLOCATE(impl::kokkosp_allocate_data)
EXPOSE_DEALLOCATE(impl::kokkosp_deallocate_data)
EXPOSE_BEGIN_DEEP_COPY(impl::kokkosp_begin_deep_copy)
EXPOSE_END_DEEP_COPY(impl::kokkosp_end_deep_copy)

}  // extern "C"
