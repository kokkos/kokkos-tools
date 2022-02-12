kp_add_library(kp_space_time_stack kp_space_time_stack.cpp)

if(USE_MPI)
  target_link_libraries(kp_space_time_stack PRIVATE MPI::MPI_CXX)
endif()