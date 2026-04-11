# This program source code file is part of KiCad, a free EDA CAD application.
#
# Copyright The KiCad Developers, see AUTHORS.txt for contributors.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 3
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

# Shared PADS format support library
# This library contains common utilities for parsing PADS files that are used
# by both pcbnew and eeschema importers.

set( PADS_COMMON_SRCS
    pads_attribute_mapper.cpp
    pads_common.cpp
    pads_unit_converter.cpp
)

add_library( pads_common STATIC
    ${PADS_COMMON_SRCS}
)

target_include_directories( pads_common PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/common
    ${CMAKE_SOURCE_DIR}/libs/kimath/include
    ${CMAKE_SOURCE_DIR}/libs/core/include
)

target_include_directories( pads_common PRIVATE
    ${CMAKE_BINARY_DIR}
)

target_link_libraries( pads_common
    kicommon
)
