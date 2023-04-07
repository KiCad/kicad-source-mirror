# - Try to find the Cairo library
#
# Read-Only variables:
# Cairo_FOUND - system has the Cairo library
# Cairo_INCLUDE_DIR - the Cairo include directory
# Cairo_LIBRARIES - The libraries needed to use Cairo
# Cairo_VERSION - This is set to $major.$minor.$revision (eg. 0.9.8)

# =============================================================================
# Copyright 2012 Dmitry Baryshnikov <polimax at mail dot ru>
# Copyright 2023 Nimish Telang <nimish@telang.net>
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
# =============================================================================
# (To distribute this file outside of CMake, substitute the full
# License text for the above reference.)

find_package(PkgConfig)
cmake_policy(SET CMP0074 NEW)

if(PKG_CONFIG_FOUND)
  message(CHECK_START "Looking for cairo using pkg-config")
  pkg_check_modules(Cairo cairo IMPORTED_TARGET)
  if (Cairo_FOUND)
    message(CHECK_PASS "found by pkg-config")
  else()
    message(CHECK_FAIL "not found by pkg-config")
  endif()
endif()

message(CHECK_START "Searching for cairo library")
FIND_LIBRARY(Cairo_LIBRARIES
  cairo
  PATH_SUFFIXES
  lib
  VC
  lib/VC
  lib/MINGW
)
if (Cairo_LIBRARIES_FOUND)
  message(CHECK_PASS "found ${Cairo_LIBRARIES}")
else()
  message(CHECK_FAIL "not found")
endif()

message(CHECK_START "Searching for Cairo debug library")
FIND_LIBRARY(Cairo_LIBRARIES_DEBUG
  cairod
  PATH_SUFFIXES
  lib
  VC
  lib/VC
  lib/MINGW

)
if (Cairo_LIBRARIES_DEBUG_FOUND)
  message(CHECK_PASS "found ${Cairo_LIBRARIES_DEBUG}")
else()
  message(CHECK_FAIL "not found")
endif()



message(CHECK_START "Looking for cairo include dirs")
find_path(Cairo_INCLUDE_DIRS
  cairo-version.h
  PATH_SUFFIXES
  cairo
  include/cairo
)
if(Cairo_INCLUDE_DIRS)
  message(CHECK_PASS "Found: ${Cairo_INCLUDE_DIRS}")
  message(CHECK_START "Extracting Cairo version")
  find_file(Cairo_VERSION_H cairo-version.h HINTS ${Cairo_INCLUDE_DIRS})
  if(Cairo_VERSION_H_FOUND)
    file(READ "${Cairo_VERSION_H}" _Cairo_VERSION_H_CONTENTS)
    string(REGEX REPLACE "^(.*\n)?#define[ \t]+CAIRO_VERSION_MAJOR[ \t]+([0-9]+).*"
      "\\2" Cairo_VERSION_MAJOR ${_Cairo_VERSION_H_CONTENTS})
    string(REGEX REPLACE "^(.*\n)?#define[ \t]+CAIRO_VERSION_MINOR[ \t]+([0-9]+).*"
      "\\2" Cairo_VERSION_MINOR ${_Cairo_VERSION_H_CONTENTS})
    string(REGEX REPLACE "^(.*\n)?#define[ \t]+CAIRO_VERSION_MICRO[ \t]+([0-9]+).*"
      "\\2" Cairo_VERSION_MICRO ${_Cairo_VERSION_H_CONTENTS})
    set(Cairo_VERSION ${Cairo_VERSION_MAJOR}.${Cairo_VERSION_MINOR}.${Cairo_VERSION_MICRO}
      CACHE INTERNAL "The version number for Cairo libraries")
  endif()
  if (Cairo_VERSION)
    message(CHECK_PASS "found Cairo version ${Cairo_VERSION}")
  else()
    message(CHECK_FAIL "failed")
  endif()

else()
  message(CHECK_FAIL "none found")
endif()

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(Cairo
  REQUIRED_VARS
  Cairo_LIBRARIES
  Cairo_INCLUDE_DIRS
  VERSION_VAR
  Cairo_VERSION
  HANDLE_VERSION_RANGE
)

MARK_AS_ADVANCED(Cairo_INCLUDE_DIRS Cairo_LIBRARIES)

if(Cairo_FOUND AND NOT TARGET Cairo::Cairo)
  if(TARGET PkgConfig::Cairo)
    add_library(Cairo::Cairo ALIAS PkgConfig::Cairo)
  else()
    add_library(Cairo::Cairo IMPORTED SHARED)
    target_include_directories(Cairo::Cairo INTERFACE ${Cairo_INCLUDE_DIRS})
    target_link_libraries(Cairo::Cairo INTERFACE ${Cairo_LIBRARIES})
    set_target_properties(Cairo::Cairo PROPERTIES
      IMPORTED_LOCATION ${Cairo_LIBRARIES}
    )

    if (Cairo_DEBUG_LIBRARIES_FOUND)
      set_target_properties(Cairo::Cairo PROPERTIES
        IMPORTED_LOCATION_DEBUG ${Cairo_LIBRARIES_DEBUG}
        IMPORTED_LOCATION_RELWITHDEBINFO ${Cairo_LIBRARIES_DEBUG}
      )
    endif()
  endif()
endif()
