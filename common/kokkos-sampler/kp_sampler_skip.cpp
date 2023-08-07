#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <dlfcn.h>
#include "../../profiling/all/kp_core.hpp"
#include "kp_config.hpp"
#include <atomic>
namespace KokkosTools {
namespace Sampler {
static std:atomic<uint64_t> uniqID   = 0;
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
static beginFunction beginFenceCallee          = NULL;
static beginFunction beginDeepCopyCallee       = NULL;
static endFunction endForCallee                = NULL;
static endFunction endScanCallee               = NULL;
static endFunction endReduceCallee             = NULL;
static endFunction endFenceCallee              = NULL;
static endFunction endDeepCopyCallee           = NULL;
void get_global_fence_choice() {
  // re-read environment variable to get most accurate value
  const char* tool_globFence_str = getenv("KOKKOS_TOOLS_GLOBALFENCES");
  if (NULL != tool_globFence_str) {
    tool_globFence = atoi(tool_globFence_str);
  }
}

// set of functions from Kokkos ToolProgrammingInterface (includes fence)
Kokkos::Tools::Experimental::ToolProgrammingInterface tpi_funcs;

void invoke_ktools_fence(uint32_t devID) {
  if (tpi_funcs.fence != nullptr) {
    tpi_funcs.fence(devID);
  } else
    printf(
        "KokkosP: FATAL: Kokkos Tools Programming Interface's tool-invoked "
        "Fence is NULL!\n");
}

uint32_t getDeviceID(uint32_t devid_in) {
  int num_device_bits   = 7;
  int num_instance_bits = 17;

  return (~((uint32_t(-1)) << num_device_bits)) &
         (devid_in >> num_instance_bits);
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

void kokkosp_request_tool_settings(const uint32_t,
                                   Kokkos_Tools_ToolSettings* settings) {
  if (0 == tool_globFence) {
    settings->requires_global_fencing = false;
  } else {
    settings->requires_global_fencing = true;
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
    tool_globFence = atoi(tool_globFence_str);
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
      beginFenceCallee =
          (beginFunction)dlsym(childLibrary, "kokkosp_begin_fence");
      beginDeepCopyCallee =
          (beginFunction)dlsym(childLibrary, "kokkosp_begin_deep_copy");
      endScanCallee =
          (endFunction)dlsym(childLibrary, "kokkosp_end_parallel_scan");
      endForCallee =
          (endFunction)dlsym(childLibrary, "kokkosp_end_parallel_for");
      endReduceCallee =
          (endFunction)dlsym(childLibrary, "kokkosp_end_parallel_reduce");
      endFenceCallee = (endFunction)dlsym(childLibrary, "kokkosp_end_fence");
      endDeepCopyCallee =
          (endFunction)dlsym(childLibrary, "kokkosp_end_deep_copy");

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
          printf("KokkosP: begin-deep-copy:      %s\n",
               (beginDeepCopyCallee == NULL) ? "no" : "yes");
        printf("KokkosP: begin-fence:     %s\n",
               (beginFenceCallee == NULL) ? "no" : "yes");
        printf("KokkosP: end-parallel-for:        %s\n",
               (endForCallee == NULL) ? "no" : "yes");
        printf("KokkosP: end-parallel-scan:       %s\n",
               (endScanCallee == NULL) ? "no" : "yes");
        printf("KokkosP: end-parallel-reduce:     %s\n",
               (endReduceCallee == NULL) ? "no" : "yes");
        printf("KokkosP: end-deep-copy:       %s\n",
               (endDeepCopyCallee == NULL) ? "no" : "yes");
        printf("KokkosP: end-fence:     %s\n",
               (endFenceCallee == NULL) ? "no" : "yes");
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
  static uint64_t invocationNum;
  ++invocationNum;
  if ((invocationNum % kernelSampleSkip) == 0) {
    get_global_fence_choice();  // re-read environment variable to get most
                                // accurate value
    if (0 < tool_globFence) {
      invoke_ktools_fence(0);  // invoke tool-induced fence from device number 0 // TODO: use getDeviceID
    }
    if (tool_verbosity > 0) {
      printf("KokkosP: sample %llu calling child-begin function...\n",
             (unsigned long long)(*kID));
    }

    if (NULL != beginForCallee) {
      uint64_t* nestedkID = 0;
      (*beginForCallee)(name, devID, kID); // TODO: replace kID with nestedkID
      // map.insert(kID, nestedkID);
    }
  }
}

void kokkosp_end_parallel_for(const uint64_t kID) {
  if (kID == uniqID) { // check whether the corresponding begin parallel for was called // TODO: fix to hashmap
    get_global_fence_choice();  // re-read environment variable to get most
                                // accurate value
    if (0 < tool_globFence) {
      invoke_ktools_fence(0);  // invoke tool-induced fence from device number
    }
    if (tool_verbosity > 0) {
      printf("KokkosP: sample %llu calling child-end function...\n",
             (unsigned long long)(kID));
    }
    if (NULL != endForCallee) {
      (*endForCallee)(kID);
      // (*endForCallee)(find(kID));
    }
  } // end uniqID sample match conditional
}

void kokkosp_begin_parallel_scan(const char* name, const uint32_t devID,
                                 uint64_t* kID) {
  *kID = uniqID++;  // set memory location value of kID to uniqID 
  static uint64_t invocationNum;
  ++invocationNum;
  if ((invocationNum % kernelSampleSkip) == 0) {
    get_global_fence_choice();  // re-read environment variable to get most
                                // accurate value
    if (0 < tool_globFence) {
      // using tool-induced fence from Kokkos_profiling rather than
      // Kokkos_C_Profiling_interface. Note that this function
      // only invokes a fence on the device 0
      invoke_ktools_fence(0);
    }
    if (tool_verbosity > 0) {
      printf("KokkosP: sample %llu calling child-begin function...\n",
             (unsigned long long)(*kID));
    }
    if (NULL != beginScanCallee) {
      (*beginScanCallee)(name, devID, kID);
    }
  }
}

void kokkosp_end_parallel_scan(const uint64_t kID) {
  if (kID == uniqID) { // check that we match the begin scan kernel call
    get_global_fence_choice();  // re-read environment variable to get most
                                // accurate value
    if (0 < tool_globFence) {
      // using tool-induced fence from Kokkos_profiling rather than
      // Kokkos_C_Profiling_interface. Note that this function
      // only invokes a global (device 0 invoked) fence.
      invoke_ktools_fence(0);
    }
    if (tool_verbosity > 0) {
      printf("KokkosP: sample %llu calling child-end function...\n",
             (unsigned long long)(kID));
    }
    if (NULL != endScanCallee) {
      (*endScanCallee)(kID);
    }
  }  // end kID sample
}

void kokkosp_begin_parallel_reduce(const char* name, const uint32_t devID,
                                   uint64_t* kID) {
  *kID = uniqID++;
  static uint64_t invocationNum;
  ++invocationNum;
  if ((invocationNum % kernelSampleSkip) == 0) {
    get_global_fence_choice();  // re-read environment variable to get most
                                // accurate value
    if (0 < tool_globFence) {
      // using tool-induced fence from Kokkos_profiling rather than
      // Kokkos_C_Profiling_interface. Note that this function
      // only invokes a global (device 0 invoked) fence.
      invoke_ktools_fence(0);
    }
    if (tool_verbosity > 0) {
      printf("KokkosP: sample %llu calling child-begin function...\n",
             (unsigned long long)(*kID));
    }
    if (NULL != beginReduceCallee) {
      (*beginReduceCallee)(name, devID, kID);
    }
  }
}

void kokkosp_end_parallel_reduce(const uint64_t kID) {
  if (kID > 0) {
    get_global_fence_choice();  // re-read environment variable to get most
                                // accurate value
    if (0 < tool_globFence) {
      invoke_ktools_fence(0);
    }
    if (tool_verbosity > 0) {
      printf("KokkosP: sample %llu calling child-end function...\n",
             (unsigned long long)(kID));
    }
    if (NULL != endReduceCallee) {
      (*endReduceCallee)(kID);
    }
  }  // end kID sample
}

void kokkosp_begin_fence(const char* name, const uint32_t devID,
                         uint64_t* kID) {
  *kID = uniqID++;
  static uint64_t invocationNum;
  ++invocationNum;
  if ((invocationNum % kernelSampleSkip) == 0) {
    get_global_fence_choice();  // re-read environment variable to get most
                                // accurate value
    if (0 < tool_globFence) {
      invoke_ktools_fence(
          0);  // invoke tool-induced fence from device 0 for now
    }
    if (tool_verbosity > 0) {
      printf("KokkosP: sample %llu calling child-begin function...\n",
             (unsigned long long)(*kID));
    }

    if (NULL != beginFenceCallee) {
      (*beginFenceCallee)(name, devID, kID);
    }
  }
}

void kokkosp_end_fence(const uint64_t kID) {
  if (kID == uniqID) {
    get_global_fence_choice();  // re-read environment variable to get most
                                // accurate value
    if (0 < tool_globFence) {
      invoke_ktools_fence(
          0);  // invoke tool-induced fence from device 0 for now
    }
    if (tool_verbosity > 0) {
      printf("KokkosP: sample %llu calling child-end function...\n",
             (unsigned long long)(kID));
    }
    if (NULL != endFenceCallee) {
      (*endFenceCallee)(kID);
    }
  }
}

void kokkosp_begin_deep_copy(const char* name, const uint32_t devID,
                             uint64_t* kID) {
  *kID = uniqID++; 
  static uint64_t invocationNum;
  ++invocationNum;
  if ((invocationNum % kernelSampleSkip) == 0) { 
    get_global_fence_choice();  // re-read environment variable to get most
                                // accurate value
    if (0 < tool_globFence) {
      invoke_ktools_fence(
          0);  // invoke tool-induced fence from device 0 for now
    } 
    if (tool_verbosity > 0) {
      printf("KokkosP: sample %llu calling child-begin function...\n",
             (unsigned long long)(*kID));
    }

    if (NULL != beginDeepCopyCallee) {
      (*beginDeepCopyCallee)(name, devID, kID);
    }
  }
}

void kokkosp_end_deep_copy(const uint64_t kID) {
  if (kID == uniqID) { // check that we match the begin deep copy kernel call
    get_global_fence_choice();  // re-read environment variable to get most
                                // accurate value
    if (0 < tool_globFence) {
      invoke_ktools_fence(
          0);  // invoke tool-induced fence from device 0 for now
    }
    if (tool_verbosity > 0) {
      printf("KokkosP: sample %llu calling child-end function...\n",
             (unsigned long long)(kID));
    }
    if (NULL != endDeepCopyCallee) {
      (*endDeepCopyCallee)(kID);
    }
  }
}

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
EXPOSE_BEGIN_DEEP_COPY(impl::kokkosp_begin_deep_copy)
EXPOSE_END_DEEP_COPY(impl::kokkosp_end_deep_copy)
EXPOSE_BEGIN_FENCE(impl::kokkosp_begin_fence)
EXPOSE_END_FENCE(impl::kokkosp_end_fence)

}  // end extern "C"
