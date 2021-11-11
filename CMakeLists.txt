cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

project(KokkosTools CXX)

# Set policies
cmake_policy(SET CMP0054 NEW) # simplify if()
cmake_policy(SET CMP0057 NEW) # support IN_LIST in if()
cmake_policy(SET CMP0111 NEW) # error if library not found

# Disable in-source builds to prevent source tree corruption.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
  message(FATAL_ERROR "FATAL: In-source builds are not allowed. You should create a separate directory for build files.")
endif()

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

message(STATUS)
message(STATUS Configuring Kokkos-Tools)
message(STATUS)

# Tools settings
option(KokkosTools_ENABLE_PAPI    "Enable PAPI support"             ON)
option(KokkosTools_ENABLE_CALIPER "Enable building Caliper library" ON)
option(KokkosTools_ENABLE_APEX    "Enable building Apex library"    ON)

# Check Kokkos settings
if(TARGET Kokkos::kokkos)
  message(STATUS "Using Kokkos settings for KokkosTools (source: ${Kokkos_SOURCE_DIR})")
else()
  message(WARNING "Not using Kokkos for tools configuration\n\tNote: using settings from installed is Kokkos not yet supported")
endif()
  # TODO: Configure based on Kokkos
#       * check+test compile flags, especially ABI relevant
#       * use installed version if Kokkos_ROOT or Kokkos_DIR is defined
#       * check if Kokkos was already included in build with add_subdirectory()
#       * error out otherwise: does it make sense to look into Kokkos
#         source code without knowing it's *actual* build options ?

#Turn MPI support off: (arbitrary or if Kokkos has MPI disabled)
#CFLAGS += -DUSE_MPI=0

# CXXFLAGS=-O3 -std=c++11 -g -shared -fPIC
# -Wall -Wextra

# Note: default the compiler settings to what Kokkos set
#       and allow user to override them with build options
set(CMAKE_CXX_COMPILER "${Kokkos_CXX_COMPILER}"        CACHE STRING "C++ Compiler")
set(CMAKE_CXX_STANDARD "${CMAKE_CXX_STANDARD_DEFAULT}" CACHE STRING "C++ Standard: 98, 11, 14, 17, 20 or 23")

# Libraries
if(KokkosTools_ENABLE_PAPI)
  find_package(PAPI REQUIRED) # TODO: papi-connector requires v6.0 or newer
  cmake_path(GET PAPI_INCLUDE_DIR PARENT_PATH PAPI_ROOT)
  message(STATUS "Found PAPI ${PAPI_VERSION_STRING} at ${PAPI_ROOT}")
else()
  message(STATUS "PAPI support disabled")
endif()
find_package(OpenMP)
find_package(MPI)

# Utilities
add_subdirectory(common/kernel-filter)
add_subdirectory(debugging/kernel-logger)

# Profilers
add_subdirectory(profiling/memory-events)
add_subdirectory(profiling/memory-hwm)
add_subdirectory(profiling/memory-hwm-mpi)
add_subdirectory(profiling/memory-usage)
add_subdirectory(profiling/chrome-tracing)
add_subdirectory(profiling/simple-kernel-timer-json)
add_subdirectory(profiling/simple-kernel-timer)
add_subdirectory(profiling/space-time-stack)

# External lib connectors
if(KokkosTools_ENABLE_PAPI)
  add_subdirectory(profiling/papi-connector)
endif()

#add_subdirectory(profiling/systemtap-connector)
add_subdirectory(profiling/variorum-connector)

# Build external
# TODO: tpls/apex

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

# Note: Caliper docs https://software.llnl.gov/Caliper/build.html
#       might miss some options, check the top CMakeLists.txt:
#       https://github.com/LLNL/Caliper/blob/master/CMakeLists.txt
macro(configure_caliper)
  #set(BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS}) # Build shared (ON - default) or static library (OFF).
  #set(BUILD_DOCS OFF)   # Build documentation.
  #set(BUILD_TESTING OFF) # Build unit tests.
  #set(WITH_FORTRAN OFF) # Build and install Fortran wrappers.
  set(WITH_KOKKOS ON) # Enable Kokkos profiling support

  # Build with support for the OpenMP tools interface.
  if(OpenMP_FOUND)
    set(WITH_OMPT ON)
  else()
    set(WITH_OMPT OFF)
  endif()

  set(WITH_SAMPLER ON)  # Enable time-based sampling on Linux.
  set(WITH_TOOLS ON)    # Build Caliper’s tools (i.e, cali-query and mpi-caliquery). Default: On.
  set(WITH_MPI ${KOKKOS_ENABLE_MPI}) # Build with MPI support.
  set(WITH_ROCM ${Kokkos_ENABLE_HIP}) # Enable AMD ROCtracer/RocTX support
  set(WITH_NVTX ${Kokkos_ENABLE_CUDA}) # Build adapters to forward Caliper annotations to NVidia’s nvtx annotation API. Set CUDA_TOOLKIT_ROOT_DIR to the CUDA installation.
  set(WITH_CUPTI ${Kokkos_ENABLE_CUDA}) # Enable support for CUDA performance analysis
                                        # (wrapping of driver/runtime API calls and CUDA activity tracing).
  # Enable PAPI hardware counter service (requires papi)
  if(KokkosTools_ENABLE_PAPI)
    set(WITH_PAPI ON)
    set(PAPI_PREFIX ${PAPI_ROOT})
  else()
    set(WITH_PAPI OFF)
  endif()
  # Build adapters to forward Caliper annotations to Intel’s VTune annotation API.
  if(VTune_ROOT)
    set(WITH_VTUNE ON)
    set(ITT_PREFIX ${VTune_ROOT}/sdk)
    # TODO: Caliper doesn't seek ITT in ${VTune_ROOT}/sdk/lib64 or ${VTune_ROOT}/sdk/lib32 (only in .../lib)
  else()
    set(WITH_VTUNE OFF)
  endif()
endmacro()

# Note: Apex docs http://uo-oaciss.github.io/apex/install/#standalone_installation
macro(configure_apex)
  #set(BUILD_STATIC_EXECUTABLES OFF)
  set(APEX_WITH_PAPI ${KokkosTools_ENABLE_PAPI})
  set(APEX_WITH_CUDA ${Kokkos_ENABLE_CUDA})
  set(APEX_WITH_BFD ON)
  #set(BFD_ROOT ...)
  if(OMPT_FOUND)
    set(APEX_WITH_OMPT ON)
    # TODO: set(OMPT_ROOT ...) ?
  else()
    set(APEX_WITH_OMPT OFF)
  endif()
  # TODO: Apex tries to fetch TPLs and install them in /usr/lib
  #       (normally failing) unless we disable them...
  set(APEX_WITH_ACTIVEHARMONY OFF)
  set(APEX_WITH_OTF2 OFF)
  set(APEX_WITH_MPI OFF)
endmacro()

# Build TPLs
if(KokkosTools_ENABLE_CALIPER)
  configure_caliper()
  add_subdirectory(tpls/Caliper)
endif()
if(KokkosTools_ENABLE_APEX)
  configure_apex()
  add_subdirectory(tpls/apex)
endif()
