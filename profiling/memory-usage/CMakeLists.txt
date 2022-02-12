kp_add_library(kp_memory_usage kp_memory_usage.cpp)

# enable headers from memory-events
target_include_directories(kp_memory_usage
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../memory-events)