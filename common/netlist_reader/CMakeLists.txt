# This program source code file is part of KiCad, a free EDA CAD application.
#
# Copyright (C) 2024 KiCad Developers, see AUTHORS.txt for contributors.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

set( NETLIST_READER_SRCS
    netlist.cpp
    netlist_reader.cpp
    legacy_netlist_reader.cpp
    kicad_netlist_reader.cpp
)

# Create an object library so the sources can be used in different targets
add_library( netlist_reader_obj OBJECT
    ${NETLIST_READER_SRCS}
)

# The netlist reader needs the same includes as common
target_include_directories( netlist_reader_obj PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/common
    ${CMAKE_SOURCE_DIR}/libs/kimath/include
    ${CMAKE_SOURCE_DIR}/libs/kicommon/include
    ${CMAKE_SOURCE_DIR}/libs/core/include
    ${CMAKE_SOURCE_DIR}/thirdparty/nlohmann_json
    ${CMAKE_BINARY_DIR}/common  # For generated netlist_lexer.h
    ${CMAKE_BINARY_DIR}  # For kicad_build_version.h
)

# Ensure netlist_lexer.h is generated before compiling netlist_reader_obj
# The lexer files are generated as part of the kicommon target
add_dependencies( netlist_reader_obj kicommon )

target_include_directories( netlist_reader_obj PUBLIC
    ${CMAKE_SOURCE_DIR}/common  # So users can find netlist_reader/netlist.h
)
