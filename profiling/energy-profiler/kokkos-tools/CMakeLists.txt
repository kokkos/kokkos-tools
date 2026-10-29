# Energy Profiler Infrastructure - Kokkos Tools with Variorum Support
# Basic kernel timer tool and Variorum-based energy monitoring tools

# Find Threads package for pthread support (needed by std::thread in daemon.cpp)
find_package(Threads REQUIRED)

# Basic kernel timer tool (no energy monitoring dependencies)
kp_add_library(kp_energy_kernel_timer kp_energy_kernel_timer.cpp
    ../common/timer_system.cpp
    ../common/filename_prefix.cpp
)

target_include_directories(kp_energy_kernel_timer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Add verbose output option
if(ENABLE_ENERGY_PROFILER_VERBOSE)
    target_compile_definitions(kp_energy_kernel_timer PRIVATE ENABLE_VERBOSE_OUTPUT)
endif()

# Variorum-based tools
if(KOKKOSTOOLS_HAS_VARIORUM)
    message(STATUS "Found Variorum, making Variorum power profiler available.")
    
    kp_add_library(kp_variorum_power kp_variorum_power.cpp
        ../common/daemon.cpp
        ../common/filename_prefix.cpp
        ../common/timer_system.cpp
        ../provider/provider_variorum.cpp
    )

    target_link_libraries(kp_variorum_power PRIVATE variorum::variorum Threads::Threads)
    
    target_include_directories(kp_variorum_power PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
    )
else()
    message(STATUS "Variorum not found, skipping Variorum power profiler.")
endif()