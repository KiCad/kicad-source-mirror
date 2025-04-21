# - Find python interpreter
# This module finds if Python interpreter is installed and determines where the
# executables are. This code sets the following variables:
#
#  PYTHONINTERP_FOUND         - Was the Python executable found
#  PYTHON_EXECUTABLE          - path to the Python interpreter
#
#  PYTHON_VERSION_STRING      - Python version found e.g. 2.5.2
#  PYTHON_VERSION_MAJOR       - Python major version found e.g. 2
#  PYTHON_VERSION_MINOR       - Python minor version found e.g. 5
#  PYTHON_VERSION_PATCH       - Python patch version found e.g. 2
#
# The Python_ADDITIONAL_VERSIONS variable can be used to specify a list of
# version numbers that should be taken into account when searching for Python.
# You need to set this variable before calling find_package(PythonInterp).
#
# You can point to a preferred python install to use by setting the following
# to the point at the root directory of the python install:
#
#  PYTHON_ROOT_DIR            - The root directory of the python install
#=============================================================================
# Copyright 2005-2010 Kitware, Inc.
# Copyright 2011 Bjoern Ricks <bjoern.ricks@gmail.com>
# Copyright 2012 Rolf Eike Beer <eike@sf-mail.de>
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

unset(_Python_NAMES)

set(_PYTHON1_VERSIONS 1.6 1.5)
set(_PYTHON2_VERSIONS 2.7 2.6 2.5 2.4 2.3 2.2 2.1 2.0)
set(_PYTHON3_VERSIONS 3.14 3.13 3.12 3.11 3.10 3.9 3.8 3.7 3.6 3.5 3.4 3.3 3.2 3.1 3.0)

# Disabling the "search every possible place" code for now
# see https://gitlab.com/kicad/code/kicad/-/issues/8553

#if(PythonInterp_FIND_VERSION)
#    if(PythonInterp_FIND_VERSION MATCHES "^[0-9]+\\.[0-9]+(\\.[0-9]+.*)?$")
#        string(REGEX REPLACE "^([0-9]+\\.[0-9]+).*" "\\1" _PYTHON_FIND_MAJ_MIN "${PythonInterp_FIND_VERSION}")
#        string(REGEX REPLACE "^([0-9]+).*" "\\1" _PYTHON_FIND_MAJ "${_PYTHON_FIND_MAJ_MIN}")
#        list(APPEND _Python_NAMES python${_PYTHON_FIND_MAJ_MIN} python${_PYTHON_FIND_MAJ})
#        unset(_PYTHON_FIND_OTHER_VERSIONS)
#        if(NOT PythonInterp_FIND_VERSION_EXACT)
#            foreach(_PYTHON_V ${_PYTHON${_PYTHON_FIND_MAJ}_VERSIONS})
#                if(NOT _PYTHON_V VERSION_LESS _PYTHON_FIND_MAJ_MIN)
#                    list(APPEND _PYTHON_FIND_OTHER_VERSIONS ${_PYTHON_V})
#                endif()
#             endforeach()
#        endif()
#        unset(_PYTHON_FIND_MAJ_MIN)
#        unset(_PYTHON_FIND_MAJ)
#    else()
#        list(APPEND _Python_NAMES python${PythonInterp_FIND_VERSION})
#        set(_PYTHON_FIND_OTHER_VERSIONS ${_PYTHON${PythonInterp_FIND_VERSION}_VERSIONS})
#    endif()
#else()
#    set(_PYTHON_FIND_OTHER_VERSIONS ${_PYTHON3_VERSIONS} ${_PYTHON2_VERSIONS} ${_PYTHON1_VERSIONS})
#endif()

list(APPEND _Python_NAMES python3 python)

# Search for the preferred executable first
if( ${PYTHON_ROOT_DIR} )
    # Search for any of the executable names solely in the directory we've
    # been pointed to. Failure to find the python executable here is a fatal
    # fail.
    find_program(PYTHON_EXECUTABLE NAMES ${_Python_NAMES}
        PATHS ${PYTHON_ROOT_DIR}
        NO_DEFAULT_PATH )
elseif(VCPKG_TOOLCHAIN)
    # this is a hack for arm64 builds for now
    # the main problem being nobody seems to actually cross-compile kicad
    # and insteads compiles it painfully on slow hardware
    set(INTERP_TRIPLET "${VCPKG_TARGET_TRIPLET}")
    if(NOT "${CMAKE_HOST_SYSTEM_PROCESSOR}" STREQUAL "ARM64")
        if(${VCPKG_TARGET_TRIPLET} STREQUAL "arm64-windows")
            set(INTERP_TRIPLET "x64-windows")
        endif()
    endif()

    # Our VCPKG usage will always place it in a known location
    find_program(PYTHON_EXECUTABLE
        NAMES ${_Python_NAMES}
        PATHS "${VCPKG_INSTALLED_DIR}/${INTERP_TRIPLET}/tools/python3"
        NO_DEFAULT_PATH
        NO_PACKAGE_ROOT_PATH
        NO_CMAKE_PATH
        NO_CMAKE_ENVIRONMENT_PATH
        NO_CMAKE_SYSTEM_PATH
        NO_SYSTEM_ENVIRONMENT_PATH
    )

else()
    # If there is no specific path given, look for python in the path
    find_program(PYTHON_EXECUTABLE NAMES ${_Python_NAMES})
endif()

# Set up the versions we know about, in the order we will search. Always add
# the user supplied additional versions to the front.
set(_Python_VERSIONS
  ${Python_ADDITIONAL_VERSIONS}
  ${_PYTHON_FIND_OTHER_VERSIONS}
  )

unset(_PYTHON_FIND_OTHER_VERSIONS)
unset(_PYTHON1_VERSIONS)
unset(_PYTHON2_VERSIONS)
unset(_PYTHON3_VERSIONS)

# Search for newest python version if python executable isn't found
if(NOT PYTHON_EXECUTABLE)

    # If using the MINGW compiler, we mustn't find the standard python
    # distribution because of multiple C-Runtime errors. We must instead
    # use the Python-a-mingw-us distribution
    if(MINGW)
        list( APPEND _Python_PPATHS ${PYTHON_ROOT_DIR} )
        list( APPEND _Python_PPATHS "C:/python/${_CURRENT_VERSION}.9" )
        list( APPEND _Python_PPATHS "C:/python/${_CURRENT_VERSION}.8" )
        list( APPEND _Python_PPATHS "C:/python/${_CURRENT_VERSION}.7" )
        list( APPEND _Python_PPATHS "C:/python/${_CURRENT_VERSION}.6" )
        list( APPEND _Python_PPATHS "C:/python/${_CURRENT_VERSION}.5" )
        list( APPEND _Python_PPATHS "C:/python/${_CURRENT_VERSION}.4" )
        list( APPEND _Python_PPATHS "C:/python/${_CURRENT_VERSION}.3" )
        list( APPEND _Python_PPATHS "C:/python/${_CURRENT_VERSION}.2" )
        list( APPEND _Python_PPATHS "C:/python/${_CURRENT_VERSION}.1" )
        list( APPEND _Python_PPATHS "C:/python/${_CURRENT_VERSION}.0" )
    else()
        list( APPEND _Python_PPATHS [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\${_CURRENT_VERSION}\\InstallPath] )
    endif()

    foreach(_CURRENT_VERSION ${_Python_VERSIONS})
      set(_Python_NAMES python${_CURRENT_VERSION})
      if(WIN32)
        list(APPEND _Python_NAMES python)
      endif()
      find_program(PYTHON_EXECUTABLE
        NAMES ${_Python_NAMES}
        PATHS ${_Python_PPATHS}
        )
    endforeach()
endif()

# determine python version string
if(PYTHON_EXECUTABLE)
    execute_process(COMMAND "${PYTHON_EXECUTABLE}" -c
                            "import sys; sys.stdout.write(';'.join([str(x) for x in sys.version_info[:3]]))"
                    OUTPUT_VARIABLE _VERSION
                    RESULT_VARIABLE _PYTHON_VERSION_RESULT
                    ERROR_QUIET)
    if(NOT _PYTHON_VERSION_RESULT)
        string(REPLACE ";" "." PYTHON_VERSION_STRING "${_VERSION}")
        list(GET _VERSION 0 PYTHON_VERSION_MAJOR)
        list(GET _VERSION 1 PYTHON_VERSION_MINOR)
        list(GET _VERSION 2 PYTHON_VERSION_PATCH)
        if(PYTHON_VERSION_PATCH EQUAL 0)
            # it's called "Python 2.7", not "2.7.0"
            string(REGEX REPLACE "\\.0$" "" PYTHON_VERSION_STRING "${PYTHON_VERSION_STRING}")
        endif()
    else()
        # sys.version predates sys.version_info, so use that
        execute_process(COMMAND "${PYTHON_EXECUTABLE}" -c "import sys; sys.stdout.write(sys.version)"
                        OUTPUT_VARIABLE _VERSION
                        RESULT_VARIABLE _PYTHON_VERSION_RESULT
                        ERROR_QUIET)
        if(NOT _PYTHON_VERSION_RESULT)
            string(REGEX REPLACE " .*" "" PYTHON_VERSION_STRING "${_VERSION}")
            string(REGEX REPLACE "^([0-9]+)\\.[0-9]+.*" "\\1" PYTHON_VERSION_MAJOR "${PYTHON_VERSION_STRING}")
            string(REGEX REPLACE "^[0-9]+\\.([0-9])+.*" "\\1" PYTHON_VERSION_MINOR "${PYTHON_VERSION_STRING}")
            if(PYTHON_VERSION_STRING MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+.*")
                string(REGEX REPLACE "^[0-9]+\\.[0-9]+\\.([0-9]+).*" "\\1" PYTHON_VERSION_PATCH "${PYTHON_VERSION_STRING}")
            else()
                set(PYTHON_VERSION_PATCH "0")
            endif()
        else()
            # sys.version was first documented for Python 1.5, so assume
            # this is older.
            set(PYTHON_VERSION_STRING "1.4")
            set(PYTHON_VERSION_MAJOR "1")
            set(PYTHON_VERSION_MAJOR "4")
            set(PYTHON_VERSION_MAJOR "0")
        endif()
    endif()
    unset(_PYTHON_VERSION_RESULT)
    unset(_VERSION)
endif()

# handle the QUIETLY and REQUIRED arguments and set PYTHONINTERP_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PythonInterp REQUIRED_VARS PYTHON_EXECUTABLE VERSION_VAR PYTHON_VERSION_STRING)

mark_as_advanced(PYTHON_EXECUTABLE)
