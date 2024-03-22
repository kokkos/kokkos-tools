#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <unordered_map>
#include <dlfcn.h>
#include "../../profiling/all/kp_core.hpp"
#include "kp_config.hpp"
#include <ctime>
#include <limits>

namespace KokkosTools {
namespace Sampler {
static uint64_t uniqID           = 0;
static uint64_t kernelSampleSkip = std::numeric_limits<uint64_t>::max();
static double tool_prob_num      = -1.0;
static int tool_verbosity        = 0;
static int tool_globFence        = 0;
static int tool_seed             = -1;

// a hash table mapping kID to nestedkID
static std::unordered_map<uint64_t, uint64_t> infokIDSample;

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
  settings->requires_global_fencing = 0;
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
    if (tool_verbosity > 1) {
      printf(
          "KokkosP: Sampler attempting to invoke"
          " tool-induced fence on device %d.\n",
          getDeviceID(devID));
    }
    (*(tpi_funcs.fence))(devID);
    if (tool_verbosity > 1) {
      printf(
          "KokkosP: Sampler sucessfully invoked"
          " tool-induced fence on device %d\n",
          getDeviceID(devID));
    }
  } else {
    printf(
        "KokkosP: FATAL: Kokkos Tools Programming Interface's tool-invoked "
        "Fence is NULL!\n");
  }
}

void kokkosp_provide_tool_programming_interface(
    uint32_t num_funcs, Kokkos_Tools_ToolProgrammingInterface funcsFromTPI) {
  if (!num_funcs) {
    if (tool_verbosity > 0)
      printf(
          "KokkosP: Note: Number of functions in Tools Programming Interface "
          "is 0!\n");
  }
  tpi_funcs = funcsFromTPI;
}

void kokkosp_init_library(const int loadSeq, const uint64_t interfaceVer,
                          const uint32_t devInfoCount, void* deviceInfo) {
  const char* tool_verbose_str   = getenv("KOKKOS_TOOLS_SAMPLER_VERBOSE");
  const char* tool_globFence_str = getenv("KOKKOS_TOOLS_GLOBALFENCES");
  const char* tool_seed_str      = getenv("KOKKOS_TOOLS_RANDOM_SEED");
  printf("tool_verbose_str: %s\n", tool_verbose_str);
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
  if (NULL != tool_seed_str) {
    tool_seed = atoi(tool_seed_str);
  }

  char* profileLibrary = getenv("KOKKOS_TOOLS_LIBS");
  if (NULL == profileLibrary) {
    printf(
        "Checking KOKKOS_PROFILE_LIBRARY. WARNING: This is a deprecated "
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

  const char* tool_sample = getenv("KOKKOS_TOOLS_SAMPLER_SKIP");
  if (NULL != tool_sample) {
    kernelSampleSkip = atoi(tool_sample) + 1;
  }

  if (tool_verbosity > 0) {
    printf("KokkosP: Sampling rate set to: %s\n", tool_sample);
  }
  const char* tool_probability = getenv("KOKKOS_TOOLS_SAMPLER_PROB");

  if (NULL != tool_probability) {
    //  read sampling probability as a float between 0 and 100, representing
    //  a percentage that data should be gathered.
    //  Connector reasons about probability as a double between 0.0 and 1.0.
    tool_prob_num = atof(tool_probability);
    if (tool_prob_num > 100.0) {
      printf(
          "KokkosP: The sampling probability value is set to be greater than "
          "100.0. "
          "The probability for the sampler will be set to 100 percent; all of "
          "the "
          "invocations of a Kokkos kernel will be profiled.\n");
      tool_prob_num = 100.0;
    } else if (tool_prob_num < 0.0) {
      printf(
          "KokkosP: The sampling probability value is set to be a negative "
          "number. The "
          "sampler's probability will be set to 0 percent; none of the "
          "invocations of "
          "a Kokkos kernel will be profiled.\n");
      tool_prob_num = 0.0;
    }
  }
  if ((tool_prob_num < 0.0) &&
      (kernelSampleSkip == std::numeric_limits<uint64_t>::max())) {
    if (tool_verbosity > 0) {
      printf(
          "KokkosP: Neither the probability "
          "nor the skip rate for sampling were set...\n");
    }
    tool_prob_num = 10.0;
    if (tool_verbosity > 0) {
      printf(
          "KokkosP: The probability "
          "for the sampler is set to the default of %f percent. The skip rate "
          "for sampler"
          "will not be used.\n",
          tool_prob_num);
    }
  }

  if (tool_verbosity > 0) {
    if (tool_verbosity > 1) {
      printf("KokkosP: Sampling skip rate provided as input is: %s\n",
             tool_sample);
      printf("KokkosP: Sampling probability provided as input is: %s\n",
             tool_probability);
    }
    printf("KokkosP: Sampling skip rate is set to: %llu\n",
           (unsigned long long)(kernelSampleSkip));
    printf("KokkosP: Sampling probability is set to %f\n", tool_prob_num);
  }

  if (0 > tool_seed) {
    srand(time(NULL));
    if (tool_verbosity > 0) {
      printf(
          "KokkosP: Seeding random number generator using clock for "
          "random sampling.\n");
    }
  } else {
    srand(tool_seed);
    if (tool_verbosity > 0) {
      printf(
          "KokkosP: Seeding random number generator using seed %u for "
          "random sampling.\n",
          tool_seed);
    }
  }

  if ((NULL != tool_probability) && (NULL != tool_sample)) {
    printf(
        "KokkosP: You set both the probability and skip rate for the sampler. "
        "Only random sampling "
        "will be done, using the probabability you set; "
        "The skip rate you set will be ignored.\n");

    if (tool_verbosity > 1) {
      printf(
          "KokkosP: Note: The skip rate will be set to 1. Sampling will not be "
          "based "
          " on a pre-defined periodicity.\n");
    }
    kernelSampleSkip = 1;
  }
}

void kokkosp_finalize_library() {
  if (NULL != finalizeProfileLibrary) (*finalizeProfileLibrary)();
}

void kokkosp_begin_parallel_for(const char* name, const uint32_t devID,
                                uint64_t* kID) {
  *kID                          = uniqID++;
  static uint64_t invocationNum = 0;
  ++invocationNum;
  if ((invocationNum % kernelSampleSkip) == 0) {
    if ((rand() / (1.0 * RAND_MAX)) < (tool_prob_num / 100.0)) {
      if (tool_verbosity > 0) {
        printf("KokkosP: sample %llu calling child-begin function...\n",
               (unsigned long long)(*kID));
      }
      if (NULL != beginForCallee) {
        if (tool_globFence) {
          invoke_ktools_fence(0);
        }
        uint64_t nestedkID = 0;
        (*beginForCallee)(name, devID, &nestedkID);
        if (tool_verbosity > 0) {
          printf("KokkosP: sample %llu finished with child-begin function.\n",
                 (unsigned long long)(*kID));
        }
        infokIDSample.insert({*kID, nestedkID});
      }
    }
  }
}

void kokkosp_end_parallel_for(const uint64_t kID) {
  if (NULL != endForCallee) {
    if (!(infokIDSample.find(kID) == infokIDSample.end())) {
      uint64_t retrievedNestedkID = infokIDSample[kID];
      if (tool_verbosity > 0) {
        printf("KokkosP: sample %llu calling child-end function...\n",
               (unsigned long long)(kID));
      }
      if (tool_globFence) {
        invoke_ktools_fence(0);
      }
      (*endForCallee)(retrievedNestedkID);
      infokIDSample.erase(kID);
    }
  }
}

void kokkosp_begin_parallel_scan(const char* name, const uint32_t devID,
                                 uint64_t* kID) {
  *kID                          = uniqID++;
  static uint64_t invocationNum = 0;
  ++invocationNum;
  if ((invocationNum % kernelSampleSkip) == 0) {
    if ((rand() / (1.0 * RAND_MAX)) < (tool_prob_num / 100.0)) {
      if (tool_verbosity > 0) {
        printf("KokkosP: sample %llu calling child-begin function...\n",
               (unsigned long long)(*kID));
      }
      if (NULL != beginScanCallee) {
        uint64_t nestedkID = 0;
        if (tool_globFence) {
          invoke_ktools_fence(0);
        }
        (*beginScanCallee)(name, devID, &nestedkID);
        if (tool_verbosity > 0) {
          printf("KokkosP: sample %llu finished with child-begin function.\n",
                 (unsigned long long)(*kID));
        }
        infokIDSample.insert({*kID, nestedkID});
      }
    }
  }
}

void kokkosp_end_parallel_scan(const uint64_t kID) {
  if (NULL != endScanCallee) {
    if (!(infokIDSample.find(kID) == infokIDSample.end())) {
      uint64_t retrievedNestedkID = infokIDSample[kID];
      if (tool_verbosity > 0) {
        printf("KokkosP: sample %llu calling child-end function...\n",
               (unsigned long long)(kID));
      }
      if (tool_globFence) {
        invoke_ktools_fence(0);
      }
      (*endScanCallee)(retrievedNestedkID);
      infokIDSample.erase(kID);
    }
  }
}

void kokkosp_begin_parallel_reduce(const char* name, const uint32_t devID,
                                   uint64_t* kID) {
  *kID                          = uniqID++;
  static uint64_t invocationNum = 0;
  ++invocationNum;
  if ((invocationNum % kernelSampleSkip) == 0) {
    if ((rand() / (1.0 * RAND_MAX)) < (tool_prob_num / 100.0)) {
      if (tool_verbosity > 0) {
        printf("KokkosP: sample %llu calling child-begin function...\n",
               (unsigned long long)(*kID));
      }
      if (NULL != beginReduceCallee) {
        uint64_t nestedkID = 0;
        if (tool_globFence) {
          invoke_ktools_fence(0);
        }
        (*beginReduceCallee)(name, devID, &nestedkID);
        if (tool_verbosity > 0) {
          printf("KokkosP: sample %llu finished with child-begin function.\n",
                 (unsigned long long)(*kID));
        }
        infokIDSample.insert({*kID, nestedkID});
      }
    }
  }
}

void kokkosp_end_parallel_reduce(const uint64_t kID) {
  if (NULL != endScanCallee) {
    if (!(infokIDSample.find(kID) == infokIDSample.end())) {
      uint64_t retrievedNestedkID = infokIDSample[kID];
      if (tool_verbosity > 0) {
        printf("KokkosP: sample %llu calling child-end function...\n",
               (unsigned long long)(kID));
      }
      if (tool_globFence) {
        invoke_ktools_fence(0);
      }

      (*endScanCallee)(retrievedNestedkID);
      infokIDSample.erase(kID);
    }
  }
}

}  // namespace Sampler
}  // end namespace KokkosTools

extern "C" {

namespace impl = KokkosTools::Sampler;
EXPOSE_TOOL_SETTINGS(impl::kokkosp_request_tool_settings)
EXPOSE_PROVIDE_TOOL_PROGRAMMING_INTERFACE(
    impl::kokkosp_provide_tool_programming_interface)
EXPOSE_INIT(impl::kokkosp_init_library)
EXPOSE_FINALIZE(impl::kokkosp_finalize_library)
EXPOSE_BEGIN_PARALLEL_FOR(impl::kokkosp_begin_parallel_for)
EXPOSE_END_PARALLEL_FOR(impl::kokkosp_end_parallel_for)
EXPOSE_BEGIN_PARALLEL_SCAN(impl::kokkosp_begin_parallel_scan)
EXPOSE_END_PARALLEL_SCAN(impl::kokkosp_end_parallel_scan)
EXPOSE_BEGIN_PARALLEL_REDUCE(impl::kokkosp_begin_parallel_reduce)
EXPOSE_END_PARALLEL_REDUCE(impl::kokkosp_end_parallel_reduce)

}  // end extern "C"
