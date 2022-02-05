# Based on Makefile authored by Zachary S. Frye (CASC at LLNL) in July 2020

set(KOKKOSTOOLS_HAS_VARIORUM OFF)

# Set Variorum_DIR for find_package() based on VARIORUM_ROOT (CMake or environment variable)
set(MSG_NOTFOUND "set Variorum_DIR CMake variable or VARIORUM_ROOT environment variable to build Variorum connector")
if(NOT DEFINED Variorum_DIR)
  if(DEFINED ENV{VARIORUM_ROOT})
    set(Variorum_ROOT $ENV{VARIORUM_ROOT})
    set(MSG_NOTFOUND "check VARIORUM_ROOT environment variable ($ENV{VARIORUM_ROOT})")
  endif()
  set(Variorum_DIR ${VARIORUM_ROOT})
else()
  set(MSG_NOTFOUND "check Variorum_DIR (${Variorum_DIR})")
endif()

find_package(Variorum QUIET)

if(Variorum_FOUND)
  set(KOKKOSTOOLS_HAS_VARIORUM TRUE)
else()
  message(WARNING "Variorum not found: ${MSG_NOTFOUND}")
endif()
