set(PROBES_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/probes.d)
set(PROBES_HEADER ${CMAKE_CURRENT_BINARY_DIR}/probes.h)
set(PROBES_OBJECT ${CMAKE_CURRENT_BINARY_DIR}/probes.o)

# Note: connect external/generated object file via imported object library
add_custom_command(OUTPUT ${PROBES_OBJECT}
  COMMAND dtrace -C -G -s ${PROBES_SOURCE} -o ${PROBES_OBJECT}
  DEPENDS ${PROBES_SOURCE} VERBATIM)
add_library(kp_systemtap_probe OBJECT IMPORTED)
set_property(TARGET kp_systemtap_probe PROPERTY IMPORTED_OBJECTS ${PROBES_OBJECT})

kp_add_library(kp_systemtap_connector kp_systemtap_connector.cpp ${PROBES_HEADER}
  $<TARGET_OBJECTS:kp_systemtap_probe>)
set_property(SOURCE ${PROBES_HEADER} PROPERTY HEADER_FILE_ONLY ON)

# Note: connect generated header
target_include_directories(kp_systemtap_connector PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
add_custom_command(OUTPUT ${PROBES_HEADER}
  COMMAND dtrace -C -h -s ${PROBES_SOURCE} -o ${PROBES_HEADER}
  DEPENDS ${PROBES_SOURCE} VERBATIM)
