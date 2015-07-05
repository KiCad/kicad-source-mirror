#
#  This program source code file is part of KICAD, a free EDA CAD application.
#
#  Copyright (C) 2010 Wayne Stambaugh <stambaughw@verizon.net>
#  Copyright (C) 2010 Kicad Developers, see AUTHORS.txt for contributors.
#
#  This program is free software; you can redistribute it and/or
#  modify it under the terms of the GNU General Public License
#  as published by the Free Software Foundation; either version 2
#  of the License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, you may find one here:
#  http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
#  or you may search the http://www.gnu.org website for the version 2 license,
#  or you may write to the Free Software Foundation, Inc.,
#  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
#
# This CMake script finds the Bazaar version control system executable and
# and fetches the veresion string to valid that Bazaar was found and executes
# properly.
#
# Usage:
#  find_package( Bazaar )
#
# User definable.
#    Bazaar_EXECUTABLE      Set this to use a version of Bazaar that is not in
#                           current path.  Defaults to bzr.
#
# Defines:
#    Bazaar_FOUND           Set to TRUE if Bazaar command line client is found
#                           and the bzr --version command executes properly.
#    Bazaar_VERSION         Result of the bzr --version command.
#

set( Bazaar_FOUND FALSE )

find_program( Bazaar_EXECUTABLE bzr
              DOC "Bazaar version control system command line client" )
mark_as_advanced( Bazaar_EXECUTABLE )

if( Bazaar_EXECUTABLE )

    # Bazaar commands should be executed with the C locale, otherwise
    # the message (which are parsed) may be translated causing the regular
    # expressions to fail.
    set( _Bazaar_SAVED_LC_ALL "$ENV{LC_ALL}" )
    set( ENV{LC_ALL} C )

    # Fetch the Bazaar executable version.
    execute_process( COMMAND ${Bazaar_EXECUTABLE} --version
                     OUTPUT_VARIABLE _bzr_version_output
                     ERROR_VARIABLE _bzr_version_error
                     RESULT_VARIABLE _bzr_version_result
                     OUTPUT_STRIP_TRAILING_WHITESPACE )

    if( ${_bzr_version_result} EQUAL 0 )
        set( Bazaar_FOUND TRUE )
        string( REGEX REPLACE "^[\n]*Bazaar \\(bzr\\) ([0-9.a-z]+).*"
                "\\1" Bazaar_VERSION "${_bzr_version_output}" )
        message( STATUS "Bazaar version control system version ${Bazaar_VERSION} found." )
    endif()

    # restore the previous LC_ALL
    set( ENV{LC_ALL} ${_Bazaar_SAVED_LC_ALL} )
endif()

if( NOT Bazaar_FOUND )
    if( NOT Bazaar_FIND_QUIETLY )
        message( STATUS "Bazaar version control command line client was not found." )
    else()
        if( Bazaar_FIND_REQUIRED )
            message( FATAL_ERROR "Bazaar version control command line client was not found." )
        endif()
    endif()
endif()
