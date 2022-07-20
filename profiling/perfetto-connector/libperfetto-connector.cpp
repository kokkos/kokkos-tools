
#include "perfetto.h"
#include <unistd.h>
#include <fcntl.h>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <ios>
#include <sstream>
#include <string>
#include <type_traits>
#include <string_view>
#include <vector>

#define KOKKOSP_PUBLIC_API __attribute__((visibility("default")))

extern "C" {
void kokkosp_init_library(const int loadSeq, const uint64_t interfaceVer,
                          const uint32_t, void*) KOKKOSP_PUBLIC_API;
void kokkosp_finalize_library() KOKKOSP_PUBLIC_API;
void kokkosp_begin_parallel_for(const char* name, const uint32_t,
                                uint64_t*) KOKKOSP_PUBLIC_API;
void kokkosp_end_parallel_for(const uint64_t) KOKKOSP_PUBLIC_API;
void kokkosp_begin_parallel_scan(const char* name, const uint32_t,
                                 uint64_t*) KOKKOSP_PUBLIC_API;
void kokkosp_end_parallel_scan(const uint64_t) KOKKOSP_PUBLIC_API;
void kokkosp_begin_parallel_reduce(const char* name, const uint32_t,
                                   uint64_t*) KOKKOSP_PUBLIC_API;
void kokkosp_end_parallel_reduce(const uint64_t) KOKKOSP_PUBLIC_API;
void kokkosp_push_profile_region(const char* regionName) KOKKOSP_PUBLIC_API;
void kokkosp_pop_profile_region() KOKKOSP_PUBLIC_API;
}

PERFETTO_DEFINE_CATEGORIES(
    perfetto::Category("kokkos.kernels").SetDescription("Kokkos Kernel Events"),
    perfetto::Category("kokkos.regions")
        .SetDescription("Kokkos Region Events"));
PERFETTO_TRACK_EVENT_STATIC_STORAGE();

namespace {
auto tracing_session = std::unique_ptr<perfetto::TracingSession>{};

template <typename Tp>
auto get_env(std::string_view _env_name, Tp _default) {
  const char* _env_val = std::getenv(_env_name.data());

  if (_env_val) {
    std::stringstream _ss{};
    _ss << std::skipws << _env_val;
    if constexpr (std::is_same<std::decay_t<Tp>, const char*>::value) {
      std::stringstream _ss{};
      _ss << _env_val;
      return _ss.str();
    } else {
      Tp _val{};
      std::istringstream{_env_val} >> _val;
      return _val;
    }
  }

  if constexpr (std::is_same<std::decay_t<Tp>, const char*>::value) {
    return std::string{_default};
  } else {
    return _default;
  }
}
}  // namespace

extern "C" {

void kokkosp_init_library(const int loadSeq, const uint64_t interfaceVer,
                          const uint32_t, void*) {
  printf("-----------------------------------------------------------\n");
  printf("KokkosP: Perfetto Connector (sequence is %d, version: %lu)\n",
         loadSeq, interfaceVer);
  printf("-----------------------------------------------------------\n");

  perfetto::TracingInitArgs args{};
  perfetto::TraceConfig cfg{};
  perfetto::protos::gen::TrackEventConfig track_event_cfg{};

  // default 10 MB shared memory and 1 GB buffer
  auto shmem_size_hint = get_env("KOKKOSP_PERFETTO_SHMEM_SIZE_HINT_KB", 10000);
  auto buffer_size = get_env("KOKKOSP_PERFETTO_BUFFER_SIZE_KB", 1000 * 1000);
  auto _fill_policy_name = get_env("KOKKOSP_PERFETTO_FILL_POLICY", "discard");

  if (std::getenv("KOKKOSP_PERFETTO_INPROCESS_BACKEND") &&
      std::stoi(std::getenv("KOKKOSP_PERFETTO_INPROCESS_BACKEND")) != 0)
    args.backends |= perfetto::kInProcessBackend;
  if (std::getenv("KOKKOSP_PERFETTO_SYSTEM_BACKEND") &&
      std::stoi(std::getenv("KOKKOSP_PERFETTO_SYSTEM_BACKEND")) != 0)
    args.backends |= perfetto::kInProcessBackend;

  if (args.backends == 0) args.backends |= perfetto::kInProcessBackend;

  auto _fill_policy =
      _fill_policy_name == "discard"
          ? perfetto::protos::gen::TraceConfig_BufferConfig_FillPolicy_DISCARD
          : perfetto::protos::gen::
                TraceConfig_BufferConfig_FillPolicy_RING_BUFFER;

  auto* buffer_config = cfg.add_buffers();
  buffer_config->set_size_kb(buffer_size);
  buffer_config->set_fill_policy(_fill_policy);

  auto* ds_cfg = cfg.add_data_sources()->mutable_config();
  ds_cfg->set_name("track_event");  // this MUST be track_event
  ds_cfg->set_track_event_config_raw(track_event_cfg.SerializeAsString());

  args.shmem_size_hint_kb = shmem_size_hint;

  perfetto::Tracing::Initialize(args);
  perfetto::TrackEvent::Register();

  tracing_session = perfetto::Tracing::NewTrace();
  tracing_session->Setup(cfg);
  tracing_session->StartBlocking();
}

void kokkosp_finalize_library() {
  printf("-----------------------------------------------------------\n");
  printf("KokkosP: Finalizing Perfetto Connector... ");
  fflush(stdout);

  perfetto::TrackEvent::Flush();
  tracing_session->FlushBlocking();
  tracing_session->StopBlocking();

  printf("Reading trace data... ");
  fflush(stdout);

  std::vector<char> _trace_data{tracing_session->ReadTraceBlocking()};

  if (!_trace_data.empty()) {
    auto _fname =
        get_env("KOKKOSP_PERFETTO_OUTPUT_FILE", "kokkosp-perfetto.pftrace");

    std::ofstream _ofs{_fname};
    if (_ofs) {
      printf("Writing '%s'... ", _fname.c_str());
      fflush(stdout);
      _ofs.write(&_trace_data[0], _trace_data.size());
    } else
      fprintf(stderr, "Error opening %s!\n", _fname.c_str());
  }

  printf("Done\n");
  printf("-----------------------------------------------------------\n");
  fflush(stdout);
}

void kokkosp_begin_parallel_for(const char* name, const uint32_t, uint64_t*) {
  TRACE_EVENT_BEGIN("kokkos.kernels", perfetto::StaticString(name));
}

void kokkosp_end_parallel_for(const uint64_t) {
  TRACE_EVENT_END("kokkos.kernels");
}

void kokkosp_begin_parallel_scan(const char* name, const uint32_t, uint64_t*) {
  TRACE_EVENT_BEGIN("kokkos.kernels", perfetto::StaticString(name));
}

void kokkosp_end_parallel_scan(const uint64_t) {
  TRACE_EVENT_END("kokkos.kernels");
}

void kokkosp_begin_parallel_reduce(const char* name, const uint32_t,
                                   uint64_t*) {
  TRACE_EVENT_BEGIN("kokkos.kernels", perfetto::StaticString(name));
}

void kokkosp_end_parallel_reduce(const uint64_t) {
  TRACE_EVENT_END("kokkos.kernels");
}

void kokkosp_push_profile_region(const char* regionName) {
  TRACE_EVENT_BEGIN("kokkos.regions", perfetto::StaticString(regionName));
}

void kokkosp_pop_profile_region() { TRACE_EVENT_END("kokkos.regions"); }
}
