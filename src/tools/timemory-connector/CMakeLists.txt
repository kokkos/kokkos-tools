cmake_minimum_required(VERSION 3.10 FATAL_ERROR)

if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
    message(STATUS "Error! Building from the source directory may overwrite Makefile")
    message(STATUS "Remove 'CMakeCache.txt' and 'CMakeFiles' and build is separate directory")
    message(FATAL_ERROR "In-source build")
endif()

project(timemory-connector LANGUAGES CXX)

include(CheckLanguage)
include(GNUInstallDirs)

set(LINKER_LANGUAGE CXX)
set(CUDA_AVAILABLE OFF)

check_language(CUDA)
if(CMAKE_CUDA_COMPILER)
    enable_language(CUDA)
    set(LINKER_LANGUAGE CUDA)
    set(CUDA_AVAILABLE ON)
else()
    message(FATAL_ERROR "CUDA compiler not found.")
endif()

if("${CMAKE_BUILD_TYPE}" STREQUAL "")
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build flags" FORCE)
endif()

set(BUILD_SHARED_LIBS ON CACHE BOOL "Build shared libraries" FORCE)
set(CMAKE_CXX_STANDARD 11 CACHE STRING "C++ language standard")
set(CMAKE_CUDA_STANDARD 11 CACHE STRING "CUDA language standard")
set(CMAKE_CXX_STANDARD_REQUIRED ON CACHE BOOL "C++ language flags required")
set(CMAKE_CUDA_STANDARD_REQUIRED ON CACHE BOOL "CUDA language flags required")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH ON CACHE BOOL "Use link path to set default rpath")

option(BUILD_SAMPLE "Build sample test" OFF)

set(_OPTIONS_LIST)
macro(ADD_CONNECTOR_OPTION _OPT _DESCRIPT _DEFAULT)
    option(${_OPT} "${_DESCRIPT}" ${_DEFAULT})
    set(${_OPT}_COMPONENTS ${ARGN})
    list(APPEND _OPTIONS_LIST ${_OPT})
endmacro()

add_connector_option(USE_MPI "Enable MPI support" OFF mpi)
add_connector_option(USE_TAU "Enable TAU support" OFF tau)
add_connector_option(USE_CUDA "Enable CUDA support" OFF cuda cupti)
add_connector_option(USE_PAPI "Enable PAPI support" OFF papi)
add_connector_option(USE_NVTX "Enable NVTX support" OFF nvtx)
add_connector_option(USE_UPCXX "Enable UPC++ support" OFF upcxx)
add_connector_option(USE_VTUNE "Enable VTune support" OFF vtune)
add_connector_option(USE_GOTCHA "Enable GOTCHA support" OFF gotcha)
add_connector_option(USE_LIKWID "Enable LIKWID support" OFF likwid)
add_connector_option(USE_CALIPER "Enable Caliper support" OFF caliper)
add_connector_option(USE_ROOFLINE "Enable roofline support" OFF roofline)
add_connector_option(USE_GPERFTOOLS "Enable gperftools support" OFF gperftools)
add_connector_option(USE_LIBRARY "Enable timemory library and its config options" OFF cxx shared)

# CMake INTERFACE target for timemory that provides include path
set(COMPONENTS headers vector)

foreach(_OPT ${_OPTIONS_LIST})
    if(${_OPT})
        list(APPEND COMPONENTS ${${_OPT}_COMPONENTS})
    endif()
endforeach()

# find the timemory package
find_package(timemory REQUIRED
    COMPONENTS headers vector
    OPTIONAL_COMPONENTS roofline cpu-roofline gpu-roofline)

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
