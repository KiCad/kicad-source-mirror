# Add all the warnings to the files
if( COMPILER_SUPPORTS_WARNINGS )
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WARN_FLAGS_CXX}")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${WARN_FLAGS_C}")
endif()

add_definitions( -DEESCHEMA )

if( KICAD_SPICE )
    set( INC_AFTER ${INC_AFTER} ${NGSPICE_INCLUDE_DIR} )

    # Find out the exact libngspice file name
    get_filename_component( NGSPICE_DLL_REALPATH "${NGSPICE_DLL}" REALPATH )
    get_filename_component( NGSPICE_DLL_FILE "${NGSPICE_DLL_REALPATH}" NAME )

    set_property( SOURCE sim/ngspice.cpp
        APPEND PROPERTY COMPILE_DEFINITIONS
        NGSPICE_DLL_FILE="${NGSPICE_DLL_FILE}"
        )
endif()

include_directories( BEFORE ${INC_BEFORE} )
include_directories(
    ./dialogs
    ./netlist_exporters
    ./widgets
    symbol_editor
    ./libview
    ./tools
    ${CMAKE_SOURCE_DIR}/common
    ${CMAKE_SOURCE_DIR}/common/dialogs
    ${CMAKE_SOURCE_DIR}/libs/sexpr/include
    ${INC_AFTER}
    )

set( EESCHEMA_SCH_PLUGINS_ALTIUM
    sch_plugins/altium/sch_altium_plugin.cpp
    sch_plugins/altium/altium_parser_sch.cpp
    )


set( EESCHEMA_SCH_PLUGINS_CADSTAR
    sch_plugins/cadstar/cadstar_sch_archive_plugin.cpp
    sch_plugins/cadstar/cadstar_sch_archive_parser.cpp
    sch_plugins/cadstar/cadstar_sch_archive_loader.cpp
    )

set( EESCHEMA_DLGS
    dialogs/dialog_annotate.cpp
    dialogs/dialog_annotate_base.cpp
    dialogs/dialog_bom.cpp
    dialogs/dialog_bom_base.cpp
    dialogs/dialog_bus_manager.cpp
    dialogs/dialog_fields_editor_global.cpp
    dialogs/dialog_fields_editor_global_base.cpp
    dialogs/dialog_change_symbols.cpp
    dialogs/dialog_change_symbols_base.cpp
    dialogs/dialog_choose_symbol.cpp
    dialogs/dialog_lib_symbol_properties.cpp
    dialogs/dialog_lib_symbol_properties_base.cpp
    dialogs/dialog_edit_components_libid.cpp
    dialogs/dialog_edit_components_libid_base.cpp
    dialogs/dialog_edit_label.cpp
    dialogs/dialog_edit_label_base.cpp
    dialogs/dialog_edit_line_style.cpp
    dialogs/dialog_edit_line_style_base.cpp
    dialogs/dialog_edit_one_field.cpp
    dialogs/dialog_eeschema_page_settings.cpp
    dialogs/dialog_erc.cpp
    dialogs/dialog_erc_base.cpp
    dialogs/dialog_global_sym_lib_table_config.cpp
    dialogs/dialog_global_edit_text_and_graphics.cpp
    dialogs/dialog_global_edit_text_and_graphics_base.cpp
    dialogs/dialog_junction_props.cpp
    dialogs/dialog_junction_props_base.cpp
    dialogs/dialog_lib_edit_draw_item.cpp
    dialogs/dialog_lib_edit_draw_item_base.cpp
    dialogs/dialog_lib_edit_pin_table.cpp
    dialogs/dialog_lib_edit_pin_table_base.cpp
    dialogs/dialog_lib_edit_text.cpp
    dialogs/dialog_lib_edit_text_base.cpp
    dialogs/dialog_lib_new_component.cpp
    dialogs/dialog_lib_new_component_base.cpp
    dialogs/dialog_migrate_buses.cpp
    dialogs/dialog_migrate_buses_base.cpp
    dialogs/dialog_netlist.cpp
    dialogs/dialog_netlist_base.cpp
    dialogs/dialog_pin_properties.cpp
    dialogs/dialog_pin_properties_base.cpp
    dialogs/dialog_plot_schematic.cpp
    dialogs/dialog_plot_schematic_base.cpp
    dialogs/dialog_print_using_printer.cpp
    dialogs/dialog_print_using_printer_base.cpp
    dialogs/dialog_rescue_each.cpp
    dialogs/dialog_rescue_each_base.cpp
    dialogs/dialog_sch_import_settings.cpp
    dialogs/dialog_sch_import_settings_base.cpp
    dialogs/dialog_sheet_pin_properties.cpp
    dialogs/dialog_sheet_pin_properties_base.cpp
    dialogs/dialog_sheet_properties.cpp
    dialogs/dialog_sheet_properties_base.cpp
    dialogs/dialog_schematic_find.cpp
    dialogs/dialog_schematic_find_base.cpp
    dialogs/dialog_schematic_setup.cpp
    dialogs/dialog_symbol_properties.cpp
    dialogs/dialog_symbol_properties_base.cpp
    dialogs/dialog_symbol_remap.cpp
    dialogs/dialog_symbol_remap_base.cpp
    dialogs/dialog_update_from_pcb.cpp
    dialogs/dialog_update_from_pcb_base.cpp
    dialogs/dialog_update_symbol_fields.cpp
    dialogs/dialog_update_symbol_fields_base.cpp
    dialogs/panel_eeschema_color_settings.cpp
    dialogs/panel_eeschema_template_fieldnames.cpp
    dialogs/panel_eeschema_template_fieldnames_base.cpp
    dialogs/panel_eeschema_display_options.cpp
    dialogs/panel_eeschema_display_options_base.cpp
    dialogs/panel_eeschema_editing_options.cpp
    dialogs/panel_eeschema_editing_options_base.cpp
    dialogs/panel_setup_formatting.cpp
    dialogs/panel_setup_formatting_base.cpp
    dialogs/panel_setup_pinmap.cpp
    dialogs/panel_setup_pinmap_base.cpp
    dialogs/panel_sym_editing_options.cpp
    dialogs/panel_sym_editing_options_base.cpp
    dialogs/panel_sym_color_settings.cpp
    dialogs/panel_sym_color_settings_base.cpp
    dialogs/panel_sym_lib_table.cpp
    dialogs/panel_sym_lib_table_base.cpp
    )

set( EESCHEMA_WIDGETS
    widgets/pin_shape_combobox.cpp
    widgets/pin_type_combobox.cpp
    widgets/symbol_preview_widget.cpp
    widgets/symbol_tree_pane.cpp
    )


set ( EESCHEMA_LIBEDIT_SRCS
    symbol_editor/symbol_editor.cpp
    symbol_editor/symbol_editor_plotter.cpp
    symbol_editor/symbol_editor_settings.cpp
    symbol_editor/symbol_editor_undo_redo.cpp
    symbol_editor/symbol_edit_frame.cpp
    symbol_editor/menubar_symbol_editor.cpp
    symbol_editor/toolbars_symbol_editor.cpp
    symbol_editor/symbol_editor_import_export.cpp
    symbol_editor/symbol_library_manager.cpp

)
set( EESCHEMA_SRCS
    ${EESCHEMA_SCH_PLUGINS_ALTIUM}
    ${EESCHEMA_SCH_PLUGINS_CADSTAR}
    ${EESCHEMA_DLGS}
    ${EESCHEMA_WIDGETS}
    ${EESCHEMA_LIBEDIT_SRCS}
    bom_plugins.cpp
    sch_view.cpp
    sch_painter.cpp
    annotate.cpp
    autoplace_fields.cpp
    bus_alias.cpp
    bus-wire-junction.cpp
    class_library.cpp
    cmp_library_lexer.cpp
    component_references_lister.cpp
    connection_graph.cpp
    cross-probing.cpp
    eeschema_config.cpp
    eeschema_settings.cpp
    erc.cpp
    erc_item.cpp
    erc_settings.cpp
    fields_grid_table.cpp
    files-io.cpp
    generate_alias_info.cpp
    getpart.cpp
    hierarch.cpp
    lib_arc.cpp
    lib_bezier.cpp
    lib_circle.cpp
    lib_item.cpp
    lib_field.cpp
    lib_pin.cpp
    lib_polyline.cpp
    lib_rectangle.cpp
    lib_symbol.cpp
    lib_text.cpp
    symbol_viewer_frame.cpp
    libarch.cpp
    menubar.cpp
    pin_number.cpp
    pin_type.cpp
    sch_draw_panel.cpp
    project_rescue.cpp
    sch_base_frame.cpp
    sch_bitmap.cpp
    sch_bus_entry.cpp
    ee_collectors.cpp
    sch_connection.cpp
    sch_plugins/eagle/sch_eagle_plugin.cpp
    sch_field.cpp
    sch_io_mgr.cpp
    sch_item.cpp
    sch_junction.cpp
    sch_plugins/legacy/sch_legacy_plugin.cpp
    sch_line.cpp
    sch_marker.cpp
    sch_no_connect.cpp
    sch_pin.cpp
    sch_plugin.cpp
    sch_preview_panel.cpp
    sch_screen.cpp
    sch_plugins/kicad/sch_sexpr_parser.cpp
    sch_plugins/kicad/sch_sexpr_plugin.cpp
    sch_sheet.cpp
    sch_sheet_path.cpp
    sch_sheet_pin.cpp
    sch_symbol.cpp
    sch_text.cpp
    sch_validators.cpp
    schematic.cpp
    schematic_settings.cpp
    schematic_undo_redo.cpp
    sch_edit_frame.cpp
    sheet.cpp
    symbol_async_loader.cpp
    symbol_lib_table.cpp
    symbol_tree_model_adapter.cpp
    symbol_tree_synchronizing_adapter.cpp
    toolbars_symbol_viewer.cpp
    toolbars_sch_editor.cpp
    transform.cpp

    netlist_exporters/netlist_exporter_base.cpp
    netlist_exporters/netlist_exporter_cadstar.cpp
    netlist_exporters/netlist_exporter_xml.cpp
    netlist_exporters/netlist_exporter_kicad.cpp
    netlist_exporters/netlist_exporter_orcadpcb2.cpp
    netlist_exporters/netlist_exporter_pspice.cpp
    netlist_exporters/netlist_generator.cpp

    # Simulator settings must get built even when the simulator build option is disabled.
    sim/spice_settings.cpp

    tools/backannotate.cpp
    tools/assign_footprints.cpp
    tools/ee_actions.cpp
    tools/ee_grid_helper.cpp
    tools/ee_inspection_tool.cpp
    tools/ee_point_editor.cpp
    tools/ee_selection.cpp
    tools/ee_selection_tool.cpp
    tools/symbol_editor_control.cpp
    tools/symbol_editor_drawing_tools.cpp
    tools/symbol_editor_edit_tool.cpp
    tools/symbol_editor_move_tool.cpp
    tools/symbol_editor_pin_tool.cpp
    tools/sch_drawing_tools.cpp
    tools/sch_edit_tool.cpp
    tools/sch_editor_control.cpp
    tools/sch_line_wire_bus_tool.cpp
    tools/sch_move_tool.cpp
    tools/sch_navigate_tool.cpp
    )


set( EESCHEMA_COMMON_SRCS
    ${CMAKE_SOURCE_DIR}/common/dialogs/panel_gal_display_options.cpp
    ${CMAKE_SOURCE_DIR}/common/dialogs/panel_setup_severities.cpp
    ${CMAKE_SOURCE_DIR}/common/common.cpp
    ${CMAKE_SOURCE_DIR}/common/base_screen.cpp
    ${CMAKE_SOURCE_DIR}/common/base_units.cpp
    ${CMAKE_SOURCE_DIR}/common/eda_text.cpp
    ${CMAKE_SOURCE_DIR}/common/page_info.cpp
    )


if( KICAD_SPICE )
    set( EESCHEMA_SRCS
        ${EESCHEMA_SRCS}
        sim/netlist_exporter_pspice_sim.cpp
        sim/ngspice.cpp
        sim/sim_plot_colors.cpp
        sim/sim_plot_frame.cpp
        sim/sim_plot_frame_base.cpp
        sim/sim_plot_panel.cpp
        sim/sim_panel_base.cpp
        sim/sim_workbook.cpp
        sim/spice_simulator.cpp
        sim/spice_value.cpp
        dialogs/dialog_signal_list.cpp
        dialogs/dialog_signal_list_base.cpp
        dialogs/dialog_sim_settings.cpp
        dialogs/dialog_sim_settings_base.cpp
        dialogs/dialog_spice_model.cpp
        dialogs/dialog_spice_model_base.cpp
        widgets/tuner_slider.cpp
        widgets/tuner_slider_base.cpp
    )
endif()


if( MINGW )
    # EESCHEMA_RESOURCES variable is set by the macro.
    mingw_resource_compiler( eeschema )
else()
    set( EESCHEMA_RESOURCES ${CMAKE_SOURCE_DIR}/resources/msw/eeschema.rc )
endif()

# Create a C++ compilable string initializer containing markdown text into a *.h file:
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/dialogs/dialog_bom_help_md.h
    COMMAND ${CMAKE_COMMAND}
        -DinputFile=${CMAKE_CURRENT_SOURCE_DIR}/dialogs/dialog_bom_help.md
        -DoutputFile=${CMAKE_CURRENT_SOURCE_DIR}/dialogs/dialog_bom_help_md.h
        -P ${CMAKE_MODULE_PATH}/BuildSteps/markdown2C.cmake
    DEPENDS ${CMAKE_MODULE_PATH}/BuildSteps/markdown2C.cmake ${CMAKE_CURRENT_SOURCE_DIR}/dialogs/dialog_bom_help.md
    COMMENT "creating ${CMAKE_CURRENT_SOURCE_DIR}/dialogs/dialog_bom_help_md.h
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
        -P ${CMAKE_MODULE_PATH}/BuildSteps/markdown2C.cmake
    DEPENDS ${CMAKE_MODULE_PATH}/BuildSteps/markdown2C.cmake ${CMAKE_CURRENT_SOURCE_DIR}/sch_text_help.md
    COMMENT "creating ${CMAKE_CURRENT_SOURCE_DIR}/sch_text_help_md.h
       from ${CMAKE_CURRENT_SOURCE_DIR}/sch_text_help.md"
    )

set_source_files_properties( sch_text.cpp
    PROPERTIES
        OBJECT_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/sch_text_help_md.h
    )

if( APPLE )
    # setup bundle
    set( EESCHEMA_RESOURCES eeschema.icns eeschema_doc.icns )
    set_source_files_properties( "${CMAKE_CURRENT_SOURCE_DIR}/eeschema.icns" PROPERTIES
        MACOSX_PACKAGE_LOCATION Resources
        )
    set_source_files_properties( "${CMAKE_CURRENT_SOURCE_DIR}/eeschema_doc.icns"  PROPERTIES
        MACOSX_PACKAGE_LOCATION Resources
        )
    set( MACOSX_BUNDLE_ICON_FILE eeschema.icns )
    set( MACOSX_BUNDLE_GUI_IDENTIFIER org.kicad-pcb.kicad )
    set( MACOSX_BUNDLE_NAME eeschema )
endif()


add_executable( eeschema WIN32 MACOSX_BUNDLE
    ${CMAKE_SOURCE_DIR}/common/single_top.cpp
    ${CMAKE_SOURCE_DIR}/common/pgm_base.cpp
    ${EESCHEMA_RESOURCES}
    )
set_source_files_properties( ${CMAKE_SOURCE_DIR}/common/single_top.cpp PROPERTIES
    COMPILE_DEFINITIONS "TOP_FRAME=FRAME_SCH;PGM_DATA_FILE_EXT=\"kicad_sch\";BUILD_KIWAY_DLL"
    )
target_link_libraries( eeschema
    #singletop  # replaces common, giving us restrictive control and link warnings.
    # There's way too much crap coming in from common yet.
    gal
    common
    ${wxWidgets_LIBRARIES}
    )

# the main Eeschema program, in DSO form.
add_library( eeschema_kiface_objects OBJECT
    ${EESCHEMA_SRCS}
    ${EESCHEMA_COMMON_SRCS}
    )

# CMake <3.9 can't link anything to object libraries,
# but we only need include directories, as we will link the kiface MODULE
target_include_directories( eeschema_kiface_objects PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    $<TARGET_PROPERTY:common,INTERFACE_INCLUDE_DIRECTORIES>
)

# Since we're not using target_link_libraries, we need to explicitly
# declare the dependency
add_dependencies( eeschema_kiface_objects common )

add_library( eeschema_kiface MODULE
    eeschema.cpp
    $<TARGET_OBJECTS:eeschema_kiface_objects>
    )

target_include_directories( eeschema_kiface PUBLIC
    $<TARGET_PROPERTY:eeschema_kiface_objects,INTERFACE_INCLUDE_DIRECTORIES>
)

target_link_libraries( eeschema_kiface
    common
    sexpr
    markdown_lib
    ${wxWidgets_LIBRARIES}
    ${GDI_PLUS_LIBRARIES}
    )

if( KICAD_SPICE )
    target_link_libraries( eeschema_kiface
        ${NGSPICE_LIBRARY}
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

if( MAKE_LINK_MAPS )
    # generate link map with cross reference
    set_target_properties( eeschema_kiface PROPERTIES
        LINK_FLAGS "-Wl,-cref,-Map=${KIFACE_PREFIX}eeschema${KIFACE_SUFFIX}.map"
        )
    set_target_properties( eeschema PROPERTIES
        LINK_FLAGS "-Wl,-cref,-Map=eeschema.map"
        )
endif()

# these 2 binaries are a matched set, keep them together:
if( APPLE )
    set_target_properties( eeschema PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST ${PROJECT_BINARY_DIR}/eeschema/Info.plist
        )

    # puts binaries into the *.app bundle while linking
    set_target_properties( eeschema_kiface PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${OSX_BUNDLE_BUILD_KIFACE_DIR}
        )
    # put individual bundle outside of main bundle as a first step
    # will be pulled into the main bundle when creating main bundle
    install( TARGETS eeschema
        DESTINATION ${KICAD_BIN}
        COMPONENT binary
        )

    install( CODE "
            # override default embedded path settings
            ${OSX_BUNDLE_OVERRIDE_PATHS}

            # do all the work
            include( BundleUtilities )
            fixup_bundle( ${KICAD_BIN}/eeschema.app/Contents/MacOS/eeschema
                \"\"
                \"\"
                )
            " COMPONENT Runtime
        )

    if( KICAD_SPICE )
        # bundle libngspice and codemodels
        get_filename_component( REAL_LIBNGSPICE ${NGSPICE_LIBRARY} REALPATH )
        get_filename_component( LIBNGSPICE_PATH ${REAL_LIBNGSPICE} DIRECTORY )
        install( FILES "${REAL_LIBNGSPICE}"
                 DESTINATION "${OSX_BUNDLE_INSTALL_PLUGIN_DIR}/sim" )
        install( DIRECTORY "${LIBNGSPICE_PATH}/ngspice"
                 DESTINATION "${OSX_BUNDLE_INSTALL_PLUGIN_DIR}/sim" )
    endif()
else()
    install( TARGETS eeschema
        DESTINATION ${KICAD_BIN}
        COMPONENT binary
        )
    install( TARGETS eeschema_kiface
        # actual filename subject to change at milestone C)
        # modular-kicad blueprint.
        DESTINATION ${KICAD_BIN}
        COMPONENT binary
        )
endif()

# auto-generate cmp_library_lexer.h and cmp_library_keywords.cpp for the component
# library format.
make_lexer(
    eeschema_kiface_objects
    cmp_library.keywords
    cmp_library_lexer.h
    cmp_library_keywords.cpp
    TLIB_T
    )

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

add_subdirectory( plugins )
