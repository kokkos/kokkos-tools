find_package(CUDAToolkit REQUIRED)
kp_add_library(kp_nvtx_connector kp_nvtx_connector.cpp)

target_link_libraries(kp_nvtx_connector CUDA::nvToolsExt)
