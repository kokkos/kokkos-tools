find_package(MPI REQUIRED)

add_library(kp_chrome_tracing SHARED kp_chrome_tracing.cpp)

if(NOT DEFINED USE_MPI OR USE_MPI)
  target_link_libraries(kp_hwm_mpi PRIVATE MPI::MPI_CXX)
endif()
