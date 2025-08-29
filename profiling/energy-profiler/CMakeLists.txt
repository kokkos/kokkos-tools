kp_add_library(kp_energy_profiler
    kp_energy_profiler.cpp
    timing_utils.cpp
    timing_export.cpp
    nvml_provider.cpp
    power_sampler.cpp
    daemon.cpp
)

# Find NVML for power monitoring
set(MIN_CUDA_VERSION 12.6)
find_package(CUDAToolkit ${MIN_CUDA_VERSION} QUIET)
if (CUDAToolkit_FOUND)
    find_package(CUDA::nvml QUIET)
    if(TARGET CUDA::nvml)
        message(STATUS "Found CUDA NVML (version ${CUDAToolkit_VERSION}), enabling power profiling in energy profiler.")
        target_link_libraries(kp_energy_profiler PRIVATE CUDA::nvml)
        target_compile_definitions(kp_energy_profiler PRIVATE KOKKOS_ENERGY_PROFILER_HAS_NVML)
    else()
        message(STATUS "CUDA::nvml target not found, power profiling disabled in energy profiler.")
    endif()
else()
    message(STATUS "CUDAToolkit ${MIN_CUDA_VERSION} or higher not found, power profiling disabled in energy profiler.")
endif()
