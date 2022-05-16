function(kp_add_library TARGET)
  add_library(${TARGET} ${ARGN}) # SOURCES = ${ARGN}

  # add this library to the list of profilers linked to single library
  list(APPEND SINGLELIB_PROFILERS ${TARGET})
  set(SINGLELIB_PROFILERS ${SINGLELIB_PROFILERS} CACHE STRING "" FORCE)

  # add this library to exported targets
  list(APPEND EXPORT_TARGETS ${TARGET})
  set(EXPORT_TARGETS ${EXPORT_TARGETS} CACHE STRING "" FORCE)
endfunction()

macro(set_cache NAME VAL)
  set(${NAME} ON CACHE BOOL "")
endmacro()

function(acquire_kokkos_config)
  if(NOT TARGET Kokkos::kokkos)
    find_package(Kokkos QUIET)
    if(Kokkos_FOUND)
      set(Kokkos_FOUND_MSG "Found Kokkos installation")
      get_property(Kokkos_INSTALL_DIR TARGET Kokkos::kokkoscore PROPERTY INTERFACE_INCLUDE_DIRECTORIES)
      cmake_path(GET Kokkos_INSTALL_DIR PARENT_PATH Kokkos_INSTALL_DIR)
    endif()
  elseif(DEFINED Kokkos_DEVICES)
    set(Kokkos_FOUND_MSG "Found Kokkos package already imported by superproject")
    get_property(Kokkos_INSTALL_DIR TARGET Kokkos::kokkoscore PROPERTY INTERFACE_INCLUDE_DIRECTORIES)
    cmake_path(GET Kokkos_INSTALL_DIR PARENT_PATH Kokkos_INSTALL_DIR)
  else()
    set(Kokkos_FOUND_MSG "Found Kokkos included by source in superproject")
    get_property(Kokkos_INSTALL_DIR TARGET Kokkos::kokkos PROPERTY BINARY_DIR)
    # Include Kokkos exported settings like we would have them from find_package(Kokkos)
    set(Kokkos_FIND_QUIETLY ON)
    include(${Kokkos_INSTALL_DIR}/KokkosConfigCommon.cmake)
  endif()
  foreach(VAR_NAME Kokkos_FOUND_MSG Kokkos_INSTALL_DIR
      # Settings exported by Kokkos
      Kokkos_DEVICES Kokkos_ARCH Kokkos_TPLS Kokkos_CXX_COMPILER Kokkos_CXX_COMPILER_ID Kokkos_OPTIONS
      Kokkos_ENABLE_OPENMP Kokkos_ENABLE_CUDA Kokkos_ENABLE_HIP
      # Kokkos exports the flags as well
      CMAKE_CXX_FLAGS)
    set(${VAR_NAME} ${${VAR_NAME}} PARENT_SCOPE)
  endforeach()
endfunction()
