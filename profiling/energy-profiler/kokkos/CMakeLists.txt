# Find Threads package for pthread support (needed by std::thread in daemon.cpp)
find_package(Threads REQUIRED)

kp_add_library(kp_energy_kernel_timer kp_energy_kernel_timer.cpp
    ../common/timer.cpp
    ../common/filename_prefix.cpp
    ../tools/kernel_timer_tool.cpp
)

target_include_directories(kp_energy_kernel_timer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

if(KOKKOSTOOLS_HAS_VARIORUM)
    kp_add_library(kp_variorum_power kp_variorum_power.cpp
    ../common/daemon.cpp
    ../common/filename_prefix.cpp
    ../common/timer.cpp
    ../provider/provider_variorum.cpp
    ../tools/kernel_timer_tool.cpp
)

# Find Threads package for pthread support (needed by std::thread in daemon.cpp)
find_package(Threads REQUIRED)
target_link_libraries(kp_variorum_power PRIVATE variorum::variorum Threads::Threads)
endif()

find_package(CUDAToolkit QUIET)

if (CUDAToolkit_FOUND)
    find_package(CUDA::nvml QUIET)
    message(STATUS "Found CUDA NVML, making NVML power profiler available.")
else()
    message(STATUS "CUDAToolkit not found, skipping NVML power profiler.")
    return()
endif()

kp_add_library(kp_nvml_power kp_nvml_power.cpp
    ../common/daemon.cpp
    ../common/filename_prefix.cpp
    ../common/timer.cpp
    ../provider/provider_nvml.cpp
    ../tools/kernel_timer_tool.cpp
)
target_link_libraries(kp_nvml_power PRIVATE CUDA::nvml Threads::Threads)

target_include_directories(kp_nvml_power PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# NVML Direct Power Tool
kp_add_library(kp_nvml_direct_power kp_nvml_direct_power.cpp
    ../common/daemon.cpp
    ../common/filename_prefix.cpp
    ../common/timer.cpp
    ../provider/provider_nvml.cpp
    ../tools/kernel_timer_tool.cpp
)
target_link_libraries(kp_nvml_direct_power PRIVATE CUDA::nvml Threads::Threads)

target_include_directories(kp_nvml_direct_power PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# NVML Energy Consumption Tool (no daemon needed)
kp_add_library(kp_nvml_energy_consumption kp_nvml_energy_consumption.cpp
    ../common/filename_prefix.cpp
    ../common/timer.cpp
    ../provider/provider_nvml.cpp
    ../tools/kernel_timer_tool.cpp
)
target_link_libraries(kp_nvml_energy_consumption PRIVATE CUDA::nvml)

target_include_directories(kp_nvml_energy_consumption PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)