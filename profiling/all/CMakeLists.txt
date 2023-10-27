set(LIBNAME kokkostools)

#if(NOT SINGLELIB_PROFILERS)
#  message(FATAL_ERROR "Can't build ${kokkostools}: no profilers enabled")
#  return()
#endif()

add_library(${LIBNAME} ${KOKKOSTOOLS_LIBRARY_MODE} kp_all.cpp)

target_include_directories(${LIBNAME}
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
         $<BUILD_INTERFACE:${COMMON_HEADERS_PATH}>
         $<INSTALL_INTERFACE:${EXPORT_INCLUDE_DIR}>)

if(SINGLELIB_PROFILERS)
  target_link_libraries(${LIBNAME} PUBLIC ${SINGLELIB_PROFILERS})
endif()

file(GLOB_RECURSE HEADER_FILES CONFIGURE_DEPENDS kp_all.hpp "${COMMON_HEADERS_PATH}/*.hpp")

install(FILES ${HEADER_FILES} DESTINATION ${EXPORT_INCLUDE_DIR})
install(TARGETS ${LIBNAME} EXPORT ${EXPORT_NAME})