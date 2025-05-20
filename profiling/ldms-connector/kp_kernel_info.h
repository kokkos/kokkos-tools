
#ifndef _H_KOKKOSP_KERNEL_INFO
#define _H_KOKKOSP_KERNEL_INFO

#include <stdio.h>
#include <string>
#include <cstring>
#include <unistd.h>

#if defined(__GXX_ABI_VERSION)
#define HAVE_GCC_ABI_DEMANGLE
#endif

#if defined(HAVE_GCC_ABI_DEMANGLE)
#include <cxxabi.h>
#endif // HAVE_GCC_ABI_DEMANGLE

#include "kp_kernel_timer.h"

#include <ldms/ldms.h>
#include <ldms/ldmsd_stream.h>
#include <ovis_util/util.h>

char* demangleName(char* kernelName)
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

enum KernelExecutionType {
	PARALLEL_FOR = 0,
	PARALLEL_REDUCE = 1,
	PARALLEL_SCAN = 2,
        REGION = 3
};

static uint64_t kernel_ex = 0;
static double total_time = 0;

class KernelPerformanceInfo {
	public:

		KernelPerformanceInfo(std::string kName, KernelExecutionType kernelType, ldms_t* the_ldms,
				const char* node_name,
				const int rank_no,
				const int job_id,
				const double job_start,
				const uint64_t job_epoch_start,
				const uint16_t kernel_nest_level,
				const int tool_verbosity,
				bool* ldms_global_publish):
			kType(kernelType), ldms(the_ldms),
				nodename(node_name), rank(rank_no),
				jobid(job_id), jobStartTime(job_start),
				jobStartEpochTimeMS(job_epoch_start),
				nestingLevel(kernel_nest_level),
				verbosity(tool_verbosity),
				ldms_publish(ldms_global_publish) {

			kernelName = (char*) malloc(sizeof(char) * (kName.size() + 1));
			strcpy(kernelName, kName.c_str());

			callCount = 0;

			const char* tool_sample_rate = getenv("KOKKOS_SAMPLER_RATE");
			kernelSampleRate = 0;

 			if (NULL != tool_sample_rate) {
				kernelSampleRate = atoi(tool_sample_rate);
 			} else {
 				kernelSampleRate = 1;
			}
		}

		~KernelPerformanceInfo() {
			free(kernelName);
		}

		KernelExecutionType getKernelType() {
			return kType;
		}

		void incrementCount() {
			callCount++;
			kernel_ex++;
		}

		void addTime(double t) {
			timeSq += (t*t);
			total_time += t;
		}

		void addFromTimer() {
			const double now = seconds();
			const double sample_time = now - startTime;
			addTime(sample_time);
			incrementCount();

			if( (*ldms_publish) ) {
				const int buffer_size = (NULL == kernelName) ? 4096 :
					( strlen(kernelName) > 3072 ? 2048 + strlen(kernelName) : 4096 );

				char* big_buffer = (char*) malloc( sizeof(char) * buffer_size );

				double epoch_stamp = (double) jobStartEpochTimeMS;
				epoch_stamp += static_cast<double>( now - jobStartTime ) * 1000.0;
				epoch_stamp = epoch_stamp / 1000.0;

				snprintf( big_buffer, buffer_size, "{ \"job-id\" : %d, \"node-name\" : \"%s\", \"rank\" : %d, \"timestamp\" : \"%.6f\", \"kokkos-perf-data\" : [ { \"name\" : \"%s\", \"type\" : %d, \"current-kernel-count\" : %llu, \"total-kernel-count\" : %llu, \"level\" : %u, \"current-kernel-time\" : %.9f, \"total-kernel-time\" : %.9f } ] }\n",
					jobid, nodename, rank, epoch_stamp,
					(NULL==kernelName) ? "" : kernelName,
					(int) kType, callCount, kernel_ex * kernelSampleRate, nestingLevel, sample_time, total_time );

				if( verbosity > 0 ) {
					printf("%s", big_buffer);
				}

                                int rc = ldmsd_stream_publish( (*ldms), "kokkos-perf-data", LDMSD_STREAM_JSON,
					big_buffer, strlen(big_buffer) + 1);

				//int rc = ldmsd_stream_publish( (*ldms), "kokkos-perf-data", LDMSD_STREAM_JSON,                                                                                                                big_buffer, strlen(big_buffer) + 1);
				// always check your return codes :p
				free( big_buffer );
			}
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


	private:
		char* kernelName;
		uint64_t callCount;
		double time;
		double timeSq;
		double startTime;
		double jobStartTime;

		uint64_t kernelSampleRate;

		KernelExecutionType kType;
		ldms_t* ldms;

		bool* ldms_publish;
		const uint16_t nestingLevel;
		const char* nodename;
		const int rank;
		const int jobid;
		const int verbosity;
		const uint64_t jobStartEpochTimeMS;
};

#endif
