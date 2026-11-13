kp_add_executable_and_test(
    TARGET_NAME test_energy_profiler_daemon
    SOURCE_FILE test_daemon.cpp
)
target_link_libraries(test_energy_profiler_daemon PRIVATE kp_energy_profiler_daemon)
