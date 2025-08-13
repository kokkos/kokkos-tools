# Tests for Energy Profiler

enable_testing()

# Find Threads package for pthread support (needed by std::thread in daemon.cpp)
find_package(Threads REQUIRED)

# Daemon test
add_executable(daemon_test
    daemon_test.cpp
    ../common/daemon.cpp
)

target_include_directories(daemon_test PRIVATE
    ../common
)

target_link_libraries(daemon_test PRIVATE Threads::Threads)

add_test(NAME daemon_test COMMAND daemon_test)

# NVML Provider test
add_executable(nvml_provider_test
    nvml_provider_test.cpp
    ../provider/provider_nvml.cpp
)

target_include_directories(nvml_provider_test PRIVATE
    ../provider
)

# Find CUDA Toolkit and NVML for NVML tests
find_package(CUDAToolkit QUIET)
if(CUDAToolkit_FOUND)
    find_package(CUDA::nvml QUIET)
    if(TARGET CUDA::nvml)
        message(STATUS "Found CUDA NVML, making NVML tests available.")
        target_link_libraries(nvml_provider_test PRIVATE CUDA::nvml)
        add_test(NAME nvml_provider_test COMMAND nvml_provider_test) # Register test if NVML is found
    else()
        message(WARNING "CUDA::nvml target not found. NVML tests may not work properly.")
    endif()
else()
    message(STATUS "CUDAToolkit not found, NVML tests may not work properly.")
endif()

# Daemon NVML Integration test
add_executable(daemon_nvml_integration_test
    daemon_nvml_integration_test.cpp
    ../common/daemon.cpp
    ../provider/provider_nvml.cpp
)

target_include_directories(daemon_nvml_integration_test PRIVATE
    ../common
    ../provider
)

if(CUDAToolkit_FOUND AND TARGET CUDA::nvml)
    target_link_libraries(daemon_nvml_integration_test PRIVATE CUDA::nvml Threads::Threads)
    add_test(NAME daemon_nvml_integration_test COMMAND daemon_nvml_integration_test) # Register test if NVML is found
else()
    target_link_libraries(daemon_nvml_integration_test PRIVATE Threads::Threads)
endif()

# Fast Daemon NVML Integration test (20ms sampling)
add_executable(daemon_nvml_fast_test
    daemon_nvml_fast_test.cpp
    ../common/daemon.cpp
    ../provider/provider_nvml.cpp
)

target_include_directories(daemon_nvml_fast_test PRIVATE
    ../common
    ../provider
)

if(CUDAToolkit_FOUND AND TARGET CUDA::nvml)
    target_link_libraries(daemon_nvml_fast_test PRIVATE CUDA::nvml Threads::Threads)
    add_test(NAME daemon_nvml_fast_test COMMAND daemon_nvml_fast_test) # Register test if NVML is found
else()
    target_link_libraries(daemon_nvml_fast_test PRIVATE Threads::Threads)
endif()

# Variorum Provider test
add_executable(variorum_provider_test
    variorum_provider_test.cpp
    ../provider/provider_variorum.cpp
)

target_include_directories(variorum_provider_test PRIVATE
    ../provider
)

# Use Variorum configuration from root CMake
if(KOKKOSTOOLS_HAS_VARIORUM)
    message(STATUS "Using Variorum configuration from root CMake, making Variorum tests available.")
    target_link_libraries(variorum_provider_test PRIVATE variorum::variorum)
    add_test(NAME variorum_provider_test COMMAND variorum_provider_test) # Register test if Variorum is found
else()
    message(STATUS "Variorum not available from root CMake, Variorum tests may not work properly.")
endif()

# Daemon Variorum Integration test
add_executable(daemon_variorum_integration_test
    daemon_variorum_integration_test.cpp
    ../common/daemon.cpp
    ../provider/provider_variorum.cpp
)

target_include_directories(daemon_variorum_integration_test PRIVATE
    ../common
    ../provider
)

if(KOKKOSTOOLS_HAS_VARIORUM)
    target_link_libraries(daemon_variorum_integration_test PRIVATE variorum::variorum Threads::Threads)
    add_test(NAME daemon_variorum_integration_test COMMAND daemon_variorum_integration_test) # Register test if Variorum is found
else()
    target_link_libraries(daemon_variorum_integration_test PRIVATE Threads::Threads)
endif()

add_executable(daemon_variorum_fast_test
    daemon_variorum_fast_test.cpp
    ../common/daemon.cpp
    ../provider/provider_variorum.cpp
)

target_include_directories(daemon_variorum_fast_test PRIVATE
    ../common
    ../provider
)

if(KOKKOSTOOLS_HAS_VARIORUM)
    target_link_libraries(daemon_variorum_fast_test PRIVATE variorum::variorum Threads::Threads)
    add_test(NAME daemon_variorum_fast_test COMMAND daemon_variorum_fast_test) # Register test if Variorum is found
else()
    target_link_libraries(daemon_variorum_fast_test PRIVATE Threads::Threads)
endif()

add_executable(timer_test 
    timer_test.cpp
    ../common/timer.cpp
)

target_include_directories(timer_test PRIVATE
    ../common
    ../tools
)

add_test(NAME timer_test COMMAND timer_test)

# CSV export test
add_executable(csv_export_test
    csv_export_test.cpp
    ../common/timer.cpp
    ../common/filename_prefix.cpp
    ../tools/kernel_timer_tool.cpp
)

target_include_directories(csv_export_test PRIVATE
    ../common
    ../tools
)

add_test(NAME csv_export_test COMMAND csv_export_test)