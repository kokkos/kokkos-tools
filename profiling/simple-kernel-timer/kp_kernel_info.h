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
#include <string>
#include <cstring>

#include "utils/demangle.hpp"

#include "../../common/FrameType.hpp"
#include "../../common/utils_time.hpp"

namespace KokkosTools::KernelTimer {

class KernelPerformanceInfo {
 public:
  KernelPerformanceInfo(std::string kName, FrameType kernelType)
      : kernelName(std::move(kName)), kType(kernelType) {}

  auto getKernelType() const { return kType; }

  void incrementCount() { callCount++; }

  void addTime(double t) {
    time += t;
    timeSq += (t * t);
  }

  void addFromTimer() {
    addTime(KokkosTools::seconds() - startTime);

    incrementCount();
  }

  void startTimer() { startTime = KokkosTools::seconds(); }

  uint64_t getCallCount() const { return callCount; }

  double getTime() const { return time; }

  double getTimeSq() { return timeSq; }

  const std::string& getName() const { return kernelName; }

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

    this->kernelName = std::string(&entry[nextIndex], kernelNameLength);

    kernelName = demangleNameKokkos(kernelName);

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
    } else if (kernelT == 3) {
      kType = REGION;
    }

    free(entry);
    return true;
  }

  void writeToBinaryFile(FILE* output) {
    const uint32_t kernelNameLen = kernelName.size();
    const uint32_t recordLen = sizeof(uint32_t) + sizeof(char) * kernelNameLen +
                               sizeof(uint64_t) + sizeof(double) +
                               sizeof(double) + sizeof(uint32_t);

    uint32_t nextIndex = 0;
    char* entry        = (char*)malloc(recordLen);

    copy(&entry[nextIndex], (char*)&kernelNameLen, sizeof(kernelNameLen));
    nextIndex += sizeof(kernelNameLen);

    copy(&entry[nextIndex], kernelName.c_str(), kernelNameLen);
    nextIndex += kernelNameLen;

    copy(&entry[nextIndex], (char*)&callCount, sizeof(callCount));
    nextIndex += sizeof(callCount);

    copy(&entry[nextIndex], (char*)&time, sizeof(time));
    nextIndex += sizeof(time);

    copy(&entry[nextIndex], (char*)&timeSq, sizeof(timeSq));
    nextIndex += sizeof(timeSq);

    uint32_t kernelTypeOutput = (uint32_t)kType;
    copy(&entry[nextIndex], (char*)&kernelTypeOutput, sizeof(kernelTypeOutput));
    nextIndex += sizeof(kernelTypeOutput);

    fwrite(&recordLen, sizeof(uint32_t), 1, output);
    fwrite(entry, recordLen, 1, output);
    free(entry);
  }

  void writeToJSONFile(FILE* output, const char* indent) {
    fprintf(output, "%s{\n", indent);

    char* indentBuffer = (char*)malloc(sizeof(char) * 256);
    snprintf(indentBuffer, 256, "%s    ", indent);

    fprintf(output, "%s\"kernel-name\"    : \"%s\",\n", indentBuffer,
            kernelName.c_str());
    // fprintf(output, "%s\"region\"         : \"%s\",\n", indentBuffer,
    // regionName);
    fprintf(output, "%s\"call-count\"     : %llu,\n", indentBuffer,
            (unsigned long long)(callCount));
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

  std::string kernelName;
  // const char* regionName;
  uint64_t callCount = 0;
  double time        = 0;
  double timeSq      = 0;
  double startTime   = 0;
  FrameType kType;
};

}  // namespace KokkosTools::KernelTimer

#endif
