## tests for simple kernel timer init/finalize with print_ascii output

kp_add_executable_and_test(
    TARGET_NAME       test_simple_kernel_timer
    SOURCE_FILE       test_simple_kernel_timer.cpp
    KOKKOS_TOOLS_LIBS kp_kernel_timer
)
