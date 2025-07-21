find_package(CUDAToolkit QUIET)

if (CUDAToolkit_FOUND)
    find_package(CUDA::nvml QUIET)
    message(STATUS "Found CUDA NVML, making NVML power profiler available.")
else()
    message(STATUS "CUDAToolkit not found, skipping NVML power profiler.")
    return()
endif()

kp_add_library(kp_energy_nvml kp_energy_nvml.cpp kp_nvml_energy_profiler.cpp)

target_link_libraries(kp_energy_nvml PRIVATE CUDA::nvml pthread)

target_include_directories(kp_energy_nvml PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)