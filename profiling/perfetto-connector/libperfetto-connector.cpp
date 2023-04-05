
#include "perfetto.h"
#include <cstdlib>
#include <fcntl.h>
#include <fstream>
#include <ios>
#include <iostream>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <unistd.h>
#include <vector>

#include "kp_core.hpp"



PERFETTO_DEFINE_CATEGORIES(
    perfetto::Category("kokkos.parallel_for")
        .SetDescription("Kokkos Parallel For Region"),
    perfetto::Category("kokkos.parallel_reduce")
        .SetDescription("Kokkos Parallel Reduce Region"),
    perfetto::Category("kokkos.parallel_scan")
        .SetDescription("Kokkos Parallel Scan Region"),
    perfetto::Category("kokkos.fence")
        .SetDescription("Kokkos Synchronization Fence"),
    perfetto::Category("kokkos.deep_copy")
        .SetDescription("Kokkos Deep Data Copy"),
    perfetto::Category("kokkos.allocate")
        .SetDescription("Kokkos Data Allocation"),
    perfetto::Category("kokkos.deallocate")
        .SetDescription("Kokkos Data Deallocation"),
    perfetto::Category("kokkos.profile_event")
        .SetDescription("Kokkos User-defined Profiling Event"),
    perfetto::Category("kokkos.dual_view_sync")
        .SetDescription("Kokkos Dual View Synchronization"),
    perfetto::Category("kokkos.dual_view_modify")
        .SetDescription("Kokkos Dual View Modification"),
    perfetto::Category("kokkos.region")
        .SetDescription("Kokkos User-defined Region"));
PERFETTO_TRACK_EVENT_STATIC_STORAGE();

namespace {
auto tracing_session = std::unique_ptr<perfetto::TracingSession>{};

template <typename Tp>
auto add_annotation(perfetto::EventContext &ctx, const char *_name, Tp &&_val) {
  auto *_dbg = ctx.event()->add_debug_annotations();
  _dbg->set_name(_name);

  using type = std::remove_const_t<std::decay_t<Tp>>;
  if constexpr (std::is_same<type, char *>::value)
    _dbg->set_string_value(_val);
  else if constexpr (std::is_integral<type>::value &&
                     std::is_unsigned<type>::value)
    _dbg->set_uint_value(_val);
  else if constexpr (std::is_pointer<type>::value)
    _dbg->set_pointer_value(reinterpret_cast<uint64_t>(_val));
  else
    static_assert(std::is_empty<type>::value, "Error! unsupported data type");
}

template <typename Tp> auto get_env(const std::string &_env_name, Tp _default) {
  const char *_env_val = std::getenv(_env_name.c_str());

  if (_env_val) {
    std::stringstream _ss{};
    _ss << std::skipws << _env_val;
    if constexpr (std::is_same<std::decay_t<Tp>, const char *>::value) {
      std::stringstream _ss{};
      _ss << _env_val;
      return _ss.str();
    } else {
      Tp _val{};
      std::istringstream{_env_val} >> _val;
      return _val;
    }
  }

  if constexpr (std::is_same<std::decay_t<Tp>, const char *>::value) {
    return std::string{_default};
  } else {
    return _default;
  }
}
} // namespace

namespace KokkosTools {
namespace PerfettoConnector {

void kokkosp_init_library(const int loadSeq, const uint64_t interfaceVer,
                          const uint32_t, void *) {
  std::cout << "-----------------------------------------------------------\n"
            << "KokkosP: Perfetto Connector (sequence is " << loadSeq
            << ", version: " << interfaceVer << ")\n"
            << "-----------------------------------------------------------\n";

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

  if (args.backends == 0)
    args.backends |= perfetto::kInProcessBackend;

  auto _fill_policy =
      _fill_policy_name == "discard"
          ? perfetto::protos::gen::TraceConfig_BufferConfig_FillPolicy_DISCARD
          : perfetto::protos::gen::
                TraceConfig_BufferConfig_FillPolicy_RING_BUFFER;

  auto *buffer_config = cfg.add_buffers();
  buffer_config->set_size_kb(buffer_size);
  buffer_config->set_fill_policy(_fill_policy);

  auto *ds_cfg = cfg.add_data_sources()->mutable_config();
  ds_cfg->set_name("track_event"); // this MUST be track_event
  ds_cfg->set_track_event_config_raw(track_event_cfg.SerializeAsString());

  args.shmem_size_hint_kb = shmem_size_hint;

  perfetto::Tracing::Initialize(args);
  perfetto::TrackEvent::Register();

  tracing_session = perfetto::Tracing::NewTrace();
  tracing_session->Setup(cfg);
  tracing_session->StartBlocking();
}

void kokkosp_finalize_library() {
  std::cout << "-----------------------------------------------------------\n"
            << "KokkosP: Finalizing Perfetto Connector... " << std::endl;

  perfetto::TrackEvent::Flush();
  tracing_session->FlushBlocking();
  tracing_session->StopBlocking();

  std::cout << "Reading trace data... " << std::flush;

  std::vector<char> _trace_data{tracing_session->ReadTraceBlocking()};

  if (!_trace_data.empty()) {
    auto _fname =
        get_env("KOKKOSP_PERFETTO_OUTPUT_FILE", "kokkosp-perfetto.pftrace");

    std::ofstream _ofs{_fname};
    if (_ofs) {
      std::cout << "Writing '" << _fname << "'... " << std::flush;
      _ofs.write(&_trace_data[0], _trace_data.size());
    } else
      std::cerr << "Error opening " << _fname << std::endl;
  }

  std::cout << "Done\n"
            << "-----------------------------------------------------------"
            << std::endl;
}

void kokkosp_begin_parallel_for(const char *name, const uint32_t dev_id,
                                uint64_t *) {
  TRACE_EVENT_BEGIN("kokkos.parallel_for", perfetto::StaticString{name},
                    "device", dev_id);
}

void kokkosp_end_parallel_for(const uint64_t) {
  TRACE_EVENT_END("kokkos.parallel_for");
}

void kokkosp_begin_parallel_scan(const char *name, const uint32_t dev_id,
                                 uint64_t *) {
  TRACE_EVENT_BEGIN("kokkos.parallel_scan", perfetto::StaticString{name},
                    "device", dev_id);
}

void kokkosp_end_parallel_scan(const uint64_t) {
  TRACE_EVENT_END("kokkos.parallel_scan");
}

void kokkosp_begin_parallel_reduce(const char *name, const uint32_t dev_id,
                                   uint64_t *) {
  TRACE_EVENT_BEGIN("kokkos.parallel_reduce", perfetto::StaticString{name},
                    "device", dev_id);
}

void kokkosp_end_parallel_reduce(const uint64_t) {
  TRACE_EVENT_END("kokkos.parallel_reduce");
}

void kokkosp_push_profile_region(const char *name) {
  TRACE_EVENT_BEGIN("kokkos.region", perfetto::StaticString{name});
}

void kokkosp_pop_profile_region() { TRACE_EVENT_END("kokkos.region"); }

void kokkosp_begin_deep_copy(SpaceHandle dst_handle, const char *dst_name,
                             const void *dst_ptr, SpaceHandle src_handle,
                             const char *src_name, const void *src_ptr,
                             uint64_t size) {
  TRACE_EVENT_BEGIN("kokkos.deep_copy", nullptr,
                    [&](perfetto::EventContext ctx) {
                      std::stringstream _name;
                      _name << dst_name << "->" << src_name;
                      ctx.event()->set_name(_name.str());
                      add_annotation(ctx, "size", size);
                      add_annotation(ctx, "dest_type", dst_handle.name);
                      add_annotation(ctx, "src_type", src_handle.name);
                      add_annotation(ctx, "dest_address", dst_ptr);
                      add_annotation(ctx, "src_address", src_ptr);
                    });
}

void kokkosp_end_deep_copy() { TRACE_EVENT_END("kokkos.deep_copy"); }

void kokkosp_allocate_data(const SpaceHandle space, const char *name,
                           const void *const, const uint64_t size) {
  TRACE_EVENT_INSTANT("kokkos.allocate", perfetto::StaticString{name}, "type",
                      space.name, "size", size);
}

void kokkosp_deallocate_data(const SpaceHandle space, const char *name,
                             const void *const, const uint64_t size) {
  TRACE_EVENT_INSTANT("kokkos.deallocate", perfetto::StaticString{name}, "type",
                      space.name, "size", size);
}

void kokkosp_profile_event(const char *name) {
  TRACE_EVENT_INSTANT("kokkos.profile_event", perfetto::StaticString{name});
}

void kokkosp_dual_view_sync(const char *name, const void *const ptr,
                            bool is_device) {
  TRACE_EVENT_INSTANT("kokkos.dual_view_sync", perfetto::StaticString{name},
                      "address", ptr, "is_device", is_device);
}

void kokkosp_dual_view_modify(const char *name, const void *const ptr,
                              bool is_device) {
  TRACE_EVENT_INSTANT("kokkos.dual_view_modify", perfetto::StaticString{name},
                      "address", ptr, "is_device", is_device);
}
} // namespace PerfettoConnector
} // namespace KokkosTools


extern "C" {

namespace impl = KokkosTools::PerfettoConnector;

EXPOSE_INIT(impl::kokkosp_init_library)
EXPOSE_FINALIZE(impl::kokkosp_finalize_library)
EXPOSE_PUSH_REGION(impl::kokkosp_push_profile_region)
EXPOSE_POP_REGION(impl::kokkosp_pop_profile_region)
EXPOSE_ALLOCATE(impl::kokkosp_allocate_data)
EXPOSE_DEALLOCATE(impl::kokkosp_deallocate_data)
EXPOSE_BEGIN_PARALLEL_FOR(impl::kokkosp_begin_parallel_for)
EXPOSE_END_PARALLEL_FOR(impl::kokkosp_end_parallel_for)
EXPOSE_BEGIN_PARALLEL_SCAN(impl::kokkosp_begin_parallel_scan)
EXPOSE_END_PARALLEL_SCAN(impl::kokkosp_end_parallel_scan)
EXPOSE_BEGIN_PARALLEL_REDUCE(impl::kokkosp_begin_parallel_reduce)
EXPOSE_END_PARALLEL_REDUCE(impl::kokkosp_end_parallel_reduce)
EXPOSE_PROFILE_EVENT(impl::kokkosp_profile_event)
}  // extern "C"
