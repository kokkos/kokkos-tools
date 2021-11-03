#[=======================================================================[.rst:
FindPAPI
--------

Find the native PAPI headers and libraries.

IMPORTED Targets
^^^^^^^^^^^^^^^^

This module defines :prop_tgt:`IMPORTED` target ``PAPI::libpapi``, if PAPI has been found.

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

This module defines ``PAPI::libpapi`` target for PAPI library.

#]=======================================================================]

# Look for the header file.
if(NOT PAPI_INCLUDE_DIR)
  find_path(
    PAPI_INCLUDE_DIR
    NAMES papi.h
    HINTS /usr/include /usr/local/include)
  mark_as_advanced(PAPI_INCLUDE_DIR)
endif()

if(NOT PAPI_LIBRARY)
  # Look for the library (sorted from most current/relevant entry to least).
  find_library(
    PAPI_LIBRARY
    NAMES papi
    HINTS /usr/lib /usr/local/lib)
  mark_as_advanced(PAPI_LIBRARY)
endif()

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

find_package_handle_standard_args(
  PAPI
  REQUIRED_VARS PAPI_LIBRARY PAPI_INCLUDE_DIR
  VERSION_VAR PAPI_VERSION_STRING
  HANDLE_COMPONENTS)

# Handle missing library
if(NOT PAPI_FOUND)
  if(PAPI_FIND_REQUIRED)
    message(FATAL_ERROR "PAPI library not found")
  elseif(NOT PAPI_FIND_QUIETLY)
    message(WARNING "PAPI library not found")
  endif()
  return()
endif()

# Skip target if already defined
if(TARGET PAPI::libpapi)
  return()
endif()

# Set up imported target
add_library(PAPI::libpapi UNKNOWN IMPORTED)

set_property(
  TARGET PAPI::libpapi APPEND PROPERTY
  IMPORTED_CONFIGURATIONS RELEASE DEBUG)

set_target_properties(
  PAPI::libpapi PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES "${PAPI_INCLUDE_DIR}"
  IMPORTED_LINK_INTERFACE_LANGUAGES "C"
  IMPORTED_LOCATION "${PAPI_LIBRARY}"
  IMPORTED_LOCATION_RELEASE "${PAPI_LIBRARY}"
  IMPORTED_LOCATION_DEBUG "${PAPI_LIBRARY}")
