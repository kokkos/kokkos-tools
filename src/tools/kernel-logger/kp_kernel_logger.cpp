
#include <cstdio>
#include <inttypes.h>
#include <vector>
#include <string>



std::vector<std::string> regions;
static uint64_t uniqID;
struct SpaceHandle {
  char name[64];
};

void kokkosp_print_region_stack_indent(const int level) {
	printf("KokkosP: ");
	
	for(int i = 0; i < level; i++) {
		printf("  ");
	}
}

int kokkosp_print_region_stack() {
	int level = 0;

	for(auto regItr = regions.begin(); regItr != regions.end(); regItr++) {
		kokkosp_print_region_stack_indent(level);
		printf("%s\n", (*regItr).c_str());
		
		level++;
	}
	
	return level;
}

extern "C" void kokkosp_init_library(const int loadSeq,
	const uint64_t interfaceVer,
	const uint32_t devInfoCount,
	void* deviceInfo) {

	printf("KokkosP: Example Library Initialized (sequence is %d, version: %llu)\n", loadSeq, interfaceVer);
	uniqID = 0;
	
}

extern "C" void kokkosp_finalize_library() {
	printf("KokkosP: Kokkos library finalization called.\n");
}

extern "C" void kokkosp_begin_parallel_for(const char* name, const uint32_t devID, uint64_t* kID) {
	*kID = uniqID++;

	printf("KokkosP: Executing parallel-for kernel on device %d with unique execution identifier %llu\n",
		devID, *kID);

	int level = kokkosp_print_region_stack();
	kokkosp_print_region_stack_indent(level);
	
	printf("%s\n", name);
}

extern "C" void kokkosp_end_parallel_for(const uint64_t kID) {
	printf("KokkosP: Execution of kernel %llu is completed.\n", kID);
}

extern "C" void kokkosp_begin_parallel_scan(const char* name, const uint32_t devID, uint64_t* kID) {
	*kID = uniqID++;

	printf("KokkosP: Executing parallel-scan kernel on device %d with unique execution identifier %llu\n",
		devID, *kID);

	int level = kokkosp_print_region_stack();
	kokkosp_print_region_stack_indent(level);
	
	printf("%s\n", name);
}

extern "C" void kokkosp_end_parallel_scan(const uint64_t kID) {
	printf("KokkosP: Execution of kernel %llu is completed.\n", kID);
}

extern "C" void kokkosp_begin_parallel_reduce(const char* name, const uint32_t devID, uint64_t* kID) {
	*kID = uniqID++;

	printf("KokkosP: Executing parallel-reduce kernel on device %d with unique execution identifier %llu\n",
		devID, *kID);

	int level = kokkosp_print_region_stack();
	kokkosp_print_region_stack_indent(level);
	
	printf("%s\n", name);
}

extern "C" void kokkosp_end_parallel_reduce(const uint64_t kID) {
	printf("KokkosP: Execution of kernel %llu is completed.\n", kID);
}

extern "C" void kokkosp_push_profile_region(char* regionName) {
	printf("KokkosP: Entering profiling region: %s\n", regionName);

	std::string regionNameStr(regionName);
	regions.push_back(regionNameStr);
}

extern "C" void kokkosp_pop_profile_region() {
	if(regions.size() > 0) {
		printf("KokkosP: Exiting profiling region: %s\n", regions.back().c_str());
		regions.pop_back();
	}
}

extern "C" void kokkosp_allocate_data(SpaceHandle handle, const char* name, void* ptr, uint64_t size) {
  printf("KokkosP: Allocate<%s> name: %s pointer: %p size: %lu\n",handle.name,name,ptr,size);
}

extern "C" void kokkosp_deallocate_data(SpaceHandle handle, const char* name, void* ptr, uint64_t size) {
  printf("KokkosP: Deallocate<%s> name: %s pointer: %p size: %lu\n",handle.name,name,ptr,size);
}

extern "C" void kokkosp_begin_deep_copy(
    SpaceHandle dst_handle, const char* dst_name, const void* dst_ptr,
    SpaceHandle src_handle, const char* src_name, const void* src_ptr,
    uint64_t size) {
  printf("KokkosP: DeepCopy<%s,%s> DST(name: %s pointer: %p) SRC(name: %s pointer %p) Size: %lu\n",
    dst_handle.name, src_handle.name,
    dst_name, dst_ptr, src_name, src_ptr, size);
}
