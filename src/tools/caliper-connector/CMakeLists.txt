project(caliperKokkosTool)
cmake_minimum_required(VERSION 3.0)

find_package(caliper)

include_directories(${caliper_INCLUDE_DIR})
set(CMAKE_CXX_STANDARD 11)
add_library(kokkos_caliper SHARED kp_caliper_connector.cpp)
target_link_libraries(kokkos_caliper caliper)
