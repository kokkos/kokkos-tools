function(kp_add_library TARGET MODE)
  add_library(${TARGET} ${MODE} ${ARGN}) # SOURCES = ${ARGN}

  # add this library to the list of profilers linked to single library
  list(APPEND SINGLELIB_PROFILERS ${TARGET})
  set(SINGLELIB_PROFILERS ${SINGLELIB_PROFILERS} CACHE STRING "" FORCE)
endfunction()
