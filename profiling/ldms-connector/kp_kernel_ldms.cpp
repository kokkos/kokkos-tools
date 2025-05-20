#include <stdio.h>
#include <inttypes.h>
#include <execinfo.h>
#include <cstdlib>
#include <cstring>
#include <map>
#include <vector>
#include <algorithm>
#include <string>
#include <sys/time.h>
#include <iostream>
#include <unistd.h>
#include <limits.h>
#include "kp_kernel_timer.h"
#include "kp_kernel_info.h"
#include "kp_all.hpp"

#include <ldms/ldms.h>
#include <ldms/ldmsd_stream.h>
#include <ovis_util/util.h>

using namespace KokkosTools;

static uint64_t uniqID = 0;
static KernelPerformanceInfo* currentEntry;
static std::map<std::string, KernelPerformanceInfo*> count_map;
static double initTime;
static uint64_t initTimeEpochMS;
static char* outputDelimiter;
static int current_region_level = 0;
static KernelPerformanceInfo* regions[512];

static ldms_t ldms;
static bool ldms_publish;
static int slurm_rank;
static int slurm_job_id;
static int tool_verbosity;
static char hostname_kp[HOST_NAME_MAX];


void increment_counter(const char* name, KernelExecutionType kType) {
	std::string nameStr(name);

	if(count_map.find(name) == count_map.end()) {
		KernelPerformanceInfo* info = new KernelPerformanceInfo(nameStr, kType, &ldms, hostname_kp,
				slurm_rank, slurm_job_id, initTime, initTimeEpochMS,
				0, tool_verbosity, &ldms_publish);
		count_map.insert(std::pair<std::string, KernelPerformanceInfo*>(nameStr, info));

		currentEntry = info;
	} else {
		currentEntry = count_map[nameStr];
	}

	currentEntry->startTimer();
}

void increment_counter_region(const char* name, KernelExecutionType kType) {
	std::string nameStr(name);

	if(count_map.find(name) == count_map.end()) {
		KernelPerformanceInfo* info = new KernelPerformanceInfo(nameStr, kType, &ldms, hostname_kp,
				slurm_rank, slurm_job_id, initTime, initTimeEpochMS,
				0, tool_verbosity, &ldms_publish);
		count_map.insert(std::pair<std::string, KernelPerformanceInfo*>(nameStr, info));

		regions[current_region_level] = info;
	} else {
		regions[current_region_level] = count_map[nameStr];
	}

	regions[current_region_level]->startTimer();
	current_region_level++;
}

static void event_cb(ldms_t x, ldms_xprt_event_t e, void *cb_arg)
{
	switch (e->type) {
	case LDMS_XPRT_EVENT_CONNECTED:
		x->sem_rc = 0;
		ldms_publish = true;
		break;
	case LDMS_XPRT_EVENT_REJECTED:
		ldms_xprt_put(x);
		x->sem_rc = 200;
		ldms_publish = false;

		fprintf(stderr, "KokkosP: LDMS server has rejected a connection attempt for rank %d, will abort attempts to publish events. Slurm_ID = %d, Hostname = %s\n",
			slurm_rank,slurm_job_id,hostname_kp);

		break;
	case LDMS_XPRT_EVENT_DISCONNECTED:
		ldms_xprt_put(x);
		x->sem_rc = 300;
		ldms_publish = false;

		fprintf(stderr, "KokkosP: LDMS has disconncted from its connection for rank %d, will abort attempts to publish events. Slurm_ID = %d, Hostname = %s\n",
			slurm_rank,slurm_job_id,hostname_kp);

		break;
	case LDMS_XPRT_EVENT_ERROR:
		x->sem_rc = 400;
		ldms_publish = false;

		fprintf(stderr, "KokkosP: LDMS server reports an error for rank %d, will abort attempts to publish events. Slurm_ID = %d, Hostname = %s\n",
			slurm_rank,slurm_job_id,hostname_kp);
		break;
	case LDMS_XPRT_EVENT_RECV:
		int server_rc = ldmsd_stream_response(e);
		break;
	}
	sem_post(&x->sem);
}

extern "C" void kokkosp_init_library(const int loadSeq,
	const uint64_t interfaceVer,
	const uint32_t devInfoCount,
	void* deviceInfo) {

	ldms_publish = false;

	// initialize regions to 0s so we know if there is an object there
	memset(&regions[0], 0, 512 * sizeof(KernelPerformanceInfo*));

	const char* env_ldms_host    = getenv("KOKKOS_LDMS_HOST");
	const char* env_ldms_port    = getenv("KOKKOS_LDMS_PORT");

	const char* slurm_job_id_str = getenv("SLURM_JOB_ID");
	const char* openmpi_job_str  = getenv("OMPI_COMM_WORLD_RANK");
	const char* slurm_rank_str   = getenv("SLURM_PROCID");

	const char* tool_verbose_str = getenv("KOKKOS_LDMS_VERBOSE");
	const char* slurm_node_list_char = getenv("SLURM_JOB_NODELIST");


	slurm_job_id = slurm_job_id_str == NULL ? 0 : atoi( slurm_job_id_str );
	slurm_rank   = openmpi_job_str == NULL ? slurm_rank_str   == NULL ? 0 : atoi( slurm_rank_str ) : atoi( openmpi_job_str );


	gethostname(hostname_kp, HOST_NAME_MAX);


	const char* ldms_port = (env_ldms_port == NULL) ? "10411" : env_ldms_port;
	const char* ldms_host = (env_ldms_host == NULL) ? "localhost" : env_ldms_host;

	char* xprt = getenv("KOKKOS_LDMS_XPRT");
	if( NULL == xprt ) {
		xprt = "sock";
	}

	char* auth = getenv("KOKKOS_LDMS_AUTH");
	if( NULL == auth ) {
		auth = "munge";
	}

	if( NULL == tool_verbose_str ) {
		tool_verbosity = 0;
	} else {
		tool_verbosity = std::atoi(tool_verbose_str);
	}

	ldms = ldms_xprt_new_with_auth(xprt, NULL, auth, NULL);
	int ldms_rc = ldms_xprt_connect_by_name(ldms, ldms_host, ldms_port, event_cb, NULL);
	struct timespec ts;
	ts.tv_sec = time(NULL) + 1;
	ts.tv_nsec = 0;
	int rc = sem_timedwait(&ldms->sem, &ts);
	if (!rc) {
		if(ldms->sem_rc) {
			fprintf(stderr, "Error connecting to LDMS return-code: %d. Slurm_ID = %d, Rank = %d, Hostname = %s\n", ldms->sem_rc,slurm_job_id,slurm_rank,hostname_kp);
			return;
		}
	}
	else {
                rc=errno;
		fprintf(stderr, "Error connection timed-out, connect_by_name: %d, %s. Slurm_ID =  %d, Rank = %d, Hostname = %s\n", ldms_rc, STRERROR(rc),slurm_job_id,slurm_rank,hostname_kp);
		return;
	}

	printf("KokkosP: LDMS Connector Interface Initialized (sequence is %d, version: %llu, job: %d / rank: %d, LDMS: %s:%s)\n", loadSeq, interfaceVer,
		slurm_job_id, slurm_rank, ldms_host, ldms_port);

	initTime = seconds();
	initTimeEpochMS = getEpochMS();
}

extern "C" void kokkosp_finalize_library() {

}

extern "C" void kokkosp_begin_parallel_for(const char* name, const uint32_t devID, uint64_t* kID) {
	if( (NULL == name) || (strcmp("", name) == 0) ) {
		fprintf(stderr, "Error: kernel is empty\n");
		exit(-1);
	}
	increment_counter(name, PARALLEL_FOR);
}

extern "C" void kokkosp_end_parallel_for(const uint64_t kID) {
	currentEntry->addFromTimer();
}

extern "C" void kokkosp_begin_parallel_scan(const char* name, const uint32_t devID, uint64_t* kID) {
	if( (NULL == name) || (strcmp("", name) == 0) ) {
		fprintf(stderr, "Error: kernel is empty\n");
		exit(-1);
	}
	increment_counter(name, PARALLEL_SCAN);
}

extern "C" void kokkosp_end_parallel_scan(const uint64_t kID) {
	currentEntry->addFromTimer();
}

extern "C" void kokkosp_begin_parallel_reduce(const char* name, const uint32_t devID, uint64_t* kID) {
	if( (NULL == name) || (strcmp("", name) == 0) ) {
		fprintf(stderr, "Error: kernel is empty\n");
		exit(-1);
	}
	increment_counter(name, PARALLEL_REDUCE);
}

extern "C" void kokkosp_end_parallel_reduce(const uint64_t kID) {
	currentEntry->addFromTimer();
}

extern "C" void kokkosp_push_profile_region(char* regionName) {
	increment_counter_region(regionName, REGION);
}

extern "C" void kokkosp_pop_profile_region() {
	current_region_level--;

	// current_region_level is out of bounds, inform the user they
	// called popRegion too many times.
	if (current_region_level < 0) {
	   current_region_level = 0;
	   std::cerr << "WARNING:: Kokkos::Profiling::popRegion() called outside " <<
		   " of an actve region. Previous regions: ";

	  /* This code block will walk back through the non-null regions
	   * pointers and print the names.  This takes advantage of a slight
	   * issue with regions logic: we never actually delete the
	   * KernelPerformanceInfo objects.  If that ever changes this needs
	   * to be updated.
	   */
	   for (int i = 0; i < 5; i++) {
	      if (regions[i] != 0 ) {
		 std::cerr << (i == 0 ? " " : ";") << regions[i]->getName();
	      } else {
		 break;
	      }
	   }
	   std::cerr << "\n";
	} else {
	   // don't call addFromTimer if we are outside an active region
	   regions[current_region_level]->addFromTimer();
	}
}
