#include <caliper/cali.h>
#include <caliper/cali_datatracker.h>
#include <inttypes.h>
#include <string>

static std::string kernel_name;

extern "C" {

void kokkosp_init_library(const int loadSeq,
                          const uint64_t interfaceVer,
                          const uint32_t devInfoCount,
                          void* deviceInfo)
{
}

void kokkosp_finalize_library()
{
}

void kokkosp_begin_parallel_for(const char* name,
                                const uint32_t devID,
                                uint64_t* kID)
{
  if (cali_function_attr_id == CALI_INV_ID)
    cali_init();
  kernel_name = std::string(name);
  cali_begin_string(cali_function_attr_id, kernel_name.c_str());
}

void kokkosp_end_parallel_for(const uint64_t kID)
{
  cali_safe_end_string(cali_function_attr_id, kernel_name.c_str());
}


void kokkosp_begin_parallel_scan(const char* name,
                                 const uint32_t devID,
                                 uint64_t* kID)
{
  if (cali_function_attr_id == CALI_INV_ID)
    cali_init();
  kernel_name = std::string(name);
  cali_begin_string(cali_function_attr_id, kernel_name.c_str());
}

void kokkosp_end_parallel_scan(const uint64_t kID)
{
  cali_safe_end_string(cali_function_attr_id, kernel_name.c_str());
}

void kokkosp_begin_parallel_reduce(const char* name,
                                   const uint32_t devID,
                                   uint64_t* kID)
{
  if (cali_function_attr_id == CALI_INV_ID)
    cali_init();
  kernel_name = std::string(name);
  cali_begin_string(cali_function_attr_id, kernel_name.c_str());
}

void kokkosp_end_parallel_reduce(const uint64_t kID)
{
  cali_safe_end_string(cali_function_attr_id, kernel_name.c_str());
}

struct SpaceHandle {
  char name[64];
};

void kokkosp_begin_deep_copy(
  SpaceHandle dst_handle, const char* dst_name, const void* dst_ptr,
  SpaceHandle src_handle, const char* src_name, const void* src_ptr,
  uint64_t size)
{
  if (cali_function_attr_id == CALI_INV_ID)
    cali_init();
  kernel_name = std::string("deep_copy_") + dst_name + "_" + std::string(src_name);
  cali_begin_string(cali_function_attr_id, kernel_name.c_str());
}

void kokkosp_end_deep_copy()
{
  cali_safe_end_string(cali_function_attr_id, kernel_name.c_str());
}

void kokkosp_allocate_data(const SpaceHandle space,
                           const char* label,
                           const void* const ptr,
                           const uint64_t size)
{
  if (size > 0 && ptr != NULL)
    cali_datatracker_track(ptr, label, size);
}

void kokkosp_deallocate_data(const SpaceHandle space,
                             const char* label,
                             const void* const ptr,
                             const uint64_t size)
{
  if (size > 0 && ptr != NULL)
    cali_datatracker_untrack(ptr);
}

}
