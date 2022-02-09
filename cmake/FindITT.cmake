# Note: Package is named "ITT" here because we reuse Caliper's FindITTAPI.cmake find module
# and it calls find_package_handle_standard_args() with "ITT" package name internally, so CMake
# expectes find_package() calles to use "ITT" package name as well.

function(is_architecture_x64 OUT_ARCH64)
  # heuristic to catch x86_64 on Unix and AMD64 on Windows
  string(REGEX MATCH "64$" ARCH64 ${CMAKE_SYSTEM_PROCESSOR})
  if(${ARCH64} STREQUAL "64")
    set(${OUT_ARCH64} ON PARENT_SCOPE)
  else()
    set(${OUT_ARCH64} OFF PARENT_SCOPE)
  endif()
endfunction()

#--------------------------------------------------------------------------------#
# 2022-02-14 On some x64 platforms (encountered on Ubuntu 20.04 in Win11/WSL2)
# CMake does NOT enable FIND_LIBRARY_USE_LIB64_PATHS as it should, which leads to
# Intel oneAPI libs not being found in .../lib64 folders.
# See: https://cmake.org/cmake/help/latest/command/find_library.html
get_property(USE_LIB32 GLOBAL PROPERTY FIND_LIBRARY_USE_LIB32_PATHS)
get_property(USE_LIB64 GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS)
is_architecture_x64(ARCH64)
if(ARCH64 AND NOT USE_LIB32)
  set_property(GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS ON)
elseif(NOT USE_LIB64)
  set_property(GLOBAL PROPERTY FIND_LIBRARY_USE_LIB32_PATHS ON)
endif()
#--------------------------------------------------------------------------------#

# Just reuse find module implemented in Caliper
set(ITT_PREFIX ${VTune_ROOT})
include(${PROJECT_SOURCE_DIR}/tpls/Caliper/cmake/FindITTAPI.cmake)

# Set up imported target
if(NOT TARGET ittnotify)
  add_library(ittnotify INTERFACE IMPORTED)
  target_include_directories(ittnotify INTERFACE ${ITT_INCLUDE_DIR})
  target_link_libraries(ittnotify INTERFACE ${ITT_LIBRARY})
endif()
