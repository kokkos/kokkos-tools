//@HEADER
// ************************************************************************
//
//                        Kokkos v. 4.0
//       Copyright (2022) National Technology & Engineering
//               Solutions of Sandia, LLC (NTESS).
//
// Under the terms of Contract DE-NA0003525 with NTESS,
// the U.S. Government retains certain rights in this software.
//
// Part of Kokkos, under the Apache License v2.0 with LLVM Exceptions.
// See https://kokkos.org/LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//@HEADER

#include <stdio.h>
#include <inttypes.h>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <unordered_set>
#include <string>
#include <regex>
#include <cxxabi.h>
#include <dlfcn.h>

bool filterKernels;
uint64_t nextKernelID;
std::vector<std::regex> kernelNames;
std::unordered_set<uint64_t> activeKernels;

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

bool kokkospFilterMatch(const char* name) {
  bool matched = false;
  std::string nameStr(name);

  for (auto nextRegex : kernelNames) {
    if (std::regex_match(nameStr, nextRegex)) {
      matched = true;
      break;
    }
  }

  return matched;
}

bool kokkospReadLine(FILE* kernelFile, char* lineBuffer) {
  bool readData        = false;
  bool continueReading = !feof(kernelFile);
  int nextIndex        = 0;

  while (continueReading) {
    const int nextChar = fgetc(kernelFile);

    if (EOF == nextChar) {
      continueReading = false;
    } else {
      readData = true;

      if (nextChar == '\n' || nextChar == '\f' || nextChar == '\r') {
        continueReading = false;
      } else {
        lineBuffer[nextIndex++] = (char)nextChar;
      }
    }
  }

  lineBuffer[nextIndex] = '\0';
  return readData;
}

extern "C" void kokkosp_init_library(const int loadSeq,
                                     const uint64_t interfaceVer,
                                     const uint32_t devInfoCount,
                                     void* deviceInfo) {
  const char* kernelFilterPath = getenv("KOKKOSP_KERNEL_FILTER");
  nextKernelID                 = 0;

  if (NULL == kernelFilterPath) {
    filterKernels = false;
    printf("============================================================\n");
    printf(
        "KokkosP: No kernel filtering data provided, disabled from this tool "
        "onwards\n");
    printf("============================================================\n");
  } else {
    printf("============================================================\n");
    printf("KokkosP: Filter File: %s\n", kernelFilterPath);
    printf("============================================================\n");

    FILE* kernelFilterFile = fopen(kernelFilterPath, "rt");

    if (NULL == kernelFilterFile) {
      fprintf(stderr, "Unable to open kernel filter: %s\n", kernelFilterPath);
      exit(-1);
    } else {
      char* lineBuffer = (char*)malloc(sizeof(char) * 65536);

      while (kokkospReadLine(kernelFilterFile, lineBuffer)) {
        printf("KokkosP: Filter [%s]\n", lineBuffer);

        std::regex nextRegEx(lineBuffer, std::regex::optimize);
        kernelNames.push_back(nextRegEx);
      }

      free(lineBuffer);
    }

    filterKernels = (kernelNames.size() > 0);

    printf("KokkosP: Kernel Filtering is %s\n",
           (filterKernels ? "enabled" : "disabled"));

    if (filterKernels) {
      char* profileLibrary = getenv("KOKKOS_PROFILE_LIBRARY");
      char* envBuffer =
          (char*)malloc(sizeof(char) * (strlen(profileLibrary) + 1));
      sprintf(envBuffer, "%s", profileLibrary);

      char* nextLibrary = strtok(envBuffer, ";");

      for (int i = 0; i < loadSeq; i++) {
        nextLibrary = strtok(NULL, ";");
      }

      nextLibrary = strtok(NULL, ";");

      if (NULL == nextLibrary) {
        printf("KokkosP: No child library to call in %s\n", profileLibrary);
      } else {
        printf("KokkosP: Next library to call: %s\n", nextLibrary);
        printf("KokkosP: Loading child library ..\n");

        void* childLibrary = dlopen(nextLibrary, RTLD_NOW | RTLD_GLOBAL);

        if (NULL == childLibrary) {
          fprintf(stderr, "KokkosP: Error: Unable to load: %s (Error=%s)\n",
                  nextLibrary, dlerror());
        } else {
          beginForCallee =
              (beginFunction)dlsym(childLibrary, "kokkosp_begin_parallel_for");
          beginScanCallee =
              (beginFunction)dlsym(childLibrary, "kokkosp_begin_parallel_scan");
          beginReduceCallee = (beginFunction)dlsym(
              childLibrary, "kokkosp_begin_parallel_reduce");

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
        }
      }

      free(envBuffer);
    }

    printf("============================================================\n");
  }
}

extern "C" void kokkosp_finalize_library() {
  if (NULL != finalizeProfileLibrary) {
    (*finalizeProfileLibrary)();
  }

  // Set all profile hooks to NULL to prevent
  // any additional calls. Once we are told to
  // finalize, we mean it
  beginForCallee         = NULL;
  beginScanCallee        = NULL;
  beginReduceCallee      = NULL;
  endScanCallee          = NULL;
  endForCallee           = NULL;
  endReduceCallee        = NULL;
  initProfileLibrary     = NULL;
  finalizeProfileLibrary = NULL;

  printf("============================================================\n");
  printf("KokkosP: Kernel filtering library, finalized.\n");
  printf("============================================================\n");
}

extern "C" void kokkosp_begin_parallel_for(const char* name,
                                           const uint32_t devID,
                                           uint64_t* kID) {
  if (filterKernels) {
    if (kokkospFilterMatch(name)) {
      if (NULL != beginForCallee) {
        (*beginForCallee)(name, devID, kID);
        activeKernels.insert(*kID);
      } else {
        *kID = nextKernelID++;
      }
    } else {
      *kID = nextKernelID++;
    }
  } else {
    if (NULL != beginForCallee) {
      (*beginForCallee)(name, devID, kID);
      activeKernels.insert(*kID);
    } else {
      *kID = nextKernelID++;
    }
  }
}

extern "C" void kokkosp_end_parallel_for(const uint64_t kID) {
  auto findKernel = activeKernels.find(kID);

  if (activeKernels.end() != findKernel) {
    if (NULL != endForCallee) {
      (*endForCallee)(kID);
    }

    activeKernels.erase(findKernel);
  }
}

extern "C" void kokkosp_begin_parallel_scan(const char* name,
                                            const uint32_t devID,
                                            uint64_t* kID) {
  if (filterKernels) {
    if (kokkospFilterMatch(name)) {
      if (NULL != beginScanCallee) {
        (*beginScanCallee)(name, devID, kID);
        activeKernels.insert(*kID);
      } else {
        *kID = nextKernelID++;
      }
    } else {
      *kID = nextKernelID++;
    }
  } else {
    if (NULL != beginScanCallee) {
      (*beginScanCallee)(name, devID, kID);
      activeKernels.insert(*kID);
    } else {
      *kID = nextKernelID++;
    }
  }
}

extern "C" void kokkosp_end_parallel_scan(const uint64_t kID) {
  auto findKernel = activeKernels.find(kID);

  if (activeKernels.end() != findKernel) {
    if (NULL != endScanCallee) {
      (*endScanCallee)(kID);
    }

    activeKernels.erase(findKernel);
  }
}

extern "C" void kokkosp_begin_parallel_reduce(const char* name,
                                              const uint32_t devID,
                                              uint64_t* kID) {
  if (filterKernels) {
    if (kokkospFilterMatch(name)) {
      if (NULL != beginScanCallee) {
        (*beginReduceCallee)(name, devID, kID);
        activeKernels.insert(*kID);
      } else {
        *kID = nextKernelID++;
      }
    } else {
      *kID = nextKernelID++;
    }
  } else {
    if (NULL != beginScanCallee) {
      (*beginReduceCallee)(name, devID, kID);
      activeKernels.insert(*kID);
    } else {
      *kID = nextKernelID++;
    }
  }
}

extern "C" void kokkosp_end_parallel_reduce(const uint64_t kID) {
  auto findKernel = activeKernels.find(kID);

  if (activeKernels.end() != findKernel) {
    if (NULL != endReduceCallee) {
      (*endReduceCallee)(kID);
    }

    activeKernels.erase(findKernel);
  }
}
