#[=======================================================================[.rst:
FindPAPI
--------

Find the native PAPI headers and libraries.

IMPORTED Targets
^^^^^^^^^^^^^^^^

This module defines :prop_tgt:`IMPORTED` target ``PAPI::PAPI``, if PAPI has been found.

Result Variables
^^^^^^^^^^^^^^^^

This module defines the following variables:

``PAPI_FOUND``
  "True" if ``papi`` found.

``PAPI_INCLUDE_DIR``
  where to find ``papi``/``papi.h``, etc.

``PAPI_LIBRARY``
  List of libraries when using ``papi``.

``PAPI_VERSION_STRING``
  The version of ``papi`` found.

This module defines ``PAPI::PAPI`` target for PAPI library.

#]=======================================================================]

# Look for the header file.
find_path(
  PAPI_INCLUDE_DIR
  NAMES papi.h
  HINTS /usr/include /usr/local/include)
mark_as_advanced(PAPI_INCLUDE_DIR)

# Look for the library (sorted from most current/relevant entry to least).
find_library(
  PAPI_LIBRARY
  NAMES papi
  HINTS /usr/lib /usr/local/lib)
mark_as_advanced(PAPI_LIBRARY)

#define PAPI_VERSION  			PAPI_VERSION_NUMBER(6,0,0,1)

if(PAPI_INCLUDE_DIR AND NOT PAPI_VERSION_STRING AND EXISTS "${PAPI_INCLUDE_DIR}/papi.h")
  file(
    STRINGS "${PAPI_INCLUDE_DIR}/papi.h"
    PAPI_VERSION_STRING
    REGEX "^#define[\t ]+PAPI_VERSION[\t ]+PAPI_VERSION_NUMBER\(.*\)")
  string(
    REGEX REPLACE
    "^#define[\t ]+PAPI_VERSION[\t ]+PAPI_VERSION_NUMBER\\((.*)\\)"
    "\\1"
    PAPI_VERSION_STRING
    "${PAPI_VERSION_STRING}")
  string(REPLACE "," "."  PAPI_VERSION_STRING "${PAPI_VERSION_STRING}")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
  PAPI
  REQUIRED_VARS PAPI_LIBRARY PAPI_INCLUDE_DIR
  VERSION_VAR PAPI_VERSION_STRING)


# Skip target if already defined
if(TARGET PAPI::PAPI)
  return()
endif()

# Set up imported target
add_library(PAPI::PAPI INTERFACE IMPORTED)

target_include_directories(PAPI::PAPI INTERFACE ${PAPI_INCLUDE_DIR})
target_link_libraries(PAPI::PAPI INTERFACE ${PAPI_LIBRARY})

set(PAPI_INCLUDE_DIRS ${PAPI_INCLUDE_DIR})
set(PAPI_LIBRARIES ${PAPI_LIBRARY})
