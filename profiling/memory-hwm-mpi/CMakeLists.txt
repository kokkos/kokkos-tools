find_package(MPI REQUIRED)
message(STATUS "Found MPI ${MPI_CXX_VERSION}")

add_library(kp_hwm_mpi SHARED kp_hwm_mpi.cpp)

target_link_libraries(kp_hwm_mpi PRIVATE MPI::MPI_CXX)
