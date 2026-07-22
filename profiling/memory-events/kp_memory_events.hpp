// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// SPDX-FileCopyrightText: Copyright Contributors to the Kokkos project
#define MEMOP_ALLOCATE 1
#define MEMOP_DEALLOCATE 2
#define MEMOP_PUSH_REGION 3
#define MEMOP_POP_REGION 4

#include <cstdio>
#include <inttypes.h>
#include <string>

#include "kp_core.hpp"

namespace KokkosTools::MemoryEvents {

extern char space_name[16][64];

struct EventRecord {
  const void* ptr;
  uint64_t size;
  int operation;
  int space;
  double time;
  std::string name;

  EventRecord(const void* const ptr_, const uint64_t size_,
              const int operation_, const int space_, const double time_,
              const char* const name_) {
    ptr       = ptr_;
    size      = size_;
    operation = operation_;
    space     = space_;
    time      = time_;
    name      = name_;
  }

  void print_record(FILE* ofile) const {
    if (operation == MEMOP_ALLOCATE)
      fprintf(ofile, "%lf %16p %14" PRId64 " %16s Allocate   %s\n", time, ptr,
              size, space < 0 ? "" : space_name[space], name.c_str());
    if (operation == MEMOP_DEALLOCATE)
      fprintf(ofile, "%lf %16p %14" PRId64 " %16s DeAllocate %s\n", time, ptr,
              -size, space < 0 ? "" : space_name[space], name.c_str());
    if (operation == MEMOP_PUSH_REGION)
      fprintf(ofile, "%lf PushRegion %s {\n", time, name.c_str());
    if (operation == MEMOP_POP_REGION)
      fprintf(ofile, "%lf } PopRegion %s\n", time, name.c_str());
  }
};

}  // namespace KokkosTools::MemoryEvents
