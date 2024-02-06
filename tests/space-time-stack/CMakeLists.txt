kp_add_executable_and_test(
    TARGET_NAME       test_space_time_stack_demangling
    SOURCE_FILE       test_demangling.cpp
    KOKKOS_TOOLS_LIBS kp_space_time_stack
)
