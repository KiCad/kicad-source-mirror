#.rst:
# FindSWIG
# --------
#
# Find SWIG
#
# This module finds an installed SWIG.  It sets the following variables:
#
# ::
#
#   SWIG_FOUND - set to true if SWIG is found
#   SWIG_DIR - the directory where swig is installed
#   SWIG_EXECUTABLE - the path to the swig executable
#   SWIG_VERSION   - the version number of the swig executable
#
#
#
# The minimum required version of SWIG can be specified using the
# standard syntax, e.g.  find_package(SWIG 1.1)
#
# All information is collected from the SWIG_EXECUTABLE so the version
# to be found can be changed from the command line by means of setting
# SWIG_EXECUTABLE

#=============================================================================
# Copyright 2004-2009 Kitware, Inc.
# Copyright 2011 Mathieu Malaterre <mathieu.malaterre@gmail.com>
# Copyright 2014 Sylvain Joubert <joubert.sy@gmail.com>
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# * Redistributions of source code must retain the above copyright
#   notice, this list of conditions and the following disclaimer.
#
# * Redistributions in binary form must reproduce the above copyright
#   notice, this list of conditions and the following disclaimer in the
#   documentation and/or other materials provided with the distribution.
#
# * Neither the names of Kitware, Inc., the Insight Software Consortium,
#   nor the names of their contributors may be used to endorse or promote
#   products derived from this software without specific prior written
#   permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

find_program(SWIG_EXECUTABLE NAMES swig4.2 swig4.1 swig4.0 swig3.0 swig2.0 swig)

if(SWIG_EXECUTABLE)
  execute_process(COMMAND ${SWIG_EXECUTABLE} -swiglib
    OUTPUT_VARIABLE SWIG_swiglib_output
    ERROR_VARIABLE SWIG_swiglib_error
    RESULT_VARIABLE SWIG_swiglib_result)

  if(SWIG_swiglib_result)
    if(SWIG_FIND_REQUIRED)
      message(SEND_ERROR "Command \"${SWIG_EXECUTABLE} -swiglib\" failed with output:\n${SWIG_swiglib_error}")
    else()
      message(STATUS "Command \"${SWIG_EXECUTABLE} -swiglib\" failed with output:\n${SWIG_swiglib_error}")
    endif()
  else()
    string(REGEX REPLACE "[\n\r]+" ";" SWIG_swiglib_output ${SWIG_swiglib_output})
    find_path(SWIG_DIR swig.swg PATHS ${SWIG_swiglib_output} NO_CMAKE_FIND_ROOT_PATH)
    if(SWIG_DIR)
      set(SWIG_USE_FILE UseSWIG)
      execute_process(COMMAND ${SWIG_EXECUTABLE} -version
        OUTPUT_VARIABLE SWIG_version_output
        ERROR_VARIABLE SWIG_version_output
        RESULT_VARIABLE SWIG_version_result)
      if(SWIG_version_result)
        message(SEND_ERROR "Command \"${SWIG_EXECUTABLE} -version\" failed with output:\n${SWIG_version_output}")
      else()
        string(REGEX REPLACE ".*SWIG Version[^0-9.]*\([0-9.]+\).*" "\\1"
          SWIG_version_output "${SWIG_version_output}")
        set(SWIG_VERSION ${SWIG_version_output} CACHE STRING "Swig version" FORCE)
      endif()
    endif()
  endif()
endif()

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(SWIG  REQUIRED_VARS SWIG_EXECUTABLE SWIG_DIR
                                        VERSION_VAR SWIG_VERSION )

mark_as_advanced(SWIG_DIR SWIG_VERSION)
