kp_add_library(kp_chrome_tracing kp_chrome_tracing.cpp)

if(USE_MPI)
  target_link_libraries(kp_chrome_tracing PRIVATE MPI::MPI_CXX)
endif()
