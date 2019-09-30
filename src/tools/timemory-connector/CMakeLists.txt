cmake_minimum_required(VERSION 3.10 FATAL_ERROR)

if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
    message(STATUS "Error! Building from the source directory may overwrite Makefile")
    message(STATUS "Remove 'CMakeCache.txt' and 'CMakeFiles' and build is separate directory")
    message(FATAL_ERROR "In-source build")
endif()

project(timemory-connector LANGUAGES CXX)

include(CheckLanguage)
include(GNUInstallDirs)

if("${CMAKE_BUILD_TYPE}" STREQUAL "")
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build flags" FORCE)
endif()
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH ON CACHE BOOL "Use link path to set default rpath")

option(BUILD_SAMPLE "Build sample test" OFF)

set(LINKER_LANGUAGE CXX)
if(ENABLE_CUDA)
    check_language(CUDA)
    if(CMAKE_CUDA_COMPILER)
        enable_language(CUDA)
        set(LINKER_LANGUAGE CUDA)
    else()
        message(FATAL_ERROR "CUDA compiler not found. ENABLE_CUDA=ON ignored")
      endif()
endif()

# CMake INTERFACE target for timemory that provides include path
set(COMPONENTS headers cxx shared vector cuda cudart papi cupti cudart-device caliper)

# find the timemory package
find_package(timemory REQUIRED COMPONENTS ${COMPONENTS})

add_library(kp_timemory SHARED kp_timemory.cpp)
target_link_libraries(kp_timemory timemory)
set_target_properties(kp_timemory PROPERTIES
    PREFIX          ""
    LINKER_LANGUAGE ${LINKER_LANGUAGE})

install(TARGETS kp_timemory DESTINATION ${CMAKE_INSTALL_LIBDIR})

if(BUILD_SAMPLE)
    # create test executable
    add_executable(sample sample/sample.cpp)
    target_link_libraries(sample kp_timemory)
endif()
