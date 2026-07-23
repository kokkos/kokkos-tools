kp_add_executable_and_test(
    TARGET_NAME       test_vov_bug_finder
    SOURCE_FILE       test_view_of_views_bug_finder.cpp
    KOKKOS_TOOLS_LIBS kp_view_of_views_bug_finder
)
