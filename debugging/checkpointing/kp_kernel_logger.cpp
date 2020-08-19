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

#include <cstdio>
#include <inttypes.h>
#include <vector>
#include <iostream>
#include <signal.h>
#include <protocols/checkpointing.pb.h>
#include <fstream>
#include <string>
#include <cstring>
#include <cuda_runtime.h>
#include <set>
#include <map>
#include <algorithm>
#include <list>
struct ptr_info {
  std::string who;
  void* what;
  size_t how_much;
  bool where;
  void* canonical; // look, I'm in a hurry, I'm sorry
};
std::string rank_string = "0";
struct variable_data {
  std::vector<ptr_info> instances;
  void insert(ptr_info in){
    instances.push_back(in);
  }
  void remove(void* out){
    auto er =
    std::remove_if(instances.begin(), instances.end(), [=](const ptr_info& test){
       bool rem= (test.what == out);
       return rem;
    });
    if(er != instances.end()) { instances.erase(er); }
  }
  void remove(ptr_info& out){
    auto er = std::remove_if(instances.begin(), instances.end(), [=](const ptr_info& test){
       return test.what == out.what;
    });
    if(er != instances.end()) { instances.erase(er); }
  }
};

std::string trigger;
std::string output;
struct SpaceHandle {
  char name[64];
};
std::map<std::string, variable_data> allocations;

bool should_reraise(int signo){
  return ((signo != 999) && (signo != SIGINT));
}

void dump_checkpoint(int signo){
  static bool second;
  if(second){
    if(should_reraise(signo)){
            raise(signo);
    }
  }
  else{
          second = true;
  }
  std::ofstream out(output);
  int size_sum = 0;
  for(auto& alloc : allocations){
    size_sum += alloc.second.instances.size();
  }
  out << size_sum << " ";
  for(auto& variable_handle: allocations){
    auto& alloc_list = variable_handle.second; 
    for(auto& alloc: alloc_list.instances){
     kokkos_checkpointing::View v;
      v.set_size(alloc.how_much);
      v.set_name(alloc.who);
      v.set_data(alloc.canonical, alloc.how_much);
      size_t message_size  =v.ByteSizeLong();
      out << message_size;
      bool success = v.SerializeToOstream(&out);
      if(!success){
        std::cout << "Error serializing a View named "<<alloc.who<<std::endl;
        exit(1);
      }
      //out << alloc.who << " "<< alloc.how_much; 
      //out.write((char*)alloc.canonical,alloc.how_much);
    }
  }
  std::cout <<"Finished writing on rank "<<rank_string<<", signal was "<<signo<<std::endl;
  out.close();
    if(should_reraise(signo)){
            raise(signo);
    }
}

void at_exit(){
        dump_checkpoint(999);
}

extern "C" void kokkosp_init_library(const int loadSeq,
	const uint64_t interfaceVer,
	const uint32_t devInfoCount,
	void* deviceInfo) {
   std::cout << "KokkosP: Initialized checkpoint tool\n";
   const char* index = getenv("OMPI_COMM_WORLD_RANK");
   trigger = getenv("CHECKPOINT_TRIGGER_ATTR");
   output = getenv("CHECKPOINT_OUTPUT") ? getenv("CHECKPOINT_OUTPUT") : "checkpoint.kokkos";
   if(index){
     output+=".rank"+std::string(index);
     rank_string = index;
   }
   signal(SIGSEGV, dump_checkpoint);
   signal(SIGTERM, dump_checkpoint);
   signal(SIGINT, dump_checkpoint);
   signal(SIGABRT, dump_checkpoint);
   atexit(at_exit);
}

bool operator<(const ptr_info& l, const ptr_info& r){
  return l.what < r.what;
}


extern "C" void kokkosp_finalize_library() {
}

void checkpoint(){
  cudaDeviceSynchronize();
  for(auto& variable_handle: allocations){
    auto& alloc_list = variable_handle.second; 
    for(auto& alloc: alloc_list.instances){
      if(!alloc.canonical){
        alloc.canonical = malloc(alloc.how_much);
      }
      if(alloc.where){
      cudaMemcpy(alloc.canonical, alloc.what, alloc.how_much, cudaMemcpyDefault);
      }          
      else{
        memcpy(alloc.canonical, alloc.what, alloc.how_much);
      }
    }
  }
}

extern "C" void kokkosp_begin_parallel_for(const char* name, const uint32_t devID, uint64_t* kID) {
  if(std::string(name) == trigger){
          checkpoint();
  }
}




extern "C" void kokkosp_end_parallel_for(const uint64_t kID) {
}

extern "C" void kokkosp_begin_parallel_scan(const char* name, const uint32_t devID, uint64_t* kID) {
  if(std::string(name) == trigger){
          checkpoint();
  }
}

extern "C" void kokkosp_end_parallel_scan(const uint64_t kID) {
}

extern "C" void kokkosp_begin_parallel_reduce(const char* name, const uint32_t devID, uint64_t* kID) {
  if(std::string(name) == trigger){
          checkpoint();
  }
}

extern "C" void kokkosp_end_parallel_reduce(const uint64_t kID) {
}

extern "C" void kokkosp_push_profile_region(char* regionName) {
}

extern "C" void kokkosp_pop_profile_region() {
}




extern "C" void kokkosp_allocate_data(SpaceHandle handle, const char* name, void* ptr, uint64_t size) {
  bool device = (handle.name[0] == 'C');
  ptr_info info;
  info.who = name;
  info.what = ptr + 128;
  info.how_much = size; 
  info.where = device;
  info.canonical = nullptr;
  allocations[name].insert(info);
}

extern "C" void kokkosp_deallocate_data(SpaceHandle handle, const char* name, void* ptr, uint64_t size) {
  allocations[name].remove(ptr + 128);
}

extern "C" void kokkosp_begin_deep_copy(
    SpaceHandle dst_handle, const char* dst_name, const void* dst_ptr,
    SpaceHandle src_handle, const char* src_name, const void* src_ptr,
    uint64_t size) {
}
