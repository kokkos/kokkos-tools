project(caliperKokkosTool)


find_package(caliper)

include_directories(${caliper_INCLUDE_DIR})

add_library(kokkos_caliper SHARED kp_kernel_timer.cpp)
target_link_libraries(kokkos_caliper caliper)
