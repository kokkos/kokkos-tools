
#include <cassert>
#include <execinfo.h>
#include <iostream>
#include <string>
#include <timemory/timemory.hpp>
#include <vector>

extern "C"
{
    void timemory_init_library(int, char**);
    void timemory_finalize_library();
    void timemory_begin_record(const char*, uint64_t*);
    void timemory_end_record(uint64_t);
}

static std::vector<uint64_t> regions;
static std::string           spacer =
    "#-------------------------------------------------------------------------#";

using section_entry_t   = std::tuple<std::string, uint64_t>;
using section_map_t     = std::unordered_map<uint64_t, section_entry_t>;
using section_map_ptr_t = std::unique_ptr<section_map_t>;

static uint64_t
get_unique_id()
{
    static thread_local uint64_t _instance = 0;
    return _instance++;
}

static section_map_t*
get_section_map()
{
    static thread_local section_map_ptr_t _instance(new section_map_t);
    return _instance.get();
}

//--------------------------------------------------------------------------------------//
//
//      Kokkos symbols
//
//--------------------------------------------------------------------------------------//

extern "C" void
kokkosp_init_library(const int loadSeq, const uint64_t interfaceVer,
                     const uint32_t devInfoCount, void* deviceInfo)
{
    printf("%s\n", spacer.c_str());
    printf("KokkosP: TiMemory Connector (sequence is %d, version: %llu)\n", loadSeq,
           (long long int) interfaceVer);
    printf("%s\n\n", spacer.c_str());

    char* hostname = (char*) malloc(sizeof(char) * 256);
    gethostname(hostname, 256);

    std::stringstream folder;
    folder << hostname << "_" << ((int) getpid());
    free(hostname);

    tim::settings::auto_output() = true;   // print when destructing
    tim::settings::cout_output() = true;   // print to stdout
    tim::settings::text_output() = true;   // print text files
    tim::settings::json_output() = true;   // print to json
    tim::settings::banner()      = false;  // suppress banner

    std::stringstream ss;
    ss << loadSeq << "_" << interfaceVer << "_" << devInfoCount;
    auto cstr = const_cast<char*>(ss.str().c_str());
    timemory_init_library(1, &cstr);
    tim::timemory_init(1, &cstr, "", "");
    tim::settings::output_path() = folder.str();
}

extern "C" void
kokkosp_finalize_library()
{
    printf("\n%s\n", spacer.c_str());
    printf("KokkosP: Finalization of TiMemory Connector. Complete.\n");
    printf("%s\n\n", spacer.c_str());

    timemory_finalize_library();
}

extern "C" void
kokkosp_begin_parallel_for(const char* name, uint32_t devid, uint64_t* kernid)
{
    auto tname = TIMEMORY_JOIN(" ", TIMEMORY_JOIN("", "[", devid, "]"), name);
    timemory_begin_record(tname.c_str(), kernid);
}

extern "C" void
kokkosp_end_parallel_for(uint64_t kernid)
{
    timemory_end_record(kernid);
}

extern "C" void
kokkosp_begin_parallel_reduce(const char* name, uint32_t devid, uint64_t* kernid)
{
    auto tname = TIMEMORY_JOIN(" ", TIMEMORY_JOIN("", "[", devid, "]"), name);
    timemory_begin_record(tname.c_str(), kernid);
}

extern "C" void
kokkosp_end_parallel_reduce(uint64_t kernid)
{
    timemory_end_record(kernid);
}

extern "C" void
kokkosp_begin_parallel_scan(const char* name, uint32_t devid, uint64_t* kernid)
{
    auto tname = TIMEMORY_JOIN(" ", TIMEMORY_JOIN("", "[", devid, "]"), name);
    timemory_begin_record(tname.c_str(), kernid);
}

extern "C" void
kokkosp_end_parallel_scan(uint64_t kernid)
{
    timemory_end_record(kernid);
}

extern "C" void
kokkosp_push_profile_region(const char* name)
{
    uint64_t regid = 0;
    timemory_begin_record(name, &regid);
    regions.push_back(regid);
}

extern "C" void
kokkosp_pop_profile_region()
{
    timemory_end_record(regions.back());
    regions.pop_back();
}

/*
extern "C" void
kokkosp_profile_event(const char* name)
{
}
*/

extern "C" void
kokkosp_create_profile_section(const char* name, uint32_t* sec_id)
{
    *sec_id = get_unique_id();
    auto tname =
        TIMEMORY_JOIN("_", "section", *sec_id, TIMEMORY_JOIN("", "[", name, "]"));
    auto* _sections = get_section_map();
    assert(_sections);
    (*_sections)[*sec_id] = section_entry_t(tname, 0);
}

extern "C" void
kokkosp_destroy_profile_section(uint32_t sec_id)
{
    auto* _sections = get_section_map();
    assert(_sections);
    if(_sections->find(sec_id) != _sections->end())
        _sections->erase(sec_id);
}

extern "C" void
kokkosp_start_profile_section(uint32_t sec_id)
{
    auto* _psections = get_section_map();
    assert(_psections);
    auto& _sections = *_psections;
    if(_sections.find(sec_id) != _sections.end())
        timemory_begin_record(std::get<0>(_sections[sec_id]).c_str(),
                              &std::get<1>(_sections[sec_id]));
}

extern "C" void
kokkosp_stop_profile_section(uint32_t sec_id)
{
    auto* _sections = get_section_map();
    if(_sections->find(sec_id) != _sections->end())
        timemory_end_record(std::get<1>((*_sections)[sec_id]));
}
