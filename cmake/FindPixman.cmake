# - Try to find the Pixman library
#
# Read-Only variables:
# Pixman_FOUND - system has the Pixman library
# Pixman_INCLUDE_DIR - the Pixman include directory
# Pixman_LIBRARIES - The libraries needed to use Pixman
# Pixman_VERSION - This is set to $major.$minor.$revision (eg. 0.9.8)

# =============================================================================
# Copyright 2012 Dmitry Baryshnikov <polimax at mail dot ru>
# Copyright 2017 Simon Richter <Simon.Richter at hogyros dot de>
# Copyright 2023 Nimish Telang <nimish@telang.net>

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
  message(CHECK_START "Looking for pixman using pkg-config")
  pkg_check_modules(Pixman pixman-1 IMPORTED_TARGET)
  if (Pixman_FOUND)
    message(CHECK_PASS "found by pkg-config")
  else()
    message(CHECK_FAIL "not found by pkg-config")
  endif()
endif()

message(CHECK_START "Searching for pixman library")
FIND_LIBRARY(Pixman_LIBRARIES
  pixman
  pixman-1
  pixman-1.0
  PATH_SUFFIXES
  lib
  VC
  lib/VC
)
if (Pixman_LIBRARIES_FOUND)
  message(CHECK_PASS "found ${Pixman_LIBRARIES}")
else()
  message(CHECK_FAIL "not found")
endif()

message(CHECK_START "Looking for pixman include dirs")
find_path(Pixman_INCLUDE_DIRS
  pixman-version.h
  PATH_SUFFIXES
  include
  include/pixman-1
)
if(Pixman_INCLUDE_DIRS)
  message(CHECK_PASS "Found: ${Pixman_INCLUDE_DIRS}")
  message(CHECK_START "Extracting Pixman version")
  find_file(Pixman_VERSION_H pixman-version.h HINTS ${Pixman_INCLUDE_DIRS})
  if(Pixman_VERSION_H_FOUND)
    file(READ "${Pixman_VERSION_H}" _Pixman_VERSION_H_CONTENTS)
    string(REGEX REPLACE "^(.*\n)?#define[ \t]+CAIRO_VERSION_MAJOR[ \t]+([0-9]+).*"
      "\\2" Pixman_VERSION_MAJOR ${_Pixman_VERSION_H_CONTENTS})
    string(REGEX REPLACE "^(.*\n)?#define[ \t]+CAIRO_VERSION_MINOR[ \t]+([0-9]+).*"
      "\\2" Pixman_VERSION_MINOR ${_Pixman_VERSION_H_CONTENTS})
    string(REGEX REPLACE "^(.*\n)?#define[ \t]+CAIRO_VERSION_MICRO[ \t]+([0-9]+).*"
      "\\2" Pixman_VERSION_MICRO ${_Pixman_VERSION_H_CONTENTS})
    set(Pixman_VERSION ${Pixman_VERSION_MAJOR}.${Pixman_VERSION_MINOR}.${Pixman_VERSION_MICRO}
      CACHE INTERNAL "The version number for Pixman libraries")
  endif()
  if (Pixman_VERSION)
    message(CHECK_PASS "found Pixman version ${Pixman_VERSION}")
  else()
    message(CHECK_FAIL "failed")
  endif()

else()
  message(CHECK_FAIL "none found")
endif()

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(Pixman
  REQUIRED_VARS
  Pixman_LIBRARIES
  Pixman_INCLUDE_DIRS
  VERSION_VAR
  Pixman_VERSION
  HANDLE_VERSION_RANGE
)

MARK_AS_ADVANCED(Pixman_INCLUDE_DIRS Pixman_LIBRARIES)

if(Pixman_FOUND AND NOT TARGET Pixman::Pixman)
  if(TARGET PkgConfig::Pixman)
    add_library(Pixman::Pixman ALIAS PkgConfig::Pixman)
  else()
    add_library(Pixman::Pixman IMPORTED SHARED)
    target_include_directories(Pixman::Pixman INTERFACE ${Pixman_INCLUDE_DIRS})
    target_link_libraries(Pixman::Pixman INTERFACE ${Pixman_LIBRARIES})
    set_target_properties(Pixman::Pixman PROPERTIES
      IMPORTED_LOCATION ${Pixman_LIBRARIES}
    )
  endif()
endif()
