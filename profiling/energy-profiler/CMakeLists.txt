# Periodic sampling thread for the energy profiling tools. It has no Kokkos
# Tools entry points, so it is built as a static building block, not a tool.
find_package(Threads REQUIRED)

add_library(kp_energy_profiler_daemon STATIC daemon.cpp)
set_target_properties(kp_energy_profiler_daemon
  PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(kp_energy_profiler_daemon
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(kp_energy_profiler_daemon PUBLIC Threads::Threads)
