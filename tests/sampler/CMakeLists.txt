kp_add_executable_and_test(
    TARGET_NAME       test_sampler_randomizedsampling
    SOURCE_FILE       test_randomized_sampling.cpp
    KOKKOS_TOOLS_LIBS kp_sampler
)
