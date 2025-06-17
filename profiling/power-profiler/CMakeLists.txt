kp_add_library(kp_power_profiler power-profiler.cpp)

target_link_libraries(kp_power_profiler PRIVATE variorum::variorum)

if(USE_MPI)
  target_link_libraries(kp_power_profiler PRIVATE MPI::MPI_CXX)
endif()
