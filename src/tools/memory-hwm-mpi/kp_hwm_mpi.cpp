#include <sys/resource.h>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cinttypes>
#include <mpi.h>

static int world_rank = 0;
static int world_size = 1;

extern "C" void kokkosp_init_library(const int loadSeq,
  const uint64_t interfaceVer,
  const uint32_t devInfoCount,
  void* deviceInfo) {
  int mpi_is_initialized;
  MPI_Initialized(&mpi_is_initialized);
  if (!mpi_is_initialized) {
    printf("KokkosP: you must initialize MPI first!\n");
    exit(-1);
  }

  MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
  MPI_Comm_size(MPI_COMM_WORLD, &world_size);

  if (world_rank == 0) {
    printf("KokkosP: Example Library Initialized (sequence is %d, version: %" PRIu64 ")\n", loadSeq, interfaceVer);
  }
}

extern "C" void kokkosp_finalize_library() {
  if (world_rank == 0) {
    printf("\n");
    printf("KokkosP: Finalization of profiling library.\n");
  }

  struct rusage sys_resources;
  getrusage(RUSAGE_SELF, &sys_resources);
  long hwm = sys_resources.ru_maxrss;

  // Max
  long hwm_max;
  MPI_Reduce(&hwm, &hwm_max, 1, MPI_LONG, MPI_MAX, 0, MPI_COMM_WORLD);

  // Min
  long hwm_min;
  MPI_Reduce(&hwm, &hwm_min, 1, MPI_LONG, MPI_MIN, 0, MPI_COMM_WORLD);

  // Average
  long hwm_ave;
  MPI_Reduce(&hwm, &hwm_ave, 1, MPI_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
  hwm_ave /= world_size;

  if (world_rank == 0) {
    printf("KokkosP: High water mark memory consumption: %ld kB\n",
      hwm_max);
    printf("  Max: %ld, Min: %ld, Ave: %ld kB\n",
      hwm_max,hwm_min,hwm_ave);
    printf("\n");
  }
}
