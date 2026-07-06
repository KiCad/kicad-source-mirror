#
# This program source code file is part of KiCad, a free EDA CAD application.
#
# Copyright (C) 2017 CERN
# @author Alejandro García Montoro <alejandro.garciamontoro@gmail.com>
# Copyright (C) 2022-2023, 2024 KiCad Developers, see AUTHORS.txt for contributors.
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


include_directories( BEFORE ${INC_BEFORE} )

include_directories(
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/pcbnew
    # symbol_edit_frame.h includes <symbol_tree_pane.h>, which lives under eeschema/widgets.
    ${CMAKE_SOURCE_DIR}/eeschema/widgets
    # sch_tool_base.h (pulled in by tools/sch_editor_control.h) includes the bare
    # <symbol_edit_frame.h>, which lives under eeschema/symbol_editor.
    ${CMAKE_SOURCE_DIR}/eeschema/symbol_editor
    # dialog_sim_model.h (white-box tested for issue 23687) includes <dialog_sim_model_base.h>.
    ${CMAKE_SOURCE_DIR}/eeschema/dialogs
    ${CMAKE_SOURCE_DIR}/qa/mocks/include
    ${CMAKE_SOURCE_DIR}/qa/qa_utils
    ${CMAKE_SOURCE_DIR}/qa
    ${INC_AFTER}
    )

set( QA_EESCHEMA_SRCS
    # need the mock Pgm for many functions
    ${CMAKE_SOURCE_DIR}/qa/mocks/kicad/common_mocks.cpp

    # The main test entry points
    test_module.cpp

    # Base internal units (1=100nm) testing.
    test_sch_biu.cpp

    # Shared between programs, but dependent on the BIU
    ${CMAKE_SOURCE_DIR}/qa/tests/common/test_format_units.cpp
    ${CMAKE_SOURCE_DIR}/qa/tests/common/test_array_options.cpp

    sch_io/altium/test_altium_parser_sch.cpp
    sch_io/altium/test_altium_sch_import.cpp
    sch_io/database/test_sch_io_database_cycle.cpp
    sch_io/pads/test_pads_sch_parser.cpp
    sch_io/pads/test_pads_sch_import.cpp
    sch_io/geda/test_geda_sch_import.cpp
    sch_io/cadstar/test_issue14447_arc_connection.cpp

    erc/test_erc_four_way.cpp
	erc/test_erc_label_not_connected.cpp
	erc/test_erc_multiple_pin_to_pin.cpp
	erc/test_erc_rule_area.cpp
	erc/test_erc_stacking_pins.cpp
    erc/test_erc_label_names.cpp
	erc/test_erc_global_labels.cpp
	erc/test_erc_no_connect.cpp
    erc/test_erc_hierarchical_schematics.cpp
    erc/test_erc_label_multiple_wires.cpp
    erc/test_erc_marker_counts.cpp
    erc/test_erc_shared_pin_multiunit.cpp
    erc/test_erc_unconnected_wire_endpoints.cpp
    erc/test_erc_wire_bus_entry.cpp
    erc/test_erc_ground_pins.cpp
    erc/test_erc_bus_member_label_local_pins.cpp
    erc/test_erc_marker_deduplication.cpp
    erc/test_erc_power_pin_marker_location.cpp
    erc/test_erc_lib_symbol_mismatch.cpp
    erc/test_erc_lib_symbol_field_position.cpp
    erc/test_erc_footprint_filters.cpp

    test_annotation_refdes_tracker_units.cpp
    test_annotation_units_conflicts.cpp
    test_issue24314_power_flag_designators.cpp
    test_annotation_units_integration.cpp
    test_connectivity_algo.cpp
    test_issue18119_bus_hierarchy.cpp
    test_issue18299_bus_member_rename.cpp
    test_issue23840_bus_group_to_vector.cpp
    test_issue20173_paste_subsheet.cpp
    test_issue21401_paste_pin_numbers.cpp
    test_issue21798_bus_sheet_copy.cpp
    test_issue22286_pin_alternate.cpp
    test_issue22620_group_annotation.cpp
    test_sch_item_swap_group.cpp
    test_issue24335_group_move_ancestor.cpp
    test_issue22651_sheet_annotation.cpp
    test_issue22864_align_sheet_pins.cpp
    test_issue22938_local_power.cpp
    test_issue23058_jumper_pin_groups.cpp
    test_issue23143_junction_midpoint.cpp
    test_issue23403_shared_subsheet_screen.cpp
    test_issue22576_subsheet_standalone.cpp
    test_issue22694_global_hier_label.cpp
    test_issue24044_expression_net_names.cpp
    test_issue23719_global_label_chain.cpp
    test_issue24107_multiunit_grouping.cpp
    test_issue24072_symbol_arc_edit_mode.cpp
    test_issue24290_local_labels_across_top_sheets.cpp
    test_issue24409_cli_hierarchical_instances.cpp
    test_issue24760_plot_page_numbers.cpp
    test_issue24663_sheet_dnp_fields.cpp
    test_issue24360_root_sheetname.cpp
    test_issue24217_cli_variant_export.cpp
    test_issue16915_bus_netclass.cpp
    test_issue24220_hierarchical_bus_netclass.cpp
    test_issue24477_http_network_loss.cpp
    test_issue23023_http_fields.cpp
    test_issue24403_directive_label_i18n.cpp
    test_issue24297_http_migrate.cpp
    test_issue24606_derived_symbol_fields.cpp
    test_issue23687_sim_value_field.cpp
    test_nc_pin_connectivity.cpp
    test_crossing_label.cpp
    test_eagle_plugin.cpp
    test_junction_helpers.cpp
    test_junction_place.cpp
    test_bus_alias_whitespace.cpp
    test_bus_entry_concurrency.cpp
    test_bus_net_name_determinism.cpp
    test_bus_parsing.cpp
    test_label_bus_connectivity.cpp
    test_label_bounding_box.cpp
    test_lib_part.cpp
    test_ee_grid_helper.cpp
    test_netlist_exporter_kicad.cpp
    test_issue24330_netlist_pin_truncation.cpp
    test_ee_item.cpp
    test_incremental_netlister.cpp
    test_legacy_power_symbols.cpp
    test_history_autosave_legacy.cpp
    test_sch_commit.cpp
    test_shape_corner_radius.cpp
    test_sch_group.cpp
    test_pin_numbers.cpp
    test_sch_netclass.cpp
    test_sch_pin.cpp
    test_sch_rule_area_destruction.cpp
    test_sch_rtree.cpp
    test_sch_reference_list.cpp
    test_sch_screen.cpp
    test_sch_sheet.cpp
    test_sch_sheet_path.cpp
    test_sch_sheet_list.cpp
    test_sch_symbol.cpp
    test_sch_table.cpp
    test_sch_textbox.cpp
    test_schematic.cpp
    test_flat_hierarchy.cpp
    test_multi_top_level_sheets.cpp
    test_save_load_schematic.cpp
    test_project_name_instances.cpp
    test_legacy_load.cpp
    test_symbol_library_adapter.cpp
    test_symbol_library_manager.cpp
    test_symbol_import_manager.cpp
    test_stacked_pin_nomenclature.cpp
    test_stacked_pin_conversion.cpp
    test_pin_stacked_layout.cpp
    test_pin_text_overlap.cpp
    test_remote_symbol_download_security.cpp
    test_remote_symbol_import.cpp
    test_remote_symbol_load_from_payload.cpp
    test_backannotate_unit_swap.cpp
    test_netlist_exporter_xml_stacked.cpp
    test_netlist_exporter_xml_nested_groups.cpp
    test_resolve_drivers.cpp
    test_saveas_copy_subsheets.cpp
    test_update_items_connectivity.cpp
    test_design_block_duplicate.cpp
    test_symbol_library.cpp
    test_symbol_library_parse_error.cpp
    test_symbol_embedded_files.cpp
    test_symbol_clipboard_export.cpp
    test_schematic_clipboard_export.cpp
    test_autoplace_fields.cpp
    test_issue23020_deep_hierarchy.cpp
)

# Database library tests need a live ODBC driver, so gate them the same way the common suite does.
if( KICAD_TEST_DATABASE_LIBRARIES )
    list( APPEND QA_EESCHEMA_SRCS sch_io/database/test_sch_io_database.cpp )

    set_source_files_properties( sch_io/database/test_sch_io_database.cpp PROPERTIES
            COMPILE_DEFINITIONS
            "QA_DBLIB_SETTINGS_PATH=(\"${CMAKE_SOURCE_DIR}/qa/data/dblib/qa_dblib.kicad_dbl\")"
            )
endif()

if( WIN32 )
    # We want to declare a resource manifest on Windows to enable UTF8 mode
    # Without UTF8 mode, some random IO tests may fail, we set the active code page on normal
    # kicad to UTF8 as well.
    if( MINGW )
        # QA_EESCHEMA_RESOURCES variable is set by the macro.
        mingw_resource_compiler( qa_eeschema )
    else()
        set( QA_EESCHEMA_RESOURCES ${CMAKE_SOURCE_DIR}/resources/msw/qa_eeschema.rc )
    endif()
endif()

add_executable( qa_eeschema
    ${QA_EESCHEMA_SRCS}
    ${QA_EESCHEMA_RESOURCES}
)

target_link_libraries( qa_eeschema
PRIVATE
    eeschema_kiface_objects
    common
    kicommon
    pads_common
    scripting
    kimath
    qa_utils
    qa_schematic_utils
    markdown_lib
    ${GDI_PLUS_LIBRARIES}
    Boost::headers
    Boost::unit_test_framework
)

# Eeschema tests, so pretend to be eeschema (for units, etc)
target_compile_definitions( qa_eeschema
    PUBLIC EESCHEMA
)

kicad_add_boost_test( qa_eeschema qa_eeschema )

setup_qa_env( qa_eeschema )
