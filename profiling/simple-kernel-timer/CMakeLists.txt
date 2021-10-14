add_library(kp_kernel_timer SHARED kp_kernel_timer.cpp)

add_executable(kp_reader kp_reader.cpp)
target_link_libraries(kp_reader PRIVATE kp_kernel_timer)

add_executable(kp_json_writer kp_json_writer.cpp)
target_link_libraries(kp_json_writer PRIVATE kp_kernel_timer)
