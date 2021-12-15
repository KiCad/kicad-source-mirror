# This program source code file is part of KiCad, a free EDA CAD application.
#
# Copyright (C) 2019 KiCad Developers, see AUTHORS.txt for contributors.
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
# along with this program; if not, you may find one here:
# http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
# or you may search the http://www.gnu.org website for the version 2 license,
# or you may write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA

# kicad2step s-expr handling routines

if( NOT TARGET kicad2step_lib )
    # Can't build this test without the underlying library
    return()
endif()

set( K2S_TEST_SRCS
    test_module.cpp
    pcb/test_base.cpp
)

if( MINGW )
    list( APPEND K2S_TEST_SRCS ${CMAKE_SOURCE_DIR}/common/streamwrapper.cpp )
endif( MINGW )

add_executable( qa_kicad2step ${K2S_TEST_SRCS} )

target_link_libraries( qa_kicad2step
    kicad2step_lib
    qa_utils
    ${wxWidgets_LIBRARIES}
)

target_include_directories( qa_sexpr PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

kicad_add_boost_test( qa_kicad2step qa_kicad2step )
