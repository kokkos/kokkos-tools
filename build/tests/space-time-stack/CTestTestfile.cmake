# CMake generated Testfile for 
# Source directory: /home/runner/work/kokkos-tools/kokkos-tools/tests/space-time-stack
# Build directory: /home/runner/work/kokkos-tools/kokkos-tools/build/tests/space-time-stack
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(test_space_time_stack_demangling "/home/runner/work/kokkos-tools/kokkos-tools/build/tests/space-time-stack/test_space_time_stack_demangling")
set_tests_properties(test_space_time_stack_demangling PROPERTIES  ENVIRONMENT "KOKKOS_TOOLS_LIBS=/home/runner/work/kokkos-tools/kokkos-tools/build/profiling/space-time-stack/libkp_space_time_stack.so" _BACKTRACE_TRIPLES "/home/runner/work/kokkos-tools/kokkos-tools/tests/CMakeLists.txt;40;add_test;/home/runner/work/kokkos-tools/kokkos-tools/tests/space-time-stack/CMakeLists.txt;1;kp_add_executable_and_test;/home/runner/work/kokkos-tools/kokkos-tools/tests/space-time-stack/CMakeLists.txt;0;")
