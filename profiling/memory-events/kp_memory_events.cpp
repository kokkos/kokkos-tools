/*
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
// Questions? Contact  David Poliakoff (dzpolia@sandia.gov)
//
// ************************************************************************
//@HEADER
// */


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

namespace KokkosTools {
namespace MemoryEvents {

char space_name[16][64];

std::vector<EventRecord> events;

int num_spaces;
std::vector<std::tuple<double,uint64_t,double> > space_size_track[16];
uint64_t space_size[16];

static std::mutex m;

Kokkos::Timer timer;

double max_mem_usage() {
  struct rusage app_info;
  getrusage(RUSAGE_SELF, &app_info);
  const double max_rssKB = app_info.ru_maxrss;
  return max_rssKB*1024;
}

void kokkosp_init_library(const int loadSeq, const uint64_t interfaceVer,
                          const uint32_t devInfoCount,
                          Kokkos_Profiling_KokkosPDeviceInfo* deviceInfo) {

  num_spaces = 0;
  for(int i=0; i<16; i++)
    space_size[i] = 0;

  printf("KokkosP: MemoryEvents loaded (sequence: %d, version: %llu)\n", loadSeq, interfaceVer);

  timer.reset();
}

void kokkosp_finalize_library() {
  char* hostname = (char*) malloc(sizeof(char) * 256);
  gethostname(hostname, 256);
  int pid = getpid();

  {
    char* fileOutput = (char*) malloc(sizeof(char) * 256);
    sprintf(fileOutput, "%s-%d.mem_events", hostname, pid);

    FILE* ofile = fopen(fileOutput, "wb");
    free(fileOutput);

    fprintf(ofile,"# Memory Events\n");
    fprintf(ofile,"# Time     Ptr                  Size        MemSpace      Op         Name\n");
    for(int i=0; i<events.size();i++)
      events[i].print_record(ofile);
    fclose(ofile);
  }

  for(int s = 0; s<num_spaces; s++) {
    char* fileOutput = (char*) malloc(sizeof(char) * 256);
    sprintf(fileOutput, "%s-%d-%s.memspace_usage", hostname, pid, space_name[s]);

    FILE* ofile = fopen(fileOutput, "wb");
    free(fileOutput);

    fprintf(ofile,"# Space %s\n",space_name[s]);
    fprintf(ofile,"# Time(s)  Size(MB)   HighWater(MB)   HighWater-Process(MB)\n");
    uint64_t maxvalue = 0;
    for(int i=0; i<space_size_track[s].size(); i++) {
      if(std::get<1>(space_size_track[s][i]) > maxvalue) maxvalue = std::get<1>(space_size_track[s][i]);
      fprintf(ofile,"%lf %.1lf %.1lf %.1lf\n",
              std::get<0>(space_size_track[s][i]),
              1.0*std::get<1>(space_size_track[s][i])/1024/1024,
              1.0*maxvalue/1024/1024,
              1.0*std::get<2>(space_size_track[s][i])/1024/1024);
    }
    fclose(ofile);
 }
  free(hostname);
}

void kokkosp_allocate_data(const SpaceHandle space, const char* label, const void* const ptr, const uint64_t size) {
  std::lock_guard<std::mutex> lock(m);

  double time = timer.seconds();

  int space_i = num_spaces;
  for(int s = 0; s<num_spaces; s++)
    if(strcmp(space_name[s],space.name)==0)
      space_i = s;

  if(space_i == num_spaces) {
    strncpy(space_name[num_spaces],space.name,64);
    num_spaces++;
  }
  space_size[space_i] += size;
  space_size_track[space_i].push_back(std::make_tuple(time,space_size[space_i],max_mem_usage()));

  int i=events.size();
  events.push_back(EventRecord(ptr,size,MEMOP_ALLOCATE,space_i,time,label));
}


void kokkosp_deallocate_data(const SpaceHandle space, const char* label, const void* const ptr, const uint64_t size) {
  std::lock_guard<std::mutex> lock(m);

  double time = timer.seconds();

  int space_i = num_spaces;
  for(int s = 0; s<num_spaces; s++)
    if(strcmp(space_name[s],space.name)==0)
      space_i = s;

  if(space_i == num_spaces) {
    strncpy(space_name[num_spaces],space.name,64);
    num_spaces++;
  }
  if(space_size[space_i] >= size) {
    space_size[space_i] -= size;
    space_size_track[space_i].push_back(std::make_tuple(time,space_size[space_i],max_mem_usage()));
  }

  int i=events.size();
  events.push_back(EventRecord(ptr,size,MEMOP_DEALLOCATE,space_i,time,label));
}

void kokkosp_push_profile_region(const char* name) {
  std::lock_guard<std::mutex> lock(m);
  double time = timer.seconds();
  events.push_back(EventRecord(nullptr,0,MEMOP_PUSH_REGION,0,time,name));
}

void kokkosp_pop_profile_region() {
  std::lock_guard<std::mutex> lock(m);
  double time = timer.seconds();
  events.push_back(EventRecord(nullptr,0,MEMOP_POP_REGION,0,time,""));
}

Kokkos::Tools::Experimental::EventSet get_event_set() {
    Kokkos::Tools::Experimental::EventSet my_event_set;
    memset(&my_event_set, 0, sizeof(my_event_set)); // zero any pointers not set here
    my_event_set.init = kokkosp_init_library;
    my_event_set.finalize = kokkosp_finalize_library;
    my_event_set.allocate_data = kokkosp_allocate_data;
    my_event_set.deallocate_data = kokkosp_deallocate_data;
    my_event_set.push_region = kokkosp_push_profile_region;
    my_event_set.pop_region = kokkosp_pop_profile_region;
    return my_event_set;
}

}} // namespace KokkosTools::MemoryEvents

extern "C" {

namespace impl = KokkosTools::MemoryEvents;

EXPOSE_INIT(impl::kokkosp_init_library)
EXPOSE_FINALIZE(impl::kokkosp_finalize_library)
EXPOSE_ALLOCATE(impl::kokkosp_allocate_data)
EXPOSE_DEALLOCATE(impl::kokkosp_deallocate_data)
EXPOSE_PUSH_REGION(impl::kokkosp_push_profile_region)
EXPOSE_POP_REGION(impl::kokkosp_pop_profile_region)

} // extern "C"