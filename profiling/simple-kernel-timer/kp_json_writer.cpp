// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// SPDX-FileCopyrightText: Copyright Contributors to the Kokkos project

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

void fill_info(FILE* file, std::vector<KernelPerformanceInfo*>& kernelInfo) {
  while (!feof(file)) {
    KernelPerformanceInfo* new_kernel =
        new KernelPerformanceInfo("", PARALLEL_FOR);
    if (new_kernel->readFromFile(file)) {
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

  double totalExecuteTime = 0;

  for (int i = commandline_args; i < argc; i++) {
    FILE* the_file = fopen(argv[i], "rb");

    double fileExecuteTime = 0;
    [[maybe_unused]] auto ignore =
        fread(&fileExecuteTime, sizeof(fileExecuteTime), 1, the_file);

    totalExecuteTime += fileExecuteTime;

    fill_info(the_file, kernelInfo);

    fclose(the_file);
  }
  std::ostream& fout = std::cout;
  json_format_kernel_list(totalExecuteTime, kernelInfo, fout);

  return 0;
}
