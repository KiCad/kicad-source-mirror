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
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

add_executable( qa_pcbnew_tools
    # This is needed for the global mock objects
    ${CMAKE_SOURCE_DIR}/qa/mocks/kicad/common_mocks.cpp

    # The main entry point
    pcbnew_tools.cpp

    tools/pcb_parser/pcb_parser_tool.cpp

    tools/fill_bench/fill_bench_tool.cpp

    tools/polygon_generator/polygon_generator.cpp

    tools/polygon_triangulation/polygon_triangulation.cpp

    tools/extract_zone_fills/extract_zone_fills.cpp
)

include_directories(
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/qa/mocks/include
    ${INC_AFTER}
)

# Anytime we link to the kiface_objects, we have to add a dependency on the last object
# to ensure that the generated lexer files are finished being used before the qa runs in a
# multi-threaded build
add_dependencies( qa_pcbnew_tools pcbnew )

target_link_libraries( qa_pcbnew_tools
    pcbnew_kiface_objects
    qa_pcbnew_utils
    3d-viewer
    connectivity
    pcbcommon
    pnsrouter
    gal
    dxflib_qcad
    tinyspline_lib
    nanosvg
    idf3
    common
    qa_utils
    markdown_lib
    ${PCBNEW_IO_LIBRARIES}
    ${wxWidgets_LIBRARIES}
    ${GDI_PLUS_LIBRARIES}
    Boost::headers
    ${PCBNEW_EXTRA_LIBS}    # -lrt must follow Boost
)

kicad_add_utils_executable( qa_pcbnew_tools )
