kp_add_executable_and_test(
    TARGET_NAME       test_space_time_stack_demangling
    SOURCE_FILE       test_demangling.cpp
    KOKKOS_TOOLS_LIBS kp_space_time_stack
)
include(CheckCXXCompilerFlag)
check_cxx_compiler_flag("-Wrestrict" HAVE_RESTRICT_WARNING)
if(HAVE_RESTRICT_WARNING)
target_compile_options(test_space_time_stack_demangling PRIVATE "-Wno-restrict")
endif()

if(KokkosTools_ENABLE_MPI)
  kp_add_executable_and_test(
      TARGET_NAME       test_threshold_with_imbalance
      SOURCE_FILE       test_threshold_with_imbalance.cpp
      KOKKOS_TOOLS_LIBS kp_space_time_stack
      RANKS             2
  )
  target_link_libraries(test_threshold_with_imbalance PRIVATE MPI::MPI_CXX)
endif()
