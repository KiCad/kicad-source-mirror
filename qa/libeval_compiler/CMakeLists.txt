#
# This program source code file is part of KiCad, a free EDA CAD application.
#
# Copyright (C) 2017 CERN
# @author Alejandro García Montoro <alejandro.garciamontoro@gmail.com>
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

find_package(Boost COMPONENTS unit_test_framework REQUIRED)
find_package( wxWidgets 3.0.0 COMPONENTS gl aui adv html core net base xml stc REQUIRED )

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

    generate_lemon_grammar( libeval_grammar2 grammar.lemon grammar.c )

endif()

add_definitions(-DBOOST_TEST_DYN_LINK -DPCBNEW)

if( BUILD_GITHUB_PLUGIN )
    set( GITHUB_PLUGIN_LIBRARIES github_plugin )
endif()

add_dependencies( pnsrouter pcbcommon pcad2kicadpcb libeval_grammar2 ${GITHUB_PLUGIN_LIBRARIES} )

add_executable( libeval_compiler_test
    libeval_compiler_test.cpp
    libeval_compiler.cpp
    ../qa_utils/mocks.cpp
    ../../common/base_units.cpp
    ../../3d-viewer/3d_viewer/3d_viewer_settings.cpp
)

include_directories( BEFORE ${INC_BEFORE} )
include_directories(
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/3d-viewer
    ${CMAKE_SOURCE_DIR}/common
    ${CMAKE_SOURCE_DIR}/pcbnew
    ${CMAKE_SOURCE_DIR}/pcbnew/router
    ${CMAKE_SOURCE_DIR}/pcbnew/tools
    ${CMAKE_SOURCE_DIR}/pcbnew/dialogs
    ${CMAKE_SOURCE_DIR}/polygon
    ${CMAKE_SOURCE_DIR}/common/geometry
    ${CMAKE_SOURCE_DIR}/qa/common
    ${CMAKE_SOURCE_DIR}/qa/qa_utils
    ${Boost_INCLUDE_DIR}
    ${INC_AFTER}
)

target_link_libraries( libeval_compiler_test
    pnsrouter
    common
    pcbcommon
    bitmaps
    pnsrouter
    common
    pcbcommon
    bitmaps
    pnsrouter
    common
    pcbcommon
    bitmaps
    pnsrouter
    common
    pcbcommon
    bitmaps
    gal
    pcad2kicadpcb
    altium2kicadpcb
    common
    pcbcommon
    ${GITHUB_PLUGIN_LIBRARIES}
    common
    pcbcommon
    ${Boost_FILESYSTEM_LIBRARY}
    ${Boost_SYSTEM_LIBRARY}
    ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
    ${wxWidgets_LIBRARIES}
)
