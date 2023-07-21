#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <dlfcn.h>
#include "../../profiling/all/kp_core.hpp"
#include "kp_config.hpp"
#include <ctime>  // for random number generation

namespace KokkosTools {
namespace Sampler {
static uint64_t uniqID = 0;
static uint64_t kernelSampleSkip =
    101;  // Default skip rate of every 100 invocations
static float tool_prob_num =
    1.0;  // Default probability of 1 percent of all invocations
static int tool_verbosity = 0;
static int tool_globFence = 0;

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
        "variable. Please use KOKKOS_TOOLS_LIBS. \n");
    profileLibrary = getenv("KOKKOS_PROFILE_LIBRARY");
    if (NULL == profileLibrary) {
      printf("KokkosP: No library to call in %s.\n", profileLibrary);
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
      endScanCallee =
          (endFunction)dlsym(childLibrary, "kokkosp_end_parallel_scan");
      endForCallee =
          (endFunction)dlsym(childLibrary, "kokkosp_end_parallel_for");
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
        printf("KokkosP: end-parallel-reduce:     %s\n",
               (endReduceCallee == NULL) ? "no" : "yes");
      }
    }
  }

  free(envBuffer);

  uniqID = 1;

  const char* tool_sample      = getenv("KOKKOS_TOOLS_SAMPLER_SKIP");
  const char* tool_probability = getenv("KOKKOS_TOOLS_SAMPLER_PROBABILITY");
  // composing the periodicity and probability parameter. Set to 1 if
  // probability for each periodic sample. Set to 0 to default to probability
  // even if periodicity (skip rate) is defined.

  if (NULL != tool_sample) {
    kernelSampleSkip = atoi(tool_sample) + 1;
  }

  if (NULL != tool_probability) {
    //  read sampling probability as an float between 0 and 100, representing
    //  a percentage that data should be gathered.
    //  Connector reasons about probability as a double between 0.0 and 1.0.
    tool_prob_num = atof(tool_probability);
    if (tool_prob_num > 100.0) {
      printf(
          "KokkosP: The sampling probability value is set to be greater than "
          "100.0. "
          "Setting sampling probability to 100 percent; all of the "
          "invocations of a Kokkos Kernel will be profiled.\n");
      tool_prob_num = 100.0;
    } else if (tool_prob_num < 0.0) {
      printf(
          "KokkosP: The sampling probability value is set to be negative "
          "number. Setting "
          "sampling probability to 0 percent; none of the invocations of "
          "a Kokkos Kernel will be profiled.\n");
      tool_prob_num = 0.0;
    }
  }
  // srand48((unsigned)clock());
  // seed48(0);

  if (tool_verbosity > 0) {
    printf("KokkosP: Sampling rate set to: %s\n", tool_sample);
    printf("KokkosP: Sampling probability set to: %s\n", tool_probability);
    printf(
        "KokkosP: seeding Random Number Generator using clock for "
        "probabilistic sampling\n");
  }
  srand(time(NULL));

  if ((NULL != tool_probability) && (NULL != tool_sample)) {
    printf(
        "KokkosP: Note that both probability and skip rate are set. The Kokkos "
        "Tools Sampler utility will invoke a Kokkos Tool child event you "
        "specified "
        "(e.g., the profiler or debugger tool connector you specified "
        "in KOKKOS_TOOLS_LIBS) with the specified sampling probability applied "
        "to the "
        "specified sampling skip rate set.\n");
  }
}  // end kokkosp_init_library

void kokkosp_finalize_library() {
  if (NULL != finalizeProfileLibrary) (*finalizeProfileLibrary)();
}

void kokkosp_begin_parallel_for(const char* name, const uint32_t devID,
                                uint64_t* kID) {
  *kID = 0;
  static uint64_t invocationNum;
  ++invocationNum;
  if ((invocationNum % kernelSampleSkip) == 0) {
    if ((rand() / RAND_MAX) < (tool_prob_num / 100.0)) {
      *kID = 1;  // set kernel ID to 1 so that it is matched with the end.
      if (tool_verbosity > 0) {
        printf(
            "KokkosP: sample %llu on (a parallel_for on its invocation number "
            "%d) calling child-begin function...\n",
            (unsigned long long)(*kID), (int)invocationNum);
      }
      if (NULL != beginForCallee) {
        (*beginForCallee)(name, devID, kID);
      }
    }
  }
}  // kokkosp_begin_parallel_for

void kokkosp_end_parallel_for(const uint64_t kID) {
  if (kID > 0) {
    if (tool_verbosity > 0) {
      printf(
          "KokkosP: sample %llu (a parallel_for) calling child-end "
          "function...\n",
          (unsigned long long)(kID));
    }

    if (NULL != endForCallee) {
      (*endForCallee)(kID);
    }
  }
}  // kokkosp_end_parallel_for

void kokkosp_begin_parallel_scan(const char* name, const uint32_t devID,
                                 uint64_t* kID) {
  *kID = 0;
  static uint64_t invocationNum;
  ++invocationNum;
  if ((invocationNum % kernelSampleSkip) == 0) {
    if ((rand() / RAND_MAX) < (tool_prob_num / 100.0)) {
      *kID = 1;  // set kernel ID to 1 so that it is matched with the end.
      if (tool_verbosity > 0) {
        printf(
            "KokkosP: sample %llu (parallel_scan on its invocation num %d) "
            "calling child-begin function...\n",
            (unsigned long long)(*kID), (int)invocationNum);
      }
      if (NULL != beginScanCallee) {
        (*beginScanCallee)(name, devID, kID);
      }
    }
  }
}  // kokkosp_begin_parallel_scan

void kokkosp_end_parallel_scan(const uint64_t kID) {
  if (kID > 0) {
    if (tool_verbosity > 0) {
      printf(
          "KokkosP: sample %llu (a parallel_scan) calling child-end "
          "function...\n",
          (unsigned long long)(kID));
    }
    if (NULL != endScanCallee) {
      (*endScanCallee)(kID);
    }
  }
}  // kokkosp_end_parallel_scan

void kokkosp_begin_parallel_reduce(const char* name, const uint32_t devID,
                                   uint64_t* kID) {
  *kID = 0;
  static uint64_t invocationNum;
  ++invocationNum;
  if ((invocationNum % kernelSampleSkip) == 0) {
    if ((rand() / RAND_MAX) < tool_prob_num / 100.0) {
      if (tool_verbosity > 0) {
        printf(
            "KokkosP: sample %llu (a parallel_reduce on its invocation number "
            "%d) calling child-begin function...\n",
            (unsigned long long)(*kID), (int)invocationNum);
      }
      *kID = 1;
      if (NULL != beginReduceCallee) {
        (*beginReduceCallee)(name, devID, kID);
      }
    }
  }
}  // kokkosp_begin_parallel_reduce

void kokkosp_end_parallel_reduce(const uint64_t kID) {
  if (kID > 0) {
    if (tool_verbosity > 0) {
      printf(
          "KokkosP: sample %llu (a parallel_reduce) calling child-end "
          "function...\n",
          (unsigned long long)(kID));
    }
    if (NULL != endReduceCallee) {
      (*endReduceCallee)(kID);
    }
  }
}  // kokkosp_end_parallel_reduce

}  // namespace Sampler
}  // namespace KokkosTools

extern "C" {

namespace impl = KokkosTools::Sampler;

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
