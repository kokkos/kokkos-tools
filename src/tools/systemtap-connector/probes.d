provider kokkos {
	probe begin_parallel_for(const char*, const uint32_t, uint64_t*);
	probe begin_parallel_scan(const char*, const uint32_t, uint64_t*);
	probe begin_parallel_reduce(const char*, const uint32_t, uint64_t*);

	probe end_parallel_scan(uint64_t);
	probe end_parallel_for(uint64_t);
	probe end_parallel_reduce(uint64_t);

	probe init_library(const int,
                           const uint64_t,
                           const uint32_t,
                           KokkosPDeviceInfo*);
	probe finalize_library();

	probe push_profile_region(const char*);
	probe pop_profile_region();

	probe allocate_data(const SpaceHandle, const char*, const void*, const uint64_t);
	probe deallocate_data(const SpaceHandle, const char*, const void*, const uint64_t);

	probe begin_deep_copy(SpaceHandle, const char*, const void*,
	    SpaceHandle, const char*, const void*,
	    uint64_t);
	probe end_deep_copy();
      
	probe create_profile_section(const char*, uint32_t*);
	probe start_profile_section(const uint32_t);
	probe stop_profile_section(const uint32_t);
	probe destroy_profile_section(const uint32_t);
      
	probe profile_event(const char*);
};