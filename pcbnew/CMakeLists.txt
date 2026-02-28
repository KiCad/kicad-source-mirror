# Add all the warnings to the files
if( COMPILER_SUPPORTS_WARNINGS )
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WARN_FLAGS_CXX}")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${WARN_FLAGS_C}")
endif()

add_compile_definitions( PCBNEW )

add_subdirectory(connectivity)

add_subdirectory(router)

#psnrouter depends on make_lexer outputs in common( bug #1285878 )
add_dependencies( pnsrouter pcbcommon )


file( MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${KICMAKE_CFG_INTDIR}/swig )

#Infrequently needed headers go at end of search paths, append to INC_AFTER
set( INC_AFTER ${INC_AFTER} python/scripting )
set( INC_AFTER ${INC_AFTER} ${CMAKE_SOURCE_DIR}/common/swig )

include_directories( BEFORE ${INC_BEFORE} )
include_directories(
    ./dialogs
    ./autorouter
    ./generators
    ${CMAKE_SOURCE_DIR}/3d-viewer
    ${CMAKE_SOURCE_DIR}/3d-viewer/dialogs
    ${CMAKE_SOURCE_DIR}/common
    ${CMAKE_SOURCE_DIR}/common/dialogs
    ./exporters
    ./importers
    ${CMAKE_SOURCE_DIR}/utils/idftools
    ./specctra_import_export
    ${INC_AFTER}
    )

set( PCBNEW_DIALOGS
    dialogs/dialog_filter_selection.cpp
    dialogs/dialog_filter_selection_base.cpp
    dialogs/dialog_barcode_properties.cpp
    dialogs/dialog_barcode_properties_base.cpp
    dialogs/dialog_board_setup.cpp
    dialogs/dialog_board_statistics.cpp
    dialogs/dialog_board_statistics_base.cpp
    dialogs/dialog_board_reannotate.cpp
    dialogs/dialog_board_reannotate_base.cpp
    dialogs/dialog_cleanup_graphics.cpp
    dialogs/dialog_cleanup_graphics_base.cpp
    dialogs/dialog_cleanup_tracks_and_vias.cpp
    dialogs/dialog_cleanup_tracks_and_vias_base.cpp
    dialogs/dialog_copper_zones.cpp
    dialogs/dialog_copper_zones_base.cpp
    dialogs/dialog_create_array.cpp
    dialogs/dialog_create_array_base.cpp
    dialogs/dialog_dimension_properties.cpp
    dialogs/dialog_dimension_properties_base.cpp
    dialogs/dialog_drc.cpp
    dialogs/dialog_drc_base.cpp
    dialogs/dialog_drc_job_config.cpp
    dialogs/dialog_board_stats_job.cpp
    dialogs/dialog_board_stats_job_base.cpp
    dialogs/dialog_footprint_associations.cpp
    dialogs/dialog_footprint_associations_base.cpp
    dialogs/dialog_footprint_checker.cpp
    dialogs/dialog_footprint_checker_base.cpp
    dialogs/dialog_footprint_properties.cpp
    dialogs/dialog_footprint_properties_base.cpp
    dialogs/dialog_footprint_properties_fp_editor.cpp
    dialogs/dialog_footprint_properties_fp_editor_base.cpp
    dialogs/dialog_enum_pads.cpp
    dialogs/dialog_enum_pads_base.cpp
    dialogs/dialog_exchange_footprints.cpp
    dialogs/dialog_exchange_footprints_base.cpp
    dialogs/dialog_export_2581.cpp
    dialogs/dialog_export_2581_base.cpp
    dialogs/dialog_export_idf.cpp
    dialogs/dialog_export_idf_base.cpp
    dialogs/dialog_export_step.cpp
    dialogs/dialog_export_odbpp.cpp
    dialogs/dialog_export_odbpp_base.cpp
    dialogs/dialog_export_step_base.cpp
    dialogs/dialog_export_step_process.cpp
    dialogs/dialog_export_step_process_base.cpp
    dialogs/dialog_export_vrml.cpp
    dialogs/dialog_export_vrml_base.cpp
    dialogs/dialog_find.cpp
    dialogs/dialog_find_base.cpp
    dialogs/dialog_footprint_wizard_list.cpp
    dialogs/dialog_footprint_wizard_list_base.cpp
    dialogs/dialog_gen_footprint_position.cpp
    dialogs/dialog_gen_footprint_position_file_base.cpp
    dialogs/dialog_gencad_export_options.cpp
    dialogs/dialog_gendrill.cpp
    dialogs/dialog_gendrill_base.cpp
    dialogs/dialog_generators.cpp
    dialogs/dialog_generators_base.cpp
    dialogs/dialog_get_footprint_by_name.cpp
    dialogs/dialog_get_footprint_by_name_base.cpp
    dialogs/dialog_global_deletion.cpp
    dialogs/dialog_global_deletion_base.cpp
    dialogs/dialog_global_edit_tracks_and_vias.cpp
    dialogs/dialog_global_edit_tracks_and_vias_base.cpp
    dialogs/dialog_global_edit_teardrops.cpp
    dialogs/dialog_global_edit_teardrops_base.cpp
    dialogs/dialog_global_edit_text_and_graphics.cpp
    dialogs/dialog_global_edit_text_and_graphics_base.cpp
    dialogs/dialog_push_pad_properties.cpp
    dialogs/dialog_push_pad_properties_base.cpp
    dialogs/dialog_shape_properties.cpp
    dialogs/dialog_shape_properties_base.cpp
    dialogs/dialog_reference_image_properties.cpp
    dialogs/dialog_reference_image_properties_base.cpp
    dialogs/dialog_import_settings.cpp
    dialogs/dialog_import_settings_base.cpp
    dialogs/dialog_items_list.cpp
    dialogs/dialog_imported_layers_base.cpp
    dialogs/dialog_map_layers.cpp
    dialogs/dialog_rule_area_properties.cpp
    dialogs/dialog_rule_area_properties_base.cpp
    dialogs/dialog_layer_selection_base.cpp
    dialogs/dialog_move_exact.cpp
    dialogs/dialog_move_exact_base.cpp
    dialogs/dialog_import_netlist.cpp
    dialogs/dialog_import_netlist_base.cpp
    dialogs/dialog_non_copper_zones_properties.cpp
    dialogs/dialog_non_copper_zones_properties_base.cpp
    dialogs/dialog_offset_item.cpp
    dialogs/dialog_offset_item_base.cpp
    dialogs/dialog_outset_items.cpp
    dialogs/dialog_outset_items_base.cpp
    dialogs/dialog_pad_properties.cpp
    dialogs/dialog_pad_properties_base.cpp
    dialogs/dialog_fp_edit_pad_table.cpp
    dialogs/dialog_plot.cpp
    dialogs/dialog_plot_base.cpp
    dialogs/dialog_pns_diff_pair_dimensions.cpp
    dialogs/dialog_pns_diff_pair_dimensions_base.cpp
    dialogs/dialog_pns_settings.cpp
    dialogs/dialog_pns_settings_base.cpp
    dialogs/dialog_position_relative.cpp
    dialogs/dialog_position_relative_base.cpp
    dialogs/dialog_print_pcbnew.cpp
    dialogs/dialog_render_job_base.cpp
    dialogs/dialog_render_job.cpp
    dialogs/dialog_swap_layers.cpp
    dialogs/dialog_swap_layers_base.cpp
    dialogs/dialog_table_properties.cpp
    dialogs/dialog_table_properties_base.cpp
    dialogs/dialog_tablecell_properties.cpp
    dialogs/dialog_tablecell_properties_base.cpp
    dialogs/dialog_target_properties.cpp
    dialogs/dialog_target_properties_base.cpp
    dialogs/dialog_text_properties.cpp
    dialogs/dialog_text_properties_base.cpp
    dialogs/dialog_textbox_properties.cpp
    dialogs/dialog_textbox_properties_base.cpp
    dialogs/dialog_track_via_properties.cpp
    dialogs/dialog_track_via_properties_base.cpp
    dialogs/dialog_track_via_size.cpp
    dialogs/dialog_track_via_size_base.cpp
    dialogs/dialog_tuning_pattern_properties.cpp
    dialogs/dialog_tuning_pattern_properties_base.cpp
    dialogs/dialog_unused_pad_layers.cpp
    dialogs/dialog_unused_pad_layers_base.cpp
    dialogs/dialog_update_pcb.cpp
    dialogs/dialog_update_pcb_base.cpp
    dialogs/dialog_multichannel_generate_rule_areas_base.cpp
    dialogs/dialog_multichannel_generate_rule_areas.cpp
    dialogs/dialog_multichannel_repeat_layout_base.cpp
    dialogs/dialog_multichannel_repeat_layout.cpp
    dialogs/panel_display_options.cpp
    dialogs/panel_display_options_base.cpp
    dialogs/panel_edit_options.cpp
    dialogs/panel_edit_options_base.cpp
    dialogs/panel_fp_lib_table.cpp
    dialogs/panel_fp_lib_table_base.cpp
    dialogs/panel_fp_editor_color_settings.cpp
    dialogs/panel_fp_editor_field_defaults.cpp
    dialogs/panel_fp_editor_field_defaults_base.cpp
    dialogs/panel_fp_editor_graphics_defaults.cpp
    dialogs/panel_fp_editor_graphics_defaults_base.cpp
    dialogs/panel_fp_user_layer_names.cpp
    dialogs/panel_fp_user_layer_names_base.cpp
    dialogs/panel_fp_properties_3d_model.cpp
    dialogs/panel_fp_properties_3d_model_base.cpp
    dialogs/panel_pcbnew_action_plugins.cpp
    dialogs/panel_pcbnew_action_plugins_base.cpp
    dialogs/panel_pcbnew_color_settings.cpp
    dialogs/panel_pcbnew_display_origin.cpp
    dialogs/panel_pcbnew_display_origin_base.cpp
    dialogs/panel_rule_area_properties_keepout_base.cpp
    dialogs/panel_rule_area_properties_placement_base.cpp
    dialogs/panel_assign_component_classes.cpp
    dialogs/panel_assign_component_classes_base.cpp
    dialogs/panel_setup_constraints.cpp
    dialogs/panel_setup_constraints_base.cpp
    dialogs/panel_setup_defaults.cpp
    dialogs/panel_setup_defaults_base.cpp
    dialogs/panel_setup_dimensions.cpp
    dialogs/panel_setup_dimensions_base.cpp
    dialogs/panel_setup_formatting.cpp
    dialogs/panel_setup_formatting_base.cpp
    dialogs/panel_setup_mask_and_paste.cpp
    dialogs/panel_setup_mask_and_paste_base.cpp
    dialogs/panel_setup_layers.cpp
    dialogs/panel_setup_layers_base.cpp
    dialogs/panel_setup_rules.cpp
    dialogs/panel_setup_rules_base.cpp
    dialogs/panel_setup_teardrops.cpp
    dialogs/panel_setup_teardrops_base.cpp
    dialogs/panel_setup_text_and_graphics.cpp
    dialogs/panel_setup_text_and_graphics_base.cpp
    dialogs/panel_setup_tracks_and_vias.cpp
    dialogs/panel_setup_tracks_and_vias_base.cpp
    dialogs/panel_setup_tuning_patterns.cpp
    dialogs/panel_setup_tuning_patterns_base.cpp
    dialogs/panel_setup_tuning_profiles_base.cpp
    dialogs/panel_setup_tuning_profiles.cpp
    dialogs/panel_setup_tuning_profile_info_base.cpp
    dialogs/panel_setup_tuning_profile_info.cpp
    dialogs/panel_setup_zones.cpp
    dialogs/panel_setup_zones_base.cpp
    dialogs/panel_setup_zone_hatch_offsets.cpp
    dialogs/panel_setup_zone_hatch_offsets_base.cpp
    dialogs/panel_zone_properties.cpp
    dialogs/panel_zone_properties_base.cpp
    footprint_wizard.cpp
    footprint_wizard_frame.cpp
    footprint_wizard_frame_functions.cpp
    )

set( PCBNEW_BRDSTACKUP_MGR
    board_stackup_manager/dielectric_material.cpp
    board_stackup_manager/stackup_predefined_prms.cpp
    board_stackup_manager/panel_board_finish.cpp
    board_stackup_manager/panel_board_finish_base.cpp
    board_stackup_manager/panel_board_stackup.cpp
    board_stackup_manager/panel_board_stackup_base.cpp
    board_stackup_manager/board_stackup_reporter.cpp
    board_stackup_manager/dialog_dielectric_list_manager_base.cpp
    board_stackup_manager/dialog_dielectric_list_manager.cpp
    )

set( PCBNEW_IMPORT_GFX
    import_gfx/dialog_import_graphics.cpp
    import_gfx/dialog_import_graphics_base.cpp
    import_gfx/graphics_importer_pcbnew.cpp
    )


set( PCBNEW_EXPORTERS
    exporters/export_hyperlynx.cpp
    exporters/export_d356.cpp
    exporters/export_footprint_associations.cpp
    exporters/export_gencad.cpp
    exporters/export_gencad_writer.cpp
    exporters/export_idf.cpp
    exporters/step/exporter_step.cpp
    exporters/step/kicad3d_info.cpp
    exporters/step/step_pcb_model.cpp
    exporters/step/KI_XCAFDoc_AssemblyGraph.cxx
    exporters/u3d/bit_stream_writer.cpp
    exporters/u3d/constants.cpp
    exporters/u3d/context_manager.cpp
    exporters/u3d/data_block.cpp
    exporters/u3d/writer.cpp
    exporters/exporter_vrml.cpp
    exporters/place_file_exporter.cpp
    exporters/gendrill_excellon_writer.cpp
    exporters/gendrill_writer_base.cpp
    exporters/gendrill_gerber_writer.cpp
    exporters/gerber_jobfile_writer.cpp
    exporters/gerber_placefile_writer.cpp
    )

set( PCBNEW_MICROWAVE_SRCS
    microwave/microwave_footprint.cpp
    microwave/microwave_inductor.cpp
    microwave/microwave_polygon.cpp
    microwave/microwave_tool.cpp
    )

set( PCBNEW_GENERATORS_SRCS
    generators/pcb_tuning_pattern.cpp
    )

set( PCBNEW_DRC_SRCS
    drc/drc_interactive_courtyard_clearance.cpp
    drc/drc_creepage_utils.cpp
    drc/drc_report.cpp
    drc/drc_test_provider.cpp
    drc/drc_test_provider_annular_width.cpp
    drc/drc_test_provider_disallow.cpp
    drc/drc_test_provider_creepage.cpp
    drc/drc_test_provider_connectivity.cpp
    drc/drc_test_provider_connection_width.cpp
    drc/drc_test_provider_copper_clearance.cpp
    drc/drc_test_provider_physical_clearance.cpp
    drc/drc_test_provider_courtyard_clearance.cpp
    drc/drc_test_provider_edge_clearance.cpp
    drc/drc_test_provider_footprint_checks.cpp
    drc/drc_test_provider_hole_to_hole.cpp
    drc/drc_test_provider_hole_size.cpp
    drc/drc_test_provider_library_parity.cpp
    drc/drc_test_provider_schematic_parity.cpp
    drc/drc_test_provider_misc.cpp
    drc/drc_test_provider_text_dims.cpp
    drc/drc_test_provider_track_angle.cpp
    drc/drc_test_provider_track_width.cpp
    drc/drc_test_provider_track_segment_length.cpp
    drc/drc_test_provider_zone_connections.cpp
    drc/drc_test_provider_via_diameter.cpp
    drc/drc_test_provider_solder_mask.cpp
    drc/drc_test_provider_silk_clearance.cpp
    drc/drc_test_provider_matched_length.cpp
    drc/drc_test_provider_diff_pair_coupling.cpp
    drc/drc_test_provider_sliver_checker.cpp
    drc/drc_test_provider_text_mirroring.cpp

    drc/rule_editor/drc_rule_editor_utils.cpp
    drc/rule_editor/drc_re_panel_matcher.cpp
    drc/rule_editor/drc_re_rule_loader.cpp
    drc/rule_editor/drc_re_rule_saver.cpp
    drc/rule_editor/dialog_drc_rule_editor.cpp
    drc/rule_editor/panel_drc_rule_editor_base.cpp
    drc/rule_editor/panel_drc_rule_editor.cpp
    drc/rule_editor/drc_re_bitmap_overlay_panel.cpp
    drc/rule_editor/drc_re_object_selector_panel.cpp
    drc/rule_editor/drc_re_overlay_field.cpp
    drc/rule_editor/drc_re_routing_width_overlay_panel.cpp
    drc/rule_editor/drc_re_rtg_diff_pair_overlay_panel.cpp
    drc/rule_editor/drc_re_min_txt_ht_th_overlay_panel.cpp
    drc/rule_editor/drc_re_abs_length_two_overlay_panel.cpp
    drc/rule_editor/drc_re_matched_length_diff_pair_overlay_panel.cpp
    drc/rule_editor/drc_re_via_style_overlay_panel.cpp
    drc/rule_editor/drc_re_numeric_input_overlay_panel.cpp
    drc/rule_editor/drc_re_bool_input_overlay_panel.cpp
    drc/rule_editor/drc_re_vias_under_smd_overlay_panel.cpp
    drc/rule_editor/drc_re_permitted_layers_overlay_panel.cpp
    drc/rule_editor/drc_re_allowed_orientation_overlay_panel.cpp
    drc/rule_editor/drc_re_condition_row_panel.cpp
    drc/rule_editor/drc_re_condition_group_panel.cpp
    drc/rule_editor/drc_re_custom_rule_panel.cpp
    drc/rule_editor/panel_drc_group_header_base.cpp
    drc/rule_editor/panel_drc_group_header.cpp
    drc/rule_editor/drc_re_validator_checkbox_list.cpp
    drc/rule_editor/drc_re_validator_combo_ctrl.cpp
    drc/rule_editor/drc_re_validator_min_max_ctrl.cpp
    drc/rule_editor/drc_re_validator_min_preferred_max_ctrl.cpp
    drc/rule_editor/drc_re_validator_numeric_ctrl.cpp
    )

set( PCBNEW_NETLIST_SRCS
    netlist_reader/board_netlist_updater.cpp
    netlist_reader/netlist.cpp
    )

set( ZONE_MANAGER_SRCS
    zone_manager/board_edges_bounding_item.cpp
    zone_manager/dialog_zone_manager.cpp
    zone_manager/dialog_zone_manager_base.cpp
    zone_manager/model_zones_overview.cpp
    zone_manager/zone_preview_notebook.cpp
    zone_manager/zone_preview_canvas.cpp
    )

set( PCBNEW_CLASS_SRCS
    ${PCBNEW_DIALOGS}
    ${PCBNEW_EXPORTERS}
    ${PCBNEW_GENERATORS_SRCS}
    ${PCBNEW_DRC_SRCS}
    ${PCBNEW_IMPORT_GFX}
    ${PCBNEW_NETLIST_SRCS}
    ${ZONE_MANAGER_SRCS}
    ${PCBNEW_BRDSTACKUP_MGR}

    autorouter/spread_footprints.cpp
    autorouter/ar_autoplacer.cpp
    autorouter/ar_matrix.cpp
    autorouter/autoplace_tool.cpp

    action_plugin.cpp
    array_pad_number_provider.cpp
    board_statistics.cpp
    board_statistics_report.cpp
    board_tables/board_characteristics_table.cpp
    board_tables/board_stackup_table.cpp
    build_BOM_from_board.cpp
    cleanup_item.cpp
    convert_shape_list_to_polygon.cpp
    cross-probing.cpp
    edit_track_width.cpp
    files.cpp
    footprint_info_impl.cpp
    footprint_wizard.cpp
    footprint_editor_utils.cpp
    footprint_editor_settings.cpp
    fp_tree_synchronizing_adapter.cpp
    footprint_chooser_frame.cpp
    footprint_edit_frame.cpp
    footprint_libraries_utils.cpp
    footprint_viewer_frame.cpp
    fp_tree_model_adapter.cpp
    generate_footprint_info.cpp
    graphics_cleaner.cpp
    grid_layer_box_helpers.cpp
    grid_layer_box_helpers.h
    initpcb.cpp
    layer_pairs.cpp
    load_select_footprint.cpp
    menubar_footprint_editor.cpp
    menubar_pcb_editor.cpp
    padstack.cpp
    pcb_base_edit_frame.cpp
    pcb_design_block_utils.cpp
    pcb_layer_box_selector.cpp
    pcb_edit_frame.cpp
    pcb_plotter.cpp
    pcbnew_config.cpp
    pcbnew_jobs_handler.cpp
    pcbnew_printout.cpp
    pcbnew_settings.cpp
    pcbplot.cpp
    plot_board_layers.cpp
    plot_brditems_plotter.cpp
    specctra_import_export/specctra.cpp
    specctra_import_export/specctra_export.cpp
    specctra_import_export/specctra_import.cpp
    pcb_fields_grid_table.cpp
    toolbars_footprint_editor.cpp
    toolbars_footprint_viewer.cpp
    toolbars_footprint_wizard.cpp
    toolbars_pcb_editor.cpp
    tracks_cleaner.cpp
    undo_redo.cpp
    zone_filler.cpp
    zone_settings_bag.cpp
    edit_zone_helpers.cpp

    ratsnest/ratsnest.cpp

    tools/array_tool.cpp
    tools/board_editor_control.cpp
    tools/board_inspection_tool.cpp
    tools/board_reannotate_tool.cpp
    tools/convert_tool.cpp
    tools/drawing_tool.cpp
    tools/drc_tool.cpp
    tools/edit_tool.cpp
    tools/edit_tool_move_fct.cpp
    tools/pcb_edit_table_tool.cpp
    tools/global_edit_tool.cpp
    tools/footprint_editor_control.cpp
    tools/footprint_chooser_selection_tool.cpp
    tools/footprint_wizard_tools.cpp
    tools/item_modification_routine.cpp
    tools/pad_tool.cpp
    tools/pcb_control.cpp
    tools/pcb_design_block_control.cpp
    tools/pcb_group_tool.cpp
    tools/pcb_picker_tool.cpp
    tools/pcb_selection.cpp
    tools/pcb_selection_conditions.cpp
    tools/pcb_selection_tool.cpp
    tools/pcb_tool_base.cpp
    tools/align_distribute_tool.cpp
    tools/pcb_point_editor.cpp
    tools/pcb_tool_utils.cpp
    tools/position_relative_tool.cpp
    tools/tool_event_utils.cpp
    tools/zone_create_helper.cpp
    tools/zone_filler_tool.cpp
    tools/generator_tool.cpp
    tools/generator_tool_pns_proxy.cpp
    tools/multichannel_tool.cpp
    tools/drc_rule_editor_tool.cpp

    footprint_preview_panel.cpp
    footprint_tree_pane.cpp

    widgets/appearance_controls.cpp
    widgets/appearance_controls_base.cpp
    widgets/panel_footprint_chooser.cpp
    widgets/panel_selection_filter.cpp
    widgets/panel_selection_filter_base.cpp
    widgets/pcb_design_block_pane.cpp
    widgets/pcb_design_block_preview_widget.cpp
    widgets/pcb_properties_panel.cpp
    widgets/pcb_search_pane.cpp
    widgets/search_handlers.cpp
    widgets/net_inspector_panel.cpp
    widgets/pcb_net_inspector_panel.cpp
    widgets/vertex_editor_pane.cpp

    component_classes/component_class.cpp
    component_classes/component_class_cache_proxy.cpp
    component_classes/component_class_assignment_rule.cpp
    component_classes/component_class_manager.cpp

    )

set( PCBNEW_GIT_SRCS
    git/kigit_pcb_merge.cpp
    )

set( PCBNEW_SRCS
    ${PCBNEW_MICROWAVE_SRCS}
    ${PCBNEW_CLASS_SRCS}
    ${PCBNEW_DIALOGS}
    ${PCBNEW_GIT_SRCS}
    )

#extra sources from common
set( PCBNEW_COMMON_SRCS
    )

set( PCBNEW_SCRIPTING_PYTHON_HELPERS
    ${CMAKE_SOURCE_DIR}/common/swig/wx_python_helpers.cpp
    python/scripting/pcbnew_action_plugins.cpp
    python/scripting/pcbnew_footprint_wizards.cpp
    python/scripting/pcbnew_scripting_helpers.cpp
    python/scripting/pcbnew_scripting.cpp
    python/scripting/pcbnew_utils_3d.cpp
    python/scripting/pcb_scripting_tool.cpp
    )

if( KICAD_IPC_API )
    set( PCBNEW_SRCS ${PCBNEW_SRCS}
        api/api_handler_pcb.cpp
        )
endif()

#Disable all warnings for the SWIG file
if( CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang" )
    if( MSYS )
#For some reason the.cxx file generated by SWIG cannot be linked if compiled with debug options:
#it creates a* lot* of not found symbols.So compile it with release mode
        set_source_files_properties( pcbnew_wrap.cxx PROPERTIES COMPILE_FLAGS "-O2 -w" )
    else()
        set_source_files_properties( pcbnew_wrap.cxx PROPERTIES COMPILE_FLAGS "-w" )
    endif()
endif()

set( PCBNEW_SCRIPTING_SRCS
    pcbnew_wrap.cxx
    ${PCBNEW_SCRIPTING_PYTHON_HELPERS}
    )

set( SWIG_FLAGS
    -I${CMAKE_CURRENT_SOURCE_DIR}
    -I${CMAKE_SOURCE_DIR}/include
    -I${CMAKE_SOURCE_DIR}/scripting
    -I${CMAKE_SOURCE_DIR}/pcbnew/python/scripting
    -I${CMAKE_SOURCE_DIR}/common/swig
    -I${CMAKE_SOURCE_DIR}/libs/core/include
    -I${CMAKE_SOURCE_DIR}/libs/kimath/include
    -I${CMAKE_BINARY_DIR}
    )

if( DEBUG )
    set( SWIG_FLAGS ${SWIG_FLAGS} -DDEBUG )
endif()

#collect CFLAGS, and pass them to swig later
get_directory_property( DirDefs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} COMPILE_DEFINITIONS )
foreach( d ${DirDefs} )
    set( SWIG_FLAGS ${SWIG_FLAGS} -D${d} )
endforeach()


#We deliberately do not use the CMake support for swig here,
#i.e.swig_add_footprint() ) because we want full       control.

#Avoid threading in SWIG( breaks threads in pcbnew )
set( SWIG_OPTS -python -c++ -nothreads -fastdispatch -fvirtual )

set( SWIG_OPTS ${SWIG_OPTS} -outdir ${CMAKE_CURRENT_BINARY_DIR}/${KICMAKE_CFG_INTDIR}/ ${SWIG_FLAGS} )

#Avoid "nothing known about base class" warning output
set( SWIG_OPTS ${SWIG_OPTS} -w401 -w402 )

if( EXISTS ${CMAKE_CURRENT_BINARY_DIR}/${KICMAKE_CFG_INTDIR}/doxygen-xml )
    set( SWIG_OPTS ${SWIG_OPTS} -DENABLE_DOCSTRINGS_FROM_DOXYGEN )
    set( SWIG_OPTS ${SWIG_OPTS} -I${CMAKE_CURRENT_BINARY_DIR}/${KICMAKE_CFG_INTDIR}/docstrings )
endif()

add_custom_command( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${KICMAKE_CFG_INTDIR}/pcbnew_wrap.cxx
                    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${KICMAKE_CFG_INTDIR}/pcbnew.py

    DEPENDS python/swig/pcbnew.i

    DEPFILE ${CMAKE_CURRENT_BINARY_DIR}/${KICMAKE_CFG_INTDIR}/pcbnew_wrap.d

    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/${KICMAKE_CFG_INTDIR}/docstrings

    # Make docstrings.i available if it doesn't exist
    COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_CURRENT_BINARY_DIR}/${KICMAKE_CFG_INTDIR}/docstrings/docstrings.i

    COMMAND ${SWIG_EXECUTABLE}
        ${SWIG_OPTS} -MMD -MF ${CMAKE_CURRENT_BINARY_DIR}/${KICMAKE_CFG_INTDIR}/pcbnew_wrap.d -MT pcbnew/pcbnew_wrap.cxx -o ${CMAKE_CURRENT_BINARY_DIR}/${KICMAKE_CFG_INTDIR}/pcbnew_wrap.cxx ${CMAKE_SOURCE_DIR}/pcbnew/python/swig/pcbnew.i

#was needed only with SWIG version < 4 to disable a python section "def swig_import_helper()"
#found in pcbnew.py but not existing in SWIG version >= 4
#So it is left                               here for documentation purpose
#COMMAND ${ PYTHON_EXECUTABLE }
#${ CMAKE_SOURCE_DIR } / tools / build / fix_swig_imports.py ${ CMAKE_CURRENT_BINARY_DIR } / pcbnew.py

    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )

if( UNIX AND NOT APPLE )
    list( APPEND PCBNEW_EXTRA_LIBS rt )
endif()


###
#Doxygen python documentation
###

if( DOXYGEN_FOUND )
#create XML files from doxygen parsing
    add_custom_target( doxygen-python-xml
        ${CMAKE_COMMAND} -E remove_directory doxygen-python-xml
        COMMAND SOURCES_DIR=${CMAKE_SOURCE_DIR} ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile_xml
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${KICMAKE_CFG_INTDIR}/
        DEPENDS Doxyfile_xml
        COMMENT "building doxygen docs into directory doxygen-python/html"
        )

    add_dependencies( doxygen-python-xml version_header )

#create.i files from XML doxygen parsing, docstrings.i will include all of them
    add_custom_target( xml-to-docstrings
        COMMAND ${CMAKE_COMMAND} -E remove_directory docstrings
        COMMAND ${CMAKE_COMMAND} -E make_directory docstrings
        COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/build/extract_docstrings.py pcbnew.py doxygen-xml/xml docstrings
        COMMAND ${CMAKE_COMMAND} -E remove pcbnew.py # force removal so it will be recreated later with the new docstrings
        COMMENT "building docstring files"
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${KICMAKE_CFG_INTDIR}/
        DEPENDS pcbnew.py
        )

    add_dependencies( xml-to-docstrings doxygen-python-xml )

#The sources to give to the Python Doxygen target
    set( DOXYGEN_PYTHON_SOURCES
        ${CMAKE_CURRENT_BINARY_DIR}/${KICMAKE_CFG_INTDIR}/pcbnew.py
        ${CMAKE_CURRENT_SOURCE_DIR}/${KICMAKE_CFG_INTDIR}/python/plugins/FootprintWizardBase.py
        ${CMAKE_CURRENT_SOURCE_DIR}/${KICMAKE_CFG_INTDIR}/python/plugins/PadArray.py )

#The Doxyfile expects a space - separated list in the env var
    string(REPLACE ";" " " DOXYGEN_PYTHON_SOURCES_STR "${DOXYGEN_PYTHON_SOURCES}")

#Create doxygen - python html
    add_custom_target( doxygen-python
        ${CMAKE_COMMAND} -E remove_directory doxygen-python
        COMMAND ${CMAKE_COMMAND} -E env
            PYTHON_SOURCES_TO_DOC=${DOXYGEN_PYTHON_SOURCES_STR}
            CMAKE_SOURCE_DIR=${CMAKE_SOURCE_DIR}
            ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile_python
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${KICMAKE_CFG_INTDIR}/
        DEPENDS Doxyfile_python
        DEPENDS ${DOXYGEN_PYTHON_SOURCES}
        COMMENT "building doxygen docs into directory doxygen-python/html"
        )

    add_dependencies( doxygen-python version_header xml-to-docstrings )
endif()

if( WIN32 )
    if( MINGW )
#PCBNEW_RESOURCES variable is set by the macro.
        mingw_resource_compiler( pcbnew )
    else()
        set( PCBNEW_RESOURCES ${CMAKE_SOURCE_DIR}/resources/msw/pcbnew.rc )
    endif()
endif()


#Function to process a markdown(.md ) file into a header(.h ) file
function(callMrkdn2C inFile outFile)
    add_custom_command(
        OUTPUT "${outFile}"
        COMMAND ${CMAKE_COMMAND} -DinputFile="${inFile}" -DoutputFile="${outFile}" -P "${KICAD_CMAKE_MODULE_PATH}/BuildSteps/markdown2C.cmake"
        DEPENDS "${inFile}" "${KICAD_CMAKE_MODULE_PATH}/BuildSteps/markdown2C.cmake"
        COMMENT "Generating header: ${outFile}"
    )
endfunction()

#Locate all.md files in the specified directory
file(GLOB markdownFiles ${CMAKE_CURRENT_SOURCE_DIR}/dialogs/panel_setup_rules_help_*.md ${CMAKE_CURRENT_SOURCE_DIR}/dialogs/panel_setup_condition_help_*.md )

#Create a custom target to generate all.h files
add_custom_target(generate_headers ALL)

#Loop over all.md files and convert them into.h files in the same directory as the.md files

foreach(file ${markdownFiles})
#Extract the filename without extension and define output path in the same folder as the.md file
    get_filename_component(filename "${file}" NAME_WE)
    set(outputHeader "${CMAKE_CURRENT_SOURCE_DIR}/dialogs/${filename}.h")

    message(VERBOSE "Processing markdown file: ${file}")
    message(VERBOSE "Output header will be: ${outputHeader}")

#Call the custom function to process the file
    callMrkdn2C("${file}" "${outputHeader}")

#Create a per - file custom target and add it as a dependency
    add_custom_target(${filename}_gen DEPENDS "${outputHeader}")
    add_dependencies(generate_headers ${filename}_gen)

#Mark the output header as a dependency of the source file
    get_source_file_property(existing_depends dialogs/panel_setup_rules.cpp OBJECT_DEPENDS)
    message(VERBOSE "Existing OBJECT_DEPENDS for dialogs/panel_setup_rules.cpp: ${existing_depends}")
    if(NOT existing_depends OR existing_depends STREQUAL "NOTFOUND")
        set(existing_depends "")
    else()
        set(existing_depends "${existing_depends};")
    endif()
    set_source_files_properties(dialogs/panel_setup_rules.cpp
        PROPERTIES OBJECT_DEPENDS "${existing_depends}${outputHeader}")
#Print out the new OBJECT_DEPENDS property for verification
    get_source_file_property(new_depends dialogs/panel_setup_rules.cpp OBJECT_DEPENDS)
    message(VERBOSE "New OBJECT_DEPENDS for dialogs/panel_setup_rules.cpp: ${new_depends}")
endforeach()

# Generate pcb_text_help_md.h from pcb_text_help.md for text variable syntax help
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/pcb_text_help_md.h
    COMMAND ${CMAKE_COMMAND}
        -DinputFile=${CMAKE_CURRENT_SOURCE_DIR}/pcb_text_help.md
        -DoutputFile=${CMAKE_CURRENT_SOURCE_DIR}/pcb_text_help_md.h
        -P ${KICAD_CMAKE_MODULE_PATH}/BuildSteps/markdown2C.cmake
    DEPENDS ${KICAD_CMAKE_MODULE_PATH}/BuildSteps/markdown2C.cmake ${CMAKE_CURRENT_SOURCE_DIR}/pcb_text_help.md
    COMMENT "creating pcb_text_help_md.h from pcb_text_help.md"
)

add_custom_target(pcb_text_help_md_gen DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/pcb_text_help_md.h)

# pcbcommon compiles pcb_text.cpp which includes pcb_text_help_md.h
add_dependencies( pcbcommon pcb_text_help_md_gen )

# Add pcb_text_help_md.h as a dependency for source files that will include it
set_source_files_properties(pcb_text.cpp
    PROPERTIES OBJECT_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/pcb_text_help_md.h)
set_source_files_properties(dialogs/dialog_text_properties.cpp
    PROPERTIES OBJECT_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/pcb_text_help_md.h)
set_source_files_properties(dialogs/dialog_textbox_properties.cpp
    PROPERTIES OBJECT_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/pcb_text_help_md.h)
set_source_files_properties(dialogs/dialog_table_properties.cpp
    PROPERTIES OBJECT_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/pcb_text_help_md.h)
set_source_files_properties(dialogs/dialog_tablecell_properties.cpp
    PROPERTIES OBJECT_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/pcb_text_help_md.h)

if( APPLE )
#setup bundle
    set( PCBNEW_RESOURCES pcbnew.icns pcbnew_doc.icns fpedit.icns fpedit_doc.icns )
    set_source_files_properties(
        "${CMAKE_CURRENT_SOURCE_DIR}/pcbnew.icns"
        "${CMAKE_CURRENT_SOURCE_DIR}/pcbnew_doc.icns"
        "${CMAKE_CURRENT_SOURCE_DIR}/fpedit.icns"
        "${CMAKE_CURRENT_SOURCE_DIR}/fpedit_doc.icns"
        PROPERTIES MACOSX_PACKAGE_LOCATION Resources
        )
    set( MACOSX_BUNDLE_ICON_FILE pcbnew.icns )
    set( MACOSX_BUNDLE_GUI_IDENTIFIER org.kicad.kicad )
    set( MACOSX_BUNDLE_NAME pcbnew )
endif()


add_subdirectory( pcb_io/pcad )
add_subdirectory( pcb_io/allegro )
add_subdirectory( pcb_io/altium )
add_subdirectory( pcb_io/cadstar )
add_subdirectory( pcb_io/easyeda )
add_subdirectory( pcb_io/easyedapro )
add_subdirectory( pcb_io/fabmaster )
add_subdirectory( pcb_io/ipc2581 )
add_subdirectory( pcb_io/odbpp )
add_subdirectory( pcb_io/pads )

set( PCBNEW_IO_LIBRARIES pcad2kicadpcb allegro2pcbnew altium2pcbnew cadstar2pcbnew easyeda easyedapro fabmaster ipc2581 odbpp pads2pcbnew CACHE INTERNAL "")

#a very small program launcher for pcbnew_kiface
add_executable( pcbnew WIN32 MACOSX_BUNDLE
    ${CMAKE_SOURCE_DIR}/common/single_top.cpp
    ${PCBNEW_RESOURCES}
    )

set_source_files_properties( ${CMAKE_SOURCE_DIR}/common/single_top.cpp PROPERTIES
    COMPILE_DEFINITIONS "TOP_FRAME=FRAME_PCB_EDITOR;PGM_DATA_FILE_EXT=\"kicad_pcb\";BUILD_KIWAY_DLL"
    )

target_link_libraries( pcbnew
    kicommon
    ${wxWidgets_LIBRARIES}
    )

target_link_options( pcbnew PRIVATE
    $<$<BOOL:${KICAD_MAKE_LINK_MAPS}>:-Wl,--cref,-Map=pcbnew.map>
    )

#the main pcbnew program, in DSO form.
add_library( pcbnew_kiface_objects OBJECT
    ${PCBNEW_SRCS}
    ${PCBNEW_COMMON_SRCS}
    ${PCBNEW_SCRIPTING_SRCS}
    )

if( KICAD_USE_PCH )
    target_precompile_headers( pcbnew_kiface_objects
        PRIVATE
            <vector>
            <map>
            <memory>
            <unordered_map>
            <pcb_edit_frame.h>
            <board.h>
            <footprint.h>
            <macros.h>
            <pgm_base.h>
            <wx/wx.h> )
endif()

#auto - generate specctra_lexer.h and specctra_keywords.cpp
make_lexer(
    pcbnew_kiface_objects
    specctra_import_export/specctra.keywords
    specctra_import_export/specctra_lexer.h
    specctra_import_export/specctra_keywords.cpp
    DSN
    )

target_link_libraries( pcbnew_kiface_objects
    PRIVATE
        common
        core
        nlohmann_json
        rectpack2d
        gzip-hpp
        nanoflann
        Boost::boost
        ZLIB::ZLIB
        ${OCC_LIBRARIES}
    )

add_library( pcbnew_kiface MODULE
    pcbnew.cpp
    )

set_target_properties( pcbnew_kiface PROPERTIES
#Decorate OUTPUT_NAME with PREFIX and SUFFIX, creating something like
#_pcbnew.so, _pcbnew.dll, or _pcbnew.kiface
    OUTPUT_NAME     pcbnew
    PREFIX          ${KIFACE_PREFIX}
    SUFFIX          ${KIFACE_SUFFIX}
    )

if ( KICAD_BUILD_TESTS )
    if ( UNIX )
	add_custom_command(TARGET pcbnew_kiface POST_BUILD
	 COMMAND ln -sf _pcbnew.kiface lib_pcbnew_kiface.so )
    else()
	add_custom_command(TARGET pcbnew_kiface POST_BUILD
	 COMMAND copy _pcbnew.kiface lib_pcbnew_kiface.dll )
    endif()


endif ()

set( PCBNEW_KIFACE_LIBRARIES
    pcbnew_kiface_objects
    3d-viewer
    connectivity
    pcbcommon
    pnsrouter
    kicommon
    kiplatform
    common
    gal
    scripting
    core
    idf3
    markdown_lib
    ${PCBNEW_IO_LIBRARIES}
    ${wxWidgets_LIBRARIES}
    ${PYTHON_LIBRARIES}
    Boost::headers
    ${PCBNEW_EXTRA_LIBS}    # -lrt must follow Boost
    )

target_link_libraries( pcbnew_kiface
    PRIVATE
        ${PCBNEW_KIFACE_LIBRARIES}
    )

#The KIFACE is in pcbnew.cpp, export it:
set_source_files_properties( pcbnew.cpp PROPERTIES
    COMPILE_DEFINITIONS     "BUILD_KIWAY_DLL;COMPILING_DLL"
    )

target_link_options( pcbnew_kiface PRIVATE
    $<$<BOOL:${KICAD_MAKE_LINK_MAPS}>:-Wl,--cref,-Map=_pcbnew.kiface.map>
    )

#if building pcbnew, then also build pcbnew_kiface if out of date.
add_dependencies( pcbnew pcbnew_kiface )

if( WIN32 )
#Copy dynamic lib dependency to build dir to allow running directly
    add_custom_command( TARGET pcbnew POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:kicad_3dsg>" "$<TARGET_FILE_DIR:pcbnew>"
        )
    add_custom_command( TARGET pcbnew POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:kiapi>" "$<TARGET_FILE_DIR:pcbnew>"
        )
endif()

#these 2 binaries are a matched set, keep them together:
if( APPLE )
    set_target_properties( pcbnew PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST ${PROJECT_BINARY_DIR}/pcbnew/Info.plist
        )

#puts binaries into the*.app bundle while linking
    set_target_properties( pcbnew_kiface PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${OSX_BUNDLE_BUILD_KIFACE_DIR}
        )

    set_target_properties( pcbnew PROPERTIES INSTALL_RPATH
            "@executable_path/../Frameworks;@executable_path/../Frameworks/Python.framework" )

    set_target_properties( pcbnew_kiface PROPERTIES INSTALL_RPATH
            "@executable_path/../Frameworks;@executable_path/../Frameworks/Python.framework" )
    set_target_properties( pcbnew_kiface PROPERTIES BUILD_WITH_INSTALL_RPATH 1 )

#put individual bundle outside of main bundle as a first step
#will be pulled into the main bundle when creating main bundle
    install( TARGETS pcbnew
        DESTINATION ${KICAD_BIN}
        COMPONENT binary
        )

    install( CODE "
        set( KICAD_CMAKE_MODULE_PATH \"${KICAD_CMAKE_MODULE_PATH}\" )
        set( KICAD_BIN \"${KICAD_BIN}\" )
        set( OSX_BUNDLE_INSTALL_BIN_DIR \"${OSX_BUNDLE_INSTALL_BIN_DIR}\" )
        set( OSX_BUNDLE_INSTALL_LIB_DIR \"${OSX_BUNDLE_INSTALL_LIB_DIR}\" )
        set( OSX_BUNDLE_BUILD_KIFACE_DIR \"${OSX_BUNDLE_BUILD_KIFACE_DIR}\" )
    " )

    install( CODE [[
        include( ${KICAD_CMAKE_MODULE_PATH}/InstallSteps/InstallMacOS.cmake )

#Install any dependencies
        install_runtime_deps( "${KICAD_BIN}/pcbnew.app/Contents/MacOS/pcbnew"
            "${OSX_BUNDLE_BUILD_KIFACE_DIR}/_pcbnew.kiface"
            ""
            )
    ]] )
else()
    if( MSVC )
        target_sources( pcbnew_kiface PRIVATE ${CMAKE_SOURCE_DIR}/resources/msw/pcbnew-dll.rc )
    endif()

    install( TARGETS pcbnew
        DESTINATION ${KICAD_BIN}
        COMPONENT binary
        )
    install( TARGETS pcbnew_kiface
        DESTINATION ${KICAD_KIFACE}
        COMPONENT binary
        )
endif()

if( KICAD_WIN32_INSTALL_PDBS )
#Get the PDBs to copy over for MSVC
    install(FILES $<TARGET_PDB_FILE:pcbnew> DESTINATION ${KICAD_BIN})
    install(FILES $<TARGET_PDB_FILE:pcbnew_kiface> DESTINATION ${KICAD_KIFACE})
endif()

if( NOT APPLE )
    install( FILES ${CMAKE_BINARY_DIR}/pcbnew/pcbnew.py DESTINATION ${PYTHON_DEST} )
else()
#put into bundle at build time, it is relocated at install
    add_custom_target( ScriptingPcbnewPyCopy ALL
        COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_BINARY_DIR}/${KICMAKE_CFG_INTDIR}/pcbnew/pcbnew.py" "${PYTHON_DEST}/"
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${KICMAKE_CFG_INTDIR}/pcbnew.py
        COMMENT "Copying pcbnew.py into ${PYTHON_DEST}"
        )
    add_dependencies( ScriptingPcbnewPyCopy ScriptingPythonDirectoryCreation )
endif()

#python plugins
install( DIRECTORY ${PROJECT_SOURCE_DIR}/pcbnew/python/plugins/
    DESTINATION ${KICAD_DATA}/scripting/plugins
    FILE_PERMISSIONS OWNER_EXECUTE OWNER_READ OWNER_WRITE GROUP_EXECUTE GROUP_READ WORLD_EXECUTE WORLD_READ
)


#Here is built the                    _pcbnew.{ so, pyd } which is the native part of the pcbnew Python library
#when Python is used from the command line.

if( WIN32 )
    install( FILES ${CMAKE_BINARY_DIR}/pcbnew/_pcbnew.pyd DESTINATION ${PYTHON_DEST} )
    set( PYMOD_EXT "pyd" )
elseif( APPLE )
#put everything into bundle at build time, it is relocated at install
    add_custom_target( ScriptingModulesPcbnewSoCopy ALL
        COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_BINARY_DIR}/${KICMAKE_CFG_INTDIR}//pcbnew/_pcbnew.so" "${PYTHON_DEST}/"
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/_pcbnew.so
        COMMENT "Copying _pcbnew.so into ${PYTHON_DEST}"
        )
    add_dependencies( ScriptingModulesPcbnewSoCopy ScriptingPythonDirectoryCreation )
    set( PYMOD_EXT "so" )
else()
#Linux is the remaining platform, and all that has to be installed is the created symlink.
    set( PYMOD_EXT "so" )

    install( FILES ${CMAKE_CURRENT_BINARY_DIR}/${KICMAKE_CFG_INTDIR}/python/_pcbnew.${PYMOD_EXT} DESTINATION ${PYTHON_DEST} )
endif()

if( APPLE )
     add_custom_command( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${KICMAKE_CFG_INTDIR}/_pcbnew.${PYMOD_EXT}
        DEPENDS pcbnew_kiface
        COMMAND ${CMAKE_COMMAND} -E copy  ${OSX_BUNDLE_BUILD_KIFACE_DIR}/_pcbnew.kiface _pcbnew.${PYMOD_EXT}
        COMMENT "Creating python's pcbnew native module _pcbnew.${PYMOD_EXT} for command line use."
        )

     set( PYTHON_FILES ${CMAKE_CURRENT_BINARY_DIR}/${KICMAKE_CFG_INTDIR}/_pcbnew.${PYMOD_EXT} )
elseif( WIN32 )
    # For phase 1, copy _pcbnew.kiface to the python module.
    add_custom_command( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${KICMAKE_CFG_INTDIR}/_pcbnew.${PYMOD_EXT}
        DEPENDS pcbnew_kiface
        COMMAND ${CMAKE_COMMAND} -E copy _pcbnew${KIFACE_SUFFIX} _pcbnew.${PYMOD_EXT}
        COMMENT "Creating python's pcbnew native module _pcbnew.${PYMOD_EXT} for command line use."
        )

    set( PYTHON_FILES ${CMAKE_CURRENT_BINARY_DIR}/${KICMAKE_CFG_INTDIR}/_pcbnew.${PYMOD_EXT} )
else()
    # For linux, just create a symlink in the build directory to ensure the unit tests can find the library
    # We don't actually do anything with this symlink though, since we create the proper one for the install
    # later.
    add_custom_command( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${KICMAKE_CFG_INTDIR}/_pcbnew.${PYMOD_EXT}
        DEPENDS pcbnew_kiface
        COMMAND ${CMAKE_COMMAND} -E create_symlink _pcbnew${KIFACE_SUFFIX} _pcbnew.${PYMOD_EXT}
        COMMENT "Symlinking _pcbnew.${PYMOD_EXT} to _pcbnew${KIFACE_SUFFIX}."
        )

    if( IS_ABSOLUTE ${KICAD_KIFACE} )
        file( RELATIVE_PATH PCBNEW_PYTHON_SYMLINK ${PYTHON_FULL_DEST} ${KICAD_KIFACE}/_pcbnew${KIFACE_SUFFIX} )
    else()
        file( RELATIVE_PATH PCBNEW_PYTHON_SYMLINK ${PYTHON_FULL_DEST} ${CMAKE_INSTALL_PREFIX}/${KICAD_KIFACE}/_pcbnew${KIFACE_SUFFIX} )
    endif()

    file( MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/python )

    # This is the symlink we use in the installation directory, so it will not resolve in the build directory.
    add_custom_command( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${KICMAKE_CFG_INTDIR}/python/_pcbnew.${PYMOD_EXT}
        DEPENDS pcbnew_kiface
        COMMAND ln -sf "${PCBNEW_PYTHON_SYMLINK}" "${CMAKE_CURRENT_BINARY_DIR}/${KICMAKE_CFG_INTDIR}/python/_pcbnew.${PYMOD_EXT}"
        COMMENT "Creating install symlink from _pcbnew.${PYMOD_EXT} to _pcbnew${KIFACE_SUFFIX}."
        )

    set( PYTHON_FILES
         ${CMAKE_CURRENT_BINARY_DIR}/${KICMAKE_CFG_INTDIR}/_pcbnew.${PYMOD_EXT}
         ${CMAKE_CURRENT_BINARY_DIR}/${KICMAKE_CFG_INTDIR}/python/_pcbnew.${PYMOD_EXT}
        )
endif()

add_custom_target(
    pcbnew_python_module ALL
    DEPENDS ${PYTHON_FILES}
    )


if( APPLE )
#If we don't have wxPython, then we must create the site-packages directory
    add_custom_target( ScriptingPythonDirectoryCreation ALL
        COMMAND ${CMAKE_COMMAND} -E make_directory "${PYTHON_DEST}"
        COMMENT "Creating Python library directory ${PYTHON_DEST}"
        )
endif()
