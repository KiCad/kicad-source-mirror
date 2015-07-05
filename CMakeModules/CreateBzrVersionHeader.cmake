#
#  This program source code file is part of KICAD, a free EDA CAD application.
#
#  Copyright (C) 2010 Wayne Stambaugh <stambaughw@verizon.net>
#  Copyright (C) 2010-2015 Kicad Developers, see AUTHORS.txt for contributors.
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

include( WriteVersionHeader )

macro( create_bzr_version_header )
    # If bzr is not found or an error occurs using the bzr commands to determine the repo
    # version, set the build version string to "no-bzr"
    set( KICAD_BUILD_VERSION "no-bzr" )

    # Include Bazaar support to automagically create version header file.
    find_package( Bazaar )

    if( Bazaar_FOUND )
        set( _Bazaar_SAVED_LC_ALL "$ENV{LC_ALL}" )
        set( ENV{LC_ALL} C )

        # Get the tree revision
        execute_process(
            COMMAND ${Bazaar_EXECUTABLE} revno --tree ${PROJECT_SOURCE_DIR}
            OUTPUT_VARIABLE _bzr_TREE_DATE
            RESULT_VARIABLE _bzr_revno_result
            OUTPUT_STRIP_TRAILING_WHITESPACE )

        if( ${_bzr_revno_result} EQUAL 0 )
            # Get more info about that revision
            execute_process(
                COMMAND ${Bazaar_EXECUTABLE} log -r${_bzr_TREE_DATE} ${PROJECT_SOURCE_DIR}
                OUTPUT_VARIABLE _bzr_LAST_CHANGE_LOG
                ERROR_VARIABLE _bzr_log_error
                RESULT_VARIABLE _bzr_log_result
                OUTPUT_STRIP_TRAILING_WHITESPACE )

            if( ${_bzr_log_result} EQUAL 0 )
                string( REGEX REPLACE "^(.*\n)?revno: ([^ \n]+).*"
                    "\\2" Kicad_REPO_REVISION "${_bzr_LAST_CHANGE_LOG}" )
                string( REGEX REPLACE "^(.*\n)?committer: ([^\n]+).*"
                    "\\2" Kicad_REPO_LAST_CHANGED_AUTHOR "${_bzr_LAST_CHANGE_LOG}" )
                string( REGEX REPLACE "^(.*\n)?timestamp: [a-zA-Z]+ ([^ \n]+).*"
                    "\\2" Kicad_REPO_LAST_CHANGED_DATE "${_bzr_LAST_CHANGE_LOG}" )
            endif()
        endif()

        set( ENV{LC_ALL} ${_Bazaar_SAVED_LC_ALL} )
    endif()

    # Check to make sure 'bzr log' command did not fail.  Otherwise, default
    # to "no-bzr" as the revision.
    if( Kicad_REPO_LAST_CHANGED_DATE )
        string( REGEX REPLACE "^([0-9]+)\\-([0-9]+)\\-([0-9]+)" "\\1-\\2-\\3"
            _kicad_bzr_date ${Kicad_REPO_LAST_CHANGED_DATE} )
      	set( KICAD_BUILD_VERSION "(${_kicad_bzr_date} BZR ${Kicad_REPO_REVISION})" )
    endif()

    write_version_header( ${KICAD_BUILD_VERSION} )
endmacro()
