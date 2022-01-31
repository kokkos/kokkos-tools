//@HEADER
// ************************************************************************
//
//                        Kokkos v. 3.0
//       Copyright (2020) National Technology & Engineering
//               Solutions of Sandia, LLC (NTESS).
//
// Under the terms of Contract DE-NA0003525 with NTESS,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY NTESS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL NTESS OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact David Poliakoff (dzpolia@sandia.gov)
//
// ************************************************************************
//@HEADER

#ifndef _H_KOKKOSP_KERNEL_SHARED
#define _H_KOKKOSP_KERNEL_SHARED

#include <map>
#include <memory>
#include "kp_kernel_info.h"

namespace KokkosTools::KernelTimer {

extern uint64_t uniqID;
extern KernelPerformanceInfo* currentEntry;
extern std::map<std::string, KernelPerformanceInfo*> count_map;
extern double initTime;
extern char* outputDelimiter;
extern int current_region_level;
extern KernelPerformanceInfo* regions[512];

inline void increment_counter(const char* name, KernelExecutionType kType) {
	std::string nameStr(name);

	if(count_map.find(name) == count_map.end()) {
		KernelPerformanceInfo *info = new KernelPerformanceInfo(nameStr, kType);
		count_map.insert(std::pair<std::string, KernelPerformanceInfo*>(nameStr, info));

		currentEntry = info;
	} else {
		currentEntry = count_map[nameStr];
	}

	currentEntry->startTimer();
}

inline void increment_counter_region(const char* name, KernelExecutionType kType) {
        std::string nameStr(name);

        if(count_map.find(name) == count_map.end()) {
		KernelPerformanceInfo* info = new KernelPerformanceInfo(nameStr, kType);
		count_map.insert(std::pair<std::string, KernelPerformanceInfo*>(nameStr, info));

                regions[current_region_level] = info;
        } else {
                regions[current_region_level] = count_map[nameStr];
        }

        regions[current_region_level]->startTimer();
        current_region_level++;
}

inline bool compareKernelPerformanceInfo(KernelPerformanceInfo* left, KernelPerformanceInfo* right)
{
	return left->getTime() > right->getTime();
};

} // namespace KokkosTools::KernelTimer

#endif // _H_KOKKOSP_KERNEL_SHARED