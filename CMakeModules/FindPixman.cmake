# - Try to find the PIXMAN library
# Once done this will define
#
#  PIXMAN_ROOT_DIR - Set this variable to the root installation of PIXMAN
#
# Read-Only variables:
#  PIXMAN_FOUND - system has the PIXMAN library
#  PIXMAN_INCLUDE_DIR - the PIXMAN include directory
#  PIXMAN_LIBRARIES - The libraries needed to use PIXMAN
#  PIXMAN_VERSION - This is set to $major.$minor.$revision (eg. 0.9.8)

#=============================================================================
# Copyright 2012 Dmitry Baryshnikov <polimax at mail dot ru>
# Copyright 2017 Simon Richter <Simon.Richter at hogyros dot de>
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================
# (To distribute this file outside of CMake, substitute the full
#  License text for the above reference.)

find_package(PkgConfig)

if(PKG_CONFIG_FOUND)
    pkg_check_modules(_PIXMAN pixman-1)
endif (PKG_CONFIG_FOUND)

SET(_PIXMAN_ROOT_HINTS
  $ENV{PIXMAN}
  ${PIXMAN_ROOT_DIR}
  )
SET(_PIXMAN_ROOT_PATHS
  $ENV{PIXMAN}/src
  /usr
  /usr/local
  )
SET(_PIXMAN_ROOT_HINTS_AND_PATHS
  HINTS ${_PIXMAN_ROOT_HINTS}
  PATHS ${_PIXMAN_ROOT_PATHS}
  )

FIND_PATH(PIXMAN_INCLUDE_DIR
  NAMES
    pixman.h
  HINTS
    ${_PIXMAN_INCLUDEDIR}
  ${_PIXMAN_ROOT_HINTS_AND_PATHS}
  PATH_SUFFIXES
    include
    "include/pixman-1"
)

IF(NOT PKGCONFIG_FOUND AND WIN32 AND NOT CYGWIN)
  # MINGW should go here too
  IF(MSVC)
    # Implementation details:
    # We are using the libraries located in the VC subdir instead of the parent directory eventhough :
    FIND_LIBRARY(PIXMAN
      NAMES
        pixman-1
      ${_PIXMAN_ROOT_HINTS_AND_PATHS}
      PATH_SUFFIXES
        "lib"
        "VC"
        "lib/VC"
    )

    MARK_AS_ADVANCED(PIXMAN)
    set( PIXMAN_LIBRARIES ${PIXMAN})
  ELSEIF(MINGW)
    # same player, for MingW
    FIND_LIBRARY(PIXMAN
      NAMES
        pixman-1
      ${_PIXMAN_ROOT_HINTS_AND_PATHS}
      PATH_SUFFIXES
        "lib"
        "lib/MinGW"
    )

    MARK_AS_ADVANCED(PIXMAN)
    set( PIXMAN_LIBRARIES ${PIXMAN})
  ELSE(MSVC)
    # Not sure what to pick for -say- intel, let's use the toplevel ones and hope someone report issues:
    FIND_LIBRARY(PIXMAN
      NAMES
        pixman-1
      HINTS
        ${_PIXMAN_LIBDIR}
      ${_PIXMAN_ROOT_HINTS_AND_PATHS}
      PATH_SUFFIXES
        lib
    )

    MARK_AS_ADVANCED(PIXMAN)
    set( PIXMAN_LIBRARIES ${PIXMAN} )
  ENDIF(MSVC)
ELSE()

  FIND_LIBRARY(PIXMAN_LIBRARY
    NAMES
        pixman-1
    HINTS
      ${_PIXMAN_LIBDIR}
    ${_PIXMAN_ROOT_HINTS_AND_PATHS}
    PATH_SUFFIXES
      "lib"
      "local/lib"
  )

  MARK_AS_ADVANCED(PIXMAN_LIBRARY)

  # compat defines
  SET(PIXMAN_LIBRARIES ${PIXMAN_LIBRARY})

ENDIF()

#message( STATUS "Pixman_FIND_VERSION=${Pixman_FIND_VERSION}.")
#message( STATUS "PIXMAN_INCLUDE_DIR=${PIXMAN_INCLUDE_DIR}.")

# Fetch version from pixman-version.h if a version was requested by find_package()
if(PIXMAN_INCLUDE_DIR AND Pixman_FIND_VERSION)
  file(READ "${PIXMAN_INCLUDE_DIR}/pixman-version.h" _PIXMAN_VERSION_H_CONTENTS)
  string(REGEX REPLACE "^(.*\n)?#define[ \t]+PIXMAN_VERSION_MAJOR[ \t]+([0-9]+).*"
         "\\2" PIXMAN_VERSION_MAJOR ${_PIXMAN_VERSION_H_CONTENTS})
  string(REGEX REPLACE "^(.*\n)?#define[ \t]+PIXMAN_VERSION_MINOR[ \t]+([0-9]+).*"
         "\\2" PIXMAN_VERSION_MINOR ${_PIXMAN_VERSION_H_CONTENTS})
  string(REGEX REPLACE "^(.*\n)?#define[ \t]+PIXMAN_VERSION_MICRO[ \t]+([0-9]+).*"
         "\\2" PIXMAN_VERSION_MICRO ${_PIXMAN_VERSION_H_CONTENTS})
  set(PIXMAN_VERSION ${PIXMAN_VERSION_MAJOR}.${PIXMAN_VERSION_MINOR}.${PIXMAN_VERSION_MICRO}
      CACHE INTERNAL "The version number for Pixman libraries")
endif()

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(Pixman
  REQUIRED_VARS
    PIXMAN_LIBRARIES
    PIXMAN_INCLUDE_DIR
  VERSION_VAR
    PIXMAN_VERSION
)

MARK_AS_ADVANCED(PIXMAN_INCLUDE_DIR PIXMAN_LIBRARIES)
