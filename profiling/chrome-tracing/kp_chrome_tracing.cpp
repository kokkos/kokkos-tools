#include <algorithm>
#include <cassert>
#include <cinttypes>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <ios>
#include <iostream>
#include <memory>
#include <queue>
#include <set>
#include <sstream>
#include <string>
#include <sys/resource.h>
#include <unistd.h>

#include "kp_core.hpp"

#if USE_MPI
#include <mpi.h>
#endif

#include <chrono>

namespace KokkosTools {
namespace ChromeTracing {

enum Space {
  SPACE_HOST,
  SPACE_CUDA
};

Space get_space(SpaceHandle const& handle) {
  switch (handle.name[0]) {
    case 'H': return SPACE_HOST;
    case 'C': return SPACE_CUDA;
  }
  abort();
  return SPACE_HOST;
}

struct Now {
  typedef std::chrono::time_point<std::chrono::high_resolution_clock> Impl;
  Impl impl;
};

Now now() {
  Now t;
  t.impl = std::chrono::high_resolution_clock::now();
  return t;
}

uint64_t operator-(Now b, Now a) {
  return std::chrono::duration_cast<std::chrono::nanoseconds>(b.impl - a.impl).count();
}

enum StackKind {
  STACK_FOR,
  STACK_REDUCE,
  STACK_SCAN,
  STACK_REGION,
  STACK_COPY
};

std::ostream & operator<<(std::ostream & os, StackKind kind)
{
      switch (kind) {
        case STACK_FOR: os << "[for]"; break;
        case STACK_REDUCE: os << "[reduce]"; break;
        case STACK_SCAN: os << "[scan]"; break;
        case STACK_REGION: os << "[region]"; break;
        case STACK_COPY: os << "[copy]"; break;
      };
      return os;
}

struct StackNode {
  std::string name;
  StackKind kind;
  int rank;
  double total_runtime;
  Now start_time;
  Now base_time;
  StackNode(std::string &&name_in, StackKind kind_in, int r, Now b)
      : name(std::move(name_in)), kind(kind_in), rank(r), total_runtime(0.),
        start_time(now()), base_time(std::move(b)) {}

  void print(std::ostream &os) const {
    // {"name": "Asub", "cat": "PERF", "ph": "B", "pid": 22630, "tid": 22630, "ts": 829},
    os << "{\"name\": \"" << name
        << "\", \"cat\": \"" << kind
        << "\", \"ph\": \"X"
        << "\", \"ts\": \"" << start_time - base_time
        << "\", \"dur\": \"" << now() - start_time
        << "\", \"pid\": \"" << rank
        << "\", \"tid\": \"" << 0
        << "\", \"args\": {\"dummy\": 1}}\n";
  }
};

struct State {
  std::ofstream outfile;
  std::vector<StackNode> current_stack;
  Now my_base_time;
  int my_mpi_rank = -1;
  bool first = true;
  State()
  : my_base_time(now())
  {
    char* mpi_rank = getenv("OMPI_COMM_WORLD_RANK");

    char* hostname = (char*) malloc(sizeof(char) * 256);
    gethostname(hostname, 256);

    char* fileOutput = (char*) malloc(sizeof(char) * 256);
    sprintf(fileOutput, "%s-%d-%s.json", hostname, (int) getpid(),
      (NULL == mpi_rank) ? "0" : mpi_rank);
#if defined(USE_MPI) && USE_MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &my_mpi_rank);
#else
    my_mpi_rank = 0;
#endif

    free(hostname);
    outfile.open(fileOutput, std::ios::out);
    free(fileOutput);

    outfile << "[\n";
    current_stack.reserve(20);
  }

  ~State() {
    outfile << "]\n";
  }
  void begin_frame(const char *name, StackKind kind) {
    std::string name_str(name);
    current_stack.emplace_back(std::move(name_str), kind, my_mpi_rank, my_base_time);
  }
  void end_frame() {
    if (current_stack.empty()) {
      std::cerr << "Attempting to end root stack frame before Kokkos::finalize()\n";
      return;
    }

    if(!first) outfile << ",\n";
    first = false;

    current_stack.back().print(outfile);
    current_stack.pop_back();
  }

  std::uint64_t begin_kernel(const char *name, StackKind kind) {
    begin_frame(name, kind);
    return 0;
  }
  void end_kernel(std::uint64_t) {
    end_frame();
  }
  void push_region(const char *name) { begin_frame(name, STACK_REGION); }
  void pop_region() { end_frame(); }
  void begin_deep_copy(Space dst, const char *dst_name, const void *, Space src,
                       const char *src_name, const void *, std::uint64_t len) {
    std::string frame_name;
    frame_name += dst_name;
    frame_name += " SPACE ";
    frame_name += (dst == SPACE_HOST) ? 'H' : 'D';
    frame_name += " COPYFROM ";
    frame_name += src_name;
    frame_name += " SPACE ";
    frame_name += (src == SPACE_HOST) ? 'H' : 'D';
    frame_name += " LENGTH ";
    frame_name += std::to_string(len);
    frame_name += " ";
    begin_frame(frame_name.c_str(), STACK_COPY);
  }
  void end_deep_copy() { end_frame(); }
};

State *global_state = nullptr;

void kokkosp_init_library(int loadseq, uint64_t, uint32_t ndevinfos,
                                     Kokkos_Profiling_KokkosPDeviceInfo *devinfos) {
  (void)loadseq;
  (void)ndevinfos;
  (void)devinfos;
  global_state = new State();
}

void kokkosp_finalize_library() {
  delete global_state;
  global_state = nullptr;
}

void kokkosp_begin_parallel_for(const char *name,
                                           std::uint32_t devid,
                                           std::uint64_t *kernid) {
  (void)devid;
  *kernid = global_state->begin_kernel(name, STACK_FOR);
}

void kokkosp_begin_parallel_reduce(const char *name,
                                              std::uint32_t devid,
                                              std::uint64_t *kernid) {
  (void)devid;
  *kernid = global_state->begin_kernel(name, STACK_REDUCE);
}

void kokkosp_begin_parallel_scan(const char *name,
                                            std::uint32_t devid,
                                            std::uint64_t *kernid) {
  (void)devid;
  *kernid = global_state->begin_kernel(name, STACK_SCAN);
}

void kokkosp_end_parallel_for(std::uint64_t kernid) {
  global_state->end_kernel(kernid);
}

void kokkosp_end_parallel_reduce(std::uint64_t kernid) {
  global_state->end_kernel(kernid);
}

void kokkosp_end_parallel_scan(std::uint64_t kernid) {
  global_state->end_kernel(kernid);
}

void kokkosp_push_profile_region(const char *name) {
  global_state->push_region(name);
}

void kokkosp_pop_profile_region() { global_state->pop_region(); }

void kokkosp_begin_deep_copy(SpaceHandle dst_handle,
                                        const char *dst_name,
                                        const void *dst_ptr,
                                        SpaceHandle src_handle,
                                        const char *src_name,
                                        const void *src_ptr, uint64_t size) {
  auto dst_space = get_space(dst_handle);
  auto src_space = get_space(src_handle);
  global_state->begin_deep_copy(dst_space, dst_name, dst_ptr, src_space,
                                src_name, src_ptr, size);
}

void kokkosp_end_deep_copy() { global_state->end_deep_copy(); }

Kokkos::Tools::Experimental::EventSet get_event_set() {
    Kokkos::Tools::Experimental::EventSet my_event_set;
    memset(&my_event_set, 0, sizeof(my_event_set)); // zero any pointers not set here
    my_event_set.init = kokkosp_init_library;
    my_event_set.finalize = kokkosp_finalize_library;
    my_event_set.push_region = kokkosp_push_profile_region;
    my_event_set.pop_region = kokkosp_pop_profile_region;
    my_event_set.begin_parallel_for = kokkosp_begin_parallel_for;
    my_event_set.begin_parallel_reduce = kokkosp_begin_parallel_reduce;
    my_event_set.begin_parallel_scan = kokkosp_begin_parallel_scan;
    my_event_set.end_parallel_for = kokkosp_end_parallel_for;
    my_event_set.end_parallel_reduce = kokkosp_end_parallel_reduce;
    my_event_set.end_parallel_scan = kokkosp_end_parallel_scan;
    my_event_set.begin_deep_copy = kokkosp_begin_deep_copy;
    my_event_set.end_deep_copy = kokkosp_end_deep_copy;
    return my_event_set;
}

}} // namespace KokkosTools::ChromeTracing

extern "C" {

namespace impl = KokkosTools::ChromeTracing;

EXPOSE_INIT(impl::kokkosp_init_library)
EXPOSE_FINALIZE(impl::kokkosp_finalize_library)
EXPOSE_PUSH_REGION(impl::kokkosp_push_profile_region)
EXPOSE_POP_REGION(impl::kokkosp_pop_profile_region)
EXPOSE_BEGIN_PARALLEL_FOR(impl::kokkosp_begin_parallel_for)
EXPOSE_END_PARALLEL_FOR(impl::kokkosp_end_parallel_for)
EXPOSE_BEGIN_PARALLEL_SCAN(impl::kokkosp_begin_parallel_scan)
EXPOSE_END_PARALLEL_SCAN(impl::kokkosp_end_parallel_scan)
EXPOSE_BEGIN_PARALLEL_REDUCE(impl::kokkosp_begin_parallel_reduce)
EXPOSE_END_PARALLEL_REDUCE(impl::kokkosp_end_parallel_reduce)
EXPOSE_BEGIN_DEEP_COPY(impl::kokkosp_begin_deep_copy)
EXPOSE_END_DEEP_COPY(impl::kokkosp_end_deep_copy)

} // extern "C"