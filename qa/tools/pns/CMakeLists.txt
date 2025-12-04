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

add_compile_definitions( BOOST_TEST_DYN_LINK PCBNEW USE_TOOL_MANAGER)


set( COMMON_SRCS
    ../../../pcbnew/drc/drc_creepage_utils.cpp
    ../../../pcbnew/drc/drc_rule.cpp
    ../../../pcbnew/drc/drc_rule_condition.cpp
    ../../../pcbnew/drc/drc_rule_parser.cpp
    ../../../pcbnew/drc/drc_test_provider.cpp
    ../../../pcbnew/drc/drc_test_provider_creepage.cpp
    ../../../pcbnew/drc/drc_test_provider_copper_clearance.cpp
    ../../../pcbnew/drc/drc_test_provider_hole_to_hole.cpp
    ../../../pcbnew/drc/drc_test_provider_edge_clearance.cpp
    ../../../pcbnew/drc/drc_test_provider_hole_size.cpp
    ../../../pcbnew/drc/drc_test_provider_disallow.cpp
    ../../../pcbnew/drc/drc_test_provider_track_width.cpp
    ../../../pcbnew/drc/drc_test_provider_annular_width.cpp
    ../../../pcbnew/drc/drc_test_provider_connectivity.cpp
    ../../../pcbnew/drc/drc_test_provider_courtyard_clearance.cpp
    ../../../pcbnew/drc/drc_test_provider_via_diameter.cpp
    ../../../pcbnew/drc/drc_test_provider_schematic_parity.cpp
    ../../../pcbnew/drc/drc_test_provider_misc.cpp
    ../../../pcbnew/drc/drc_test_provider_solder_mask.cpp
    ../../../pcbnew/drc/drc_test_provider_silk_clearance.cpp
    ../../../pcbnew/drc/drc_test_provider_matched_length.cpp
    ../../../pcbnew/drc/drc_test_provider_diff_pair_coupling.cpp
    ../../../pcbnew/drc/drc_engine.cpp
    ../../../pcbnew/drc/drc_item.cpp
    ../../../pcbnew/board_stackup_manager/stackup_predefined_prms.cpp
    pns_log_file.cpp
    pns_log_player.cpp
    pns_test_debug_decorator.cpp
)

add_executable( pns_debug_tool
    ${COMMON_SRCS}
    pns_log_viewer_frame.cpp
    pns_log_viewer_frame_base.cpp
    label_manager.cpp
    ../../qa_utils/pcb_test_frame.cpp
    ../../qa_utils/pcb_test_selection_tool.cpp
    ../../qa_utils/test_app_main.cpp
    ../../qa_utils/utility_program.cpp
    ../../qa_utils/mocks.cpp

    playground.cpp
    pns_debug_tool_main.cpp
  )

add_executable( qa_pns_regressions
  ${COMMON_SRCS}
  ../../qa_utils/pcb_test_frame.cpp
  ../../qa_utils/pcb_test_selection_tool.cpp
  ../../qa_utils/test_app_main.cpp
  ../../qa_utils/utility_program.cpp
  ../../qa_utils/mocks.cpp
  qa_pns_regressions_main.cpp
)


# Pcbnew tests, so pretend to be pcbnew (for units, etc)
target_compile_definitions( pns_debug_tool
    PRIVATE PCBNEW
)
target_compile_definitions( qa_pns_regressions
    PRIVATE PCBNEW TEST_APP_NO_MAIN
)
# Anytime we link to the kiface_objects, we have to add a dependency on the last object
# to ensure that the generated lexer files are finished being used before the qa runs in a
# multi-threaded build
add_dependencies( pns_debug_tool pcbnew )
add_dependencies( qa_pns_regressions pcbnew )


target_link_libraries( pns_debug_tool
    qa_pcbnew_utils
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
    pcbcommon
    markdown_lib
    3d-viewer
    ${PCBNEW_IO_LIBRARIES}
    ${wxWidgets_LIBRARIES}
    ${GDI_PLUS_LIBRARIES}
    ${PYTHON_LIBRARIES}
    Boost::headers
    ${PCBNEW_EXTRA_LIBS}    # -lrt must follow Boost
)


target_link_libraries( qa_pns_regressions
    qa_pcbnew_utils
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
    pcbcommon
    markdown_lib
    3d-viewer
    ${PCBNEW_IO_LIBRARIES}
    ${wxWidgets_LIBRARIES}
    ${GDI_PLUS_LIBRARIES}
    ${PYTHON_LIBRARIES}
    Boost::headers
    ${PCBNEW_EXTRA_LIBS}    # -lrt must follow Boost
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
    ${CMAKE_SOURCE_DIR}/qa/qa_utils/include
    ${INC_AFTER}
)

# kicad_add_boost_test( qa_pns_regressions qa_pns_regressions )
