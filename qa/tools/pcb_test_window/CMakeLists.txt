
#
# This program source code file is part of KiCad, a free EDA CAD application.
#
# Copyright (C) 2017-2020 CERN
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

find_package( Boost COMPONENTS unit_test_framework REQUIRED )
find_package( wxWidgets 3.0.0 COMPONENTS gl aui adv html core net base xml propgrid stc REQUIRED )

add_compile_definitions( BOOST_TEST_DYN_LINK PCBNEW )

add_dependencies( pnsrouter pcbcommon ${PCBNEW_IO_LIBRARIES} )

add_executable(test_window WIN32
    test.cpp
    ${CMAKE_SOURCE_DIR}/qa/qa_utils/pcb_test_frame.cpp
    ${CMAKE_SOURCE_DIR}/qa/qa_utils/mocks.cpp

    ${CMAKE_SOURCE_DIR}/common/base_units.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/tools/pcb_tool_base.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/tools/pcbnew_selection.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/tools/selection_tool.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/tools/tool_event_utils.cpp
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
    ${CMAKE_SOURCE_DIR}/common/geometry
    ${CMAKE_SOURCE_DIR}/qa/qa_utils
    ${Boost_INCLUDE_DIR}
    ${INC_AFTER}
)

target_link_libraries( test_window
    pnsrouter
    pcbcommon
    3d-viewer
    gal
    common
    ${PCBNEW_IO_LIBRARIES}
    Boost::headers
    Boost::unit_test_framework
    ${wxWidgets_LIBRARIES}
)
