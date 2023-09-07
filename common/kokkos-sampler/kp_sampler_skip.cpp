#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <dlfcn.h>
#include "../../profiling/all/kp_core.hpp"
#include "kp_config.hpp"
#include <ctime>  // for random number generation
#include <limits>

namespace KokkosTools {
namespace Sampler {
static uint64_t uniqID          = 0;
static int64_t kernelSampleSkip = 0;
static float tool_prob_num =
    -1.0;  // Default probability of undefined percent of all invocations
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
    settings->requires_global_fencing = 0;
  } else {
    settings->requires_global_fencing = 1;
  }
}

void kokkosp_init_library(const int loadSeq, const uint64_t interfaceVer,
                          const uint32_t devInfoCount, void* deviceInfo) {
  const char* tool_verbose_str   = getenv("KOKKOS_TOOLS_SAMPLER_VERBOSE");
  const char* tool_globFence_str = getenv("KOKKOS_TOOLS_GLOBALFENCES");
  kernelSampleSkip               = 0;  // use min for undefined skip rate

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
  if ((tool_prob_num < 0.0) && (kernelSampleSkip == 0)) {
    if (tool_verbosity > 0) {
      printf(
          "KokkosP: Neither sampling utility's probability for sampling "
          "nor sampling utility's skip rate were set. \n");
    }
    tool_prob_num = 10.0;
    if (tool_verbosity > 0) {
      printf(
          "KokkosP: Set the sampling utility's probability "
          "for sampling to be %f percent. Sampler's skip rate "
          "will not be used.\n",
          tool_prob_num);
    }
  }

  if (tool_verbosity > 0) {
    if (tool_verbosity > 1) {
      printf("KokkosP: Sampling rate provided as input: %s\n", tool_sample);
      printf("KokkosP: Sampling probability provided as input: %s\n",
             tool_probability);
    }
    printf("KokkosP: Sampling rate set to: %llu\n",
           (unsigned long long)(kernelSampleSkip));
    printf("KokkosP: Sampling probability set to %f\n", tool_prob_num);
    printf(
        "KokkosP: seeding Random Number Generator using clock for "
        "probabilistic sampling.\n");
  }
  srand(time(NULL));

  if ((NULL != tool_probability) && (NULL != tool_sample)) {
    printf(
        "KokkosP: Note that both probability and skip rate are set. The Kokkos "
        "Tools Sampler utility will invoke a Kokkos Tool child event you "
        "specified "
        "(e.g., the profiler or debugger tool connector you specified "
        "in KOKKOS_TOOLS_LIBS) with only specified sampling probability "
        "applied "
        "and sampling skip rate set is ignored with no "
        "predefined periodicity for sampling used.\n");
  }
  if (tool_verbosity > 0) {
    printf(
        "KokkosP: The skip rate in the sampler utility "
        "is being set to 1.\n");
  }
  kernelSampleSkip = 1;
}  // end kokkosp_init_library

void kokkosp_finalize_library() {
  if (NULL != finalizeProfileLibrary) (*finalizeProfileLibrary)();
}

void kokkosp_begin_parallel_for(const char* name, const uint32_t devID,
                                uint64_t* kID) {
  *kID = uniqID++;
  static uint64_t invocationNum;
  ++invocationNum;
  if ((invocationNum % kernelSampleSkip) == 0) {
    if ((rand() / (1.0 * RAND_MAX)) < (tool_prob_num / 100.0)) {
      if (tool_verbosity > 0) {
        printf(
            "KokkosP: sample %llu of parallel_for on its invocation number %d "
            "calling"
            "child-begin function...\n",
            (unsigned long long)(*kID), (int)invocationNum);
      }
      uint64_t* nestedkID;
      *nestedkID = 0;
      if (NULL != beginForCallee) {
        (*beginForCallee)(name, devID, nestedkID);
      }
      if (tool_verbosity > 1)
        printf(
            "KokkosP: sample for a parallel_for with kernel ID %llu on its "
            "invocation number "
            "%d called child-begin function...\n",
            (unsigned long long)(*kID), (int)invocationNum);
    }
  }  // end sampling

}  // kokkosp_begin_parallel_for

void kokkosp_end_parallel_for(const uint64_t kID) {
  // match the corresponding kokkosp_begin_parallel_for gathered a sample
  if (kID == uniqID) {
    if (tool_verbosity > 0) {
      printf(
          "KokkosP: sample for a parallel_for with kernel ID %llu calling "
          "child-end "
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
  *kID = uniqID++;  // set kernel ID to a uniqID to match it
  static uint64_t invocationNum;
  ++invocationNum;
  if ((invocationNum % kernelSampleSkip) == 0) {
    if (rand() / (1.0 * RAND_MAX) < (tool_prob_num / 100.0)) {
      if (tool_verbosity > 0) {
        printf(
            "KokkosP: sample %llu for parallel_scan on its invocation number "
            "%d calling "
            "child-begin function...\n",
            (unsigned long long)(*kID), (int)invocationNum);
      }
      uint64_t* nestedkID;
      *nestedkID = 0;
      if (NULL != beginScanCallee) {
        (*beginScanCallee)(name, devID, nestedkID);
      }
      if (tool_verbosity > 0) {
        printf(
            "KokkosP: sample for parallel_scan with kernelID %llu on its "
            "invocation number %d "
            "called child-begin function...\n",
            (unsigned long long)(*kID), (int)invocationNum);
      }
    }
  }  // end sampling
}  // kokkosp_begin_parallel_scan

void kokkosp_end_parallel_scan(const uint64_t kID) {
  // match corresponding scan with kernel ID kID
  if (kID == uniqID) {
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
  static uint64_t invocationNum;
  ++invocationNum;
  *kID = uniqID++;
  if ((invocationNum % kernelSampleSkip) == 0) {
    if ((rand() / (1.0 * RAND_MAX)) < tool_prob_num / 100.0) {
      if (tool_verbosity > 0) {
        printf(
            "KokkosP: sample %llu for a parallel_reduce on its invocation "
            "number "
            "%d calling child-begin function...\n",
            (unsigned long long)(*kID), (int)invocationNum);
      }

      uint64_t* nestedkID;  // set nested kernel ID
      *nestedkID = 0;
      if (NULL != beginReduceCallee) {
        (*beginReduceCallee)(name, devID, nestedkID);
      }

      if (tool_verbosity > 1) {
        printf(
            "KokkosP: sample for parallel_reduce with kID %llu on its "
            "invocation number "
            "%d called child-begin function...\n",
            (unsigned long long)(*kID), (int)invocationNum);
      }
    }
  }  // end sampling
}  // kokkosp_begin_parallel_reduce

void kokkosp_end_parallel_reduce(const uint64_t kID) {
  if (kID == uniqID) {
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
