find_library(ROCM_ROCTX_LIB roctx64 REQUIRED HINTS $ENV{ROCM_PATH}/lib)
find_path(ROCM_ROCTX_INCLUDE roctx.h REQUIRED HINTS $ENV{ROCM_PATH}/include/roctracer)

kp_add_library(kp_roctx_connector kp_roctx_connector.cpp)

target_include_directories(kp_roctx_connector PRIVATE ${ROCM_ROCTX_INCLUDE})
target_link_libraries(kp_roctx_connector PRIVATE ${ROCM_ROCTX_LIB})
