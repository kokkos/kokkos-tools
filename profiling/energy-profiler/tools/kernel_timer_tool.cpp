#include "kernel_timer_tool.hpp"
#include <iostream>
#include <chrono>

void KernelTimerTool::init_library(
    const int loadSeq, const uint64_t interfaceVer, const uint32_t devInfoCount,
    Kokkos_Profiling_KokkosPDeviceInfo* deviceInfo) {
  (void)devInfoCount;
  (void)deviceInfo;
  std::cout << "Kokkos Power Profiler: Initializing with load sequence "
            << loadSeq << " and interface version " << interfaceVer
            << std::endl;
  std::cout << "Kokkos Power Profiler: Library initialized" << std::endl;
}

void KernelTimerTool::finalize_library() {
  // Implementation is empty
}

void KernelTimerTool::start_region(const std::string& name, RegionType type,
                                   uint64_t id) {
  TimingInfo region;
  region.name       = name;
  region.type       = type;
  region.start_time = std::chrono::high_resolution_clock::now();
  region.id         = id;
  active_regions_.push_back(region);
}

void KernelTimerTool::end_region() {
  if (!active_regions_.empty()) {
    auto region = active_regions_.back();
    active_regions_.pop_back();
    region.end_time = std::chrono::high_resolution_clock::now();
    region.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(
        region.end_time - region.start_time);
    if (region.type == RegionType::UserRegion)
      completed_regions_.push_back(region);
    else if (region.type == RegionType::DeepCopy)
      completed_deepcopies_.push_back(region);
    else
      completed_kernels_.push_back(region);
  }
}

void KernelTimerTool::begin_parallel_for(const char* name, const uint32_t devID,
                                         uint64_t kID) {
  (void)devID;
  start_region(name, RegionType::ParallelFor, kID);
}

void KernelTimerTool::end_parallel_for(uint64_t kID) { 
  (void)kID;
  end_region(); 
}

void KernelTimerTool::begin_parallel_scan(const char* name,
                                          const uint32_t devID, uint64_t* kID) {
  (void)devID;
  start_region(name, RegionType::ParallelScan, *kID);
}

void KernelTimerTool::end_parallel_scan(uint64_t kID) { 
  (void)kID;
  end_region(); 
}

void KernelTimerTool::begin_parallel_reduce(const char* name,
                                            const uint32_t devID,
                                            uint64_t* kID) {
  (void)devID;
  start_region(name, RegionType::ParallelReduce, *kID);
}

void KernelTimerTool::end_parallel_reduce(uint64_t kID) { 
  (void)kID;
  end_region(); 
}

void KernelTimerTool::begin_deep_copy(Kokkos::Tools::SpaceHandle dst_handle,
                                      const char* dst_name, const void* dst_ptr,
                                      Kokkos::Tools::SpaceHandle src_handle,
                                      const char* src_name, const void* src_ptr,
                                      uint64_t size) {
  (void)dst_handle;
  (void)src_handle;
  (void)src_name;
  (void)src_ptr;
  (void)size;
  start_region(dst_name, RegionType::DeepCopy,
               reinterpret_cast<uint64_t>(dst_ptr));
}

void KernelTimerTool::end_deep_copy() { end_region(); }

void KernelTimerTool::push_profile_region(const char* region_name) {
  start_region(region_name, RegionType::UserRegion, next_region_id_++);
}

void KernelTimerTool::pop_profile_region() { end_region(); }

const std::deque<TimingInfo>& KernelTimerTool::get_kernel_timings() const {
  return completed_kernels_;
}

const std::deque<TimingInfo>& KernelTimerTool::get_region_timings() const {
  return completed_regions_;
}

const std::deque<TimingInfo>& KernelTimerTool::get_deep_copy_timings() const {
  return completed_deepcopies_;
}
