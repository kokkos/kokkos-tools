# shared global objects
add_library(kp_kernel_shared STATIC kp_shared.cpp)
list(APPEND EXPORT_TARGETS kp_kernel_shared)
set(EXPORT_TARGETS ${EXPORT_TARGETS} CACHE STRING "" FORCE)

if(NOT MSVC)
  set_property(TARGET kp_kernel_shared PROPERTY POSITION_INDEPENDENT_CODE ON)
endif()

# Add JSON kernel-timer
kp_add_library(kp_kernel_timer_json kp_kernel_timer_json.cpp)
target_link_libraries(kp_kernel_timer_json PRIVATE kp_kernel_shared)

# Add binary kernel-timer
kp_add_library(kp_kernel_timer kp_kernel_timer.cpp)
target_link_libraries(kp_kernel_timer PRIVATE kp_kernel_shared)

# Add binary utilities
add_executable(kp_reader kp_reader.cpp)
target_link_libraries(kp_reader PRIVATE kp_kernel_timer)

add_executable(kp_json_writer kp_json_writer.cpp)
target_link_libraries(kp_json_writer PRIVATE kp_kernel_timer)
