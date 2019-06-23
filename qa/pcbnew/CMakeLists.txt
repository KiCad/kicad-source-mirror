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

if( BUILD_GITHUB_PLUGIN )
    set( GITHUB_PLUGIN_LIBRARIES github_plugin )
endif()

add_executable( qa_pcbnew
    # A single top to load the pcnew kiface
    # ../../common/single_top.cpp

    # stuff from common due to...units?
    ../../common/eda_text.cpp

    # stuff from common which is needed...why?
    ../../common/colors.cpp
    ../../common/observable.cpp

    # The main test entry points
    test_module.cpp

    # testing utility routines
    board_test_utils.cpp
    drc/drc_test_utils.cpp

    # test compilation units (start test_)
    test_array_pad_name_provider.cpp
    test_graphics_import_mgr.cpp
    test_pad_naming.cpp

    drc/test_drc_courtyard_invalid.cpp
    drc/test_drc_courtyard_overlap.cpp

    # Older CMakes cannot link OBJECT libraries
    # https://cmake.org/pipermail/cmake/2013-November/056263.html
    $<TARGET_OBJECTS:pcbnew_kiface_objects>
)

# Anytime we link to the kiface_objects, we have to add a dependency on the last object
# to ensure that the generated lexer files are finished being used before the qa runs in a
# multi-threaded build
add_dependencies( qa_pcbnew pcbnew )

target_link_libraries( qa_pcbnew
    qa_pcbnew_utils
    3d-viewer
    connectivity
    pcbcommon
    pnsrouter
    pcad2kicadpcb
    gal
    common
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

kicad_add_boost_test( qa_pcbnew pcbnew )