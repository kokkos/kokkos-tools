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

#ifndef _H_KOKKOSP_KERNEL_INFO
#define _H_KOKKOSP_KERNEL_INFO

#include <stdio.h>
#include <sys/time.h>
#include <cstring>

#if defined(__GXX_ABI_VERSION)
#define HAVE_GCC_ABI_DEMANGLE
#endif

#if defined(HAVE_GCC_ABI_DEMANGLE)
#include <cxxabi.h>
#endif  // HAVE_GCC_ABI_DEMANGLE

char* demangleName(char* kernelName) {
#if defined(HAVE_GCC_ABI_DEMANGLE)
  int status = -1;
  char* demangledKernelName =
      abi::__cxa_demangle(kernelName, NULL, NULL, &status);
  if (status == 0) {
    free(kernelName);
    kernelName = demangledKernelName;
  }
#endif  // HAVE_GCC_ABI_DEMANGLE
  return kernelName;
}

double seconds() {
  struct timeval now;
  gettimeofday(&now, NULL);

  return (double)(now.tv_sec + (now.tv_usec * 1.0e-6));
}

enum KernelExecutionType {
  PARALLEL_FOR    = 0,
  PARALLEL_REDUCE = 1,
  PARALLEL_SCAN   = 2
};

class KernelPerformanceInfo {
 public:
  KernelPerformanceInfo(std::string kName, KernelExecutionType kernelType)
      : kType(kernelType) {
    kernelName = (char*)malloc(sizeof(char) * (kName.size() + 1));
    regionName = "";
    strcpy(kernelName, kName.c_str());

    callCount = 0;
    time      = 0;
  }

  ~KernelPerformanceInfo() { free(kernelName); }

  KernelExecutionType getKernelType() { return kType; }

  void incrementCount() { callCount++; }

  void addTime(double t) {
    time += t;
    timeSq += (t * t);
  }

  void addFromTimer() {
    addTime(seconds() - startTime);

    incrementCount();
  }

  void startTimer() { startTime = seconds(); }

  uint64_t getCallCount() { return callCount; }

  double getTime() { return time; }

  double getTimeSq() { return timeSq; }

  char* getName() { return kernelName; }

  void addCallCount(const uint64_t newCalls) { callCount += newCalls; }

  bool readFromFile(FILE* input) {
    uint32_t recordLen   = 0;
    uint32_t actual_read = fread(&recordLen, sizeof(recordLen), 1, input);
    if (actual_read != 1) return false;

    char* entry = (char*)malloc(recordLen);
    fread(entry, recordLen, 1, input);

    uint32_t nextIndex = 0;
    uint32_t kernelNameLength;
    copy((char*)&kernelNameLength, &entry[nextIndex], sizeof(kernelNameLength));
    nextIndex += sizeof(kernelNameLength);

    if (strlen(kernelName) > 0) {
      free(kernelName);
    }

    kernelName = (char*)malloc(sizeof(char) * (kernelNameLength + 1));
    copy(kernelName, &entry[nextIndex], kernelNameLength);
    kernelName[kernelNameLength] = '\0';

    kernelName = demangleName(kernelName);

    nextIndex += kernelNameLength;

    copy((char*)&callCount, &entry[nextIndex], sizeof(callCount));
    nextIndex += sizeof(callCount);

    copy((char*)&time, &entry[nextIndex], sizeof(time));
    nextIndex += sizeof(time);

    copy((char*)&timeSq, &entry[nextIndex], sizeof(timeSq));
    nextIndex += sizeof(timeSq);

    uint32_t kernelT = 0;
    copy((char*)&kernelT, &entry[nextIndex], sizeof(kernelT));
    nextIndex += sizeof(kernelT);

    if (kernelT == 0) {
      kType = PARALLEL_FOR;
    } else if (kernelT == 1) {
      kType = PARALLEL_REDUCE;
    } else if (kernelT == 2) {
      kType = PARALLEL_SCAN;
    }

    free(entry);
    return true;
  }

  void writeToFile(FILE* output, char* indent) {
    fprintf(output, "%s{\n", indent);

    char* indentBuffer = (char*)malloc(sizeof(char) * 256);
    sprintf(indentBuffer, "%s    ", indent);

    fprintf(output, "%s\"kernel-name\"    : \"%s\",\n", indentBuffer,
            kernelName);
    fprintf(output, "%s\"region\"         : \"%s\",\n", indentBuffer,
            regionName);
    fprintf(output, "%s\"call-count\"     : %lu,\n", indentBuffer, callCount);
    fprintf(output, "%s\"total-time\"     : %f,\n", indentBuffer, time);
    fprintf(output, "%s\"time-per-call\"  : %16.8f,\n", indentBuffer,
            (time / static_cast<double>(
                        std::max(static_cast<uint64_t>(1), callCount))));
    fprintf(
        output, "%s\"kernel-type\"    : \"%s\"\n", indentBuffer,
        (kType == PARALLEL_FOR)
            ? "PARALLEL-FOR"
            : (kType == PARALLEL_REDUCE) ? "PARALLEL-REDUCE" : "PARALLEL-SCAN");

    fprintf(output, "%s}", indent);
  }

 private:
  void copy(char* dest, const char* src, uint32_t len) {
    for (uint32_t i = 0; i < len; i++) {
      dest[i] = src[i];
    }
  }

  char* kernelName;
  char* regionName;
  uint64_t callCount;
  double time;
  double timeSq;
  double startTime;
  KernelExecutionType kType;
};

#endif
