# CMake script for finding libngspice

# Copyright (C) 2016 CERN
# Author: Maciej Suminski <maciej.suminski@cern.ch>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, you may find one here:
# http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
# or you may search the http://www.gnu.org website for the version 2 license,
# or you may write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA

find_path( NGSPICE_INCLUDE_DIR ngspice/sharedspice.h
    PATHS ${NGSPICE_ROOT_DIR} $ENV{NGSPICE_ROOT_DIR} ${NGSPICE_INCLUDE_PATH}
          ${NGSPICE_ROOT_DIR}/include
    PATH_SUFFIXES src/include share/ngspice/include share/ngspice/include/ngspice
)

find_library( NGSPICE_LIBRARY ngspice
    PATHS ${NGSPICE_ROOT_DIR} $ENV{NGSPICE_ROOT_DIR} ${NGSPICE_LIBRARY_PATH}
    PATH_SUFFIXES src/.libs lib
)

if( WIN32 AND MSYS )
    # NGSPICE_LIBRARY points to libngspice.dll.a on Windows,
    # but the goal is to find out the DLL name.
    # Note: libngspice-0.dll or libngspice-1.dll must be in a executable path
    find_library( NGSPICE_DLL NAMES libngspice-0.dll libngspice-1.dll )

    if( NGSPICE_DLL STREQUAL "NGSPICE_DLL-NOTFOUND" )
        message( ERROR ":\n***** libngspice-x.dll not found in any executable path *****\n\n" )
    endif()
else()
    set( NGSPICE_DLL "${NGSPICE_LIBRARY}" )
endif()


include( FindPackageHandleStandardArgs )

if( ${NGSPICE_INCLUDE_DIR} STREQUAL "NGSPICE_INCLUDE_DIR-NOTFOUND" OR ${NGSPICE_LIBRARY} STREQUAL "NGSPICE_LIBRARY-NOTFOUND" )
    message( "" )
    message( "*** NGSPICE library missing ***" )
    message( "Most of ngspice packages do not provide the required libngspice library." )
    message( "You can either compile ngspice configured with --with-ngshared parameter" )
    message( "or run a script that does the job for you:" )
    message( "  cd ./scripting/build_tools" )
    message( "  chmod +x get_libngspice_so.sh" )
    message( "  ./get_libngspice_so.sh" )
    message( "  sudo ./get_libngspice_so.sh install" )
    message( "" )
endif()

find_package_handle_standard_args( ngspice
	REQUIRED_VARS NGSPICE_INCLUDE_DIR NGSPICE_LIBRARY NGSPICE_DLL )

mark_as_advanced(
    NGSPICE_INCLUDE_DIR
    NGSPICE_LIBRARY
    NGSPICE_DLL
)
