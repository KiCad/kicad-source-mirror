#
#  This program source code file is part of KiCad, a free EDA CAD application.
#
#  Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

# Extract the active (uncommented) format-version #define from an authoritative ledger
# header and re-export it under a caller-chosen name. The ledgers comment out historical
# versions with a leading "//", so anchoring the regex at "^#define" selects only the live one.
function( _extract_sexpr_version aHeader aDefineName aOutVar )
    file( STRINGS "${aHeader}" _matches
          REGEX "^[ \t]*#[ \t]*define[ \t]+${aDefineName}[ \t]+[0-9]+" )

    list( LENGTH _matches _count )

    if( NOT _count EQUAL 1 )
        message( FATAL_ERROR
                 "Expected exactly one active '${aDefineName}' in ${aHeader}, found ${_count}." )
    endif()

    string( REGEX REPLACE "^[ \t]*#[ \t]*define[ \t]+${aDefineName}[ \t]+([0-9]+).*" "\\1"
            _value "${_matches}" )

    set( ${aOutVar} "${_value}" PARENT_SCOPE )
endfunction()

set( _board_versions_header "${PROJECT_SOURCE_DIR}/pcbnew/pcb_io/kicad_sexpr/pcb_io_kicad_sexpr.h" )
set( _symbol_versions_header "${PROJECT_SOURCE_DIR}/eeschema/sch_file_versions.h" )

_extract_sexpr_version( "${_board_versions_header}"  SEXPR_BOARD_FILE_VERSION      SEXPR_BOARD_FILE_VERSION )
_extract_sexpr_version( "${_symbol_versions_header}" SEXPR_SYMBOL_LIB_FILE_VERSION SEXPR_SYMBOL_LIB_FILE_VERSION )

configure_file( "${KICAD_CMAKE_MODULE_PATH}/sexpr_file_versions.h.in"
                "${CMAKE_BINARY_DIR}/io/kicad/sexpr_file_versions.h"
                @ONLY )

# Re-run configure (and thus regenerate the header) whenever a ledger changes.
set_property( DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
              "${_board_versions_header}" "${_symbol_versions_header}" )
