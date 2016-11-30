/*
//@HEADER
// ************************************************************************
// // 
// //                        Kokkos v. 2.0
// //              Copyright (2014) Sandia Corporation
// // 
// // Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
// // the U.S. Government retains certain rights in this software.
// // 
// // Redistribution and use in source and binary forms, with or without
// // modification, are permitted provided that the following conditions are
// // met:
// //
// // 1. Redistributions of source code must retain the above copyright
// // notice, this list of conditions and the following disclaimer.
// //
// // 2. Redistributions in binary form must reproduce the above copyright
// // notice, this list of conditions and the following disclaimer in the
// // documentation and/or other materials provided with the distribution.
// //
// // 3. Neither the name of the Corporation nor the names of the
// // contributors may be used to endorse or promote products derived from
// // this software without specific prior written permission.
// //
// // THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// // EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// // IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// // PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// // CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// // EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// // PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// // PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// // LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// // NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// // SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// //
// // Questions? Contact  H. Carter Edwards (hcedwar@sandia.gov)
// // 
// // ************************************************************************
// //@HEADER
// */

#define MEMOP_ALLOCATE 1
#define MEMOP_DEALLOCATE 2
#include <cstring>

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


  EventRecord(const void* const ptr_, const uint64_t size_, const int operation_,
              const int space_, const double time_, const char* const name_) {
    ptr = ptr_;
    size = size_;
    operation = operation_;
    space = space_;
    time = time_;
    strncpy(name,name_,256);
  }

  void print_record() const {
    if(operation == MEMOP_ALLOCATE)
      printf("%lf %16p %14d %16s Allocate   %s\n",time,ptr,size,space<0?"":space_name[space],name);
    if(operation == MEMOP_DEALLOCATE)
      printf("%lf %16p %14d %16s DeAllocate %s\n",time,ptr,-size,space<0?"":space_name[space],name);
  }
  void print_record(FILE* ofile) const {
    if(operation == MEMOP_ALLOCATE)
      fprintf(ofile,"%lf %16p %14d %16s Allocate   %s\n",time,ptr,size,space<0?"":space_name[space],name);
    if(operation == MEMOP_DEALLOCATE)
      fprintf(ofile,"%lf %16p %14d %16s DeAllocate %s\n",time,ptr,-size,space<0?"":space_name[space],name);
  }
};

