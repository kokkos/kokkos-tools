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

#ifndef _H_KOKKOSP_KERNEL_INFO
#define _H_KOKKOSP_KERNEL_INFO

#include <stdio.h>
#include <sys/time.h>
#include <string>
#include <cstring>

#if defined(__GXX_ABI_VERSION)
#define HAVE_GCC_ABI_DEMANGLE
#endif

#if defined(HAVE_GCC_ABI_DEMANGLE)
#include <cxxabi.h>
#endif // HAVE_GCC_ABI_DEMANGLE

namespace KokkosTools::KernelTimer {

inline char* demangleName(char* kernelName)
{
#if defined(HAVE_GCC_ABI_DEMANGLE)
	int status = -1;
	char* demangledKernelName = abi::__cxa_demangle(kernelName, NULL, NULL, &status);
	if (status==0) {
		free(kernelName);
		kernelName = demangledKernelName;
	}
#endif // HAVE_GCC_ABI_DEMANGLE
	return kernelName;
}

inline double seconds() {
	struct timeval now;
	gettimeofday(&now, NULL);

	return (double) (now.tv_sec + (now.tv_usec * 1.0e-6));
}

enum KernelExecutionType {
	PARALLEL_FOR = 0,
	PARALLEL_REDUCE = 1,
	PARALLEL_SCAN = 2,
    REGION = 3
};

class KernelPerformanceInfo {
	public:
		KernelPerformanceInfo(std::string kName, KernelExecutionType kernelType) :
			kType(kernelType) {

			kernelName = (char*) malloc(sizeof(char) * (kName.size() + 1));
			strcpy(kernelName, kName.c_str());
			// regionName = "";

			callCount = 0;
			time = 0;
		}

		~KernelPerformanceInfo() {
			free(kernelName);
		}

		KernelExecutionType getKernelType() const {
			return kType;
		}

		void incrementCount() {
			callCount++;
		}

		void addTime(double t) {
			time   += t;
			timeSq += (t*t);
		}

		void addFromTimer() {
			addTime(seconds() - startTime);

			incrementCount();
		}

		void startTimer() {
			startTime = seconds();
		}

		uint64_t getCallCount() const {
			return callCount;
		}

		double getTime() const {
			return time;
		}

		double getTimeSq() {
			return timeSq;
		}

		char* getName() const {
			return kernelName;
		}

		void addCallCount(const uint64_t newCalls) {
			callCount += newCalls;
		}

		bool readFromFile(FILE* input) {
			uint32_t recordLen = 0;
			uint32_t actual_read = fread(&recordLen, sizeof(recordLen), 1, input);
	                if(actual_read != 1) return false;

			char* entry = (char*) malloc(recordLen);
                        fread(entry, recordLen, 1, input);

			uint32_t nextIndex = 0;
			uint32_t kernelNameLength;
			copy((char*) &kernelNameLength, &entry[nextIndex], sizeof(kernelNameLength));
			nextIndex += sizeof(kernelNameLength);

			if(strlen(kernelName) > 0) {
				free(kernelName);
			}

			kernelName = (char*) malloc( sizeof(char) * (kernelNameLength + 1));
			copy(kernelName, &entry[nextIndex], kernelNameLength);
			kernelName[kernelNameLength] = '\0';

			kernelName = demangleName(kernelName);

			nextIndex += kernelNameLength;

			copy((char*) &callCount, &entry[nextIndex], sizeof(callCount));
			nextIndex += sizeof(callCount);

			copy((char*) &time, &entry[nextIndex], sizeof(time));
			nextIndex += sizeof(time);

			copy((char*) &timeSq, &entry[nextIndex], sizeof(timeSq));
			nextIndex += sizeof(timeSq);

			uint32_t kernelT = 0;
			copy((char*) &kernelT, &entry[nextIndex], sizeof(kernelT));
			nextIndex += sizeof(kernelT);

			if(kernelT == 0) {
				kType = PARALLEL_FOR;
			} else if(kernelT == 1) {
				kType = PARALLEL_REDUCE;
			} else if(kernelT == 2) {
				kType = PARALLEL_SCAN;
			} else if(kernelT == 3) {
        kType = REGION;
      }

			free(entry);
                        return true;
		}

		void writeToBinaryFile(FILE* output) {
			const uint32_t kernelNameLen = (uint32_t) strlen(kernelName);

			const uint32_t recordLen =
				sizeof(uint32_t) +
				sizeof(char) * kernelNameLen +
				sizeof(uint64_t) +
				sizeof(double) +
				sizeof(double) +
				sizeof(uint32_t);

			uint32_t nextIndex = 0;
			char* entry = (char*) malloc(recordLen);

			copy(&entry[nextIndex], (char*) &kernelNameLen, sizeof(kernelNameLen));
			nextIndex += sizeof(kernelNameLen);

			copy(&entry[nextIndex], kernelName, kernelNameLen);
			nextIndex += kernelNameLen;

			copy(&entry[nextIndex], (char*) &callCount, sizeof(callCount));
			nextIndex += sizeof(callCount);

			copy(&entry[nextIndex], (char*) &time, sizeof(time));
			nextIndex += sizeof(time);

			copy(&entry[nextIndex], (char*) &timeSq, sizeof(timeSq));
			nextIndex += sizeof(timeSq);

			uint32_t kernelTypeOutput = (uint32_t) kType;
			copy(&entry[nextIndex], (char*) &kernelTypeOutput, sizeof(kernelTypeOutput));
			nextIndex += sizeof(kernelTypeOutput);

			fwrite(&recordLen, sizeof(uint32_t), 1, output);
			fwrite(entry, recordLen, 1, output);
			free(entry);
		}

		void writeToJSONFile(FILE* output, const char* indent) {
			fprintf(output, "%s{\n", indent);

			char* indentBuffer = (char*) malloc( sizeof(char) * 256 );
			sprintf(indentBuffer, "%s    ", indent);

			fprintf(output, "%s\"kernel-name\"    : \"%s\",\n", indentBuffer, kernelName);
			// fprintf(output, "%s\"region\"         : \"%s\",\n", indentBuffer, regionName);
			fprintf(output, "%s\"call-count\"     : %lu,\n", indentBuffer, callCount);
			fprintf(output, "%s\"total-time\"     : %f,\n", indentBuffer, time);
			fprintf(output, "%s\"time-per-call\"  : %16.8f,\n", indentBuffer, (time /
				static_cast<double>(std::max(
					static_cast<uint64_t>(1), callCount))));
			fprintf(output, "%s\"kernel-type\"    : \"%s\"\n", indentBuffer,
				(kType == PARALLEL_FOR) ? "PARALLEL-FOR" :
				(kType == PARALLEL_REDUCE) ? "PARALLEL-REDUCE" : "PARALLEL-SCAN");

			fprintf(output, "%s}", indent);
		}

	private:
		void copy(char* dest, const char* src, uint32_t len) {
			for(uint32_t i = 0; i < len; i++) {
				dest[i] = src[i];
			}
		}

		char* kernelName;
		// const char* regionName;
		uint64_t callCount;
		double time;
		double timeSq;
		double startTime;
		KernelExecutionType kType;
};

} // namespace KokkosTools::KernelTimer

#endif
