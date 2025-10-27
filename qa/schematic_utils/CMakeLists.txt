# This program source code file is part of KiCad, a free EDA CAD application.
#
# Copyright (C) 2022 KiCad Developers, see CHANGELOG.TXT for contributors.
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
# along with this program; if not, you may find one here:
# http://www.gnu.org/licenses/

# Eeschema-related auxiliary functions that are useful for QA purposes

set( QA_SCHEMATIC_UTILS_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/eeschema_test_utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/schematic_file_util.cpp
)

add_library( qa_schematic_utils STATIC ${QA_SCHEMATIC_UTILS_SRCS} )


target_include_directories( qa_schematic_utils BEFORE PUBLIC ${INC_BEFORE} )

target_include_directories( qa_schematic_utils PUBLIC
    include

    # target_include_directories and made PUBLIC)
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/eeschema
    ${CMAKE_SOURCE_DIR}/pcbnew
    ${CMAKE_SOURCE_DIR}/common
    ${INC_AFTER}
)

target_link_libraries( qa_schematic_utils PUBLIC
    qa_utils
)

# # we need to pretend to be something to appease the units code
target_compile_definitions( qa_schematic_utils
    PUBLIC EESCHEMA
)

target_include_directories( qa_schematic_utils PRIVATE
    $<TARGET_PROPERTY:eeschema_kiface_objects,INTERFACE_INCLUDE_DIRECTORIES>
    )

# Ensure that schematic_lexer.h, which needs to be included, is
# created before building this
add_dependencies( qa_schematic_utils eeschema_kiface_objects)
