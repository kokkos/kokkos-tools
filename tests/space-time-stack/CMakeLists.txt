# A function to add such "simple" tests in 'tests/CMakeLists.txt' might be a good option.
add_executable(test_space_time_stack_demangling)
target_sources(
    test_space_time_stack_demangling
    PRIVATE
        test_demangling.cpp
)
target_link_libraries(
    test_space_time_stack_demangling
    PRIVATE
        Kokkos::kokkos kokkostools
)
add_test(
    NAME test_space_time_stack_demangling
    COMMAND $<TARGET_FILE:test_space_time_stack_demangling>
        --kokkos-tools-libs=$<TARGET_FILE:kp_space_time_stack>
        --kokkos-tools-args=1e-9
)
