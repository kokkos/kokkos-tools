// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// SPDX-FileCopyrightText: Copyright Contributors to the Kokkos project
#define MEMOP_ALLOCATE 1
#define MEMOP_DEALLOCATE 2
#define MEMOP_PUSH_REGION 3
#define MEMOP_POP_REGION 4

#include <cstdio>
#include <inttypes.h>
#include <iomanip>
#include <iosfwd>

#include "kp_core.hpp"

namespace KokkosTools::MemoryEvents {

extern char space_name[16][64];

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

  void print_record(std::ostream& os) const {
    if (operation == MEMOP_ALLOCATE) {
      os << time << " " << std::setw(16) << ptr << " " << std::setw(14) << size
         << " " << std::setw(16) << (space < 0 ? "" : space_name[space]) << " "
         << "Allocate   " << name << "\n";
    }

    if (operation == MEMOP_DEALLOCATE) {
      os << time << " " << std::setw(16) << ptr << " " << std::setw(14) << -size
         << " " << std::setw(16) << (space < 0 ? "" : space_name[space]) << " "
         << "DeAllocate " << name << "\n";
    }

    if (operation == MEMOP_PUSH_REGION) {
      os << time << " PushRegion " << name << " {\n";
    }

    if (operation == MEMOP_POP_REGION) {
      os << time << " } PopRegion " << name << "\n";
    }
  }
};

}  // namespace KokkosTools::MemoryEvents
