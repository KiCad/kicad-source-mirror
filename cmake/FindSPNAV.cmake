# BSD 3-Clause License

# Copyright (c) 2008, Willow Garage, Inc.
# Copyright (c) 2020, Nils Schulte
# All rights reserved.

# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:

# * Redistributions of source code must retain the above copyright notice, this
#   list of conditions and the following disclaimer.

# * Redistributions in binary form must reproduce the above copyright notice,
#   this list of conditions and the following disclaimer in the documentation
#   and/or other materials provided with the distribution.

# * Neither the name of the copyright holder nor the names of its
#   contributors may be used to endorse or promote products derived from
#   this software without specific prior written permission.

# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# Find the spnav library and header.
#
# Sets the usual variables expected for find_package scripts:
#
# SPNAV_INCLUDE_DIR - header location
# SPNAV_LIBRARIES - library to link against
# SPNAV_FOUND - true if pugixml was found.

if(UNIX)

  find_path(SPNAV_INCLUDE_DIR spnav.h)

  find_library(SPNAV_LIBRARY
    NAMES
    spnav libspnav
)

# Support the REQUIRED and QUIET arguments, and set spnav_FOUND if found.
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SPNAV DEFAULT_MSG
  SPNAV_LIBRARY
  SPNAV_INCLUDE_DIR)

if(SPNAV_FOUND)
  set(SPNAV_LIBRARIES ${SPNAV_LIBRARY})
endif()

mark_as_advanced(
  SPNAV_LIBRARY
  SPNAV_INCLUDE_DIR)

endif()