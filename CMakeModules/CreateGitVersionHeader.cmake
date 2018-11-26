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
#  along with this program; if not, you may find one here:
#  http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
#  or you may search the http://www.gnu.org website for the version 2 license,
#  or you may write to the Free Software Foundation, Inc.,
#  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
#

macro( create_git_version_header _git_src_path )
    # Include Git support to automagically create version header file.
    find_package( Git )

    if( GIT_FOUND )
        set( _Git_SAVED_LC_ALL "$ENV{LC_ALL}" )
        set( ENV{LC_ALL} C )

        # Use `git describe --dirty` to create the KiCad version string.
        execute_process(
            COMMAND
            ${GIT_EXECUTABLE} describe --dirty
            WORKING_DIRECTORY ${_git_src_path}
            OUTPUT_VARIABLE _git_DESCRIBE
            ERROR_VARIABLE _git_log_error
            RESULT_VARIABLE _git_log_result
            OUTPUT_STRIP_TRAILING_WHITESPACE)

        set( ENV{LC_ALL} ${_Git_SAVED_LC_ALL} )
    endif( GIT_FOUND )

    # Check to make sure 'git' command did not fail.  Otherwise fallback
    # to KiCadVersion.cmake as the revision level.
    if( _git_DESCRIBE )
        set( KICAD_VERSION "(${_git_DESCRIBE})" )
    endif()

endmacro()
