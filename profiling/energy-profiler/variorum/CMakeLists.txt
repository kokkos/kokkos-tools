kp_add_library(kp_power_variorum kp_power_variorum.cpp
    variorum_power_profiler.cpp
)

target_include_directories(kp_power_variorum PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(kp_power_variorum PRIVATE variorum::variorum)

if(USE_MPI)
  target_link_libraries(kp_power_variorum PRIVATE MPI::MPI_CXX)
endif()