#pragma once

#include <cstdint>
#include "kp_core.hpp"

class ToolInterface {
 public:
  ToolInterface()          = default;
  virtual ~ToolInterface() = default;
  virtual void init_library(const int loadSeq, const uint64_t interfaceVer,
                            const uint32_t devInfoCount,
                            Kokkos_Profiling_KokkosPDeviceInfo* deviceInfo) = 0;
  virtual void finalize_library()                                           = 0;
  virtual void begin_parallel_for(const char* name, const uint32_t devID,
                                  uint64_t kID)                             = 0;
  virtual void end_parallel_for(uint64_t kID)                               = 0;
  virtual void begin_parallel_scan(const char* name, const uint32_t devID,
                                   uint64_t* kID)                           = 0;
  virtual void end_parallel_scan(uint64_t kID)                              = 0;
  virtual void begin_parallel_reduce(const char* name, const uint32_t devID,
                                     uint64_t* kID)                         = 0;
  virtual void end_parallel_reduce(uint64_t kID)                            = 0;
  virtual void begin_deep_copy(Kokkos::Tools::SpaceHandle dst_handle,
                               const char* dst_name, const void* dst_ptr,
                               Kokkos::Tools::SpaceHandle src_handle,
                               const char* src_name, const void* src_ptr,
                               uint64_t size)                               = 0;
  virtual void end_deep_copy()                                              = 0;
  virtual void push_profile_region(const char* region_name)                 = 0;
  virtual void pop_profile_region()                                         = 0;
};