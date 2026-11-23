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

# Variorum Provider test
if(KOKKOSTOOLS_HAS_VARIORUM)
    message(STATUS "Using Variorum configuration from root CMake, making Variorum tests available.")
    
    add_executable(variorum_provider_test
        variorum_provider_test.cpp
        ../provider/provider_variorum.cpp
    )

    target_include_directories(variorum_provider_test PRIVATE
        ../provider
    )

    target_link_libraries(variorum_provider_test PRIVATE variorum::variorum)
    add_test(NAME variorum_provider_test COMMAND variorum_provider_test)

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

    target_link_libraries(daemon_variorum_integration_test PRIVATE variorum::variorum Threads::Threads)
    add_test(NAME daemon_variorum_integration_test COMMAND daemon_variorum_integration_test)

    # Fast Daemon Variorum Integration test
    add_executable(daemon_variorum_fast_test
        daemon_variorum_fast_test.cpp
        ../common/daemon.cpp
        ../provider/provider_variorum.cpp
    )

    target_include_directories(daemon_variorum_fast_test PRIVATE
        ../common
        ../provider
    )

    target_link_libraries(daemon_variorum_fast_test PRIVATE variorum::variorum Threads::Threads)
    add_test(NAME daemon_variorum_fast_test COMMAND daemon_variorum_fast_test)
else()
    message(STATUS "Variorum not available from root CMake, Variorum tests will be skipped.")
endif()

add_executable(timer_test 
    timer_test.cpp
    ../common/timer_system.cpp
)

target_include_directories(timer_test PRIVATE
    ../common
)

add_test(NAME timer_test COMMAND timer_test)

# CSV export test
add_executable(csv_export_test
    csv_export_test.cpp
    ../common/timer_system.cpp
    ../common/filename_prefix.cpp
)

target_include_directories(csv_export_test PRIVATE
    ../common
)

add_test(NAME csv_export_test COMMAND csv_export_test)
