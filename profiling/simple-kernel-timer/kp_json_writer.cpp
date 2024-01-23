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
#include <cinttypes>
#include <cstdlib>
#include <vector>
#include <algorithm>
#include <map>
#include <fstream>
#include <iostream>

#include "kp_shared.h"

using namespace KokkosTools::KernelTimer;

bool is_region(KernelPerformanceInfo const& kp) {
  return kp.getKernelType() == REGION;
}

inline std::string to_string(KernelExecutionType t) {
  switch (t) {
    case PARALLEL_FOR: return "\"PARALLEL_FOR\"";
    case PARALLEL_REDUCE: return "\"PARALLEL_REDUCE\"";
    case PARALLEL_SCAN: return "\"PARALLEL_SCAN\"";
    case REGION: return "\"REGION\"";
    default: throw t;
  }
}

inline void write_json(std::ostream& os, KernelPerformanceInfo const& kp,
                       std::string indent = "") {
  os << indent << "{\n";
  os << indent << "  \"kernel-name\": \"" << kp.getName() << "\",\n";
  os << indent << "  \"call-count\": " << kp.getCallCount() << ",\n";
  os << indent << "  \"total-time\": " << kp.getTime() << ",\n";
  os << indent << "  \"time-per-call\": "
     << kp.getTime() / std::max((uint64_t)1, kp.getCallCount()) << ",\n";
  os << indent << "  \"kernel-type\": " << to_string(kp.getKernelType())
     << '\n';
  os << indent << '}';
}

int main(int argc, char* argv[]) {
  if (argc == 1) {
    fprintf(stderr, "Did you specify any data files on the command line!\n");
    fprintf(stderr, "Usage: ./kp_json_writer file1.dat [fileX.dat]*\n");
    exit(-1);
  }

  int commandline_args = 1;
  while ((commandline_args < argc) && (argv[commandline_args][0] == '-')) {
    commandline_args++;
  }

  std::vector<KernelPerformanceInfo*> kernelInfo;
  double totalKernelsTime    = 0;
  double totalExecuteTime    = 0;
  uint64_t totalKernelsCalls = 0;

  for (int i = commandline_args; i < argc; i++) {
    FILE* the_file = fopen(argv[i], "rb");

    double fileExecuteTime = 0;
    fread(&fileExecuteTime, sizeof(fileExecuteTime), 1, the_file);

    totalExecuteTime += fileExecuteTime;

    while (!feof(the_file)) {
      KernelPerformanceInfo* new_kernel =
          new KernelPerformanceInfo("", PARALLEL_FOR);
      if (new_kernel->readFromFile(the_file)) {
        if (!new_kernel->getName().empty()) {
          int kernelIndex = find_index(kernelInfo, new_kernel->getName());

          if (kernelIndex > -1) {
            kernelInfo[kernelIndex]->addTime(new_kernel->getTime());
            kernelInfo[kernelIndex]->addCallCount(new_kernel->getCallCount());
          } else {
            kernelInfo.push_back(new_kernel);
          }
        }
      }
    }

    fclose(the_file);
  }

  std::sort(kernelInfo.begin(), kernelInfo.end(), compareKernelPerformanceInfo);

  for (unsigned int i = 0; i < kernelInfo.size(); i++) {
    if (kernelInfo[i]->getKernelType() != REGION) {
      totalKernelsTime += kernelInfo[i]->getTime();
      totalKernelsCalls += kernelInfo[i]->getCallCount();
    }
  }

  // std::string filename = "test.json";
  // std::ofstream fout(filename);
  auto& fout = std::cout;

  fout << "{\n";

  fout << "  \"total-app-time\" : " << totalExecuteTime << ",\n";
  fout << "  \"total-kernel-time\" : " << totalKernelsTime << ",\n";
  fout << "  \"total-non-kernel-time\" : "
       << totalExecuteTime - totalKernelsTime << ",\n";
  fout << "  \"percent-in-kernels\" : "
       << 100. * totalKernelsTime / totalExecuteTime << ",\n";
  fout << "  \"unique-kernel-calls\" : " << totalKernelsCalls << ",\n";

  fout << "  \"region-data\" : [\n";
  {
    bool add_comma = false;
    for (auto const& kp : kernelInfo) {
      if (!is_region(*kp)) continue;
      if (add_comma) fout << ",\n";
      add_comma = true;
      write_json(fout, *kp, "    ");
    }
    fout << '\n';
  }
  fout << "  ],\n";

  fout << "  \"kernel-data\" : [\n";
  {
    bool add_comma = false;
    for (auto const& kp : kernelInfo) {
      if (is_region(*kp)) continue;
      if (add_comma) fout << ",\n";
      add_comma = true;
      write_json(fout, *kp, "    ");
    }
    fout << '\n';
  }
  fout << "  ]\n";

  fout << "}\n";

  return 0;
}
