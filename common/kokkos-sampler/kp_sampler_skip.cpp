#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <dlfcn.h>
#include "../../profiling/all/kp_core.hpp"
#include "kp_config.hpp"
#include <atomic>
#include <unordered_map>
#include <mutex>

namespace KokkosTools {
namespace Sampler {
static atomic<uint64_t> uniqID = 0;
static mutex sampler_mtx;
static unordered_map<uint64_t, pair<uint64_t, uint32_t>> infokIDSample;
static uint64_t kernelSampleSkip = 101;
static int tool_verbosity        = 0;
static int tool_globFence        = 0;

typedef void (*initFunction)(const int, const uint64_t, const uint32_t, void*);
typedef void (*finalizeFunction)();
typedef void (*beginFunction)(const char*, const uint32_t, uint64_t*);
typedef void (*endFunction)(uint64_t);

static initFunction initProfileLibrary         = NULL;
static finalizeFunction finalizeProfileLibrary = NULL;
static beginFunction beginForCallee            = NULL;
static beginFunction beginScanCallee           = NULL;
static beginFunction beginReduceCallee         = NULL;
static endFunction endForCallee                = NULL;
static endFunction endScanCallee               = NULL;
static endFunction endReduceCallee             = NULL;
void get_global_fence_choice() {
  // re-read environment variable to get most accurate value
  const char* tool_globFence_str = getenv("KOKKOS_TOOLS_GLOBALFENCES");
  if (NULL != tool_globFence_str) {
    tool_globFence = atoi(tool_globFence_str);
  }
}

// set of functions from Kokkos ToolProgrammingInterface (includes fence)
Kokkos::Tools::Experimental::ToolProgrammingInterface tpi_funcs;

uint32_t getDeviceID(uint32_t devid_in) {
  int num_device_bits   = 7;
  int num_instance_bits = 17;

  return (~((uint32_t(-1)) << num_device_bits)) &
         (devid_in >> num_instance_bits);
}

void invoke_ktools_fence(uint32_t devID) {
  if (tpi_funcs.fence != nullptr) {
    tpi_funcs.fence(devID);
    if (tool_verbosity > 1) {
      printf("KokkosP: Sampler utility sucessfully invoked " 
        " tool-induced fence on device %d\n", getDeviceID(devID));
    }
  } else
    printf(
        "KokkosP: FATAL: Kokkos Tools Programming Interface's tool-invoked "
        "Fence is NULL!\n");
}

void kokkosp_provide_tool_programming_interface(
    uint32_t num_funcs, Kokkos_Tools_ToolProgrammingInterface* funcsFromTPI) {
  if (!num_funcs) {
    if (tool_verbosity > 0)
      printf(
          "KokkosP: Note: Number of functions in Tools Programming Interface "
          "is 0!\n");
  }
  tpi_funcs = *funcsFromTPI;
}

void kokkosp_request_tool_settings(const uint32_t ,
                                   Kokkos_Tools_ToolSettings* settings) {
  if (0 < tool_globFence) {
    settings->requires_global_fencing = true;
  } else {
    settings->requires_global_fencing = false;
  }
}

void kokkosp_init_library(const int loadSeq, const uint64_t interfaceVer,
                          const uint32_t devInfoCount, void* deviceInfo) {
  const char* tool_verbose_str   = getenv("KOKKOS_TOOLS_SAMPLER_VERBOSE");
  const char* tool_globFence_str = getenv("KOKKOS_TOOLS_GLOBALFENCES");
  if (NULL != tool_verbose_str) {
    tool_verbosity = atoi(tool_verbose_str);
  } else {
    tool_verbosity = 0;
  }
  if (NULL != tool_globFence_str) {
    tool_globFence = (atoi(tool_globFence_str)) ;
  } else {
    tool_globFence = 0;
  }

  char* profileLibrary = getenv("KOKKOS_TOOLS_LIBS");
  if (NULL == profileLibrary) {
    printf(
        "KokkosP: Checking KOKKOS_PROFILE_LIBRARY. WARNING: This is a "
        "deprecated "
        "variable. Please use KOKKOS_TOOLS_LIBS\n");
    profileLibrary = getenv("KOKKOS_PROFILE_LIBRARY");
    if (NULL == profileLibrary) {
      printf("KokkosP: No library to call in %s\n", profileLibrary);
      exit(-1);
    }
  }

  char* envBuffer = (char*)malloc(sizeof(char) * (strlen(profileLibrary) + 1));
  strcpy(envBuffer, profileLibrary);

  char* nextLibrary = strtok(envBuffer, ";");

  for (int i = 0; i < loadSeq; i++) {
    nextLibrary = strtok(NULL, ";");
  }

  nextLibrary = strtok(NULL, ";");

  if (NULL == nextLibrary) {
    printf("KokkosP: No child library to call in %s\n", profileLibrary);
    exit(-1);
  } else {
    if (tool_verbosity > 0) {
      printf("KokkosP: Next library to call: %s\n", nextLibrary);
      printf("KokkosP: Loading child library ..\n");
    }
    void* childLibrary = dlopen(nextLibrary, RTLD_NOW | RTLD_GLOBAL);
    if (NULL == childLibrary) {
      fprintf(stderr, "KokkosP: Error: Unable to load: %s (Error=%s)\n",
              nextLibrary, dlerror());
      exit(-1);
    } else {
      beginForCallee =
          (beginFunction)dlsym(childLibrary, "kokkosp_begin_parallel_for");
      beginScanCallee =
          (beginFunction)dlsym(childLibrary, "kokkosp_begin_parallel_scan");
      beginReduceCallee =
          (beginFunction)dlsym(childLibrary, "kokkosp_begin_parallel_reduce");
      endForCallee =
          (endFunction)dlsym(childLibrary, "kokkosp_end_parallel_for");
      endScanCallee =
          (endFunction)dlsym(childLibrary, "kokkosp_end_parallel_scan");
      endReduceCallee =
          (endFunction)dlsym(childLibrary, "kokkosp_end_parallel_reduce");
      initProfileLibrary =
          (initFunction)dlsym(childLibrary, "kokkosp_init_library");
      finalizeProfileLibrary =
          (finalizeFunction)dlsym(childLibrary, "kokkosp_finalize_library");
      if (NULL != initProfileLibrary) {
        (*initProfileLibrary)(loadSeq + 1, interfaceVer, devInfoCount,
                              deviceInfo);
      }
      if (tool_verbosity > 0) {
        printf("KokkosP: Function Status:\n");
        printf("KokkosP: begin-parallel-for:      %s\n",
               (beginForCallee == NULL) ? "no" : "yes");
        printf("KokkosP: begin-parallel-scan:     %s\n",
               (beginScanCallee == NULL) ? "no" : "yes");
        printf("KokkosP: begin-parallel-reduce:   %s\n",
               (beginReduceCallee == NULL) ? "no" : "yes");
        printf("KokkosP: end-parallel-for:        %s\n",
               (endForCallee == NULL) ? "no" : "yes");
        printf("KokkosP: end-parallel-scan:       %s\n",
               (endScanCallee == NULL) ? "no" : "yes");
        printf("KokkosP: end-parallel-reduce:       %s\n",
               (endReduceCallee == NULL) ? "no" : "yes");
      }
    }
  }

  free(envBuffer);
  uniqID                  = 1;
  const char* tool_sample = getenv("KOKKOS_TOOLS_SAMPLER_SKIP");
  if (NULL != tool_sample) {
    kernelSampleSkip = atoi(tool_sample) + 1;
  }

  if (tool_verbosity > 0) {
    printf("KokkosP: Sampling rate set to: %s\n", tool_sample);
  }
}

void kokkosp_finalize_library() {
  if (NULL != finalizeProfileLibrary) (*finalizeProfileLibrary)();
}

void kokkosp_begin_parallel_for(const char* name, const uint32_t devID,
                                uint64_t* kID) {
  *kID = uniqID++;
  static uint64_t invocationNum = 0;
  ++invocationNum;
  if ((invocationNum % kernelSampleSkip) == 0) {
    pair<uint64_t, uint32_t> infoOfSample;
    uint32_t devNum = getDeviceID(devID);
    get_global_fence_choice();  // re-read environment variable to get most
                                // accurate
    if (0 < tool_globFence) {
      invoke_ktools_fence(devID);  // invoke tool-induced fence from device
                                    // number device number is negative
      if (tool_verbosity > 0)
        printf(
            "KokkosP: kokkosp_begin_parallel_for(): device number obtained %lu from "
            "sampler. \n",
            (unsigned long)devNum);
    }
    infoOfSample.second = devID;
    if (tool_verbosity > 0) {
      printf("KokkosP: sample %llu calling child-begin function...\n",
             (unsigned long long)(*kID));
    }
    if (NULL != beginForCallee) {
      uint64_t nestedkID = 0;
      (*beginForCallee)(name, devID, &nestedkID);  // replace kID with nestedkID
      sampler_mtx.lock();
      infoOfSample.first = nestedkID;
      infokIDSample.insert({*kID, infoOfSample});
      sampler_mtx.unlock();
    } else {  // no child to call
      if (tool_verbosity > 1) {
        printf(
            "KokkosP: warning: sampler's begin_parallel_for callback has no "
            "child"
            " function to call.\n");
      }
    }
  }  // end sample gathering insert for parallel for
}

void kokkosp_end_parallel_for(const uint64_t kID) {
  pair<uint64_t, uint32_t> infoOfMatchedSample;
  sampler_mtx.lock();

    infoOfMatchedSample = infokIDSample.at(kID);
    uint32_t devNum;
    uint32_t devID;
    devID  = infoOfMatchedSample.second;
    devNum = getDeviceID(devID);
    get_global_fence_choice();  // re-read environment variable to get most
                                // accurate value
    if (tool_globFence) {
      invoke_ktools_fence(
          devID);  // invoke tool-induced fence from device number
                    // make sure device number is not negative
      if (tool_verbosity > 0) {
        printf(
            "KokkosP: kokkosp_end_parallel_for: device number of sample's kernel ID is %lu "
            " \n",
            (unsigned long)devNum);
      }
    }

    if (tool_verbosity > 0) {
      printf("KokkosP: sample %llu calling child-end function...\n",
             (unsigned long long)(kID));
    }

    if (NULL != endForCallee) {
      uint64_t nestedkID = infoOfMatchedSample.first;
      if (tool_verbosity > 0) {
        printf(
            "KokkosP: sampler's child's kID "
            "(nested "
            " kID) is %llu. \n",
            (unsigned long long)nestedkID);
        (*endForCallee)(nestedkID);
      }
      infokIDSample.erase(kID);
    } else {
      if (tool_verbosity > 1)
        printf("KokkosP: Warning: sampler's endForCallee not found.\n");
    }
  sampler_mtx.unlock();
}

void kokkosp_begin_parallel_scan(const char* name, const uint32_t devID,
                                 uint64_t* kID) {
  *kID = uniqID++;  // set memory location value of kID to uniqID
  static uint64_t invocationNum =0 ;
  ++invocationNum;
  uint32_t devNum;
  if ((invocationNum % kernelSampleSkip) == 0) {
    pair<uint64_t, uint32_t> infoOfSample;
    devNum = getDeviceID(devID);
    get_global_fence_choice();  // re-read environment variable to get most
                                // accurate value
    if (0 < tool_globFence) {
      // using tool-induced fence from Kokkos_profiling rather than
      // Kokkos_C_Profiling_interface. Note that this function
      // only invokes a fence on the device of the devID passed
      //
      invoke_ktools_fence(devID);
      if (tool_verbosity > 1) {
        printf(
            "KokkosP: sampler begin_parallel_scan callback"
            " invoked fence on device number %lu\n",
            (unsigned long)devNum);
      }
    }

    infoOfSample.second = devID;
    if (tool_verbosity > 0) {
      printf("KokkosP: sample %llu calling child-begin function...\n",
             (unsigned long long)(*kID));
    }
    if (NULL != beginScanCallee) {
      uint64_t nestedkID = 0;
      (*beginScanCallee)(name, devID, &nestedkID);
      sampler_mtx.lock();
      infoOfSample.first = nestedkID;
      infokIDSample.insert({*kID, infoOfSample});
      sampler_mtx.unlock();
    } else {
      if (tool_verbosity > 1)
        printf(
            "KokkosP: Warning: sampler has no beginScanCallee function "
            "available to call.\n");
    }
  }  // end sample gather scan
}  // end begin scan callback

void kokkosp_end_parallel_scan(const uint64_t kID) {
  pair<uint64_t, uint32_t> infoOfMatchedSample;
  sampler_mtx.lock();

    infoOfMatchedSample = infokIDSample.at(kID);
    uint32_t devNum;
    uint32_t devID = infoOfMatchedSample.second;
    devNum         = getDeviceID(devID);
    get_global_fence_choice();  // re-read environment variable to get most
                                // accurate value
    if (0 < tool_globFence) {
      // using tool-induced fence from Kokkos_profiling rather than
      // Kokkos_C_Profiling_interface. Note that this function
      // only invokes a fence on the device fenced on the begin parallel scan.
      invoke_ktools_fence(devID);
      if (tool_verbosity > 1) {
        printf(
            "KokkosP: sampler end_parallel_scan callback"
            " invoked tool-induced fence on device %lu\n",
            (unsigned long) devNum);
      }
    }  // end invoke fence conditional
    if (tool_verbosity > 0) {
      printf("KokkosP: sample %llu calling child-end function...\n",
             (unsigned long long)(kID));
    }
    if (NULL != endScanCallee) {
      uint64_t nestedkID = infoOfMatchedSample.first;
      (*endScanCallee)(nestedkID);
      infokIDSample.erase(kID);
    } else {
      if (tool_verbosity > 1)
        printf(
            "KokkosP: warning: sampler's end parallel scan has no "
            "endScanCallee to call\n");
    }
  sampler_mtx.unlock();
}  // end end_parallel_scan callback

void kokkosp_begin_parallel_reduce(const char* name, const uint32_t devID,
                                   uint64_t* kID) {
  *kID = uniqID++;
  static uint64_t invocationNum;
  ++invocationNum;
  pair<uint64_t, uint32_t> infoOfSample;
  uint32_t devNum;
  if ((invocationNum % kernelSampleSkip) == 0) {
    devNum = getDeviceID(devID);
    get_global_fence_choice();  // re-read environment variable to get most
                                // accurate value
    if (0 < tool_globFence) {
      // using tool-induced fence from Kokkos_profiling rather than
      // Kokkos_C_Profiling_interface. Note that this function
      // only invokes a fence on devNum of devID specified
      invoke_ktools_fence(devID);
      if (tool_verbosity > 1) {
        printf(
            "KokkosP: sampler begin_parallel_reduce obtained "
            "device number %lu \n",
            (unsigned long)devNum);
      }
    }
    infoOfSample.second = devID;
    if (tool_verbosity > 0) {
      printf("KokkosP: sample %llu calling child-begin function...\n",
             (unsigned long long)(*kID));
    }
    if (NULL != beginReduceCallee) {
      uint64_t nestedkID = 0;
      (*beginReduceCallee)(name, devID, &nestedkID);
      infoOfSample.first = nestedkID;
      sampler_mtx.lock();
      infokIDSample.insert({*kID, infoOfSample});
      sampler_mtx.unlock();
    } else {
      if (tool_verbosity > 1) {
        printf(
            "KokkosP: Note: begin_parallel_reduce sampler "
            " callback has no child-begin function to call.\n");
      }
    }
  }  // end sample gather
}  // end begin_parallel_reduce callback

void kokkosp_end_parallel_reduce(const uint64_t kID) {
  pair<uint64_t, uint32_t> infoOfMatchedSample;
  uint32_t devNum;
  uint32_t devID;
  sampler_mtx.lock();
    infoOfMatchedSample = infokIDSample.at(kID);
    devID               = infoOfMatchedSample.second;
    devNum              = getDeviceID(devID);
    get_global_fence_choice();  // re-read environment variable to get most
                                // accurate value
    if (0 < tool_globFence) {
      invoke_ktools_fence(devID);
      if (tool_verbosity > 1) {
        printf("KokkosP: sampler's end parallel reduce obtained device number %lu \n",
               (unsigned long)devNum);
      }
    }  // end tool invoked fence
    if (tool_verbosity > 0) {
      printf("KokkosP: sample %llu calling child-end function...\n",
             (unsigned long long)(kID));
    }
    if (NULL != endReduceCallee) {
      uint64_t nestedkID = infoOfMatchedSample.second;
      if (tool_verbosity > 0) {
        printf("KokkosP: sampler's endReduceCallee kID (nested kID) is %llu \n",
               (unsigned long long)nestedkID);
      }
      (*endReduceCallee)(nestedkID);
      infokIDSample.erase(kID);
    } else {
      if (tool_verbosity > 1) {
        printf(
            "KokkosP: Note: end_parallel_reduce sampler "
            "callback has no child-end function to call.\n");
      }
    }
  }  // end kID sample
  sampler_mtx.unlock();
}  // end end_parallel_reduce sampler callback

}  // namespace Sampler
}  // end namespace KokkosTools

extern "C" {

namespace impl = KokkosTools::Sampler;
EXPOSE_TOOL_PROGRAMMING_INTERFACE(
    impl::kokkosp_provide_tool_programming_interface)
EXPOSE_TOOL_SETTINGS(impl::kokkosp_request_tool_settings)

EXPOSE_INIT(impl::kokkosp_init_library)
EXPOSE_FINALIZE(impl::kokkosp_finalize_library)
EXPOSE_BEGIN_PARALLEL_FOR(impl::kokkosp_begin_parallel_for)
EXPOSE_END_PARALLEL_FOR(impl::kokkosp_end_parallel_for)
EXPOSE_BEGIN_PARALLEL_SCAN(impl::kokkosp_begin_parallel_scan)
EXPOSE_END_PARALLEL_SCAN(impl::kokkosp_end_parallel_scan)
EXPOSE_BEGIN_PARALLEL_REDUCE(impl::kokkosp_begin_parallel_reduce)
EXPOSE_END_PARALLEL_REDUCE(impl::kokkosp_end_parallel_reduce)

}  // end extern "C"
