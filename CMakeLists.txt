cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

project(KokkosTools CXX)

# Include utilities
include(cmake/utils.cmake)
include(cmake/configure_tpls.cmake)

# Set policies
cmake_policy(SET CMP0054 NEW) # simplify if()
cmake_policy(SET CMP0057 NEW) # support IN_LIST in if()
cmake_policy(SET CMP0111 NEW) # error if library not found

# Disable in-source builds to prevent source tree corruption.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
  message(FATAL_ERROR "FATAL: In-source builds are not allowed. You should create a separate directory for build files.")
endif()

list(INSERT CMAKE_MODULE_PATH 0 ${PROJECT_SOURCE_DIR}/cmake)

message(STATUS)
message(STATUS Configuring Kokkos-Tools)
message(STATUS)

# Common settings
option(BUILD_SHARED_LIBS "Build shared libraries" ON)

# Tools settings
option(KokkosTools_ENABLE_SINGLE  "Build single library interfacing all profilers and dispatching at runtime" OFF)
option(KokkosTools_ENABLE_PAPI    "Enable PAPI support"             ON)
option(KokkosTools_ENABLE_MPI     "Enable MPI support"              ON)
option(KokkosTools_ENABLE_CALIPER "Enable building Caliper library" ON)
option(KokkosTools_ENABLE_APEX    "Enable building Apex library"    ON)

# Advanced settings
option(KokkosTools_REUSE_KOKKOS_COMPILER "Set the compiler and flags based on installed Kokkos settings" OFF)
mark_as_advanced(KokkosTools_REUSE_KOKKOS_COMPILER)

# Fetch Kokkos options
if(TARGET Kokkos::kokkos)
  # Kokkos is built within same parent project and was already configured
  # (all options are set, compiler and flags are set).
  message(STATUS "Detected Kokkos being built within same parent project: ${Kokkos_SOURCE_DIR}")
elseif(Kokkos_ROOT)
  # Extract configuration (options, compiler, etc.) from Kokkos installed at Kokkos_ROOT
  find_package(Kokkos REQUIRED)
  message(STATUS "Fetching Kokkos configuration from: ${Kokkos_ROOT}")

  #-------------------------------------------------------------------------#
  # TODO: Lookup options we need to use or propagete to Caliper, Apex, etc. (OpenMP, CUDA, etc.)
  #       see KokkosConfigCommon.cmake and KokkosTargets.cmake in .../install/kokkos/lib/cmake/
  message(FATAL_ERROR "NOT_IMPLEMENTED: make sure we extract and set everything right")
  #-------------------------------------------------------------------------#

  # Synchronize compiler and flags (only when explicitly requested)
  if(KokkosTools_REUSE_KOKKOS_COMPILER)
    set(CMAKE_CXX_COMPILER "${Kokkos_CXX_COMPILER}"        CACHE STRING "C++ Compiler")
    set(CMAKE_CXX_STANDARD "${CMAKE_CXX_STANDARD_DEFAULT}" CACHE STRING "C++ Standard: 98, 11, 14, 17, 20 or 23")
  endif()
else()
  # Tools are built without referring to Kokkos
  message(WARNING "Kokkos NOT found, using reasonable defaults for options")

  #-------------------------------------------------------------------------#
  # Note: original CMakes used: CXXFLAGS=-O3 -std=c++11 -g -shared -fPIC -Wall -Wextra
  message(FATAL_ERROR "NOT_IMPLEMENTED: set reasonable defaults and test this scenario")
  #-------------------------------------------------------------------------#
endif()

# Libraries
if(KokkosTools_ENABLE_PAPI)
  find_package(PAPI REQUIRED) # TODO: papi-connector requires v6.0 or newer
  cmake_path(GET PAPI_INCLUDE_DIR PARENT_PATH PAPI_ROOT)
  message(STATUS "Found PAPI ${PAPI_VERSION_STRING} at ${PAPI_ROOT}")
else()
  message(STATUS "PAPI support disabled")
endif()

if(KokkosTools_ENABLE_MPI)
  find_package(MPI REQUIRED)
  message(STATUS "Found MPI ${MPI_CXX_VERSION}: ${MPI_CXX_LIBRARIES}")
  set(USE_MPI 1)
else()
  set(USE_MPI 0)
endif()

include(cmake/configure_variorum.cmake)

set(KOKKOSTOOLS_HAS_CALIPER ${KokkosTools_ENABLE_CALIPER})

# make Kokkos profiling interface available for native profilers
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/profiling/all)

# Config file
configure_file(common/kp_config.hpp.in common/kp_config.hpp)
set(COMMON_HEADERS_PATH ${CMAKE_CURRENT_BINARY_DIR}/common)
include_directories(${COMMON_HEADERS_PATH})

set(SINGLELIB_PROFILERS "" CACHE STRING "" FORCE)

# Utilities
add_subdirectory(common/kernel-filter)
add_subdirectory(debugging/kernel-logger)

# Profilers
add_subdirectory(profiling/memory-events)
add_subdirectory(profiling/memory-hwm)
if(USE_MPI)
  add_subdirectory(profiling/memory-hwm-mpi)
else()
  message(STATUS "Skipping memory-hwm-mpi (MPI disabled)")
endif()
add_subdirectory(profiling/memory-usage)
add_subdirectory(profiling/chrome-tracing)
add_subdirectory(profiling/simple-kernel-timer)
add_subdirectory(profiling/space-time-stack)

# External lib connectors
if(KokkosTools_ENABLE_PAPI)
  add_subdirectory(profiling/papi-connector)
endif()

add_subdirectory(profiling/systemtap-connector)
if(KOKKOSTOOLS_HAS_VARIORUM)
  add_subdirectory(profiling/variorum-connector)
endif()

# GPU profilers
if(KOKKOS_ENABLE_CUDA)
  #add_subdirectory(profiling/nvprof-connector)
  #add_subdirectory(profiling/nvprof-focused-connector)
endif()
if(KOKKOS_ENABLE_HIP)
  #add_subdirectory(profiling/roctx-connector)
endif()

if(DEFINED ENV{VTUNE_HOME})
  set(VTune_ROOT $ENV{VTUNE_HOME})
endif()
if(VTune_ROOT)
  #add_subdirectory(profiling/vtune-connector)
  #add_subdirectory(profiling/vtune-focused-connector)
else()
  message(WARNING "Set VTUNE_HOME in environment or VTune_ROOT in build options to build VTune connectors")
  set(VTune_ROOT "" CACHE STRING "Path to VTune Intel compiler")
endif()

# Build TPLs
if(KokkosTools_ENABLE_CALIPER)
  configure_caliper()
  add_subdirectory(tpls/Caliper)
  list(APPEND SINGLELIB_PROFILERS "caliper")
endif()
if(KokkosTools_ENABLE_APEX)
  configure_apex()
  add_subdirectory(tpls/apex)
endif()

# Build single library interface (once we have everything set up)
if(KokkosTools_ENABLE_SINGLE)
  message(STATUS "Building single KokkosTools library with profilers: ${SINGLELIB_PROFILERS}")
  add_subdirectory(profiling/all)
else()
  message(STATUS "Single KokkosTools library skipped")
endif()
