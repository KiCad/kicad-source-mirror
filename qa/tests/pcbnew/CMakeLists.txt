# This program source code file is part of KiCad, a free EDA CAD application.
#
# Copyright (C) 2018-2022 KiCad Developers, see CHANGELOG.TXT for contributors.
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

set( QA_PCBNEW_SRCS
    # This is needed for the global mock objects
    ${CMAKE_SOURCE_DIR}/qa/mocks/kicad/common_mocks.cpp

    # The main test entry points
    test_module.cpp

    # Shared between programs, but dependent on the BIU
    ${CMAKE_SOURCE_DIR}/qa/tests/common/test_format_units.cpp
    ${CMAKE_SOURCE_DIR}/qa/tests/common/test_array_options.cpp

    # testing utility routines
    drc/drc_test_utils.cpp

    # test compilation units (start test_)
    test_array_pad_name_provider.cpp
    test_barcode_load_save.cpp
    test_board_connected_items.cpp
    test_board_item.cpp
    test_board_commit.cpp
    test_board_statistics.cpp
    test_cam_backdrill.cpp
    test_clip_line_to_pad.cpp
    test_component_classes.cpp
    test_generator_load_save.cpp
    test_graphics_load_save.cpp
    test_graphics_import_mgr.cpp
    test_group_load_save.cpp
    test_footprint_load_save.cpp
    test_fp_lib_load_save.cpp
    test_io_mgr.cpp
    test_lset.cpp
    test_meander_corner_radius.cpp
    test_pns_basics.cpp
    test_pad_flashing.cpp
    test_pad_numbering.cpp
    test_pad_tenting.cpp
    test_pcb_textbox.cpp
    test_prettifier.cpp
    test_pcb_render_settings.cpp
    test_libeval_compiler.cpp
    test_reference_image_load.cpp
    test_pdf_output_path.cpp
    test_shape_corner_radius.cpp
    test_pcb_grid_helper.cpp
    test_save_load.cpp
    test_stacked_pin_netlist.cpp
    test_teardrop_uuid.cpp
    test_tracks_cleaner.cpp
    test_triangulation.cpp
    test_multichannel.cpp
    test_variant.cpp
    test_zone.cpp
    test_zone_filler.cpp
    test_poly_simplify.cpp

    drc/test_custom_rule_severities.cpp
    drc/test_drc_backdrill_postmachining.cpp
    drc/test_drc_courtyard_invalid.cpp
    drc/test_drc_courtyard_overlap.cpp
    drc/test_drc_regressions.cpp
    drc/test_drc_solder_mask_expansion.cpp
    drc/test_drc_copper_conn.cpp
    drc/test_drc_copper_graphics.cpp
    drc/test_drc_copper_sliver.cpp
    drc/test_solder_mask_bridging.cpp
    drc/test_solder_mask_bridge_all_tracks.cpp
    drc/test_solder_mask_track_to_pad.cpp
    drc/test_drc_multi_netclasses.cpp
    drc/test_drc_skew.cpp
    drc/test_drc_component_classes.cpp
    drc/test_drc_incorrect_text_mirror.cpp
    drc/test_drc_starved_thermal.cpp
    drc/test_drc_orientation.cpp
    drc/test_drc_lengths.cpp
    drc/test_drc_tuner_agreement.cpp
    drc/test_drc_unconnected_items_exclusion_loss.cpp
    drc/test_drc_via_dangling.cpp
    drc/test_drc_tuning_profiles.cpp
    drc/test_drc_creepage_issue21482.cpp
    drc/test_drc_rule_editor.cpp
    drc/test_drc_bitmap_overlay_panel.cpp

    pcb_io/altium/test_altium_rule_transformer.cpp
    pcb_io/altium/test_altium_pcblib_import.cpp
    pcb_io/altium/test_altium_pcb_import.cpp
    pcb_io/cadstar/test_cadstar_footprints.cpp
    pcb_io/eagle/test_eagle_board_import.cpp
    pcb_io/eagle/test_eagle_lbr_import.cpp
    pcb_io/pads/test_pads_import.cpp
    pcb_io/pads/test_pads_layer_mapper.cpp
    pcb_io/pads/test_pads_arc_structures.cpp
    pcb_io/pads/test_pads_part_structures.cpp
    pcb_io/pads/test_pads_keepout.cpp
    pcb_io/geda/test_geda_pcb_import.cpp

    pcb_io/kicad_sexpr/test_kicad_sexpr.cpp

    pcb_io/easyedapro/test_easyedapro_import.cpp

    pcb_io/fabmaster/test_fabmaster_import.cpp

    pcb_io/allegro/allegro_test_utils.cpp
    pcb_io/allegro/test_allegro_boards.cpp
    pcb_io/allegro/test_allegro_import.cpp
    pcb_io/allegro/test_block_additional.cpp

    pcb_io/ipc2581/test_ipc2581_export.cpp

    group_saveload.cpp
)

if( WIN32 )
    # We want to declare a resource manifest on Windows to enable UTF8 mode
    # Without UTF8 mode, some random IO tests may fail, we set the active code page on normal kicad to UTF8 as well
    if( MINGW )
        # QA_PCBNEW_RESOURCES variable is set by the macro.
        mingw_resource_compiler( qa_pcbnew )
    else()
        set( QA_PCBNEW_RESOURCES ${CMAKE_SOURCE_DIR}/resources/msw/qa_pcbnew.rc )
    endif()
endif()

add_executable( qa_pcbnew
    ${QA_PCBNEW_SRCS}
    ${QA_PCBNEW_RESOURCES}
)

# Pcbnew tests, so pretend to be pcbnew (for units, etc)
target_compile_definitions( qa_pcbnew
    PRIVATE PCBNEW
)

# Anytime we link to the kiface_objects, we have to add a dependency on the last object
# to ensure that the generated lexer files are finished being used before the qa runs in a
# multi-threaded build
add_dependencies( qa_pcbnew pcbnew )

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

target_link_libraries( qa_pcbnew
    qa_pcbnew_utils
    pcbnew_kiface_objects
    3d-viewer
    connectivity
    pcbcommon
    pnsrouter
    gal
    common
    gal
    scripting
    qa_utils
    dxflib_qcad
    tinyspline_lib
    nanosvg
    idf3
    markdown_lib
    ${PCBNEW_IO_LIBRARIES}
    ${wxWidgets_LIBRARIES}
    ${GDI_PLUS_LIBRARIES}
    ${PYTHON_LIBRARIES}
    Boost::headers
    Boost::unit_test_framework
    ${PCBNEW_EXTRA_LIBS}    # -lrt must follow Boost
)

if( WIN32 )
    # Copy dynamic lib dependency to build dir to allow running directly
    add_custom_command( TARGET qa_pcbnew POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:kicad_3dsg>" "$<TARGET_FILE_DIR:qa_pcbnew>"
        )
endif()

kicad_add_boost_test( qa_pcbnew qa_pcbnew )

setup_qa_env( qa_pcbnew )
