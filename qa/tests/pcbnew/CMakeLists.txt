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
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

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
    test_3d_viewer_layer_presets.cpp
    test_array_pad_name_provider.cpp
    test_barcode_load_save.cpp
    test_extruded_body_load_save.cpp
    test_board_connected_items.cpp
    test_board_design_settings.cpp
    test_board_item_index.cpp
    test_board_loader.cpp
    test_board_item.cpp
    test_board_commit.cpp
    test_text_var_reactive.cpp
    test_clipboard_group.cpp
    test_board_statistics.cpp
    test_cam_backdrill.cpp
    test_backdrill_properties.cpp
    test_clip_line_to_pad.cpp
    test_clip_line_to_via.cpp
    test_component_classes.cpp
    test_via_validation.cpp
    test_convert_tool.cpp
    test_generator_load_save.cpp
    test_glb_utils.cpp
    test_graphics_load_save.cpp
    test_graphics_import_mgr.cpp
    test_group_load_save.cpp
    test_group_mirror.cpp
    test_group_save_perf.cpp
    test_headless_pcb_context.cpp
    test_history_autosave.cpp
    test_footprint.cpp
    test_footprint_editor_tabs.cpp
    test_footprint_library_adapter.cpp
    test_footprint_load_save.cpp
    test_footprint_realignment.cpp
    test_footprint_transform_sync.cpp
    test_footprint_rescue_layer.cpp
    test_issue24735.cpp
    test_issue24901.cpp
    test_fp_lib_load_save.cpp
    test_io_mgr.cpp
    test_ellipse_roundtrip.cpp
    test_custom_pad_primitive_roundtrip.cpp
    test_active_layer_update.cpp
    test_lset.cpp
    test_meander_corner_radius.cpp
    test_odb_dimension_export.cpp
    test_odb_soldermask_shape_export.cpp
    test_pcb_control_rehatch.cpp
    test_pns_basics.cpp
    test_pad_flashing.cpp
    test_pad_numbering.cpp
    test_pcb_differ.cpp
    test_diff_canvas_context.cpp
    test_pcb_merge_applier.cpp
    test_net_classes_diff_merge.cpp
    test_custom_drc_diff_merge.cpp
    test_lib_table_diff_merge.cpp
    test_applier_helpers.cpp
    test_fp_lib_differ.cpp
    test_pad_size_setters.cpp
    test_pad_swap_positions.cpp
    test_pad_tenting.cpp
    test_plot_edge_cuts_drill_marks.cpp
    test_plot_pad_mask.cpp
    test_plot_footprint_scaled_text.cpp
    test_pcb_textbox.cpp
    test_prettifier.cpp
    test_pcb_render_settings.cpp
    test_libeval_compiler.cpp
    test_reference_image_load.cpp
    test_gencad_export.cpp
    test_idf_export.cpp
    test_d356_export.cpp
    test_step_model_color_transfer.cpp
    test_board_netlist_updater_nested_groups.cpp
    test_netlist_group_instances.cpp
    test_step_export_modified_board.cpp
    test_pdf_output_path.cpp
    test_shape_corner_radius.cpp
    test_shape_hatch_knockout.cpp
    test_connect_board_shapes.cpp
    test_pcb_grid_helper.cpp
    test_save_load.cpp
    test_stacked_pin_netlist.cpp
    test_teardrop_uuid.cpp
    test_tracks_cleaner.cpp
    test_triangulation.cpp
    test_tuning_pattern_layer.cpp
    test_multichannel.cpp
    test_pns_tuning_path_through_pad.cpp
    test_variant.cpp
    test_text_variable_resolution.cpp
    test_zone.cpp
    test_zone_filler.cpp
    test_poly_simplify.cpp

    # Net chain roundtrip tests
    net_chains/test_net_chain_terminal_pads.cpp
    net_chains/test_net_chain_terminal_pad_assignment.cpp
    net_chains/test_net_chain_terminal_pad_stale.cpp
    net_chains/test_apply_chain_assignments_dry_run.cpp
    net_chains/test_update_pcb_legacy_fpid_dnp.cpp
    # Net chain aggregation / skew / total length tests
    net_chains/test_net_chain_skew_tuning.cpp
    net_chains/test_net_chain_total_length.cpp
    net_chains/test_net_chain_total_length_sexpr.cpp
    net_chains/test_net_chain_bridging_delay.cpp
    net_chains/test_net_chain_partition.cpp
    net_chains/test_net_chain_trunk_delay.cpp
    net_chains/test_tuning_bridging_clamp.cpp

    drc/test_custom_rule_severities.cpp
    drc/test_drc_backdrill_postmachining.cpp
    drc/test_drc_courtyard_invalid.cpp
    drc/test_drc_courtyard_overlap.cpp
    drc/test_drc_error_limit.cpp
    drc/test_footprint_courtyard_index.cpp
    drc/test_drc_regressions.cpp
    drc/test_drc_issue23469.cpp
    drc/test_drc_issue24525.cpp
    drc/test_drc_arc_arc_edge_clearance.cpp
    drc/test_drc_solder_mask_expansion.cpp
    drc/test_drc_copper_conn.cpp
    drc/test_drc_copper_graphics.cpp
    drc/test_drc_copper_sliver.cpp
    drc/test_drc_copper_thieving.cpp
    drc/test_solder_mask_bridging.cpp
    drc/test_solder_mask_bridge_all_tracks.cpp
    drc/test_solder_mask_track_to_pad.cpp
    drc/test_drc_solder_mask_npth_bridge.cpp
    drc/test_drc_multi_netclasses.cpp
    drc/test_drc_skew.cpp
    drc/test_drc_component_classes.cpp
    drc/test_drc_incorrect_text_mirror.cpp
    drc/test_drc_text_var_issue24442.cpp
    drc/test_drc_starved_thermal.cpp
    drc/test_drc_orientation.cpp
    drc/test_drc_lengths.cpp
    drc/test_drc_length_with_chain.cpp
    drc/test_drc_chain_topology.cpp
    drc/test_drc_chain_length_trunk.cpp
    drc/test_drc_return_path_zone_bbox.cpp
    drc/test_drc_return_path_per_segment.cpp
    drc/test_drc_return_path_reference_net.cpp
    drc/test_drc_chain_bridging_multipad.cpp
    drc/test_drc_tuner_chain_bridging_agreement.cpp
    drc/test_drc_stub_length.cpp
    drc/test_drc_tuner_agreement.cpp
    drc/test_drc_skew_arc.cpp
    drc/test_drc_unconnected_items_exclusion_loss.cpp
    drc/test_drc_via_dangling.cpp
    drc/test_drc_keepout_disallow.cpp
    drc/test_drc_tuning_profiles.cpp
    drc/test_drc_creepage.cpp
    drc/test_drc_creepage_issue20480.cpp
    drc/test_drc_creepage_issue21482.cpp
    drc/test_drc_creepage_issue23364.cpp
    drc/test_drc_creepage_issue23389.cpp
    drc/test_drc_creepage_issue23576.cpp
    drc/test_drc_creepage_issue23658.cpp
    drc/test_drc_creepage_issue23578.cpp
    drc/test_drc_creepage_issue24286.cpp
    drc/test_drc_creepage_issue24523.cpp
    drc/test_drc_creepage_issue24543.cpp
    drc/test_drc_creepage_issue24544.cpp
    drc/test_drc_creepage_issue24524.cpp
    drc/test_drc_creepage_issue24597.cpp
    drc/test_drc_creepage_issue24875.cpp
    drc/test_drc_creepage_issue24885.cpp
    drc/test_creepage_engine_parity.cpp
    drc/test_creepage_engine_incremental.cpp
    drc/test_drc_invalid_outline_issue24078.cpp
    drc/test_drc_invalid_outline_issue24241.cpp
    drc/test_drc_silk_rule_area_issue24177.cpp
    drc/test_drc_annular_overlap_issue24340.cpp
    drc/test_drc_hole_clearance_issue24355.cpp
    drc/test_drc_hole_to_hole_npth_slot_issue24541.cpp
    drc/test_drc_npth_slot_clearance_issue24799.cpp
    drc/test_drc_issue24264.cpp
    drc/test_drc_issue23868.cpp
    drc/test_drc_issue24211.cpp
    drc/test_drc_physical_clearance.cpp
    drc/test_drc_rule_editor.cpp
    drc/test_drc_bitmap_overlay_panel.cpp
    drc/test_drc_violation_layers_issue24941.cpp

    pcb_io/altium/test_altium_rule_transformer.cpp
    pcb_io/altium/test_altium_pcblib_import.cpp
    pcb_io/altium/test_altium_pcb_import.cpp
    pcb_io/cadstar/test_cadstar_footprints.cpp
    pcb_io/eagle/test_eagle_board_import.cpp
    pcb_io/eagle/test_eagle_binary_import.cpp
    pcb_io/eagle/test_eagle_lbr_import.cpp
    pcb_io/eagle/test_eagle_managed_lib_naming.cpp
    pcb_io/pads/test_pads_import.cpp
    pcb_io/pads/test_pads_layer_mapper.cpp
    pcb_io/pads/test_pads_arc_structures.cpp
    pcb_io/pads/test_pads_part_structures.cpp
    pcb_io/pads/test_pads_keepout.cpp
    pcb_io/geda/test_geda_pcb_import.cpp
    pcb_io/sprint_layout/test_sprint_layout_import.cpp

    pcb_io/kicad_sexpr/test_kicad_sexpr.cpp

    pcb_io/easyedapro/test_easyedapro_import.cpp

    pcb_io/diptrace/test_diptrace_import.cpp
    pcb_io/diptrace/test_diptrace_cross_link.cpp
    pcb_io/diptrace/test_diptrace_benchmarks.cpp

    pcb_io/autotrax/test_autotrax_import.cpp

    pcb_io/fabmaster/test_fabmaster_import.cpp

    pcb_io/allegro/allegro_test_utils.cpp
    pcb_io/allegro/test_allegro_boards.cpp
    pcb_io/allegro/test_allegro_drill_spans.cpp
    pcb_io/allegro/test_allegro_import.cpp
    pcb_io/allegro/test_block_additional.cpp

    pcb_io/ipc2581/test_ipc2581.cpp
    pcb_io/ipc2581/test_ipc2581_export.cpp

    pcb_io/odbpp/test_odbpp_netlist.cpp

    group_saveload.cpp
)

# DipTrace PCB benchmark tests that exercise proprietary reference designs kept outside the source
# tree. The file is intentionally not committed; include it only when a developer has it.
if( EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/pcb_io/diptrace/test_diptrace_benchmarks_local.cpp" )
    list( APPEND QA_PCBNEW_SRCS pcb_io/diptrace/test_diptrace_benchmarks_local.cpp )
endif()

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
    qa_utils
    dxflib_qcad
    tinyspline_lib
    nanosvg
    idf3
    markdown_lib
    ${PCBNEW_IO_LIBRARIES}
    ${wxWidgets_LIBRARIES}
    ${GDI_PLUS_LIBRARIES}
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

# Register split test groups for parallel execution.  Each group runs the same
# qa_pcbnew binary with a different --run_test= filter.  Groups were chosen by
# profiling wall-clock time so that no single group dominates.
#
# The "other" group is a catch-all that uses exclusion filters: any new test
# suites or cases added to the binary will automatically land here.

# Keep build-system dependencies from the original single-entry registration
add_dependencies( qa_all_tests qa_pcbnew )

if( NOT KICAD_BUILD_QA_TESTS )
    set_target_properties( qa_pcbnew PROPERTIES EXCLUDE_FROM_ALL TRUE )
endif()

# Helper to register one pcbnew test group
function( add_pcbnew_test_group GROUP_NAME FILTER )
    set( TEST_NAME "qa_pcbnew_${GROUP_NAME}" )
    set( BOOST_PARAMS "" )

    if( KICAD_TEST_XML_OUTPUT )
        set( BOOST_PARAMS
            --logger=JUNIT,warning,${TEST_NAME}.boost-results.xml:HRF,message )
    endif()

    add_test( NAME ${TEST_NAME}
        COMMAND $<TARGET_FILE:qa_pcbnew> "--run_test=${FILTER}" ${BOOST_PARAMS}
    )

    setup_qa_env( ${TEST_NAME} )
endfunction()

# Zone fill and zone tests (~40s)
set( PCBNEW_ZONE_TESTS
    "BasicZoneFills\
:NotchedZones\
:RegressionZoneFillTests\
:RegressionZoneClearanceWithIterativeRefill\
:RegressionSliverZoneFillTests\
:RegressionTeardropFill\
:RegressionNetTie\
:RegressionZonePriorityIsolatedIslands\
:RegressionViaFlashingUnreachableZone\
:RegressionViaZoneNetShort\
:HatchZoneThermalConnectivity\
:RegressionShallowArcZoneFill\
:RegressionIterativeRefillRespectsKeepouts\
:RegressionTHPadInnerLayerFlashing\
:RegressionThermalReliefAnnularRing45\
:RegressionRoundRectTeardropGeometry\
:OvalPadTeardropGeometry\
:LargeCircleTeardropGeometry\
:RegressionCoincidentPadClearance\
:ZoneViaNetClearance\
:ZoneLayerSpecificRules\
:RegressionZoneFillMinWidthAfterKnockout\
:RegressionSameNetOverlappingZones\
:RegressionSameNetZoneFillScheduler\
:RegressionDiffNetOverlappingZones\
:RegressionThermalReliefsToNowhere\
:OffCenterTeardropSymmetry\
:RegressionKeepoutBoundaryMissingFill\
:HatchZoneViaConnectionRespectsSetting\
:RegressionCascadingIslandRefill\
:IterativeRefillConvergenceLimit\
:Zone" )

# Allegro importer tests (~31s)
set( PCBNEW_ALLEGRO_TESTS
    "AllegroBlocks:AllegroBoards:AllegroImport:AllegroComprehensive:AllegroDrillSpans" )

# Creepage DRC tests (~16s, heavily multi-threaded internally)
set( PCBNEW_CREEPAGE_TESTS
    "CreepageHVvsGND\
:CreepageAroundSlotIssue20480\
:CreepagePerformanceIssue21482\
:CreepageDualSlotsIssue23364\
:CreepageNPTHSlotIssue23389\
:CreepagePathStartPointIssue23576\
:CreepageTwoNPTHSlotsIssue24523\
:CreepageRotatedRectPadIssue24543\
:CreepageRoundedSlotIssue24544\
:CreepageStadiumCapsBlockOuterSide\
:CreepageStadiumCapsArcOutward" )

# IPC-2581 export tests (~15s)
set( PCBNEW_IPC2581_TESTS "Ipc2581Export" )

# Tracks cleaner tests (~14s)
set( PCBNEW_TRACKS_CLEANER_TESTS
    "FailedToCleanRegressionTests:TrackCleanerRegressionTests" )

# Multichannel tool tests (~11s)
set( PCBNEW_MULTICHANNEL_TESTS "MultichannelTool" )

# DRC regression tests (~10s)
set( PCBNEW_DRC_REGRESSION_TESTS
    "DRCFalsePositiveRegressions\
:DRCFalseNegativeRegressions\
:DRCZoneFalsePositiveRegressions" )

# Solder mask and copper sliver DRC tests (~6s)
set( PCBNEW_DRC_SOLDER_MASK_TESTS
    "DRCCopperSliver\
:DRCSolderMaskBridgingTest\
:DRCSolderMaskBridgeAllTracksTest\
:DRCSolderMaskBridgeSingleErrorTest\
:DRCTrackToPadBridgeTest\
:FootprintLevelSolderMaskExpansion\
:SolderMaskExpansionPerformance" )

add_pcbnew_test_group( zone "${PCBNEW_ZONE_TESTS}" )
add_pcbnew_test_group( allegro "${PCBNEW_ALLEGRO_TESTS}" )
add_pcbnew_test_group( creepage "${PCBNEW_CREEPAGE_TESTS}" )
add_pcbnew_test_group( ipc2581 "${PCBNEW_IPC2581_TESTS}" )
add_pcbnew_test_group( tracks_cleaner "${PCBNEW_TRACKS_CLEANER_TESTS}" )
add_pcbnew_test_group( multichannel "${PCBNEW_MULTICHANNEL_TESTS}" )
add_pcbnew_test_group( drc_regression "${PCBNEW_DRC_REGRESSION_TESTS}" )
add_pcbnew_test_group( drc_solder_mask "${PCBNEW_DRC_SOLDER_MASK_TESTS}" )

# Catch-all for everything not in a named group.  New tests automatically
# land here so they are never silently skipped.
set( PCBNEW_EXCLUDED_FROM_OTHER
    "!BasicZoneFills\
:!NotchedZones\
:!RegressionZoneFillTests\
:!RegressionZoneClearanceWithIterativeRefill\
:!RegressionSliverZoneFillTests\
:!RegressionTeardropFill\
:!RegressionNetTie\
:!RegressionZonePriorityIsolatedIslands\
:!RegressionViaFlashingUnreachableZone\
:!RegressionViaZoneNetShort\
:!HatchZoneThermalConnectivity\
:!RegressionShallowArcZoneFill\
:!RegressionIterativeRefillRespectsKeepouts\
:!RegressionTHPadInnerLayerFlashing\
:!RegressionRoundRectTeardropGeometry\
:!OvalPadTeardropGeometry\
:!LargeCircleTeardropGeometry\
:!RegressionCoincidentPadClearance\
:!ZoneViaNetClearance\
:!ZoneLayerSpecificRules\
:!RegressionZoneFillMinWidthAfterKnockout\
:!RegressionSameNetOverlappingZones\
:!RegressionSameNetZoneFillScheduler\
:!RegressionDiffNetOverlappingZones\
:!RegressionThermalReliefsToNowhere\
:!OffCenterTeardropSymmetry\
:!RegressionKeepoutBoundaryMissingFill\
:!HatchZoneViaConnectionRespectsSetting\
:!RegressionCascadingIslandRefill\
:!IterativeRefillConvergenceLimit\
:!Zone\
:!AllegroBlocks\
:!AllegroBoards\
:!AllegroImport\
:!AllegroComprehensive\
:!AllegroDrillSpans\
:!CreepageHVvsGND\
:!CreepageAroundSlotIssue20480\
:!CreepagePerformanceIssue21482\
:!CreepageDualSlotsIssue23364\
:!CreepageNPTHSlotIssue23389\
:!CreepagePathStartPointIssue23576\
:!CreepageTwoNPTHSlotsIssue24523\
:!CreepageRotatedRectPadIssue24543\
:!CreepageRoundedSlotIssue24544\
:!CreepageStadiumCapsBlockOuterSide\
:!CreepageStadiumCapsArcOutward\
:!Ipc2581Export\
:!FailedToCleanRegressionTests\
:!TrackCleanerRegressionTests\
:!MultichannelTool\
:!DRCFalsePositiveRegressions\
:!DRCFalseNegativeRegressions\
:!DRCZoneFalsePositiveRegressions\
:!DRCCopperSliver\
:!DRCSolderMaskBridgingTest\
:!DRCSolderMaskBridgeAllTracksTest\
:!DRCSolderMaskBridgeSingleErrorTest\
:!DRCTrackToPadBridgeTest\
:!FootprintLevelSolderMaskExpansion\
:!SolderMaskExpansionPerformance" )

add_pcbnew_test_group( other "${PCBNEW_EXCLUDED_FROM_OTHER}" )
