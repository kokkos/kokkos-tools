set(LIBNAME kokkostools)

#if(NOT SINGLELIB_PROFILERS)
#  message(FATAL_ERROR "Can't build ${kokkostools}: no profilers enabled")
#  return()
#endif()

add_library(${LIBNAME} SHARED kp_all.cpp)

target_include_directories(${LIBNAME}
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
         $<BUILD_INTERFACE:${COMMON_HEADERS_PATH}>)
          # $<INSTALL_INTERFACE:$<INSTALL_PREFIX>/include>

if(SINGLELIB_PROFILERS)
  target_link_libraries(${LIBNAME} PUBLIC ${SINGLELIB_PROFILERS})
endif()

# install(TARGETS ${LIBNAME} EXPORT KokkosTools::KokkosTools
#   INCLUDES DESTINATION include)
# install(FILES ${headers} DESTINATION include)