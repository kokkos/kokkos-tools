#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <dlfcn.h>

static uint64_t uniqID           = 0;
static uint64_t kernelCounter    = 0;
static uint64_t kernelSampleSkip = 101;
static int tool_verbosity;

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

extern "C" void kokkosp_init_library(const int loadSeq,
                                     const uint64_t interfaceVer,
                                     const uint32_t devInfoCount,
                                     void* deviceInfo) {
  const char* tool_verbose_str = getenv("KOKKOS_SAMPLER_VERBOSE");
  if (NULL != tool_verbose_str) {
    tool_verbosity = atoi(tool_verbose_str);
  } else {
    tool_verbosity = 0;
  }

  char* profileLibrary = getenv("KOKKOS_TOOLS_LIBS");
  if (NULL == profileLibrary) {
    printf(
        "Checking KOKKOS_PROFILE_LIBRARY. WARNING: This is a depreciated "
        "variable. Please use KOKKOS_TOOLS_LIBS\n");
    profileLibrary = getenv("KOKKOS_PROFILE_LIBRARY");
    if (NULL == profileLibrary) {
      printf("KokkosP: No library to call in %s\n", profileLibrary);
      exit(-1);
    }
  }

  char* envBuffer = (char*)malloc(sizeof(char) * (strlen(profileLibrary) + 1));
  sprintf(envBuffer, "%s", profileLibrary);

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

  kernelCounter = 0;
  uniqID        = 1;

  const char* tool_sample = getenv("KOKKOS_SAMPLE_SKIP");
  if (NULL != tool_sample) {
    kernelSampleSkip = atoi(tool_sample) + 1;
  }

  if (tool_verbosity > 0) {
    printf("KokkosP: Sampling rate set to: %s\n", tool_sample);
  }
}

extern "C" void kokkosp_finalize_library() {}

extern "C" void kokkosp_begin_parallel_for(const char* name,
                                           const uint32_t devID,
                                           uint64_t* kID) {
  *kID = uniqID++;

  if (((*kID) % kernelSampleSkip) == 0) {
    if (tool_verbosity > 0) {
      printf("KokkosP: sample %llu calling child-begin function...\n", *kID);
    }

    if (NULL != beginForCallee) {
      (*beginForCallee)(name, devID, kID);
    }
  }
}

extern "C" void kokkosp_end_parallel_for(const uint64_t kID) {
  if ((kID % kernelSampleSkip) == 0) {
    if (tool_verbosity > 0) {
      printf("KokkosP: sample %llu calling child-end function...\n", kID);
    }

    if (NULL != endForCallee) {
      (*endForCallee)(kID);
    }
  }
}

extern "C" void kokkosp_begin_parallel_scan(const char* name,
                                            const uint32_t devID,
                                            uint64_t* kID) {
  *kID = uniqID++;

  if (((*kID) % kernelSampleSkip) == 0) {
    if (tool_verbosity > 0) {
      printf("KokkosP: sample %llu calling child-begin function...\n", *kID);
    }

    if (NULL != beginScanCallee) {
      (*beginScanCallee)(name, devID, kID);
    }
  }
}

extern "C" void kokkosp_end_parallel_scan(const uint64_t kID) {
  if ((kID % kernelSampleSkip) == 0) {
    if (tool_verbosity > 0) {
      printf("KokkosP: sample %llu calling child-end function...\n", kID);
    }

    if (NULL != endScanCallee) {
      (*endScanCallee)(kID);
    }
  }
}

extern "C" void kokkosp_begin_parallel_reduce(const char* name,
                                              const uint32_t devID,
                                              uint64_t* kID) {
  *kID = uniqID++;

  if (((*kID) % kernelSampleSkip) == 0) {
    if (tool_verbosity > 0) {
      printf("KokkosP: sample %llu calling child-begin function...\n", *kID);
    }

    if (NULL != beginReduceCallee) {
      (*beginReduceCallee)(name, devID, kID);
    }
  }
}

extern "C" void kokkosp_end_parallel_reduce(const uint64_t kID) {
  if ((kID % kernelSampleSkip) == 0) {
    if (tool_verbosity > 0) {
      printf("KokkosP: sample %llu calling child-end function...\n", kID);
    }

    if (NULL != endReduceCallee) {
      (*endReduceCallee)(kID);
    }
  }
}
