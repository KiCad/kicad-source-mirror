#
#  This program source code file is part of KICAD, a free EDA CAD application.
#
#  Copyright (C) 2021 Ian McInerney <Ian.S.McInerney@ieee.org>
#  Copyright (C) 2021 KiCad Developers, see AUTHORS.txt for contributors.
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

# This file will translate a linux metadata file using msgfmt

# It requires the following variables to be defined before its call:
# MSGFMT_EXE - The executable to run
# PO_DIR - The directory containing the .po files
# SRC_FILE - The full path to the source file
# DEST_FILE - The full path to the destination file

get_filename_component( SRC_FNAME  ${SRC_FILE}  NAME )
get_filename_component( DEST_FNAME ${DEST_FILE} NAME )
get_filename_component( DEST_DIR   ${DEST_FILE} DIRECTORY )

# Figure out the type of file we are translating
set( OPT_TYPE "" )

# This requires a double \\ to properly escape the regex character for some reason.
# Without it it throws an error about "Invalid escape sequence \."
if( "${SRC_FNAME}" MATCHES ".*\\.desktop\\.in" )
    set( OPT_TYPE "--desktop" )
elseif( "${SRC_FNAME}" MATCHES ".*\\.xml\\.in" )
    set( OPT_TYPE "--xml" )
endif()

# Execute the translation process
execute_process(
    COMMAND ${MSGFMT_EXE} ${OPT_TYPE} --template=${SRC_FILE} -d ${PO_DIR} -o ${DEST_FILE}
    WORKING_DIRECTORY ${DEST_DIR}
    OUTPUT_VARIABLE _msgfmt_output
    ERROR_VARIABLE  _msgfmt_error
    RESULT_VARIABLE _msgfmt_result
    OUTPUT_STRIP_TRAILING_WHITESPACE
    )

# Capture the return value and parse it to see if the translation failed
if( NOT ${_msgfmt_result} EQUAL 0)
    message( WARNING "Unable to translate file ${SRC_FNAME}\n"
                         "Error: ${_msgfmt_error}" )

    # If the translation failed, just copy the file from source to destination
    message( STATUS "Copying file ${SRC_FNAME} to ${DEST_FNAME} instead." )
    execute_process( COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SRC_FILE}" "${DEST_FILE}" )
endif()
