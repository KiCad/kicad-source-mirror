#
#  This program source code file is part of KICAD, a free EDA CAD application.
#
#  Copyright (C) 2018 Kicad Developers, see AUTHORS.txt for contributors.
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

find_program(LEMON lemon)

if( LEMON )

    macro( generate_lemon_grammar TGT_NAME GRAMMAR_LEMON GRAMMAR_C )
        add_custom_target( ${TGT_NAME}
            DEPENDS ${GRAMMAR_LEMON}
            COMMAND ${LEMON} -q ${GRAMMAR_LEMON}
            COMMENT "Running Lemon on ${GRAMMAR_LEMON} -> ${GRAMMAR_C}"
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        )
    endmacro()

    generate_lemon_grammar( libeval_grammar grammar.lemon grammar.c )

endif()