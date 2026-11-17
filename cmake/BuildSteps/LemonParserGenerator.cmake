#  This program source code file is part of KICAD, a free EDA CAD application.
#
#  Copyright (C) 2020 Kicad Developers, see AUTHORS.txt for contributors.
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

# This file takes the following variables as arguments:
#   * LEMON_EXE - The absolute path to the lemon executable
#   * LEMON_TEMPLATE - The absolute path to the lemon template file
#   * GRAMMAR_FILE - The file of the grammar to use
#   * GRAMMAR_DIR - An absolute path to where the grammar should be generated


# Get the name without extension
get_filename_component( GRAMMAR_BASE ${GRAMMAR_FILE} NAME_WE )

# The custom command decides when to regenerate, so a new lemon or template reaches an existing build
execute_process(
    COMMAND ${LEMON_EXE} -T${LEMON_TEMPLATE} -d${GRAMMAR_DIR} -q ${GRAMMAR_FILE}
    WORKING_DIRECTORY ${GRAMMAR_DIR}
    OUTPUT_VARIABLE _lemon_output
    ERROR_VARIABLE  _lemon_error
    RESULT_VARIABLE _lemon_result
    OUTPUT_STRIP_TRAILING_WHITESPACE
    )

if( NOT "${_lemon_result}" EQUAL 0 )
    # A partial output newer than its inputs would pass for current, so the next build would not retry
    file( REMOVE ${GRAMMAR_DIR}/${GRAMMAR_BASE}.c ${GRAMMAR_DIR}/${GRAMMAR_BASE}.h )
    message( FATAL_ERROR "Lemon generator for ${GRAMMAR_FILE} has failed\n"
                         "Error: ${_lemon_error}" )
endif()

# Lemon leaves an unchanged header alone, which would leave this command out of date on every build
file( TOUCH ${GRAMMAR_DIR}/${GRAMMAR_BASE}.h )
