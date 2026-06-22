# This program source code file is part of KiCad, a free EDA CAD application.
#
# Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

add_executable( qa_drc_benchmark
    drc_benchmark.cpp
    trace_capture.cpp
    corpus.cpp

    # Global mock objects pulled in by pcbnew_kiface_objects
    ${CMAKE_SOURCE_DIR}/qa/mocks/kicad/common_mocks.cpp
)

# Build against the same pcbnew definitions the kiface uses (units, etc).
target_compile_definitions( qa_drc_benchmark PRIVATE PCBNEW )

# Linking the kiface objects pulls in generated lexer sources; depend on the
# pcbnew kiface so those are finished before a multi-threaded build links us.
add_dependencies( qa_drc_benchmark pcbnew )

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
    ${CMAKE_SOURCE_DIR}/qa/qa_utils
    ${CMAKE_SOURCE_DIR}/qa/mocks/include
    ${INC_AFTER}
)

# Mirror the qa_pcbnew link set so the full 29-provider DRC engine links via
# pcbnew_kiface_objects. We omit Boost::unit_test_framework since this tool owns
# its own main().
target_link_libraries( qa_drc_benchmark
    qa_pcbnew_utils
    pcbnew_kiface_objects
    3d-viewer
    connectivity
    pcbcommon
    pnsrouter
    gal
    common
    gal
    qa_utils
    dxflib_qcad
    tinyspline_lib
    nanosvg
    idf3
    markdown_lib
    nlohmann_json
    ${PCBNEW_IO_LIBRARIES}
    ${wxWidgets_LIBRARIES}
    ${GDI_PLUS_LIBRARIES}
    Boost::headers
    ${PCBNEW_EXTRA_LIBS}    # -lrt must follow Boost
)

if( NOT KICAD_BUILD_QA_TESTS )
    set_target_properties( qa_drc_benchmark PROPERTIES EXCLUDE_FROM_ALL TRUE )
endif()
