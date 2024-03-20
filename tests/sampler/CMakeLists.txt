kp_add_executable_and_test(
    TARGET_NAME       test_sampler_randomizedsampling
    SOURCE_FILE       test_randomized_sampling.cpp
    KOKKOS_TOOLS_LIBS kp_kokkos_sampler
    KOKKOS_TOOLS_LIBS2 kp_kernel_logger
    KOKKOS_TOOLS_SAMPLER_VERBOSE 1
    KOKKOS_TOOLS_GLOBALFENCES 1
)
