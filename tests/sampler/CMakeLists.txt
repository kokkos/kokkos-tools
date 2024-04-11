
## tests for skip rate 5, no fencing selected

kp_add_executable_and_test(
    TARGET_NAME       test_sampling_parfor
    SOURCE_FILE       test_parfor.cpp
    KOKKOS_TOOLS_LIBS kp_kokkos_sampler kp_kernel_logger
    KOKKOS_TOOLS_SAMPLER_VERBOSE 2
    KOKKOS_TOOLS_SAMPLER_SKIP 5
)

kp_add_executable_and_test(
    TARGET_NAME       test_sampling_parscan
    SOURCE_FILE       test_parscan.cpp
    KOKKOS_TOOLS_LIBS kp_kokkos_sampler kp_kernel_logger
    KOKKOS_TOOLS_SAMPLER_VERBOSE 2
    KOKKOS_TOOLS_SAMPLER_SKIP 5
)

kp_add_executable_and_test(
    TARGET_NAME       test_sampling_parreduce
    SOURCE_FILE       test_parreduce.cpp
    KOKKOS_TOOLS_LIBS kp_kokkos_sampler kp_kernel_logger
    KOKKOS_TOOLS_SAMPLER_VERBOSE 2
    KOKKOS_TOOLS_SAMPLER_SKIP 5
)

## tests for probability of 51.6% (with skip rate 0), no fencing selected

kp_add_executable_and_test(
    TARGET_NAME       test_sampling_prob_parfor
    SOURCE_FILE       test_parfor_prob.cpp
    KOKKOS_TOOLS_LIBS kp_kokkos_sampler kp_kernel_logger
    KOKKOS_TOOLS_SAMPLER_VERBOSE 2
    KOKKOS_TOOLS_RANDOM_SEED 2
    KOKKOS_TOOLS_SAMPLER_PROB 51.6
)

kp_add_executable_and_test(
    TARGET_NAME       test_sampling_prob_parscan
    SOURCE_FILE       test_parscan_prob.cpp
    KOKKOS_TOOLS_LIBS kp_kokkos_sampler kp_kernel_logger
    KOKKOS_TOOLS_SAMPLER_VERBOSE 2
    KOKKOS_TOOLS_RANDOM_SEED 2
    KOKKOS_TOOLS_SAMPLER_PROB 51.6
)

kp_add_executable_and_test(
    TARGET_NAME       test_sampling_prob_parreduce
    SOURCE_FILE       test_parreduce_prob.cpp
    KOKKOS_TOOLS_LIBS kp_kokkos_sampler kp_kernel_logger
    KOKKOS_TOOLS_SAMPLER_VERBOSE 2
    KOKKOS_TOOLS_RANDOM_SEED 2
    KOKKOS_TOOLS_SAMPLER_PROB 51.6
)

## tests for probability of 51.6% (with skip rate 0), with fences

kp_add_executable_and_test(
    TARGET_NAME       test_sampling_prob_parfor_fence
    SOURCE_FILE       test_parfor_prob.cpp
    KOKKOS_TOOLS_LIBS kp_kokkos_sampler kp_kernel_logger
    KOKKOS_TOOLS_SAMPLER_VERBOSE 2
    KOKKOS_TOOLS_RANDOM_SEED 2
    KOKKOS_TOOLS_SAMPLER_PROB 51.6
    KOKKOS_TOOLS_GLOBALFENCES 1
)

kp_add_executable_and_test(
    TARGET_NAME       test_sampling_prob_parscan_fence
    SOURCE_FILE       test_parscan_prob.cpp
    KOKKOS_TOOLS_LIBS kp_kokkos_sampler kp_kernel_logger
    KOKKOS_TOOLS_SAMPLER_VERBOSE 2
    KOKKOS_TOOLS_RANDOM_SEED 2
    KOKKOS_TOOLS_SAMPLER_PROB 51.6
    KOKKOS_TOOLS_GLOBALFENCES 1
)

kp_add_executable_and_test(
    TARGET_NAME       test_sampling_prob_parreduce_fence
    SOURCE_FILE       test_parreduce_prob.cpp
    KOKKOS_TOOLS_LIBS kp_kokkos_sampler kp_kernel_logger
    KOKKOS_TOOLS_SAMPLER_VERBOSE 2
    KOKKOS_TOOLS_RANDOM_SEED 2
    KOKKOS_TOOLS_SAMPLER_PROB 51.6
    KOKKOS_TOOLS_GLOBALFENCES 1
)
