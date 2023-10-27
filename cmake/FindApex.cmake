find_package(PkgConfig REQUIRED)

# backup current CMAKE_PREFIX_PATH and PKG_CONFIG_USE_CMAKE_PREFIX_PATH
if(DEFINED CMAKE_PREFIX_PATH)
  set(_old_def ON)
  set(_old_val ${CMAKE_PREFIX_PATH})
else()
  set(_old_def OFF)
endif()
set(_old_use ${PKG_CONFIG_USE_CMAKE_PREFIX_PATH})
set(PKG_CONFIG_USE_CMAKE_PREFIX_PATH ON)

# add Apex_DIR / Apex_ROOT to module search path
if(Apex_DIR)
  set(CMAKE_PREFIX_PATH ${Apex_DIR})
elseif(Apex_ROOT)
  set(CMAKE_PREFIX_PATH ${Apex_ROOT})
endif()

# find Apex
pkg_check_modules(Apex QUIET IMPORTED_TARGET apex)
if(Apex_FOUND)
  # create "apex" target like it would be created by Apex setup
  add_library(apex ALIAS PkgConfig::Apex)
  file(REAL_PATH ${Apex_PREFIX} Apex_DIR)
endif()

# restore original variables
if(_old_def)
  set(CMAKE_PREFIX_PATH ${_old_val})
else()
  unset(CMAKE_PREFIX_PATH)
endif()
set(PKG_CONFIG_USE_CMAKE_PREFIX_PATH ${_old_use})
