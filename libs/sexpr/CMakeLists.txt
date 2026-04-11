#
#  This program source code file is part of KICAD, a free EDA CAD application.
#
#  Copyright (C) 2019 Kicad Developers, see AUTHORS.txt for contributors.
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

# Build file for the S-Expression library

set( SEXPR_LIB_FILES
    sexpr.cpp
    sexpr_parser.cpp
)

add_library( sexpr STATIC
    ${SEXPR_LIB_FILES}
)

target_include_directories( sexpr PUBLIC
    include
)

# the s-exp parser has a dep on the wxFile methods
target_link_libraries( sexpr
    fmt::fmt
    ${wxWidgets_LIBRARIES}
)

# TODO get deps from here out of common
target_include_directories( sexpr PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)