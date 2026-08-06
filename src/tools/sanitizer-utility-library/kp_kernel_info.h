
#ifndef _H_KOKKOSP_KERNEL_INFO
#define _H_KOKKOSP_KERNEL_INFO

#include <stdio.h>
#include <sys/time.h>
#include <cstring>
#if defined(HAVE_GCC_ABI_DEMANGLE)
#include <cxxabi.h>
#endif
double seconds() {
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

			callCount = 0;
			time = 0;
		}

		~KernelPerformanceInfo() {
			free(kernelName);
		}

		KernelExecutionType getKernelType() {
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

		uint64_t getCallCount() {
			return callCount;
		}

		double getTime() {
			return time;
		}

		double getTimeSq() {
			return timeSq;
		}

		char* getName() {
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
#if defined(HAVE_GCC_ABI_DEMANGLE)
			{
				int status = -1;
				char* demangledKernelName = abi::__cxa_demangle(kernelName, NULL, NULL, &status);
				if (status==0) {
					free(kernelName);
					kernelName = demangledKernelName;
				}
			}
#endif // HAVE_GCC_ABI_DEMANGLE
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

		void writeToFile(FILE* output) {
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

	private:
		void copy(char* dest, const char* src, uint32_t len) {
			for(uint32_t i = 0; i < len; i++) {
				dest[i] = src[i];
			}
		}

		char* kernelName;
		uint64_t callCount;
		double time;
		double timeSq;
		double startTime;
		KernelExecutionType kType;
};

#endif
