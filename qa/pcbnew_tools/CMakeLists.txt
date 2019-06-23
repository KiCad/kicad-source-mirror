# This program source code file is part of KiCad, a free EDA CAD application.
#
# Copyright (C) 2019 KiCad Developers, see CHANGELOG.TXT for contributors.
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

add_executable( qa_pcbnew_tools

    # The main entry point
    pcbnew_tools.cpp

    tools/drc_tool/drc_tool.cpp

    tools/pcb_parser/pcb_parser_tool.cpp

    tools/polygon_generator/polygon_generator.cpp

    tools/polygon_triangulation/polygon_triangulation.cpp

    # Older CMakes cannot link OBJECT libraries
    # https://cmake.org/pipermail/cmake/2013-November/056263.html
    $<TARGET_OBJECTS:pcbnew_kiface_objects>
)

# Anytime we link to the kiface_objects, we have to add a dependency on the last object
# to ensure that the generated lexer files are finished being used before the qa runs in a
# multi-threaded build
add_dependencies( qa_pcbnew_tools pcbnew )

target_link_libraries( qa_pcbnew_tools
    qa_pcbnew_utils
    3d-viewer
    connectivity
    pcbcommon
    pnsrouter
    pcad2kicadpcb
    gal
    lib_dxf
    idf3
    common
    qa_utils
    unit_test_utils
    ${wxWidgets_LIBRARIES}
    ${GITHUB_PLUGIN_LIBRARIES}
    ${GDI_PLUS_LIBRARIES}
    ${PYTHON_LIBRARIES}
    ${Boost_LIBRARIES}      # must follow GITHUB
    ${PCBNEW_EXTRA_LIBS}    # -lrt must follow Boost
)

kicad_add_utils_executable( qa_pcbnew_tools )