# Tests for Energy Profiler

enable_testing()

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
