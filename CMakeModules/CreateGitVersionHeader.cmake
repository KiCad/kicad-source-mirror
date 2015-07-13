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

macro( create_git_version_header _git_src_path )
    # If bzr is not found or an error occurs using the git commands to determine the repo
    # version, set the build version string to "no-git"
    set( KICAD_BUILD_VERSION "no-git" )

    # Include Git support to automagically create version header file.
    find_package( Git )

    if( GIT_FOUND )
        set( _Git_SAVED_LC_ALL "$ENV{LC_ALL}" )
        set( ENV{LC_ALL} C )

        # Get latest commit hash
        execute_process(
            COMMAND
            ${GIT_EXECUTABLE} --no-pager log -1 HEAD
            --pretty=format:%H
            WORKING_DIRECTORY ${_git_src_path}
            OUTPUT_VARIABLE _git_LONG_HASH
            ERROR_VARIABLE _git_log_error
            RESULT_VARIABLE _git_log_result
            OUTPUT_STRIP_TRAILING_WHITESPACE)

        if( ${_git_log_result} EQUAL 0 )
            execute_process(
            COMMAND
            ${GIT_EXECUTABLE} --no-pager log -1 HEAD
            --pretty=format:%h
            WORKING_DIRECTORY ${_git_src_path}
            OUTPUT_VARIABLE _git_SHORT_HASH
            OUTPUT_STRIP_TRAILING_WHITESPACE)

            execute_process(
            COMMAND
            ${GIT_EXECUTABLE} --no-pager log -1 HEAD
            --pretty=format:%cn
            WORKING_DIRECTORY ${_git_src_path}
            OUTPUT_VARIABLE _git_LAST_COMITTER
            OUTPUT_STRIP_TRAILING_WHITESPACE)

            execute_process(
            COMMAND
            ${GIT_EXECUTABLE} --no-pager log -1 HEAD
            --pretty=format:%cd --date=short
            WORKING_DIRECTORY ${_git_src_path}
            OUTPUT_VARIABLE _git_LAST_CHANGE_LOG
            OUTPUT_STRIP_TRAILING_WHITESPACE)

            execute_process(
            COMMAND
            ${GIT_EXECUTABLE} rev-list HEAD --count
            --first-parent
            WORKING_DIRECTORY ${_git_src_path}
            OUTPUT_VARIABLE _git_SERIAL
            OUTPUT_STRIP_TRAILING_WHITESPACE)

            message(STATUS "Git hash: ${_git_LONG_HASH}")

            if( ${_git_log_result} EQUAL 0 )
                string( REGEX REPLACE "^(.*\n)?revno: ([^ \n]+).*"
                        "\\2" Kicad_REPO_REVISION "BZR ${_git_SERIAL}, Git ${_git_SHORT_HASH}" )
                string( REGEX REPLACE "^(.*\n)?committer: ([^\n]+).*"
                        "\\2" Kicad_REPO_LAST_CHANGED_AUTHOR "${_git_LAST_COMITTER}")
                string( REGEX REPLACE "^(.*\n)?timestamp: [a-zA-Z]+ ([^ \n]+).*"
                        "\\2" Kicad_REPO_LAST_CHANGED_DATE "${_git_LAST_CHANGE_LOG}")
            endif()
        endif()

        set( ENV{LC_ALL} ${_Git_SAVED_LC_ALL} )
    endif( GIT_FOUND )

    # Check to make sure 'git' command did not fail.  Otherwise fallback
    # to "no-git" as the revision.
    if( Kicad_REPO_LAST_CHANGED_DATE )
        string( REGEX REPLACE "^([0-9]+)\\-([0-9]+)\\-([0-9]+)" "\\1-\\2-\\3"
                _kicad_git_date ${Kicad_REPO_LAST_CHANGED_DATE} )
        set( KICAD_BUILD_VERSION "(${_kicad_git_date} ${Kicad_REPO_REVISION})" )
    endif()

    set( KICAD_BUILD_VERSION ${KICAD_BUILD_VERSION} )
endmacro()
