#include <iostream>
#include <stdio.h>
#include <inttypes.h>
#include <execinfo.h>
#include <cstdlib>
#include <cstring>
#include <map>
#include <stack>
#include <set>
#include <vector>
#include <algorithm>
#include <functional>
#include <string>
#include <sys/time.h>
#include <cxxabi.h>
#include <unistd.h>
#include "kp_kernel_info.h"
#include "kp_memory_events.hpp"
#include <cstdio>

#include <dlfcn.h>

struct NamedPointer {
   std::uintptr_t ptr;
   const char* name;
   std::uint64_t size;
   const SpaceHandle space;
};

namespace std {
  template<>
  struct less<NamedPointer> {
    static std::less<std::uintptr_t> comp;
    bool operator()(const NamedPointer& n1, const NamedPointer& n2) const{
      return !comp(n1.ptr, n2.ptr);
    }
  };
};

std::vector<std::string> kokkos_stack;
std::set<NamedPointer> tracked_pointers;
std::map<std::string, std::set<NamedPointer>> space_map;

using poisonFunctionType = void(*)(void*, std::size_t);

poisonFunctionType poisonFunction;
poisonFunctionType unpoisonFunction;

void poisonSpace(std::string spaceName){
  std::cout << "Poisoning "<<spaceName << std::endl;
  for(auto named_pointer : space_map[spaceName]){
    poisonFunction((void*)named_pointer.ptr, named_pointer.size);
  }
}

void unpoisonSpace(std::string spaceName){
  std::cout << "Unpoisoning "<<spaceName << std::endl;
  for(auto named_pointer : space_map[spaceName]){
    unpoisonFunction((void*)named_pointer.ptr, named_pointer.size);
  }
}

extern "C" void kokkosp_init_library(const int loadSeq,
	const uint64_t interfaceVer,
	const uint32_t devInfoCount,
	void* deviceInfo) {

  void* liveProgram = dlopen(nullptr, RTLD_NOW | RTLD_GLOBAL);

  auto poisonFunctionHandle = dlsym(liveProgram, "__asan_poison_memory_region");
  auto unpoisonFunctionHandle = dlsym(liveProgram, "__asan_unpoison_memory_region");

  poisonFunction = *((poisonFunctionType*)&poisonFunctionHandle);
  unpoisonFunction = *((poisonFunctionType*)&unpoisonFunctionHandle);

  space_map["Host"] = std::set<NamedPointer>();
  space_map["HBW"] = std::set<NamedPointer>();
  poisonSpace("Host");
  poisonSpace("HBW");
  
}

extern "C" void kokkosp_finalize_library() {
}

extern "C" void kokkosp_begin_parallel_for(const char* name, const uint32_t devID, uint64_t* kID) {
  kokkos_stack.push_back(name);
  const char* iter = nullptr;
  iter = strstr(name,"Host");
  if(iter != nullptr) {
    poisonSpace("HBW");
  }
  else{
    iter = strstr(name,"Device");
    if(iter!=nullptr){
      poisonSpace("Host");
    }
  }
}

extern "C" void kokkosp_end_parallel_for(const uint64_t kID) {
  std::string name = kokkos_stack.back();
  auto iter = name.find("Host");
  if(iter != std::string::npos) {
    unpoisonSpace("HBW");
  }
  else{
    iter = name.find("Device");
    if(iter!=std::string::npos){
      unpoisonSpace("Host");
    }
  }
	kokkos_stack.pop_back();
}

extern "C" void kokkosp_begin_parallel_scan(const char* name, const uint32_t devID, uint64_t* kID) {
  kokkos_stack.push_back(name);
}

extern "C" void kokkosp_end_parallel_scan(const uint64_t kID) {
	kokkos_stack.pop_back();
}

extern "C" void kokkosp_begin_parallel_reduce(const char* name, const uint32_t devID, uint64_t* kID) {
  kokkos_stack.push_back(name);
}

extern "C" void kokkosp_end_parallel_reduce(const uint64_t kID) {
	kokkos_stack.pop_back();
}

extern "C" void kokkosp_push_profile_region(char* regionName) {
  kokkos_stack.push_back(regionName);
}

extern "C" void kokkosp_pop_profile_region() {
	kokkos_stack.pop_back();
}

extern "C" void kokkosp_deallocate_data(const SpaceHandle space, const char* label, const void* const ptr_raw, const uint64_t size) {
  auto ptr = reinterpret_cast<std::uintptr_t>(ptr_raw);
  //unpoisonFunction((void*)ptr, size);
  auto key = NamedPointer{ptr};
  tracked_pointers.erase(key);
  std::string lazy_hack = space.name;
  auto iter = space_map.find(lazy_hack);
  if(iter != space_map.end()){
    space_map[lazy_hack] = std::set<NamedPointer>();
  }
  space_map[lazy_hack].erase(key);
}

extern "C" void kokkosp_allocate_data(const SpaceHandle space, const char* label, const void* const ptr_raw, const uint64_t size) {
  auto ptr = reinterpret_cast<std::uintptr_t>(ptr_raw);
  std::size_t length = strlen(label);
  char* name = (char*)malloc(sizeof(char) * (length+1));
  strncpy(name, label, length+1);
  auto key = NamedPointer { ptr, name, size, space };
  tracked_pointers.insert(key);
  std::string lazy_hack = space.name;
  auto iter = space_map.find(lazy_hack);
  if(iter != space_map.end()){
    space_map[lazy_hack] = std::set<NamedPointer>();
  }
  space_map[lazy_hack].insert(key);
  // poisonFunction((void*)ptr, size);
}

extern "C" const char* parallel_runtime_get_pointer_name(const void* ptr_raw){
  auto ptr = reinterpret_cast<std::uintptr_t>(ptr_raw);
  auto nearest = tracked_pointers.upper_bound(NamedPointer{ptr, ""});
  return nearest->name;
}

extern "C" const char** parallel_runtime_get_callstack() {

  const char** stack = (const char**)malloc(sizeof(const char*) * (kokkos_stack.size() + 1));
  for(int i=0;i<kokkos_stack.size();++i){
    stack[i] = kokkos_stack[i].c_str();
  }
  stack[kokkos_stack.size()] = nullptr;
  return stack;
}
