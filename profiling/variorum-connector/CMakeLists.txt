# Based on Makefile authored by Zachary S. Frye (CASC at LLNL) in July 2020

# Set VARIORUM_DIR for find_package() based on VARIORUM_ROOT (CMake or environment variable)
if(NOT DEFINED VARIORUM_DIR)
  if(NOT DEFINED VARIORUM_ROOT)
    if(DEFINED ENV{VARIORUM_ROOT})
      set(VARIORUM_ROOT $ENV{VARIORUM_ROOT})
    else()
      message(WARNING "Set VARIORUM_ROOT to build Variorum connector")
      return()
    endif()
  endif()
  set(VARIORUM_DIR ${VARIORUM_ROOT})
endif()

find_package(
  Variorum REQUIRED
  NO_DEFAULT_PATH PATHS ${VARIORUM_DIR}/lib/cmake)

add_library(variorum_connector SHARED variorum-connector.cpp)

target_link_libraries(variorum_connector PRIVATE variorum::variorum)