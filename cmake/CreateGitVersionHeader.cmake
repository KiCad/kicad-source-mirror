#
#  This program source code file is part of KICAD, a free EDA CAD application.
#
#  Copyright (C) 2010 Wayne Stambaugh <stambaughw@gmail.com>
#  Copyright (C) 2010-2018 KiCad Developers, see AUTHORS.txt for contributors.
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
#  along with this program.  If not, see <https://www.gnu.org/licenses/>.
#

macro( create_git_version_header _git_src_path )
    # Include Git support to automagically create version header file.
    find_package( Git )

    if( GIT_FOUND )
        message( STATUS "Using Git to determine build version string." )

        set( _Git_SAVED_LC_ALL "$ENV{LC_ALL}" )
        set( ENV{LC_ALL} C )

        # Use `git describe --dirty` to create the KiCad version string.
        execute_process(
            COMMAND
            ${GIT_EXECUTABLE} describe --dirty
            WORKING_DIRECTORY ${_git_src_path}
            OUTPUT_VARIABLE _git_DESCRIBE
            ERROR_VARIABLE _git_describe_error
            RESULT_VARIABLE _git_describe_result
            OUTPUT_STRIP_TRAILING_WHITESPACE)

        execute_process(
            COMMAND
            ${GIT_EXECUTABLE} rev-list --count --first-parent HEAD
            WORKING_DIRECTORY ${_git_src_path}
            OUTPUT_VARIABLE _git_REV_COUNT
            ERROR_VARIABLE _git_rev_count_error
            RESULT_VARIABLE _git_rev_count_result
            OUTPUT_STRIP_TRAILING_WHITESPACE)

        execute_process(
            COMMAND
            ${GIT_EXECUTABLE} rev-parse HEAD
            WORKING_DIRECTORY ${_git_src_path}
            OUTPUT_VARIABLE _git_REV_PARSE_HEAD
            ERROR_VARIABLE _git_rev_parse_head_error
            RESULT_VARIABLE _git_rev_parse_head_result
            OUTPUT_STRIP_TRAILING_WHITESPACE)

        execute_process(
            COMMAND
            ${GIT_EXECUTABLE} show -s --format=%cd --date=format:%Y-%m-%d
            WORKING_DIRECTORY ${_git_src_path}
            OUTPUT_VARIABLE _git_SHOW_DATE_HEAD
            ERROR_VARIABLE _git_show_date_head_error
            RESULT_VARIABLE _git_show_date_head_result
            OUTPUT_STRIP_TRAILING_WHITESPACE)

        set( ENV{LC_ALL} ${_Git_SAVED_LC_ALL} )
    endif( GIT_FOUND )

    # Check to make sure 'git' command did not fail.  Otherwise fallback
    # to KiCadVersion.cmake as the revision level.
    if( _git_describe_result EQUAL 0 )
        set( KICAD_VERSION "${_git_DESCRIBE}" )
    else()
        message( STATUS "git describe returned error ${_git_describe_result}: ${_git_describe_error}" )
    endif()

    if( _git_rev_parse_head_result EQUAL 0 )
        set( KICAD_COMMIT_HASH "${_git_REV_PARSE_HEAD}" )
    else()
        message( STATUS "git rev-parse returned error ${_git_rev_parse_head_result}: ${_git_rev_parse_head_error}" )
        # placeholder if we can't get a real hash
        set( KICAD_COMMIT_HASH "0000000000000000000000000000000000000000" )
    endif()

    if( _git_rev_count_result EQUAL 0 )
        set( KICAD_GIT_REV "${_git_REV_COUNT}" )

        # Sanity check
        if (NOT KICAD_GIT_REV MATCHES "^[0-9]+$")
            set( KICAD_GIT_REV "0" )
        endif ()
    else()
        message( STATUS "git rev-list --count returned error ${_git_rev_count_result}: ${_git_rev_count_error}" )
        # Incase the command failed, we can just default to 0, only a problem in CI right now
        set( KICAD_GIT_REV "0" )
    endif()

    if( _git_show_date_head_result EQUAL 0 )
        set( KICAD_COMMIT_DATE "${_git_SHOW_DATE_HEAD}" )
    else()
        message( STATUS "git show -s -format=%cd --date=format:%Y-%m-%d returned error ${_git_show_date_head_result}: ${_git_show_date_head_error}" )
        # Incase command failed, default to 0 to flag a problem
        set( KICAD_COMMIT_DATE "0")
    endif()

endmacro()
