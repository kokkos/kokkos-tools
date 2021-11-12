if(NOT MPI_FOUND OR NOT TARGET MPI::MPI_CXX)
  message(FATAL_ERROR "kp_hwm_mpi requires MPI")
endif()

add_library(kp_hwm_mpi SHARED kp_hwm_mpi.cpp)

target_link_libraries(kp_hwm_mpi PRIVATE MPI::MPI_CXX)
