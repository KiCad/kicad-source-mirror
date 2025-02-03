# Add all the warnings to the files
if( COMPILER_SUPPORTS_WARNINGS )
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WARN_FLAGS_CXX}")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${WARN_FLAGS_C}")
endif()

if( CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang" )
    if( MSYS )
        # For some reason the big file sim/ngspice_models.cpp creates an issue when
        # compiled in debug mode on msys2
        # (Probably some out of memory issue that crashes Eeschema at start)
        # So compile it in release mode that generates much smaller binaries
        set_source_files_properties( sim/ngspice_models.cpp PROPERTIES COMPILE_FLAGS "-O2" )
    endif()
endif()

# Add ngspice to eeschema
set( INC_AFTER ${INC_AFTER} ${NGSPICE_INCLUDE_DIR} )

# Find out the exact libngspice file name
get_filename_component( NGSPICE_DLL_ABSPATH "${NGSPICE_DLL}" ABSOLUTE )
get_filename_component( NGSPICE_DLL_FILE "${NGSPICE_DLL_ABSPATH}" NAME )
get_filename_component( NGSPICE_DLL_DIR "${NGSPICE_DLL_ABSPATH}" DIRECTORY )

set_property( SOURCE sim/ngspice.cpp
    APPEND PROPERTY COMPILE_DEFINITIONS
    NGSPICE_DLL_FILE="${NGSPICE_DLL_FILE}"
    NGSPICE_DLL_DIR="${NGSPICE_DLL_DIR}"
    )

include_directories( BEFORE ${INC_BEFORE} )
include_directories(
    ${CMAKE_SOURCE_DIR}/common
    ${CMAKE_SOURCE_DIR}/common/dialogs
    ${CMAKE_SOURCE_DIR}/libs/sexpr/include
    ${INC_AFTER}
    ./dialogs
    ./libview
    ./symbol_editor
    ./tools
    ./widgets
    )

set( EESCHEMA_SCH_IO
    sch_io/sch_io_lib_cache.cpp

    sch_io/sch_io.cpp
    sch_io/sch_io_mgr.cpp

    # KiCad IO plugin
    sch_io/kicad_sexpr/sch_io_kicad_sexpr.cpp
    sch_io/kicad_sexpr/sch_io_kicad_sexpr_lib_cache.cpp
    sch_io/kicad_sexpr/sch_io_kicad_sexpr_common.cpp
    sch_io/kicad_sexpr/sch_io_kicad_sexpr_parser.cpp

    # Legacy IO plugin
    sch_io/kicad_legacy/sch_io_kicad_legacy_lib_cache.cpp
    sch_io/kicad_legacy/sch_io_kicad_legacy.cpp
    sch_io/kicad_legacy/sch_io_kicad_legacy_helpers.cpp

    # HTTP IO plugin
    sch_io/http_lib/sch_io_http_lib.cpp

    # Database IO plugin
    sch_io/database/sch_io_database.cpp

    # Eagle IO plugin
    sch_io/eagle/sch_io_eagle.cpp

    # Altium IO plugin
    sch_io/altium/altium_parser_sch.cpp
    sch_io/altium/sch_io_altium.cpp

    # Cadstar IO plugin
    sch_io/cadstar/cadstar_sch_archive_loader.cpp
    sch_io/cadstar/cadstar_sch_archive_parser.cpp
    sch_io/cadstar/sch_io_cadstar_archive.cpp

    # LTSpice IO plugin
    sch_io/ltspice/ltspice_schematic.cpp
    sch_io/ltspice/sch_io_ltspice.cpp
    sch_io/ltspice/sch_io_ltspice_parser.cpp

    # EasyEDA IO plugin
    sch_io/easyeda/sch_easyeda_parser.cpp
    sch_io/easyeda/sch_io_easyeda.cpp

    # EasyEDA Pro IO plugin
    sch_io/easyedapro/sch_easyedapro_parser.cpp
    sch_io/easyedapro/sch_io_easyedapro.cpp
    )

set( EESCHEMA_DLGS
    dialogs/dialog_annotate.cpp
    dialogs/dialog_annotate_base.cpp
    dialogs/dialog_bom.cpp
    dialogs/dialog_bom_base.cpp
    dialogs/dialog_change_symbols.cpp
    dialogs/dialog_change_symbols_base.cpp
    dialogs/dialog_database_lib_settings_base.cpp
    dialogs/dialog_database_lib_settings.cpp
    dialogs/dialog_design_block_properties_base.cpp
    dialogs/dialog_design_block_properties.cpp
    dialogs/dialog_edit_symbols_libid.cpp
    dialogs/dialog_edit_symbols_libid_base.cpp
    dialogs/dialog_eeschema_page_settings.cpp
    dialogs/dialog_erc.cpp
    dialogs/dialog_erc_base.cpp
    dialogs/dialog_export_netlist.cpp
    dialogs/dialog_export_netlist_base.cpp
    dialogs/dialog_field_properties.cpp
    dialogs/dialog_field_properties_base.cpp
    dialogs/dialog_global_edit_text_and_graphics.cpp
    dialogs/dialog_global_edit_text_and_graphics_base.cpp
    dialogs/dialog_global_sym_lib_table_config.cpp
    dialogs/dialog_ibis_parser_reporter.cpp
    dialogs/dialog_ibis_parser_reporter_base.cpp
    dialogs/dialog_image_properties.cpp
    dialogs/dialog_image_properties_base.cpp
    dialogs/dialog_increment_annotations_base.cpp
    dialogs/dialog_junction_props.cpp
    dialogs/dialog_junction_props_base.cpp
    dialogs/dialog_label_properties.cpp
    dialogs/dialog_label_properties_base.cpp
    dialogs/dialog_lib_edit_pin_table.cpp
    dialogs/dialog_lib_edit_pin_table_base.cpp
    dialogs/dialog_lib_new_symbol.cpp
    dialogs/dialog_lib_new_symbol_base.cpp
    dialogs/dialog_lib_symbol_properties.cpp
    dialogs/dialog_lib_symbol_properties_base.cpp
    dialogs/dialog_line_properties.cpp
    dialogs/dialog_line_properties_base.cpp
    dialogs/dialog_migrate_buses.cpp
    dialogs/dialog_migrate_buses_base.cpp
    dialogs/dialog_pin_properties.cpp
    dialogs/dialog_pin_properties_base.cpp
    dialogs/dialog_plot_schematic.cpp
    dialogs/dialog_plot_schematic_base.cpp
    dialogs/dialog_erc_job_config.cpp
    dialogs/dialog_rescue_each.cpp
    dialogs/dialog_rescue_each_base.cpp
    dialogs/dialog_sch_import_settings.cpp
    dialogs/dialog_sch_import_settings_base.cpp
    dialogs/dialog_schematic_find.cpp
    dialogs/dialog_schematic_find_base.cpp
    dialogs/dialog_schematic_setup.cpp
    dialogs/dialog_shape_properties.cpp
    dialogs/dialog_shape_properties_base.cpp
    dialogs/dialog_sheet_pin_properties.cpp
    dialogs/dialog_sheet_pin_properties_base.cpp
    dialogs/dialog_sheet_properties.cpp
    dialogs/dialog_sheet_properties_base.cpp
    dialogs/dialog_symbol_chooser.cpp
    dialogs/dialog_symbol_fields_table.cpp
    dialogs/dialog_symbol_fields_table_base.cpp
    dialogs/dialog_symbol_properties.cpp
    dialogs/dialog_symbol_properties_base.cpp
    dialogs/dialog_symbol_remap.cpp
    dialogs/dialog_symbol_remap_base.cpp
    dialogs/dialog_table_properties.cpp
    dialogs/dialog_table_properties_base.cpp
    dialogs/dialog_tablecell_properties.cpp
    dialogs/dialog_tablecell_properties_base.cpp
    dialogs/dialog_text_properties.cpp
    dialogs/dialog_text_properties_base.cpp
    dialogs/dialog_update_from_pcb.cpp
    dialogs/dialog_update_from_pcb_base.cpp
    dialogs/dialog_update_symbol_fields.cpp
    dialogs/dialog_update_symbol_fields_base.cpp
    dialogs/dialog_wire_bus_properties.cpp
    dialogs/dialog_wire_bus_properties_base.cpp
    dialogs/panel_bom_presets.cpp
    dialogs/panel_bom_presets_base.cpp
    dialogs/panel_eeschema_color_settings.cpp
    dialogs/panel_eeschema_display_options.cpp
    dialogs/panel_eeschema_display_options_base.cpp
    dialogs/panel_eeschema_annotation_options.cpp
    dialogs/panel_eeschema_annotation_options_base.cpp
    dialogs/panel_eeschema_editing_options.cpp
    dialogs/panel_eeschema_editing_options_base.cpp
    dialogs/panel_setup_buses.cpp
    dialogs/panel_setup_buses_base.cpp
    dialogs/panel_setup_formatting.cpp
    dialogs/panel_setup_formatting_base.cpp
    dialogs/panel_setup_pinmap.cpp
    dialogs/panel_setup_pinmap_base.cpp
    dialogs/panel_simulator_preferences.cpp
    dialogs/panel_simulator_preferences_base.cpp
    dialogs/panel_sym_color_settings.cpp
    dialogs/panel_sym_color_settings_base.cpp
    dialogs/panel_sym_display_options.cpp
    dialogs/panel_sym_display_options_base.cpp
    dialogs/panel_sym_editing_options.cpp
    dialogs/panel_sym_editing_options_base.cpp
    dialogs/panel_sym_lib_table.cpp
    dialogs/panel_sym_lib_table_base.cpp
    dialogs/panel_template_fieldnames.cpp
    dialogs/panel_template_fieldnames_base.cpp
    )

# The simulator source files
set( EESCHEMA_SIM_SRCS
    dialogs/dialog_sim_command.cpp
    dialogs/dialog_sim_command_base.cpp
    dialogs/dialog_sim_format_value.cpp
    dialogs/dialog_sim_format_value_base.cpp
    dialogs/dialog_sim_model.cpp
    dialogs/dialog_sim_model_base.cpp
    dialogs/dialog_user_defined_signals.cpp
    dialogs/dialog_user_defined_signals_base.cpp
    tools/simulator_control.cpp

    sim/sim_library.cpp
    sim/sim_library_spice.cpp
    sim/sim_library_ibis.cpp
    sim/sim_lib_mgr.cpp

    sim/sim_model_serializer.cpp
    sim/sim_model.cpp
    sim/sim_model_behavioral.cpp
    sim/sim_model_ideal.cpp
    sim/sim_model_l_mutual.cpp
    sim/sim_model_ibis.cpp
    sim/sim_model_ngspice.cpp
    sim/sim_model_ngspice_data_bjt.cpp
    sim/sim_model_ngspice_data_bsim1.cpp
    sim/sim_model_ngspice_data_bsim2.cpp
    sim/sim_model_ngspice_data_bsim3.cpp
    sim/sim_model_ngspice_data_bsim4.cpp
    sim/sim_model_ngspice_data_b3soi.cpp
    sim/sim_model_ngspice_data_b4soi.cpp
    sim/sim_model_ngspice_data_diode.cpp
    sim/sim_model_ngspice_data_hfet.cpp
    sim/sim_model_ngspice_data_hicum2.cpp
    sim/sim_model_ngspice_data_hsim.cpp
    sim/sim_model_ngspice_data_jfet.cpp
    sim/sim_model_ngspice_data_mes.cpp
    sim/sim_model_ngspice_data_mos.cpp
    sim/sim_model_ngspice_data_mos6.cpp
    sim/sim_model_ngspice_data_mos9.cpp
    sim/sim_model_ngspice_data_vbic.cpp
    sim/sim_model_r_pot.cpp
    sim/sim_model_raw_spice.cpp
    sim/sim_model_source.cpp
    sim/sim_model_spice.cpp
    sim/sim_model_spice_fallback.cpp
    sim/sim_model_subckt.cpp
    sim/sim_model_switch.cpp
    sim/sim_model_tline.cpp
    sim/sim_model_xspice.cpp

    sim/sim_value.cpp
    sim/spice_settings.cpp
    sim/spice_generator.cpp
    sim/spice_library_parser.cpp
    sim/spice_model_parser.cpp

    sim/kibis/ibis_parser.cpp
    sim/kibis/kibis.cpp

    sim/spice_circuit_model.cpp
    sim/ngspice.cpp
    sim/simulator_frame.cpp
    sim/simulator_frame_ui.cpp
    sim/simulator_frame_ui_base.cpp
    sim/sim_plot_colors.cpp
    sim/sim_plot_tab.cpp
    sim/sim_property.cpp
    sim/sim_tab.cpp
    sim/spice_simulator.cpp
    sim/spice_value.cpp
    sim/toolbars_simulator_frame.cpp

    sim/legacy_workbook.cpp

    widgets/tuner_slider.cpp
    widgets/tuner_slider_base.cpp
    )

set( EESCHEMA_WIDGETS
    widgets/design_block_pane.cpp
    widgets/design_block_preview_widget.cpp
    widgets/hierarchy_pane.cpp
    widgets/panel_design_block_chooser.cpp
    widgets/panel_sch_selection_filter_base.cpp
    widgets/panel_sch_selection_filter.cpp
    widgets/panel_symbol_chooser.cpp
    widgets/pin_shape_combobox.cpp
    widgets/pin_type_combobox.cpp
    widgets/symbol_diff_widget.cpp
    widgets/sch_properties_panel.cpp
    widgets/sch_search_pane.cpp
    widgets/search_handlers.cpp
    widgets/symbol_filter_combobox.cpp
    widgets/symbol_preview_widget.cpp
    widgets/symbol_tree_pane.cpp
    )


set ( EESCHEMA_LIBEDIT_SRCS
    symbol_editor/lib_logger.cpp
    symbol_editor/lib_symbol_library_manager.cpp
    symbol_editor/menubar_symbol_editor.cpp
    symbol_editor/symbol_edit_frame.cpp
    symbol_editor/symbol_editor.cpp
    symbol_editor/symbol_editor_import_export.cpp
    symbol_editor/symbol_editor_plotter.cpp
    symbol_editor/symbol_editor_settings.cpp
    symbol_editor/symbol_editor_undo_redo.cpp
    symbol_editor/toolbars_symbol_editor.cpp
    )

set( EESCHEMA_IMPORT_GFX
    import_gfx/dialog_import_gfx_sch_base.cpp
    import_gfx/dialog_import_gfx_sch.cpp
    import_gfx/graphics_importer_lib_symbol.cpp
    import_gfx/graphics_importer_sch.cpp
    )

set( EESCHEMA_PRINTING
    printing/dialog_print.cpp
    printing/dialog_print_base.cpp
    printing/sch_printout.cpp
    )

set( SYNC_SHEET_PIN_SRCS
    sync_sheet_pin/dialog_sync_sheet_pins.cpp
    sync_sheet_pin/dialog_sync_sheet_pins_base.cpp
    sync_sheet_pin/panel_sync_sheet_pins.cpp
    sync_sheet_pin/panel_sync_sheet_pins_base.cpp
    sync_sheet_pin/sheet_synchronization_agent.cpp
    sync_sheet_pin/sheet_synchronization_item.cpp
    sync_sheet_pin/sheet_synchronization_notifier.cpp
    sync_sheet_pin/sheet_synchronization_model.cpp
    )

set( EESCHEMA_ERC_SRCS
    erc/erc.cpp
    erc/erc_item.cpp
    erc/erc_report.cpp
    erc/erc_sch_pin_context.cpp
    erc/erc_settings.cpp
    )

set( EESCHEMA_SRCS
    ${EESCHEMA_DLGS}
    ${EESCHEMA_ERC_SRCS}
    ${EESCHEMA_LIBEDIT_SRCS}
    ${EESCHEMA_SCH_IO}
    ${EESCHEMA_SIM_SRCS}
    ${EESCHEMA_WIDGETS}
    ${EESCHEMA_IMPORT_GFX}
    ${SYNC_SHEET_PIN_SRCS}
    annotate.cpp
    autoplace_fields.cpp
    bom_plugins.cpp
    bus-wire-junction.cpp
    connection_graph.cpp
    cross-probing.cpp
    design_block_tree_model_adapter.cpp
    design_block_utils.cpp
    ee_collectors.cpp
    eeschema_config.cpp
    eeschema_helpers.cpp
    eeschema_jobs_handler.cpp
    eeschema_settings.cpp
    fields_data_model.cpp
    fields_grid_table.cpp
    files-io.cpp
    generate_alias_info.cpp
    gfx_import_utils.cpp
    junction_helpers.cpp
    lib_symbol.cpp
    libarch.cpp
    menubar.cpp
    net_navigator.cpp
    picksymbol.cpp
    pin_layout_cache.cpp
    pin_numbers.cpp
    pin_type.cpp
    project_sch.cpp
    project_rescue.cpp
    sch_base_frame.cpp
    sch_bitmap.cpp
    sch_bus_entry.cpp
    sch_commit.cpp
    sch_connection.cpp
    sch_draw_panel.cpp
    sch_edit_frame.cpp
    sch_field.cpp
    sch_item.cpp
    sch_junction.cpp
    sch_label.cpp
    sch_line.cpp
    sch_marker.cpp
    sch_no_connect.cpp
    sch_painter.cpp
    sch_pin.cpp
    sch_plotter.cpp
    sch_preview_panel.cpp
    sch_reference_list.cpp
    sch_render_settings.cpp
    sch_screen.cpp
    sch_shape.cpp
    sch_rule_area.cpp
    sch_sheet.cpp
    sch_sheet_path.cpp
    sch_sheet_pin.cpp
    sch_symbol.cpp
    sch_table.cpp
    sch_tablecell.cpp
    sch_text.cpp
    sch_textbox.cpp
    sch_validators.cpp
    sch_view.cpp
    schematic.cpp
    schematic_settings.cpp
    schematic_undo_redo.cpp
    sheet.cpp
    symbol.cpp
    symbol_async_loader.cpp
    symbol_checker.cpp
    symbol_chooser_frame.cpp
    symbol_lib_table.cpp
    symbol_library.cpp
    symbol_library_manager.cpp
    symbol_tree_model_adapter.cpp
    symbol_tree_synchronizing_adapter.cpp
    symbol_viewer_frame.cpp
    symb_transforms_utils.cpp
    toolbars_sch_editor.cpp
    toolbars_symbol_viewer.cpp

    api/api_sch_utils.cpp

    netlist_exporters/netlist_exporter_allegro.cpp
    netlist_exporters/netlist_exporter_base.cpp
    netlist_exporters/netlist_exporter_cadstar.cpp
    netlist_exporters/netlist_exporter_kicad.cpp
    netlist_exporters/netlist_exporter_pads.cpp
    netlist_exporters/netlist_exporter_orcadpcb2.cpp
    netlist_exporters/netlist_exporter_spice.cpp
    netlist_exporters/netlist_exporter_spice_model.cpp
    netlist_exporters/netlist_exporter_xml.cpp
    netlist_exporters/netlist_generator.cpp

    tools/assign_footprints.cpp
    tools/backannotate.cpp
    tools/ee_actions.cpp
    tools/ee_grid_helper.cpp
    tools/ee_inspection_tool.cpp
    tools/ee_point_editor.cpp
    tools/ee_selection.cpp
    tools/ee_selection_tool.cpp
    tools/ee_tool_utils.cpp
    tools/rule_area_create_helper.cpp
    tools/sch_drawing_tools.cpp
    tools/sch_design_block_control.cpp
    tools/sch_edit_table_tool.cpp
    tools/sch_edit_tool.cpp
    tools/sch_editor_control.cpp
    tools/sch_editor_conditions.cpp
    tools/sch_find_replace_tool.cpp
    tools/sch_line_wire_bus_tool.cpp
    tools/sch_move_tool.cpp
    tools/sch_navigate_tool.cpp
    tools/symbol_editor_control.cpp
    tools/symbol_editor_drawing_tools.cpp
    tools/symbol_editor_edit_tool.cpp
    tools/symbol_editor_move_tool.cpp
    tools/symbol_editor_pin_tool.cpp
    )

if( KICAD_IPC_API )
    set( EESCHEMA_SRCS
        ${EESCHEMA_SRCS}
        api/api_handler_sch.cpp
    )
endif()

if( WIN32 )
    if( MINGW )
        # EESCHEMA_RESOURCES variable is set by the macro.
        mingw_resource_compiler( eeschema )
    else()
        set( EESCHEMA_RESOURCES ${CMAKE_SOURCE_DIR}/resources/msw/eeschema.rc )
    endif()
endif()

# Create a C++ compilable string initializer containing markdown text into a *.h file:
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/dialogs/dialog_bom_help_md.h
    COMMAND ${CMAKE_COMMAND}
        -DinputFile=${CMAKE_CURRENT_SOURCE_DIR}/dialogs/dialog_bom_help.md
        -DoutputFile=${CMAKE_CURRENT_SOURCE_DIR}/dialogs/dialog_bom_help_md.h
        -P ${KICAD_CMAKE_MODULE_PATH}/BuildSteps/markdown2C.cmake
    DEPENDS ${KICAD_CMAKE_MODULE_PATH}/BuildSteps/markdown2C.cmake ${CMAKE_CURRENT_SOURCE_DIR}/dialogs/dialog_bom_help.md
    COMMENT "creating ${KICAD_CMAKE_MODULE_PATH}/dialogs/dialog_bom_help_md.h
       from ${CMAKE_CURRENT_SOURCE_DIR}/dialogs/dialog_bom_help.md"
    )

set_source_files_properties( dialogs/dialog_bom.cpp
    PROPERTIES
        OBJECT_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/dialogs/dialog_bom_help_md.h
    )

# Create a C++ compilable string initializer containing markdown text into a *.h file:
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/sch_text_help_md.h
    COMMAND ${CMAKE_COMMAND}
        -DinputFile=${CMAKE_CURRENT_SOURCE_DIR}/sch_text_help.md
        -DoutputFile=${CMAKE_CURRENT_SOURCE_DIR}/sch_text_help_md.h
        -P ${KICAD_CMAKE_MODULE_PATH}/BuildSteps/markdown2C.cmake
    DEPENDS ${KICAD_CMAKE_MODULE_PATH}/BuildSteps/markdown2C.cmake ${CMAKE_CURRENT_SOURCE_DIR}/sch_text_help.md
    COMMENT "creating ${CMAKE_CURRENT_SOURCE_DIR}/sch_text_help_md.h
       from ${CMAKE_CURRENT_SOURCE_DIR}/sch_text_help.md"
    )

set_source_files_properties( sch_text.cpp
    PROPERTIES
        OBJECT_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/sch_text_help_md.h
    )

# Create a C++ compilable string initializer containing markdown text into a *.h file:
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/sim/user_defined_signals_help_md.h
    COMMAND ${CMAKE_COMMAND}
        -DinputFile=${CMAKE_CURRENT_SOURCE_DIR}/sim/user_defined_signals_help.md
        -DoutputFile=${CMAKE_CURRENT_SOURCE_DIR}/sim/user_defined_signals_help_md.h
        -P ${KICAD_CMAKE_MODULE_PATH}/BuildSteps/markdown2C.cmake
    DEPENDS ${KICAD_CMAKE_MODULE_PATH}/BuildSteps/markdown2C.cmake sim/user_defined_signals_help.md
    COMMENT "creating ${CMAKE_CURRENT_SOURCE_DIR}/sim/user_defined_signals_help_md.h
       from ${CMAKE_CURRENT_SOURCE_DIR}/sch_spice_fcn_help.md"
    )

set_source_files_properties( dialogs/dialog_user_defined_signals.cpp
    PROPERTIES
        OBJECT_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/sim/user_defined_signals_help_md.h
    )

if( APPLE )
    # setup bundle
    set( EESCHEMA_RESOURCES eeschema.icns eeschema_doc.icns libedit.icns libedit_doc.icns )
    set_source_files_properties(
        "${CMAKE_CURRENT_SOURCE_DIR}/eeschema.icns"
        "${CMAKE_CURRENT_SOURCE_DIR}/eeschema_doc.icns"
        "${CMAKE_CURRENT_SOURCE_DIR}/libedit.icns"
        "${CMAKE_CURRENT_SOURCE_DIR}/libedit_doc.icns"
        PROPERTIES MACOSX_PACKAGE_LOCATION Resources
        )
    set( MACOSX_BUNDLE_ICON_FILE eeschema.icns )
    set( MACOSX_BUNDLE_GUI_IDENTIFIER org.kicad.kicad )
    set( MACOSX_BUNDLE_NAME eeschema )
endif()


add_executable( eeschema WIN32 MACOSX_BUNDLE
    ${CMAKE_SOURCE_DIR}/common/single_top.cpp
    ${EESCHEMA_RESOURCES}
    )

set_source_files_properties( ${CMAKE_SOURCE_DIR}/common/single_top.cpp PROPERTIES
    COMPILE_DEFINITIONS "TOP_FRAME=FRAME_SCH;PGM_DATA_FILE_EXT=\"kicad_sch\";BUILD_KIWAY_DLL"
    )

target_link_libraries( eeschema
    kicommon
    ${wxWidgets_LIBRARIES}
    )

# the main Eeschema program, in DSO form.
add_library( eeschema_kiface_objects OBJECT
    ${EESCHEMA_SRCS}
    ${EESCHEMA_PRINTING}
    ${EESCHEMA_COMMON_SRCS}
    )

target_include_directories( eeschema_kiface_objects
    PUBLIC
        .
        netlist_exporters
     )

target_link_libraries( eeschema_kiface_objects
    PUBLIC
        common )

# Since we're not using target_link_libraries, we need to explicitly
# declare the dependency
add_dependencies( eeschema_kiface_objects common )

message( STATUS "Including 3Dconnexion SpaceMouse navigation support in eeschema" )
add_subdirectory( navlib )

target_link_libraries( eeschema_kiface_objects PUBLIC eeschema_navlib)

add_dependencies( eeschema_kiface_objects eeschema_navlib )

add_library( eeschema_kiface MODULE
    eeschema.cpp
    )

target_link_libraries( eeschema_kiface
    PRIVATE
        common
        eeschema_kiface_objects
        markdown_lib
        scripting
        sexpr
        core
        Boost::headers
        ${wxWidgets_LIBRARIES}
        ${NGSPICE_LIBRARY}
    )

if( MSVC )
    # Allow for MSVC to debug ngspice from the build directory
    add_custom_command( TARGET eeschema_kiface POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${NGSPICE_DLL}" "$<TARGET_FILE_DIR:eeschema_kiface>"
        )

    add_custom_command( TARGET eeschema_kiface POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${NGSPICE_CM_DIR}" "$<TARGET_FILE_DIR:eeschema_kiface>/ngspice"
        )
endif()

set_target_properties( eeschema_kiface PROPERTIES
    # Decorate OUTPUT_NAME with PREFIX and SUFFIX, creating something like
    # _eeschema.so, _eeschema.dll, or _eeschema.kiface
    OUTPUT_NAME     eeschema
    PREFIX          ${KIFACE_PREFIX}
    SUFFIX          ${KIFACE_SUFFIX}
    )

# The KIFACE is in eeschema.cpp, export it:
set_source_files_properties( eeschema.cpp PROPERTIES
    COMPILE_DEFINITIONS     "BUILD_KIWAY_DLL;COMPILING_DLL"
    )

# if building eeschema, then also build eeschema_kiface if out of date.
add_dependencies( eeschema eeschema_kiface )

# Generate link map with cross reference
target_link_options( eeschema_kiface PRIVATE
    $<$<BOOL:${KICAD_MAKE_LINK_MAPS}>:-Wl,--cref,-Map=${KIFACE_PREFIX}eeschema${KIFACE_SUFFIX}.map>
    )
target_link_options( eeschema PRIVATE
    $<$<BOOL:${KICAD_MAKE_LINK_MAPS}>:-Wl,--cref,-Map=eeschema.map>
    )

# these 2 binaries are a matched set, keep them together:
if( APPLE )
    set_target_properties( eeschema PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST ${PROJECT_BINARY_DIR}/eeschema/Info.plist
        )

    # puts binaries into the *.app bundle while linking
    set_target_properties( eeschema_kiface PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${OSX_BUNDLE_BUILD_KIFACE_DIR}
        )

    set_target_properties( eeschema PROPERTIES INSTALL_RPATH
            "@executable_path/../Frameworks;@executable_path/../Frameworks/Python.framework" )

    set_target_properties( eeschema_kiface PROPERTIES INSTALL_RPATH
            "@executable_path/../Frameworks;@executable_path/../Frameworks/Python.framework" )
    set_target_properties( eeschema_kiface PROPERTIES BUILD_WITH_INSTALL_RPATH 1 )

    # put individual bundle outside of main bundle as a first step
    # will be pulled into the main bundle when creating main bundle
    install( TARGETS eeschema
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

    # bundle libngspice and codemodels
    get_filename_component( ABS_LIBNGSPICE ${NGSPICE_LIBRARY} ABSOLUTE )
    get_filename_component( LIBNGSPICE_PATH ${ABS_LIBNGSPICE} DIRECTORY )

    install( DIRECTORY "${LIBNGSPICE_PATH}/"
            DESTINATION "${OSX_BUNDLE_INSTALL_PLUGIN_DIR}/sim"
            FILES_MATCHING PATTERN "*.dylib")
    install( DIRECTORY "${LIBNGSPICE_PATH}/ngspice"
             DESTINATION "${OSX_BUNDLE_INSTALL_PLUGIN_DIR}/sim" )

    install( CODE [[
        include( ${KICAD_CMAKE_MODULE_PATH}/InstallSteps/InstallMacOS.cmake )

        # Install any dependencies (this will generally duplicate kicad.app but we can't be sure)
        install_runtime_deps( "${KICAD_BIN}/eeschema.app/Contents/MacOS/eeschema"
            "${OSX_BUNDLE_BUILD_KIFACE_DIR}/_eeschema.kiface"
            ""
            )
    ]] )
else()
    if( MSVC )
        target_sources( eeschema_kiface PRIVATE ${CMAKE_SOURCE_DIR}/resources/msw/eeschema-dll.rc )
    endif()

    install( TARGETS eeschema
        DESTINATION ${KICAD_BIN}
        COMPONENT binary
        )
    install( TARGETS eeschema_kiface
        # actual filename subject to change at milestone C)
        # modular-kicad blueprint.
        DESTINATION ${KICAD_KIFACE}
        COMPONENT binary
        )
endif()


if( KICAD_WIN32_INSTALL_PDBS )
    # Get the PDBs to copy over for MSVC
    install(FILES $<TARGET_PDB_FILE:eeschema> DESTINATION ${KICAD_BIN})
    install(FILES $<TARGET_PDB_FILE:eeschema_kiface> DESTINATION ${KICAD_KIFACE})
endif()

make_lexer(
    eeschema_kiface_objects
    dialogs/dialog_bom_cfg.keywords
    dialogs/dialog_bom_cfg_lexer.h
    dialogs/dialog_bom_cfg_keywords.cpp
    T_BOMCFG_T
    )

make_lexer(
    eeschema_kiface_objects
    schematic.keywords
    schematic_lexer.h
    schematic_keywords.cpp
    TSCHEMATIC_T
    )

add_subdirectory( python_scripts )
