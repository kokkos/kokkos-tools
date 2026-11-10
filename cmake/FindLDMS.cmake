# - Try to find LDMS library
# Once done this will define:
#  LDMS_FOUND - System has LDMS
#  LDMS_INCLUDE_DIRS - The LDMS include directories
#  LDMS_LIBRARIES - The libraries needed to use LDMS
#  LDMS_DEFINITIONS - Compiler switches required for using LDMS
#  LDMS::LDMS - Imported target for LDMS

find_package(PkgConfig QUIET)
pkg_check_modules(PC_LDMS QUIET ldms)

set(LDMS_DEFINITIONS ${PC_LDMS_CFLAGS_OTHER})

find_path(LDMS_INCLUDE_DIR
    NAMES ldms/ldms.h
    HINTS ${PC_LDMS_INCLUDEDIR} ${PC_LDMS_INCLUDE_DIRS} ${LDMS_ROOT} ${OVIS_DIR}
    PATH_SUFFIXES include
    )

find_library(LDMS_LIBRARY
    NAMES ldms
    HINTS ${PC_LDMS_LIBDIR} ${PC_LDMS_LIBRARY_DIRS} ${LDMS_ROOT} ${OVIS_DIR}
    PATH_SUFFIXES lib lib64
    )

set(LDMS_INCLUDE_DIRS ${LDMS_INCLUDE_DIR})
set(LDMS_LIBRARIES ${LDMS_LIBRARY})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LDMS
    FOUND_VAR LDMS_FOUND
    REQUIRED_VARS LDMS_LIBRARY LDMS_INCLUDE_DIR
    VERSION_VAR LDMS_VERSION
    )

if(LDMS_FOUND AND NOT TARGET LDMS::LDMS)
    add_library(LDMS::LDMS UNKNOWN IMPORTED)
    set_target_properties(LDMS::LDMS PROPERTIES
        IMPORTED_LOCATION "${LDMS_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${LDMS_INCLUDE_DIR}"
        INTERFACE_COMPILE_OPTIONS "${LDMS_DEFINITIONS}"
        )
    
    # Add transitive dependencies if needed
    if(PC_LDMS_LIBRARIES)
        set_target_properties(LDMS::LDMS PROPERTIES
            INTERFACE_LINK_LIBRARIES "${PC_LDMS_LIBRARIES}"
            )
    endif()
endif()

mark_as_advanced(LDMS_INCLUDE_DIR LDMS_LIBRARY)
