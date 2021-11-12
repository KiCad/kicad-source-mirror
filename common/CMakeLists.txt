# Add all the warnings to the files
if( COMPILER_SUPPORTS_WARNINGS )
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WARN_FLAGS_CXX}")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${WARN_FLAGS_C}")
endif()

if( KICAD_SPICE )
    set( INC_AFTER ${INC_AFTER} ${NGSPICE_INCLUDE_DIR} )
endif()

include_directories( BEFORE ${INC_BEFORE} )
include_directories(
    ./dialogs
    ./widgets
    ./dialog_about
    ${CMAKE_SOURCE_DIR}/bitmaps_png
    ${CMAKE_SOURCE_DIR}/3d-viewer
    ${CMAKE_SOURCE_DIR}/pcbnew
    ${INC_AFTER}
    )

set( GAL_SRCS
    # Common part
    basic_gal.cpp
    draw_panel_gal.cpp
    gl_context_mgr.cpp
    newstroke_font.cpp
    painter.cpp
    gal/color4d.cpp
    gal/cursors.cpp
    gal/dpi_scaling.cpp
    gal/gal_display_options.cpp
    gal/graphics_abstraction_layer.cpp
    gal/hidpi_gl_canvas.cpp
    gal/stroke_font.cpp

    view/view_controls.cpp
    view/view_overlay.cpp
    view/wx_view_controls.cpp
    view/zoom_controller.cpp

    # OpenGL GAL
    gal/opengl/opengl_gal.cpp
    gal/opengl/gl_resources.cpp
    gal/opengl/gl_builtin_shaders.cpp
    gal/opengl/shader.cpp
    gal/opengl/vertex_item.cpp
    gal/opengl/vertex_container.cpp
    gal/opengl/cached_container.cpp
    gal/opengl/cached_container_gpu.cpp
    gal/opengl/cached_container_ram.cpp
    gal/opengl/noncached_container.cpp
    gal/opengl/vertex_manager.cpp
    gal/opengl/gpu_manager.cpp
    gal/opengl/antialiasing.cpp
    gal/opengl/opengl_compositor.cpp
    gal/opengl/utils.cpp

    # Cairo GAL
    gal/cairo/cairo_gal.cpp
    gal/cairo/cairo_compositor.cpp
    gal/cairo/cairo_print.cpp
    )

add_library( gal STATIC ${GAL_SRCS} )

target_link_libraries( gal
    common          # This is needed until the circular dependency is removed
    kimath
    ${GLEW_LIBRARIES}
    ${CAIRO_LIBRARIES}
    ${PIXMAN_LIBRARIES}
    ${OPENGL_LIBRARIES}
    ${GDI_PLUS_LIBRARIES}
)

target_include_directories( gal PRIVATE
    $<TARGET_PROPERTY:nlohmann_json,INTERFACE_INCLUDE_DIRECTORIES>
    )


# Only for win32 cross compilation using MXE
if( WIN32 AND MSYS )
    add_definitions( -DGLEW_STATIC )
endif()


# A shared library subsetted from common which restricts what can go into
# a single_top link image.  By not linking to common, we control what does
# statically go into single_top link images.  My current thinking is that only
# wxWidgets should be a shared link from single top, everything else should be
# statically bound into it. Otherwise you will have DSO loading probs. After it
# sets the LIB PATHS however, we want the *.kiface modules to use shared linking.
add_library( singletop STATIC EXCLUDE_FROM_ALL
    confirm.cpp
    eda_doc.cpp
    kiway.cpp
    kiway_holder.cpp
    )


# A shared library used by multiple *.kiface files and one or two program
# launchers.  Object files can migrate into here over time, but only if they are
# surely needed and certainly used from more than one place without recompilation.
# Functions and data all need to use the #include <import_export.h> and be declared
# as APIEXPORT
set( LIB_KICAD_SRCS
    string_utils.cpp
    )

if( future )
add_library( lib_kicad SHARED
    )
target_link_libraries( lib_kicad
    ${wxWidgets_LIBRARIES}
    )
set_target_properties( lib_kicad PROPERTIES
    OUTPUT_NAME     ki
    )
install( TARGETS lib_kicad
    DESTINATION ${KICAD_BIN}
    COMPONENT binary
    )
endif()


# The build version string defaults to the value in the KiCadVersion.cmake file.
# If being built inside a git repository, the git tag and commit hash are used to create
# a new version string instead. The user can supply an additional string to be appended
# to the end inside the KICAD_VERSION_EXTRA variable
set( KICAD_VERSION_EXTRA "" CACHE STRING
    "User defined configuration string to append to KiCad version." )

# Generate version header file.
add_custom_target(
    version_header ALL
    COMMAND ${CMAKE_COMMAND}
        -DKICAD_VERSION_EXTRA=${KICAD_VERSION_EXTRA}
        -DOUTPUT_FILE=${CMAKE_BINARY_DIR}/kicad_build_version.h
        -DTEXT_OUTPUT_FILE=${CMAKE_BINARY_DIR}/kicad_build_version.txt
        -DSRC_PATH=${PROJECT_SOURCE_DIR}
        -DCMAKE_MODULE_PATH=${CMAKE_MODULE_PATH}
        -P ${CMAKE_MODULE_PATH}/BuildSteps/WriteVersionHeader.cmake
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    BYPRODUCTS ${CMAKE_BINARY_DIR}/kicad_build_version.h
    DEPENDS ${CMAKE_MODULE_PATH}/BuildSteps/WriteVersionHeader.cmake
    COMMENT "Generating version string header"
    )

set( COMMON_ABOUT_DLG_SRCS
    dialog_about/AboutDialog_main.cpp
    dialog_about/dialog_about.cpp
    dialog_about/dialog_about_base.cpp
    )

set( COMMON_DLG_SRCS
    dialogs/dialog_color_picker.cpp
    dialogs/dialog_color_picker_base.cpp
    dialogs/dialog_configure_paths.cpp
    dialogs/dialog_configure_paths_base.cpp
    dialogs/dialog_display_html_text_base.cpp
    dialogs/dialog_edit_library_tables.cpp
    dialogs/dialog_global_lib_table_config.cpp
    dialogs/dialog_global_lib_table_config_base.cpp
    dialogs/dialog_grid_settings.cpp
    dialogs/dialog_grid_settings_base.cpp
    dialogs/dialog_hotkey_list.cpp
    dialogs/dialog_HTML_reporter_base.cpp
    dialogs/dialog_image_editor.cpp
    dialogs/dialog_image_editor_base.cpp
    dialogs/dialog_locked_items_query.cpp
    dialogs/dialog_locked_items_query_base.cpp
    dialogs/dialog_migrate_settings.cpp
    dialogs/dialog_migrate_settings_base.cpp
    dialogs/dialog_page_settings_base.cpp
    dialogs/dialog_paste_special.cpp
    dialogs/dialog_paste_special_base.cpp
    dialogs/dialog_text_entry_base.cpp
    dialogs/dialog_page_settings.cpp
    dialogs/dialog_print_generic.cpp
    dialogs/dialog_print_generic_base.cpp
    dialogs/dialog_text_entry.cpp
    dialogs/dialog_unit_entry.cpp
    dialogs/dialog_unit_entry_base.cpp
    dialogs/eda_list_dialog.cpp
    dialogs/eda_list_dialog_base.cpp
    dialogs/eda_view_switcher.cpp
    dialogs/eda_view_switcher_base.cpp
    dialogs/html_message_box.cpp
    dialogs/panel_color_settings_base.cpp
    dialogs/panel_color_settings.cpp
    dialogs/panel_common_settings.cpp
    dialogs/panel_common_settings_base.cpp
    dialogs/panel_hotkeys_editor.cpp
    dialogs/panel_mouse_settings.cpp
    dialogs/panel_mouse_settings_base.cpp
    dialogs/panel_setup_netclasses.cpp
    dialogs/panel_setup_netclasses_base.cpp
    dialogs/panel_setup_severities.cpp
    dialogs/panel_text_variables.cpp
    dialogs/panel_text_variables_base.cpp
    dialogs/wx_html_report_box.cpp
    dialogs/wx_html_report_panel.cpp
    dialogs/wx_html_report_panel_base.cpp
    )

set( COMMON_WIDGET_SRCS
    widgets/app_progress_dialog.cpp
    widgets/bitmap_button.cpp
    widgets/bitmap_toggle.cpp
    widgets/button_row_panel.cpp
    widgets/collapsible_pane.cpp
    widgets/color_swatch.cpp
    widgets/footprint_choice.cpp
    widgets/footprint_preview_widget.cpp
    widgets/footprint_select_widget.cpp
    widgets/gal_options_panel.cpp
    widgets/grid_bitmap_toggle.cpp
    widgets/grid_color_swatch_helpers.cpp
    widgets/grid_combobox.cpp
    widgets/grid_icon_text_helpers.cpp
    widgets/grid_text_button_helpers.cpp
    widgets/grid_text_helpers.cpp
    widgets/indicator_icon.cpp
    widgets/infobar.cpp
    widgets/layer_box_selector.cpp
    widgets/lib_tree.cpp
    widgets/mathplot.cpp
    widgets/msgpanel.cpp
    widgets/number_badge.cpp
    widgets/paged_dialog.cpp
    widgets/progress_reporter_base.cpp
    widgets/split_button.cpp
    widgets/stepped_slider.cpp
    widgets/text_ctrl_eval.cpp
    widgets/ui_common.cpp
    widgets/unit_binder.cpp
    widgets/widget_save_restore.cpp
    widgets/widget_hotkey_list.cpp
    widgets/wx_aui_art_providers.cpp
    widgets/wx_aui_utils.cpp
    widgets/wx_busy_indicator.cpp
    widgets/wx_ellipsized_static_text.cpp
    widgets/wx_grid.cpp
    widgets/wx_progress_reporters.cpp
    widgets/wx_splitter_window.cpp
    )

set( COMMON_DRAWING_SHEET_SRCS
    drawing_sheet/ds_data_item.cpp
    drawing_sheet/ds_data_model.cpp
    drawing_sheet/ds_data_model_io.cpp
    drawing_sheet/drawing_sheet_default_description.cpp
    drawing_sheet/ds_draw_item.cpp
    drawing_sheet/ds_proxy_undo_item.cpp
    drawing_sheet/ds_proxy_view_item.cpp
    drawing_sheet/drawing_sheet_parser.cpp
    )

set( COMMON_PREVIEW_ITEMS_SRCS
    preview_items/arc_assistant.cpp
    preview_items/arc_geom_manager.cpp
    preview_items/centreline_rect_item.cpp
    preview_items/draw_context.cpp
    preview_items/polygon_geom_manager.cpp
    preview_items/polygon_item.cpp
    preview_items/preview_utils.cpp
    preview_items/ruler_item.cpp
    preview_items/selection_area.cpp
    preview_items/simple_overlay_item.cpp
    preview_items/two_point_assistant.cpp
    )

set( PLOTTERS_CONTROL_SRCS
    plotters/plotter.cpp
    plotters/DXF_plotter.cpp
    plotters/GERBER_plotter.cpp
    plotters/HPGL_plotter.cpp
    plotters/PDF_plotter.cpp
    plotters/PS_plotter.cpp
    plotters/SVG_plotter.cpp
    plotters/common_plot_functions.cpp
    )

set( PLUGINS_ALTIUM_SRCS
    plugins/altium/altium_parser.cpp
    plugins/altium/altium_parser_utils.cpp
    )

set( PLUGINS_CADSTAR_SRCS
    plugins/cadstar/cadstar_archive_parser.cpp
    )

set( PLUGINS_EAGLE_SRCS
    plugins/eagle/eagle_parser.cpp
    )

set( COMMON_SRCS
    ${LIB_KICAD_SRCS}
    ${COMMON_ABOUT_DLG_SRCS}
    ${COMMON_DLG_SRCS}
    ${COMMON_WIDGET_SRCS}
    ${COMMON_DRAWING_SHEET_SRCS}
    ${COMMON_PREVIEW_ITEMS_SRCS}
    ${PLOTTERS_CONTROL_SRCS}
    ${PLUGINS_ALTIUM_SRCS}
    ${PLUGINS_CADSTAR_SRCS}
    ${PLUGINS_EAGLE_SRCS}
    advanced_config.cpp
    array_axis.cpp
    array_options.cpp
    asset_archive.cpp
    base64.cpp
    bin_mod.cpp
    bitmap.cpp
    bitmap_base.cpp
    bitmap_store.cpp
    board_printout.cpp
    build_version.cpp
    commit.cpp
    common.cpp
    config_params.cpp
    confirm.cpp
    dialog_shim.cpp
    gr_text.cpp
    dsnlexer.cpp
    eda_base_frame.cpp
    eda_dde.cpp
    eda_doc.cpp
    eda_draw_frame.cpp
    eda_item.cpp
    eda_pattern_match.cpp
    eda_rect.cpp
    eda_units.cpp
    env_paths.cpp
    env_vars.cpp
    exceptions.cpp
    executable_names.cpp
    filename_resolver.cpp
    filehistory.cpp
    filter_reader.cpp
    footprint_filter.cpp
    footprint_info.cpp
    gbr_metadata.cpp
    gestfich.cpp
    getrunningmicrosecs.cpp
    gr_basic.cpp
    grid_tricks.cpp
    hotkey_store.cpp
    hotkeys_basic.cpp
    html_window.cpp
    kiface_base.cpp
    kiid.cpp
    kiway.cpp
    kiway_express.cpp
    kiway_holder.cpp
    kiway_player.cpp
    languages_menu.cpp
    launch_ext.cpp
    layer_id.cpp
    lib_id.cpp
    lib_table_base.cpp
    lib_tree_model.cpp
    lib_tree_model_adapter.cpp
    locale_io.cpp
    lockfile.cpp
    lset.cpp
    marker_base.cpp
    netclass.cpp
    observable.cpp
    origin_transforms.cpp
    paths.cpp
    printout.cpp
    project.cpp
    properties.cpp
    property_mgr.cpp
    ptree.cpp
    rc_item.cpp
    refdes_utils.cpp
    render_settings.cpp
    reporter.cpp
    richio.cpp
    scintilla_tricks.cpp
    search_stack.cpp
    searchhelpfilefullpath.cpp
    status_popup.cpp
    systemdirsappend.cpp
    template_fieldnames.cpp
    textentry_tricks.cpp
    title_block.cpp
    trace_helpers.cpp
    undo_redo_container.cpp
    utf8.cpp
    validators.cpp
    wildcards_and_files_ext.cpp
    drawing_sheet/ds_painter.cpp
    wxdataviewctrl_helpers.cpp
    wx_stl_compat.cpp
    wx_filename.cpp
    xnode.cpp
    )

if( TRUE OR NOT USE_KIWAY_DLLS )
#if( NOT USE_KIWAY_DLLS )
    # We DO NOT want pgm_base.cpp linked into the KIFACE, only into the KIWAY.
    # Check the map files to verify eda_pgm.o not being linked in.
    list( APPEND COMMON_SRCS pgm_base.cpp )
endif()

if( NOT HAVE_STRTOKR )
    list( APPEND COMMON_SRCS strtok_r.c )
endif()

list( APPEND COMMON_SRCS
    kicad_curl/kicad_curl.cpp
    kicad_curl/kicad_curl_easy.cpp
)

set( COMMON_SRCS
    ${COMMON_SRCS}

    origin_viewitem.cpp

    view/view.cpp
    view/view_item.cpp
    view/view_group.cpp

    tool/action_manager.cpp
    tool/action_menu.cpp
    tool/action_toolbar.cpp
    tool/actions.cpp
    tool/common_control.cpp
    tool/common_tools.cpp
    tool/conditional_menu.cpp
    tool/edit_constraints.cpp
    tool/edit_points.cpp
    tool/editor_conditions.cpp
    tool/grid_helper.cpp
    tool/grid_menu.cpp
    tool/picker_tool.cpp
    tool/selection.cpp
    tool/selection_tool.cpp
    tool/selection_conditions.cpp
    tool/tool_action.cpp
    tool/tool_base.cpp
    tool/tool_dispatcher.cpp
    tool/tool_event.cpp
    tool/tools_holder.cpp
    tool/tool_interactive.cpp
    tool/tool_manager.cpp
    tool/tool_menu.cpp
    tool/zoom_menu.cpp
    tool/zoom_tool.cpp

    settings/app_settings.cpp
    settings/color_settings.cpp
    settings/common_settings.cpp
    settings/json_settings.cpp
    settings/nested_settings.cpp
    settings/parameters.cpp
    settings/settings_manager.cpp

    project/board_project_settings.cpp
    project/net_settings.cpp
    project/project_archiver.cpp
    project/project_file.cpp
    project/project_local_settings.cpp
    )

add_library( common STATIC
    ${COMMON_SRCS}
    $<TARGET_OBJECTS:libcontext>
    )

target_include_directories( common PRIVATE
    $<TARGET_PROPERTY:libcontext,INTERFACE_INCLUDE_DIRECTORIES>
    )

add_dependencies( common libcontext )
add_dependencies( common version_header )
add_dependencies( common compoundfilereader )  # used by altium_parser.cpp

target_link_libraries( common
    kimath
    kiplatform
    gal
    scripting
    pybind11::embed
    ${Boost_LIBRARIES}
    ${CURL_LIBRARIES}
    ${wxWidgets_LIBRARIES}
    ${EXTRA_LIBS}
    )

target_include_directories( common PUBLIC
    $<TARGET_PROPERTY:nlohmann_json,INTERFACE_INCLUDE_DIRECTORIES>
    )

target_include_directories( common PUBLIC
    $<TARGET_PROPERTY:compoundfilereader,INTERFACE_INCLUDE_DIRECTORIES>
    )  # used by altium_parser.cpp

set( PCB_COMMON_SRCS
    base_screen.cpp
    eda_text.cpp
    eda_shape.cpp
    fp_lib_table.cpp
    hash_eda.cpp
    page_info.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/pcb_base_frame.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/pcb_expr_evaluator.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/board_commit.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/board_connected_item.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/board_design_settings.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/board.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/board_item.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/pcb_dimension.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/pcb_shape.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/fp_shape.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/pcb_group.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/pcb_marker.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/footprint.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/netinfo_item.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/netinfo_list.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/pad.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/pcb_target.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/pcb_text.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/board_stackup_manager/board_stackup.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/fp_text.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/pcb_track.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/zone.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/collectors.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/connectivity/connectivity_algo.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/connectivity/connectivity_items.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/connectivity/connectivity_data.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/connectivity/from_to_cache.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/convert_shape_list_to_polygon.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/drc/drc_engine.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/drc/drc_item.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/drc/drc_rule.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/drc/drc_rule_condition.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/drc/drc_rule_parser.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/drc/drc_test_provider.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/plugins/eagle/eagle_plugin.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/footprint_editor_settings.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/plugins/geda/gpcb_plugin.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/io_mgr.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/kicad_clipboard.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/netlist_reader/kicad_netlist_reader.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/plugins/kicad/kicad_plugin.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/netlist_reader/legacy_netlist_reader.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/plugins/legacy/legacy_plugin.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/netlist_reader/netlist_reader.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/pad_custom_shape_functions.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/pcb_display_options.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/pcb_draw_panel_gal.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/netlist_reader/pcb_netlist.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/pcb_origin_transforms.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/pcb_painter.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/plugins/kicad/pcb_parser.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/pcb_plot_params.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/pcb_screen.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/pcb_view.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/pcbnew_settings.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/plugin.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/ratsnest/ratsnest_data.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/ratsnest/ratsnest_view_item.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/sel_layer.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/zone_settings.cpp

    ${CMAKE_SOURCE_DIR}/pcbnew/tools/pcb_grid_helper.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/tools/pcb_actions.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/tools/pcb_editor_conditions.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/tools/pcb_viewer_tools.cpp

    widgets/net_selector.cpp
)

# add -DPCBNEW to compilation of these PCBNEW sources
set_source_files_properties( ${PCB_COMMON_SRCS} PROPERTIES
    COMPILE_DEFINITIONS "PCBNEW"
    )

add_library( pcbcommon STATIC ${PCB_COMMON_SRCS} )

target_include_directories( pcbcommon PUBLIC
    $<TARGET_PROPERTY:delaunator,INTERFACE_INCLUDE_DIRECTORIES>
)

target_link_libraries( pcbcommon PUBLIC
    common
    delaunator
    kimath
    kiplatform
)

add_dependencies( pcbcommon delaunator )

# The lemon grammar for the numeric evaluator
generate_lemon_grammar(
    common
    libeval
    libeval/numeric_evaluator.cpp
    libeval/grammar.lemon
    )

# The lemon grammar for the expression compiler
generate_lemon_grammar(
    common
    libeval_compiler
    libeval_compiler/libeval_compiler.cpp
    libeval_compiler/grammar.lemon
    )


# auto-generate netlist_lexer.h and netlist_keywords.cpp
make_lexer(
    common
    netlist.keywords
    netlist_lexer.h
    netlist_keywords.cpp
    NL_T
    )

# auto-generate pcb_plot_params_lexer.h and pcb_plot_params_keywords.cpp
make_lexer(
    pcbcommon
    pcb_plot_params.keywords
    pcb_plot_params_lexer.h
    pcb_plot_params_keywords.cpp
    PCBPLOTPARAMS_T
    )

# auto-generate drc_rules_lexer.h and drc_rules_keywords.cpp
make_lexer(
    common
    drc_rules.keywords
    drc_rules_lexer.h
    drc_rules_keywords.cpp
    DRCRULE_T
    )


# auto-generate pcbnew_sexpr.h and pcbnew_sexpr.cpp
make_lexer(
    pcbcommon
    pcb.keywords
    pcb_lexer.h
    pcb_keywords.cpp
    PCB_KEYS_T
    )

# auto-generate s-expression library table code.
make_lexer(
    common
    lib_table.keywords
    lib_table_lexer.h
    lib_table_keywords.cpp
    LIB_TABLE_T
    )

# auto-generate s-expression template fieldnames lexer and keywords.
make_lexer(
    common
    template_fieldnames.keywords
    template_fieldnames_lexer.h
    template_fieldnames_keywords.cpp
    TFIELD_T
    )

# auto-generate page layout reader s-expression page_layout_reader_lexer.h
# and title_block_reader_keywords.cpp.
make_lexer(
    common
    drawing_sheet/drawing_sheet.keywords
    drawing_sheet/drawing_sheet_lexer.h
    drawing_sheet/drawing_sheet_keywords.cpp
    DRAWINGSHEET_T
    )

# This one gets made only when testing.
# to build it, first enable #define STAND_ALONE at top of dsnlexer.cpp
add_executable( dsntest EXCLUDE_FROM_ALL dsnlexer.cpp )
target_link_libraries( dsntest common ${wxWidgets_LIBRARIES} rt )

