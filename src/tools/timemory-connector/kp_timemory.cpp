
#include <cassert>
#include <cstdlib>
#include <execinfo.h>
#include <iostream>
#include <string>
#include <vector>

#include <timemory/runtime/configure.hpp>
#include <timemory/timemory.hpp>

static std::string spacer =
    "#---------------------------------------------------------------------------#";

// this just differentiates Kokkos from other user_bundles
struct KokkosProfiler
{
};

using KokkosUserBundle = tim::component::user_bundle<0, KokkosProfiler>;
using profile_entry_t  = tim::component_tuple<KokkosUserBundle>;

using section_entry_t = std::tuple<std::string, profile_entry_t>;
using profile_stack_t = std::vector<profile_entry_t>;
using profile_map_t   = std::unordered_map<uint64_t, profile_entry_t>;
using section_map_t   = std::unordered_map<uint64_t, section_entry_t>;

//--------------------------------------------------------------------------------------//

static uint64_t
get_unique_id()
{
    static thread_local uint64_t _instance = 0;
    return _instance++;
}

//--------------------------------------------------------------------------------------//

template <typename _Tp>
_Tp&
get_tl_static()
{
    static thread_local _Tp _instance;
    return _instance;
}

//--------------------------------------------------------------------------------------//

static profile_map_t&
get_profile_map()
{
    return get_tl_static<profile_map_t>();
}

//--------------------------------------------------------------------------------------//

static section_map_t&
get_section_map()
{
    return get_tl_static<section_map_t>();
}

//--------------------------------------------------------------------------------------//

static profile_stack_t&
get_profile_stack()
{
    return get_tl_static<profile_stack_t>();
}

//--------------------------------------------------------------------------------------//

static void
create_profiler(const std::string& pname, uint64_t kernid)
{
    get_profile_map().insert(std::make_pair(kernid, profile_entry_t(pname)));
}

//--------------------------------------------------------------------------------------//

static void
destroy_profiler(uint64_t kernid)
{
    if(get_profile_map().find(kernid) != get_profile_map().end())
        get_profile_map().erase(kernid);
}

//--------------------------------------------------------------------------------------//

static void
start_profiler(uint64_t kernid)
{
    if(get_profile_map().find(kernid) != get_profile_map().end())
        get_profile_map().at(kernid).start();
}

//--------------------------------------------------------------------------------------//

static void
stop_profiler(uint64_t kernid)
{
    if(get_profile_map().find(kernid) != get_profile_map().end())
        get_profile_map().at(kernid).stop();
}

//======================================================================================//
//
//      Kokkos symbols
//
//======================================================================================//

extern "C" void
kokkosp_init_library(const int loadSeq, const uint64_t interfaceVer,
                     const uint32_t devInfoCount, void* deviceInfo)
{
    printf("%s\n", spacer.c_str());
    printf("# KokkosP: timemory Connector (sequence is %d, version: %llu)\n", loadSeq,
           (long long int) interfaceVer);
    printf("%s\n\n", spacer.c_str());

    char* hostname = (char*) malloc(sizeof(char) * 256);
    gethostname(hostname, 256);

    bool use_roofline = tim::get_env<bool>("KOKKOS_ROOFLINE", false);

    std::stringstream folder;
    if(!use_roofline)
        folder << hostname << "_" << ((int) getpid());
    else
    {
        std::stringstream tmp;
        tmp << hostname << "_roofline";
        folder << tim::get_env<std::string>("TIMEMORY_OUTPUT_PATH", tmp.str());
        tim::settings::output_path() = folder.str();
        printf("%s\n[%s]> KOKKOS_ROOFLINE is enabled. Output directory: \"%s\"\n%s\n",
               spacer.c_str(), "timemory-connector", folder.str().c_str(),
               spacer.c_str());
    }
    // eventually, use below for roofline but will require updates to timemory
    //  folder << hostname << "_" << ((int) getpid());

    free(hostname);

    auto papi_events              = tim::get_env<std::string>("PAPI_EVENTS", "");
    tim::settings::papi_events()  = papi_events;
    tim::settings::auto_output()  = true;   // print when destructing
    tim::settings::cout_output()  = true;   // print to stdout
    tim::settings::text_output()  = true;   // print text files
    tim::settings::json_output()  = true;   // print to json
    tim::settings::banner()       = true;   // suppress banner
    tim::settings::mpi_finalize() = false;  // don't finalize MPI during timemory_finalize

    std::stringstream ss;
    ss << loadSeq << "_" << interfaceVer << "_" << devInfoCount;
    auto cstr = const_cast<char*>(ss.str().c_str());
    tim::timemory_init(1, &cstr, "", "");
    tim::settings::output_path() = folder.str();

    // the environment variable to configure components
    std::string env_var = "KOKKOS_TIMEMORY_COMPONENTS";
    // if roofline is enabled, provide nothing by default
    // if roofline is not enabled, profile wall-clock by default
    std::string components = (use_roofline) ? "" : "wall_clock;peak_rss";
    // query the environment
    auto env_result = tim::get_env(env_var, components);
    std::transform(env_result.begin(), env_result.end(), env_result.begin(),
                   [](unsigned char c) -> unsigned char { return std::tolower(c); });
    // if a roofline component is not set in the environment, then add both the
    // cpu and gpu roofline
    if(use_roofline && env_result.find("roofline") == std::string::npos)
        env_result = TIMEMORY_JOIN(";", env_result, "gpu_roofline_flops", "cpu_roofline");
    // configure the bundle to use these components
    tim::configure<KokkosUserBundle>(tim::enumerate_components(tim::delimit(env_result)));
}

extern "C" void
kokkosp_finalize_library()
{
    printf("\n%s\n", spacer.c_str());
    printf("KokkosP: Finalization of timemory Connector. Complete.\n");
    printf("%s\n\n", spacer.c_str());

    for(auto& itr : get_profile_map())
        itr.second.stop();
    get_profile_map().clear();

    tim::timemory_finalize();
}

//--------------------------------------------------------------------------------------//

extern "C" void
kokkosp_begin_parallel_for(const char* name, uint32_t devid, uint64_t* kernid)
{
    auto pname = TIMEMORY_JOIN("/", "kokkos", TIMEMORY_JOIN("", "dev", devid), name);
    *kernid    = get_unique_id();
    create_profiler(pname, *kernid);
    start_profiler(*kernid);
}

extern "C" void
kokkosp_end_parallel_for(uint64_t kernid)
{
    stop_profiler(kernid);
    destroy_profiler(kernid);
}

//--------------------------------------------------------------------------------------//

extern "C" void
kokkosp_begin_parallel_reduce(const char* name, uint32_t devid, uint64_t* kernid)
{
    auto pname = TIMEMORY_JOIN("/", "kokkos", TIMEMORY_JOIN("", "dev", devid), name);
    *kernid    = get_unique_id();
    create_profiler(pname, *kernid);
    start_profiler(*kernid);
}

extern "C" void
kokkosp_end_parallel_reduce(uint64_t kernid)
{
    stop_profiler(kernid);
    destroy_profiler(kernid);
}

//--------------------------------------------------------------------------------------//

extern "C" void
kokkosp_begin_parallel_scan(const char* name, uint32_t devid, uint64_t* kernid)
{
    auto pname = TIMEMORY_JOIN("/", "kokkos", TIMEMORY_JOIN("", "dev", devid), name);
    *kernid    = get_unique_id();
    create_profiler(pname, *kernid);
    start_profiler(*kernid);
}

extern "C" void
kokkosp_end_parallel_scan(uint64_t kernid)
{
    stop_profiler(kernid);
    destroy_profiler(kernid);
}

//--------------------------------------------------------------------------------------//

extern "C" void
kokkosp_push_profile_region(const char* name)
{
    get_profile_stack().push_back(profile_entry_t(name));
    get_profile_stack().back().start();
}

extern "C" void
kokkosp_pop_profile_region()
{
    if(get_profile_stack().empty())
        return;
    get_profile_stack().back().stop();
    get_profile_stack().pop_back();
}

//--------------------------------------------------------------------------------------//

extern "C" void
kokkosp_create_profile_section(const char* name, uint32_t* secid)
{
    *secid     = get_unique_id();
    auto pname = TIMEMORY_JOIN("/", "kokkos", TIMEMORY_JOIN("", "section", secid), name);
    create_profiler(pname, *secid);
}

extern "C" void
kokkosp_destroy_profile_section(uint32_t secid)
{
    destroy_profiler(secid);
}

//--------------------------------------------------------------------------------------//

extern "C" void
kokkosp_start_profile_section(uint32_t secid)
{
    start_profiler(secid);
}

extern "C" void
kokkosp_stop_profile_section(uint32_t secid)
{
    start_profiler(secid);
}

//--------------------------------------------------------------------------------------//
