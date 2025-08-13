#pragma once

#include <string>
#include <deque>
#include "../common/tool_interface.hpp"
#include "../common/timer.hpp"

class KernelTimerTool : public ToolInterface {
 public:
  KernelTimerTool()           = default;
  ~KernelTimerTool() override = default;

  void init_library(const int loadSeq, const uint64_t interfaceVer,
                    const uint32_t devInfoCount,
                    Kokkos_Profiling_KokkosPDeviceInfo* deviceInfo) override;
  void finalize_library() override;

  // Stack-based timing for robust region/kernel tracking
  void start_region(const std::string& name, RegionType type, uint64_t id = 0);
  void end_region();

  // Kokkos interface
  void begin_parallel_for(const char* name, const uint32_t devID,
                          uint64_t kID) override;
  void end_parallel_for(uint64_t kID) override;

  void begin_parallel_scan(const char* name, const uint32_t devID,
                           uint64_t* kID) override;
  void end_parallel_scan(uint64_t kID) override;

  void begin_parallel_reduce(const char* name, const uint32_t devID,
                             uint64_t* kID) override;
  void end_parallel_reduce(uint64_t kID) override;

  void begin_deep_copy(Kokkos::Tools::SpaceHandle dst_handle,
                       const char* dst_name, const void* dst_ptr,
                       Kokkos::Tools::SpaceHandle src_handle,
                       const char* src_name, const void* src_ptr,
                       uint64_t size) override;
  void end_deep_copy() override;

  void push_profile_region(const char* region_name) override;
  void pop_profile_region() override;

  // Getters for summary
  const std::deque<TimingInfo>& get_kernel_timings() const;
  const std::deque<TimingInfo>& get_region_timings() const;
  const std::deque<TimingInfo>& get_deep_copy_timings() const;

 private:
  std::deque<TimingInfo> active_regions_;
  std::deque<TimingInfo> completed_kernels_;
  std::deque<TimingInfo> completed_regions_;
  std::deque<TimingInfo> completed_deepcopies_;
  uint64_t next_region_id_ = 1;
};