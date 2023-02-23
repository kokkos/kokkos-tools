# Based on Makefile authored by Zachary S. Frye (CASC at LLNL) in July 2020

kp_add_library(kp_variorum_connector variorum-connector.cpp)

target_link_libraries(kp_variorum_connector PRIVATE variorum::variorum)

if(USE_MPI)
  target_link_libraries(kp_variorum_connector PRIVATE MPI::MPI_CXX)
endif()
