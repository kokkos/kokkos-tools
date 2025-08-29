kp_add_library(kp_energy_profiler
    kp_energy_profiler.cpp
    timing_utils.cpp
    timing_export.cpp
    nvml_provider.cpp
    power_sampler.cpp
    daemon.cpp
)

target_link_libraries(kp_energy_profiler PRIVATE CUDA::nvml)
target_compile_definitions(kp_energy_profiler PRIVATE KOKKOS_ENERGY_PROFILER_HAS_NVML)
