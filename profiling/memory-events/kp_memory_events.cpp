// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// SPDX-FileCopyrightText: Copyright Contributors to the Kokkos project

#include <array>
#include <cstdio>
#include <cinttypes>
#include <vector>
#include <unordered_map>
#include <atomic>
#include <mutex>

#include <sys/resource.h>
#include <unistd.h>

#include "kp_core.hpp"
#include "kp_memory_events.hpp"
#include "kp_timer.hpp"

#include <fstream>

namespace KokkosTools {
namespace MemoryEvents {

char space_name[16][64];

std::vector<EventRecord> events;

int num_spaces;
std::vector<std::tuple<double, uint64_t, double> > space_size_track[16];
uint64_t space_size[16];

static std::mutex m;

Kokkos::Timer timer;

double max_mem_usage() {
  struct rusage app_info;
  getrusage(RUSAGE_SELF, &app_info);
  const double max_rssKB = app_info.ru_maxrss;
  return max_rssKB * 1024;
}

void kokkosp_init_library(const int loadSeq, const uint64_t interfaceVer,
                          const uint32_t /*devInfoCount*/,
                          Kokkos_Profiling_KokkosPDeviceInfo* /*deviceInfo*/) {
  num_spaces = 0;
  for (int i = 0; i < 16; i++) space_size[i] = 0;

  printf("KokkosP: MemoryEvents loaded (sequence: %d, version: %llu)\n",
         loadSeq, (unsigned long long)(interfaceVer));

  timer.reset();
}

void kokkosp_finalize_library() {
  std::array<char, 256> hostname_buf{};
  gethostname(hostname_buf.data(), hostname_buf.size());
  std::string hostname(hostname_buf.data());

  int pid = getpid();

  // ---- Memory event log ----
  std::string fileOutput = hostname + "-" + std::to_string(pid) + ".mem_events";
  std::ofstream ofile(fileOutput, std::ios::binary);

  ofile << "# Memory Events\n";
  ofile << "# Time     Ptr                  Size        MemSpace      Op       "
           "  Name\n";

  for (const auto& e : events) {
    e.print_record(ofile);
  }

  // ---- Per memory space usage ----
  for (int s = 0; s < num_spaces; ++s) {
    std::string fileName = hostname + "-" + std::to_string(pid) + "-" +
                           space_name[s] + ".memspace_usage";

    std::ofstream spaceFile(fileName, std::ios::binary);

    spaceFile << "# Space " << space_name[s] << "\n";
    spaceFile
        << "# Time(s)  Size(MB)   HighWater(MB)   HighWater-Process(MB)\n";

    uint64_t maxvalue = 0;

    for (const auto& entry : space_size_track[s]) {
      double time         = std::get<0>(entry);
      uint64_t size       = std::get<1>(entry);
      uint64_t process_hw = std::get<2>(entry);

      if (size > maxvalue) maxvalue = size;

      spaceFile << time << " " << std::fixed << std::setprecision(1)
                << (size / 1024.0 / 1024.0) << " "
                << (maxvalue / 1024.0 / 1024.0) << " "
                << (process_hw / 1024.0 / 1024.0) << "\n";
    }
  }
}

void kokkosp_allocate_data(const SpaceHandle space, const char* label,
                           const void* const ptr, const uint64_t size) {
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

  events.push_back(
      EventRecord(ptr, size, MEMOP_ALLOCATE, space_i, time, label));
}

void kokkosp_deallocate_data(const SpaceHandle space, const char* label,
                             const void* const ptr, const uint64_t size) {
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

  events.push_back(
      EventRecord(ptr, size, MEMOP_DEALLOCATE, space_i, time, label));
}

void kokkosp_push_profile_region(const char* name) {
  std::lock_guard<std::mutex> lock(m);
  double time = timer.seconds();
  events.push_back(EventRecord(nullptr, 0, MEMOP_PUSH_REGION, 0, time, name));
}

void kokkosp_pop_profile_region() {
  std::lock_guard<std::mutex> lock(m);
  double time = timer.seconds();
  events.push_back(EventRecord(nullptr, 0, MEMOP_POP_REGION, 0, time, ""));
}

Kokkos::Tools::Experimental::EventSet get_event_set() {
  Kokkos::Tools::Experimental::EventSet my_event_set;
  memset(&my_event_set, 0,
         sizeof(my_event_set));  // zero any pointers not set here
  my_event_set.init            = kokkosp_init_library;
  my_event_set.finalize        = kokkosp_finalize_library;
  my_event_set.allocate_data   = kokkosp_allocate_data;
  my_event_set.deallocate_data = kokkosp_deallocate_data;
  my_event_set.push_region     = kokkosp_push_profile_region;
  my_event_set.pop_region      = kokkosp_pop_profile_region;
  return my_event_set;
}

}  // namespace MemoryEvents
}  // namespace KokkosTools

extern "C" {

namespace impl = KokkosTools::MemoryEvents;

EXPOSE_INIT(impl::kokkosp_init_library)
EXPOSE_FINALIZE(impl::kokkosp_finalize_library)
EXPOSE_ALLOCATE(impl::kokkosp_allocate_data)
EXPOSE_DEALLOCATE(impl::kokkosp_deallocate_data)
EXPOSE_PUSH_REGION(impl::kokkosp_push_profile_region)
EXPOSE_POP_REGION(impl::kokkosp_pop_profile_region)

}  // extern "C"
