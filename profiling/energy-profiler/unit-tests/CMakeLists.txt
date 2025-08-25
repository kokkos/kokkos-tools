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
