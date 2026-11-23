option(KokkosTools_ENABLE_LDMS "Enable building LDMS connector" OFF)

if(KokkosTools_ENABLE_LDMS)
  # Users must set OVIS_DIR or LDMS_ROOT to the LDMS install prefix
  find_package(LDMS REQUIRED)

  # Use kp_add_library if available, otherwise fallback to add_library
  if(COMMAND kp_add_library)
    kp_add_library(kp_ldms_connector kp_kernel_ldms.cpp)
  else()
    add_library(kp_ldms_connector SHARED kp_kernel_ldms.cpp)
    set_target_properties(kp_ldms_connector PROPERTIES OUTPUT_NAME kp_kernel_ldms PREFIX "")
  endif()

  target_include_directories(kp_ldms_connector PRIVATE ${LDMS_INCLUDE_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
  target_link_libraries(kp_ldms_connector PRIVATE LDMS::LDMS)
endif() 