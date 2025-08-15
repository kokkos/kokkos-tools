# Energy Profiler Infrastructure - Kokkos Tools
# Basic kernel timer tool using the timing infrastructure

# Basic kernel timer tool (no energy monitoring dependencies)
kp_add_library(kp_energy_kernel_timer kp_energy_kernel_timer.cpp
    ../common/timer_system.cpp
    ../common/filename_prefix.cpp
)

target_include_directories(kp_energy_kernel_timer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Add verbose output option
if(KOKKOS_TOOLS_ENABLE_ENERGY_PROFILER_VERBOSE)
    target_compile_definitions(kp_energy_kernel_timer PRIVATE KOKKOS_TOOLS_ENABLE_VERBOSE_OUTPUT)
endif()