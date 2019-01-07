# This program source code file is part of KiCad, a free EDA CAD application.
#
# Copyright (C) 2018 KiCad Developers, see CHANGELOG.TXT for contributors.
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


add_executable( qa_pcbnew
    # A single top to load the pcnew kiface
    # ../../common/single_top.cpp

    # The main test entry points
    test_module.cpp

    # stuff from common due to...units?
    ../../common/eda_text.cpp

    # stuff from common which is needed...why?
    ../../common/colors.cpp
    ../../common/observable.cpp

    test_graphics_import_mgr.cpp
    test_pad_naming.cpp

    # Older CMakes cannot link OBJECT libraries
    # https://cmake.org/pipermail/cmake/2013-November/056263.html
    $<TARGET_OBJECTS:pcbnew_kiface_objects>
)

if( BUILD_GITHUB_PLUGIN )
    set( GITHUB_PLUGIN_LIBRARIES github_plugin )
endif()

set_source_files_properties( ../common/single_top.cpp pcbnew.cpp PROPERTIES
    COMPILE_DEFINITIONS "TOP_FRAME=FRAME_PCB;PGM_DATA_FILE_EXT=\"kicad_pcb\";BUILD_KIWAY_DLL"
)

include_directories( BEFORE ${INC_BEFORE} )
include_directories(
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/polygon
    ${CMAKE_SOURCE_DIR}/pcbnew
    ${CMAKE_SOURCE_DIR}/common
    ${CMAKE_SOURCE_DIR}/pcbnew/router
    ${CMAKE_SOURCE_DIR}/pcbnew/tools
    ${CMAKE_SOURCE_DIR}/pcbnew/dialogs
    ${INC_AFTER}
)

target_link_libraries( qa_pcbnew
    3d-viewer
    connectivity
    pcbcommon
    pnsrouter
    pcad2kicadpcb
    common
    legacy_wx
    polygon
    bitmaps
    gal
    qa_utils
    lib_dxf
    idf3
    unit_test_utils
    ${wxWidgets_LIBRARIES}
    ${GITHUB_PLUGIN_LIBRARIES}
    ${GDI_PLUS_LIBRARIES}
    ${PYTHON_LIBRARIES}
    ${Boost_LIBRARIES}      # must follow GITHUB
    ${PCBNEW_EXTRA_LIBS}    # -lrt must follow Boost
)

# we need to pretend to be something to appease the units code
target_compile_definitions( qa_pcbnew
    PRIVATE PCBNEW
)

add_test( NAME pcbnew
    COMMAND qa_pcbnew
)