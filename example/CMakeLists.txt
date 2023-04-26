# Find Kokkos
find_package(Kokkos QUIET)
if(NOT Kokkos_FOUND)
  message(FATAL_ERROR "Kokkos not found, set Kokkos_ROOT properly (current Kokkos_ROOT=${Kokkos_ROOT})")
endif()
foreach(_i "1;2;3") # cut .../lib/cmake/Kokkos suffix
cmake_path(GET Kokkos_DIR PARENT_PATH Kokkos_DIR)
endforeach()
message(STATUS "Found installed Kokkos at ${Kokkos_DIR}")

# Create target executable
set(TEST_APP kp_example)
add_executable(${TEST_APP} main.cpp)
set(LIBS "Kokkos::kokkos;kokkostools")
if(USE_MPI)
  list(APPEND LIBS MPI::MPI_CXX)
endif()
target_link_libraries(${TEST_APP} PRIVATE ${LIBS})

# Create tests
macro(add_kp_test NAME)
  add_test(NAME test_kokkos_tools_${NAME}
    COMMAND "kp_example" ${ARGN}
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/..")
endmacro()
# TODO: Read profiling results and check if the profiler had succesfully run
#       and exported output in expected format, fail the test otherwise.
if(NOT WIN32)
  add_kp_test(kernel_timer "kernel-timer")
  add_kp_test(kernel_timer_json "kernel-timer-json")
  add_kp_test(memory_events "memory-events")
  add_kp_test(memory_usage "memory-usage")
  add_kp_test(chrome_tracing "chrome-tracing")
  add_kp_test(space_time_stack "space-time-stack")
  add_kp_test(systemtap_connector "systemtap-connector")
  add_kp_test(highwater_mark "highwater-mark")
  if(USE_MPI)
    add_kp_test(highwater_mark_mpi "highwater-mark-mpi")
  endif()
endif()
if(KokkosTools_ENABLE_CALIPER)
  add_kp_test(caliper "caliper" "runtime-report(profile.kokkos)")
endif()
if(KOKKOSTOOLS_HAS_VARIORUM)
  add_kp_test(variorum "variorum")
endif()
if(KOKKOSTOOLS_HAS_VTUNE)
  add_kp_test(vtune_connector "vtune-connector")
  add_kp_test(vtune_focused_connector "vtune-focused-connector")
endif()
if(KOKKOSTOOLS_HAS_NVPROF)
  add_kp_test(vtune_connector "nvprof-connector")
  add_kp_test(vtune_focused_connector "nvprof-focused-connector")
endif()
