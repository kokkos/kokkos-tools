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

set(CMAKE_CXX_COMPILER ${Kokkos_CXX_COMPILER})
set(CMAKE_CXX_STANDARD ${CMAKE_CXX_STANDARD_DEFAULT})

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
add_subdirectory(profiling/papi-connector)
#add_subdirectory(profiling/systemtap-connector)
add_subdirectory(profiling/variorum-connector)

# Build external
# TODO: tpls/Caliper
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