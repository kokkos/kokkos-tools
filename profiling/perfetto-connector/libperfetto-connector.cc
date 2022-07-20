#include "perfetto.h"
#include <unistd.h>
#include <fcntl.h>
extern "C" void kokkosp_init_library(const int loadSeq,
                                     const uint64_t interfaceVer,
                                     const uint32_t,
                                     void*);
extern "C" void kokkosp_finalize_library();
extern "C" void kokkosp_begin_parallel_for(const char* name,
                                           const uint32_t,
                                           uint64_t*);
extern "C" void kokkosp_end_parallel_for(const uint64_t);
extern "C" void kokkosp_begin_parallel_scan(const char* name,
                                            const uint32_t,
                                            uint64_t*);
extern "C" void kokkosp_end_parallel_scan(const uint64_t);
extern "C" void kokkosp_begin_parallel_reduce(const char* name,
                                              const uint32_t,
                                              uint64_t*);
extern "C" void kokkosp_end_parallel_reduce(const uint64_t);
extern "C" void kokkosp_push_profile_region(
    const char* regionName);
extern "C" void kokkosp_pop_profile_region();


PERFETTO_DEFINE_CATEGORIES(
    perfetto::Category("kokkos.kernels")
        .SetDescription("Kokkos Kernel Events"),
    perfetto::Category("kokkos.regions").SetDescription(
        "Kokkos Region Events"));
PERFETTO_TRACK_EVENT_STATIC_STORAGE();
std::unique_ptr<perfetto::TracingSession> kokkosp_perfetto_tracing_session;
int fd;
extern "C" void kokkosp_init_library(const int loadSeq,
                                     const uint64_t interfaceVer,
                                     const uint32_t,
                                     void*) {
  printf("-----------------------------------------------------------\n");
  printf("KokkosP: Perfetto Connector (sequence is %d, version: %lu)\n",
         loadSeq, interfaceVer);
  printf("-----------------------------------------------------------\n");
  fd = open("example.perfetto-trace", O_RDWR | O_CREAT | O_TRUNC, 0600);
  perfetto::TracingInitArgs args;

  args.backends |= perfetto::kInProcessBackend;

  perfetto::Tracing::Initialize(args);
  perfetto::TrackEvent::Register();
  
  perfetto::TraceConfig cfg;
  cfg.add_buffers()->set_size_kb(1024);  // Record up to 1 MiB.
  auto* ds_cfg = cfg.add_data_sources()->mutable_config();
  perfetto::protos::gen::TrackEventConfig track_event_cfg;
  //track_event_cfg.add_disabled_categories("*");
  track_event_cfg.add_enabled_categories("kokkos.kernels");
  track_event_cfg.add_enabled_categories("kokkos.regions");
  ds_cfg->set_name("track_event");
  ds_cfg->set_track_event_config_raw(track_event_cfg.SerializeAsString());
  kokkosp_perfetto_tracing_session = perfetto::Tracing::NewTrace();
  kokkosp_perfetto_tracing_session->Setup(cfg, fd);
  kokkosp_perfetto_tracing_session->StartBlocking();
}

extern "C" void kokkosp_finalize_library() {
  printf("-----------------------------------------------------------\n");
  printf("KokkosP: Finalization of Perfetto Connector. Complete.\n");
  printf("-----------------------------------------------------------\n");
  kokkosp_perfetto_tracing_session->StopBlocking();
  close(fd);
}

extern "C" void kokkosp_begin_parallel_for(const char* name,
                                           const uint32_t,
                                           uint64_t*) {
  TRACE_EVENT_BEGIN("kokkos.kernels", perfetto::StaticString(name));
}

extern "C" void kokkosp_end_parallel_for(const uint64_t) {
  TRACE_EVENT_END("kokkos.kernels");
}

extern "C" void kokkosp_begin_parallel_scan(const char* name,
                                            const uint32_t,
                                            uint64_t*) {
  TRACE_EVENT_BEGIN("kokkos.kernels", perfetto::StaticString(name));
}

extern "C" void kokkosp_end_parallel_scan(const uint64_t) {
  TRACE_EVENT_END("kokkos.kernels");
}

extern "C" void kokkosp_begin_parallel_reduce(const char* name,
                                              const uint32_t,
                                              uint64_t*) {
  TRACE_EVENT_BEGIN("kokkos.kernels", perfetto::StaticString(name));
}

extern "C" void kokkosp_end_parallel_reduce(const uint64_t) {
  TRACE_EVENT_END("kokkos.kernels");
}

extern "C" void kokkosp_push_profile_region(const char* regionName) {
  TRACE_EVENT_BEGIN("kokkos.regions", perfetto::StaticString(regionName));
}

extern "C" void kokkosp_pop_profile_region() {
  TRACE_EVENT_END("kokkos.regions");
}
