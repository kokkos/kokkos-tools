find_package(CUDAToolkit REQUIRED)
kp_add_library(kp_nvtx_connector kp_nvtx_connector.cpp)

# Fallback since the imported target CUDA::nvtx3 is defined only as of CMake 3.25.
if(NOT TARGET CUDA::nvtx3)
    add_library(CUDA::nvtx3 INTERFACE IMPORTED)
    target_include_directories(CUDA::nvtx3 INTERFACE "${CUDAToolkit_INCLUDE_DIRS}")
    target_link_libraries(CUDA::nvtx3 INTERFACE ${CMAKE_DL_LIBS})
endif()

target_link_libraries(kp_nvtx_connector CUDA::nvtx3)
