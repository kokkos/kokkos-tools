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
#define MEMOP_ALLOCATE 1
#define MEMOP_DEALLOCATE 2
#define MEMOP_PUSH_REGION 3
#define MEMOP_POP_REGION 4

#include <cstring>
#include <cstdio>
#include <inttypes.h>

struct SpaceHandle {
  char name[64];
};

char space_name[16][64];

struct EventRecord {
  const void* ptr;
  uint64_t size;
  int operation;
  int space;
  double time;
  char name[256];

  EventRecord(const void* const ptr_, const uint64_t size_,
              const int operation_, const int space_, const double time_,
              const char* const name_) {
    ptr       = ptr_;
    size      = size_;
    operation = operation_;
    space     = space_;
    time      = time_;
    strncpy(name, name_, 256);
  }

  void print_record(FILE* ofile) const {
    if (operation == MEMOP_ALLOCATE)
      fprintf(ofile, "%lf %16p %14" PRId64 " %16s Allocate   %s\n", time, ptr,
              size, space < 0 ? "" : space_name[space], name);
    if (operation == MEMOP_DEALLOCATE)
      fprintf(ofile, "%lf %16p %14" PRId64 " %16s DeAllocate %s\n", time, ptr,
              -size, space < 0 ? "" : space_name[space], name);
    if (operation == MEMOP_PUSH_REGION)
      fprintf(ofile, "%lf PushRegion %s {\n", time, name);
    if (operation == MEMOP_POP_REGION)
      fprintf(ofile, "%lf } PopRegion %s\n", time, name);
  }
};
