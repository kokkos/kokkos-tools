# shared global objects
add_library(kp_kernel_shared SHARED kp_shared.cpp)

# Add JSON kernel-timer
kp_add_library(kp_kernel_timer_json SHARED kp_kernel_timer_json.cpp)
target_link_libraries(kp_kernel_timer_json PRIVATE kp_kernel_shared)

# Add binary kernel-timer
kp_add_library(kp_kernel_timer SHARED kp_kernel_timer.cpp)
target_link_libraries(kp_kernel_timer PRIVATE kp_kernel_shared)

# Add binary utilities
add_executable(kp_reader kp_reader.cpp)
target_link_libraries(kp_reader PRIVATE kp_kernel_timer)

add_executable(kp_json_writer kp_json_writer.cpp)
target_link_libraries(kp_json_writer PRIVATE kp_kernel_timer)
