
#include <cstdint>
#include <cstring>

extern "C"
{
    void kokkosp_init_library(const int, const uint64_t, const uint32_t, void*) {}
    void kokkosp_finalize_library() {}
    void kokkosp_begin_parallel_for(const char*, uint32_t, uint64_t*) {}
    void kokkosp_end_parallel_for(uint64_t) {}
    void kokkosp_begin_parallel_reduce(const char*, uint32_t, uint64_t*) {}
    void kokkosp_end_parallel_reduce(uint64_t) {}
    void kokkosp_begin_parallel_scan(const char*, uint32_t, uint64_t*) {}
    void kokkosp_end_parallel_scan(uint64_t) {}
    void kokkosp_push_profile_region(const char*) {}
    void kokkosp_pop_profile_region() {}
    void kokkosp_create_profile_section(const char*, uint32_t*) {}
    void kokkosp_destroy_profile_section(uint32_t) {}
    void kokkosp_start_profile_section(uint32_t) {}
    void kokkosp_stop_profile_section(uint32_t) {}
}  // extern "C"
