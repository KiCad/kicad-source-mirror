#  This program source code file is part of KICAD, a free EDA CAD application.
#
#  Copyright (C) 2025 Kicad Developers, see AUTHORS.txt for contributors.
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

# Read the input file
file(STRINGS "${INPUT_FILE}" LINES)

# Ensure output directory exists
get_filename_component(OUTPUT_DIR "${OUTPUT_FILE}" DIRECTORY)
file(MAKE_DIRECTORY "${OUTPUT_DIR}")

set(DICT_ENTRIES "")
foreach(LINE ${LINES})
    string(STRIP "${LINE}" LINE)
    # Skip empty lines and comments
    if(LINE STREQUAL "" OR LINE MATCHES "^#")
        continue()
    endif()
    list(APPEND DICT_ENTRIES "\"${LINE}\"")
endforeach()

list(JOIN DICT_ENTRIES "\n" DICT_CONTENT)
file(WRITE "${OUTPUT_FILE}" "# Auto-generated libfuzzer dictionary\n${DICT_CONTENT}")
