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
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

find_package( wxWidgets 3.0.0 COMPONENTS gl aui adv html core net base xml stc REQUIRED )

add_compile_definitions( BOOST_TEST_DYN_LINK PCBNEW)

add_dependencies( pnsrouter pcbcommon ${PCBNEW_IO_LIBRARIES} )

add_executable( libeval_compiler_test
    libeval_compiler_test.cpp
    ../qa_utils/mocks.cpp
    ../../common/base_units.cpp
    ../../3d-viewer/3d_viewer/eda_3d_viewer_settings.cpp
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
    common
    pcbcommon
    ${PCBNEW_IO_LIBRARIES}
    common
    pcbcommon
    Boost::headers
    Boost::unit_test_framework
    ${wxWidgets_LIBRARIES}
)
