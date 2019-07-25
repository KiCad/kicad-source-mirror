# .cpp files are compiled with extra ${WSHADOW_FLAGS}
if( COMPILER_SUPPORTS_WSHADOW )
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WSHADOW_FLAGS}")
endif()

add_subdirectory( libeval )

include_directories( BEFORE ${INC_BEFORE} )
include_directories(
    ./dialogs
    ./widgets
    ./dialog_about
    ${CAIRO_INCLUDE_DIR}
    ${PIXMAN_INCLUDE_DIR}
    ${GLEW_INCLUDE_DIR}
    ${GLM_INCLUDE_DIR}
    ${CURL_INCLUDE_DIRS}
    ../3d-viewer
    ../pcbnew
    ${INC_AFTER}
    ${OCE_INCLUDE_DIRS}
    ${OCC_INCLUDE_DIR}
    ${OPENSSL_INCLUDE_DIR}
    )


if( NOT APPLE )     # windows and linux use openssl under curl
    find_package( OpenSSL REQUIRED )
endif()

set( GAL_SRCS
    # Common part
    basic_gal.cpp
    draw_panel_gal.cpp
    gl_context_mgr.cpp
    newstroke_font.cpp
    painter.cpp
    text_utils.cpp
    gal/color4d.cpp
    gal/gal_display_options.cpp
    gal/graphics_abstraction_layer.cpp
    gal/hidpi_gl_canvas.cpp
    gal/stroke_font.cpp
    geometry/hetriang.cpp
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
    polygon
    bitmaps
    ${GLEW_LIBRARIES}
    ${CAIRO_LIBRARIES}
    ${PIXMAN_LIBRARIES}
    ${OPENGL_LIBRARIES}
    ${GDI_PLUS_LIBRARIES}
)


# Only for win32 cross compilation using MXE
if( WIN32 AND MSYS )
    add_definitions( -DGLEW_STATIC )
endif()


# A shared library subsetted from common which restricts what can go into
# a single_top link image.  By not linking to common, we control what does
# statically go into single_top link images.  My current thinking is that only
# wxWidgets should be a shared link from single top, everything else should be
# statically bound into it. Otherwise you will have DSO loading problems. After it
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
    colors.cpp
    dlist.cpp
    string.cpp
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


# KiCad build version string defaults to "no-vcs-found" which forces the build version header
# command to look for git to create the version string header when the .git path is found in
# the source path.
set( KICAD_BRANCH_NAME "" CACHE STRING "KiCad repository name." )
set( KICAD_VERSION_EXTRA "" CACHE STRING
    "User defined configuration string to append to KiCad version." )

# Generate version header file.
add_custom_target(
    version_header ALL
    COMMAND ${CMAKE_COMMAND}
        -DKICAD_VERSION=${KICAD_VERSION}
        -DKICAD_BRANCH_NAME=${KICAD_BRANCH_NAME}
        -DKICAD_VERSION_EXTRA=${KICAD_VERSION_EXTRA}
        -DOUTPUT_FILE=${CMAKE_BINARY_DIR}/kicad_build_version.h
        -DSRC_PATH=${PROJECT_SOURCE_DIR}
        -DCMAKE_MODULE_PATH=${CMAKE_MODULE_PATH}
        -P ${CMAKE_MODULE_PATH}/WriteVersionHeader.cmake
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
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
    dialogs/dialog_display_info_HTML_base.cpp
    dialogs/dialog_edit_library_tables.cpp
    dialogs/dialog_file_dir_picker.cpp
    dialogs/dialog_global_lib_table_config.cpp
    dialogs/dialog_global_lib_table_config_base.cpp
    dialogs/dialog_hotkey_list.cpp
    dialogs/dialog_image_editor.cpp
    dialogs/dialog_image_editor_base.cpp
    dialogs/dialog_list_selector_base.cpp
    dialogs/dialog_page_settings_base.cpp
    dialogs/dialog_text_entry_base.cpp
    dialogs/dialog_print_generic.cpp
    dialogs/dialog_print_generic_base.cpp
    dialogs/dialog_text_entry.cpp
    dialogs/panel_common_settings.cpp
    dialogs/panel_common_settings_base.cpp
    dialogs/panel_hotkeys_editor.cpp
    dialogs/wx_html_report_panel.cpp
    dialogs/wx_html_report_panel_base.cpp
    )

set( COMMON_WIDGET_SRCS
    widgets/button_row_panel.cpp
    widgets/color_swatch.cpp
    widgets/footprint_choice.cpp
    widgets/footprint_preview_widget.cpp
    widgets/footprint_select_widget.cpp
    widgets/gal_options_panel.cpp
    widgets/grid_combobox.cpp
    widgets/grid_icon_text_helpers.cpp
    widgets/grid_text_button_helpers.cpp
    widgets/indicator_icon.cpp
    widgets/layer_box_selector.cpp
    widgets/lib_tree.cpp
    widgets/mathplot.cpp
    widgets/paged_dialog.cpp
    widgets/progress_reporter.cpp
    widgets/stepped_slider.cpp
    widgets/tab_traversal.cpp
    widgets/text_ctrl_eval.cpp
    widgets/two_column_tree_list.cpp
    widgets/ui_common.cpp
    widgets/unit_binder.cpp
    widgets/widget_save_restore.cpp
    widgets/widget_hotkey_list.cpp
    widgets/wx_busy_indicator.cpp
    widgets/wx_grid.cpp
    )

set( COMMON_PAGE_LAYOUT_SRCS
    page_layout/ws_data_item.cpp
    page_layout/ws_data_model.cpp
    page_layout/ws_data_model_io.cpp
    page_layout/page_layout_default_description.cpp
    page_layout/ws_draw_item.cpp
    page_layout/ws_proxy_undo_item.cpp
    page_layout/ws_proxy_view_item.cpp
    page_layout/page_layout_reader.cpp
    )

set( COMMON_PREVIEW_ITEMS_SRCS
    preview_items/arc_assistant.cpp
    preview_items/arc_geom_manager.cpp
    preview_items/bright_box.cpp
    preview_items/centreline_rect_item.cpp
    preview_items/draw_context.cpp
    preview_items/polygon_geom_manager.cpp
    preview_items/polygon_item.cpp
    preview_items/preview_utils.cpp
    preview_items/ruler_item.cpp
    preview_items/selection_area.cpp
    preview_items/simple_overlay_item.cpp
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

set( COMMON_SRCS
    ${LIB_KICAD_SRCS}
    ${COMMON_ABOUT_DLG_SRCS}
    ${COMMON_DLG_SRCS}
    ${COMMON_WIDGET_SRCS}
    ${COMMON_PAGE_LAYOUT_SRCS}
    ${COMMON_PREVIEW_ITEMS_SRCS}
    ${PLOTTERS_CONTROL_SRCS}
    advanced_config.cpp
    array_axis.cpp
    array_options.cpp
    base_struct.cpp
    bezier_curves.cpp
    bin_mod.cpp
    bitmap.cpp
    bitmap_base.cpp
    board_printout.cpp
    build_version.cpp
    colors_design_settings.cpp
    colors.cpp
    commit.cpp
    common.cpp
    config_params.cpp
    confirm.cpp
    convert_basic_shapes_to_polygon.cpp
    cursor_store.cpp
    dialog_shim.cpp
    displlst.cpp
    dpi_scaling.cpp
    gr_text.cpp
    dsnlexer.cpp
    eagle_parser.cpp
    eda_base_frame.cpp
    eda_dde.cpp
    eda_doc.cpp
    eda_dockart.cpp
    eda_draw_frame.cpp
    eda_pattern_match.cpp
    eda_size_ctrl.cpp
    env_paths.cpp
    env_vars.cpp
    exceptions.cpp
    executable_names.cpp
    filename_resolver.cpp
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
    html_messagebox.cpp
    kiface_i.cpp
    kiway.cpp
    kiway_express.cpp
    kiway_holder.cpp
    kiway_player.cpp
    languages_menu.cpp
    lib_id.cpp
    lib_table_base.cpp
    lib_tree_model.cpp
    lib_tree_model_adapter.cpp
    lockfile.cpp
    marker_base.cpp
    md5_hash.cpp
    msgpanel.cpp
    observable.cpp
    prependpath.cpp
    printout.cpp
    project.cpp
    properties.cpp
    ptree.cpp
    refdes_utils.cpp
    reporter.cpp
    richio.cpp
    search_stack.cpp
    searchhelpfilefullpath.cpp
    settings.cpp
    status_popup.cpp
    systemdirsappend.cpp
    trace_helpers.cpp
    undo_redo_container.cpp
    utf8.cpp
    validators.cpp
    wildcards_and_files_ext.cpp
    page_layout/ws_painter.cpp
    wxdataviewctrl_helpers.cpp
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

if( BUILD_GITHUB_PLUGIN )
    list( APPEND COMMON_SRCS
        kicad_curl/kicad_curl.cpp
        kicad_curl/kicad_curl_easy.cpp
    )
endif()

# Link-time optimization (LTO) on GCC conflicts with embedded assembly (__asm),
# following GCC's recommendation to disable LTO per translation unit.
if( "${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set_source_files_properties( system/libcontext.cpp PROPERTIES
        COMPILE_FLAGS "-fno-lto"
        )
endif()

set( COMMON_SRCS
    ${COMMON_SRCS}
    system/libcontext.cpp

    view/view.cpp
    view/view_item.cpp
    view/view_group.cpp

    math/math_util.cpp

    tool/action_manager.cpp
    tool/action_menu.cpp
    tool/action_toolbar.cpp
    tool/actions.cpp
    tool/common_control.cpp
    tool/common_tools.cpp
    tool/conditional_menu.cpp
    tool/edit_constraints.cpp
    tool/edit_points.cpp
    tool/grid_menu.cpp
    tool/picker_tool.cpp
    tool/selection_conditions.cpp
    tool/tool_action.cpp
    tool/tool_base.cpp
    tool/tool_dispatcher.cpp
    tool/tool_event.cpp
    tool/tool_interactive.cpp
    tool/tool_manager.cpp
    tool/tool_menu.cpp
    tool/zoom_menu.cpp
    tool/zoom_tool.cpp

    geometry/convex_hull.cpp
    geometry/geometry_utils.cpp
    geometry/seg.cpp
    geometry/shape.cpp
    geometry/shape_collisions.cpp
    geometry/shape_arc.cpp
    geometry/shape_file_io.cpp
    geometry/shape_line_chain.cpp
    geometry/shape_poly_set.cpp
    geometry/trigo.cpp

    libeval/numeric_evaluator.cpp
    )
add_library( common STATIC ${COMMON_SRCS} )
add_dependencies( common version_header )
target_link_libraries( common
    bitmaps
    polygon
    gal
    ${Boost_LIBRARIES}
    ${CURL_LIBRARIES}
    ${OPENSSL_LIBRARIES}        # empty on Apple
    ${wxWidgets_LIBRARIES}
    )


set( PCB_COMMON_SRCS
    base_screen.cpp
    eda_text.cpp
    fp_lib_table.cpp
    hash_eda.cpp
    lset.cpp
    origin_viewitem.cpp
    page_info.cpp
    ../pcbnew/pcb_base_frame.cpp
    ../pcbnew/board_commit.cpp
    ../pcbnew/board_connected_item.cpp
    ../pcbnew/board_design_settings.cpp
    ../pcbnew/board_items_to_polygon_shape_transform.cpp
    ../pcbnew/class_board.cpp
    ../pcbnew/class_board_item.cpp
    ../pcbnew/class_dimension.cpp
    ../pcbnew/class_drawsegment.cpp
    ../pcbnew/class_edge_mod.cpp
    ../pcbnew/class_marker_pcb.cpp
    ../pcbnew/class_module.cpp
    ../pcbnew/netclass.cpp
    ../pcbnew/netinfo_item.cpp
    ../pcbnew/netinfo_list.cpp
    ../pcbnew/class_pad.cpp
    ../pcbnew/class_pcb_target.cpp
    ../pcbnew/class_pcb_text.cpp
    ../pcbnew/board_stackup_manager/class_board_stackup.cpp
    ../pcbnew/class_text_mod.cpp
    ../pcbnew/class_track.cpp
    ../pcbnew/class_zone.cpp
    ../pcbnew/collectors.cpp
    ../pcbnew/connectivity/connectivity_algo.cpp
    ../pcbnew/connectivity/connectivity_items.cpp
    ../pcbnew/connectivity/connectivity_data.cpp
    ../pcbnew/convert_drawsegment_list_to_polygon.cpp
    ../pcbnew/drc_item.cpp
    ../pcbnew/eagle_plugin.cpp
    ../pcbnew/gpcb_plugin.cpp
    ../pcbnew/io_mgr.cpp
    ../pcbnew/kicad_clipboard.cpp
    ../pcbnew/kicad_netlist_reader.cpp
    ../pcbnew/kicad_plugin.cpp
    ../pcbnew/legacy_netlist_reader.cpp
    ../pcbnew/legacy_plugin.cpp
    ../pcbnew/netlist_reader.cpp
    ../pcbnew/pad_custom_shape_functions.cpp
    ../pcbnew/pad_print_functions.cpp
    ../pcbnew/pcb_display_options.cpp
    ../pcbnew/pcb_draw_panel_gal.cpp
    ../pcbnew/pcb_general_settings.cpp
    ../pcbnew/pcb_netlist.cpp
    ../pcbnew/pcb_painter.cpp
    ../pcbnew/pcb_parser.cpp
    ../pcbnew/pcb_plot_params.cpp
    ../pcbnew/pcb_screen.cpp
    ../pcbnew/pcb_view.cpp
    ../pcbnew/plugin.cpp
    ../pcbnew/ratsnest_data.cpp
    ../pcbnew/ratsnest_viewitem.cpp
    ../pcbnew/sel_layer.cpp
    ../pcbnew/zone_settings.cpp
    widgets/net_selector.cpp
)

# add -DPCBNEW to compilation of these PCBNEW sources
set_source_files_properties( ${PCB_COMMON_SRCS} PROPERTIES
    COMPILE_DEFINITIONS "PCBNEW"
    )

add_library( pcbcommon STATIC ${PCB_COMMON_SRCS} )

target_include_directories( pcbcommon PUBLIC
)

target_link_libraries( pcbcommon PUBLIC
    common
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

# auto-generate page layout reader s-expression page_layout_reader_lexer.h
# and title_block_reader_keywords.cpp.
make_lexer(
    common
    page_layout/page_layout_reader.keywords
    page_layout/page_layout_reader_lexer.h
    page_layout/page_layout_reader_keywords.cpp
    TB_READER_T
    )

# This one gets made only when testing.
# to build it, first enable #define STAND_ALONE at top of dsnlexer.cpp
add_executable( dsntest EXCLUDE_FROM_ALL dsnlexer.cpp )
target_link_libraries( dsntest common ${wxWidgets_LIBRARIES} rt )

target_link_libraries( pcbcommon PUBLIC bitmaps )


# _kiway.so
if( false ) # future
#if( KICAD_SCRIPTING OR KICAD_SCRIPTING_MODULES )

    set( SWIG_FLAGS
        -I${CMAKE_CURRENT_SOURCE_DIR}/../include
        )

    if( DEBUG )
        set( SWIG_FLAGS ${SWIG_FLAGS} -DDEBUG )
    endif()

    # call SWIG in C++ mode: https://cmake.org/cmake/help/v3.2/module/UseSWIG.html
    set_source_files_properties( swig/kiway.i PROPERTIES CPLUSPLUS ON )

    # collect CFLAGS , and pass them to swig later
    get_directory_property( DirDefs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} COMPILE_DEFINITIONS )
    foreach( d ${DirDefs} )
        set( SWIG_FLAGS ${SWIG_FLAGS} -D${d} )
    endforeach()

    set( CMAKE_SWIG_FLAGS ${SWIG_FLAGS} )

    include_directories( BEFORE ${INC_BEFORE} )
    include_directories(
        ../common
        ${INC_AFTER}
        )

    set( SWIG_MODULE_kiway_EXTRA_DEPS
        ../common/swig/ki_exception.i
        ../common/swig/kicad.i
        )

    swig_add_module( kiway python
        swig/kiway.i
        )

    swig_link_libraries( kiway
        common
        ${wxWidgets_LIBRARIES}
        ${PYTHON_LIBRARIES}
        )

    set_source_files_properties( ${swig_generated_file_fullname} PROPERTIES
        # See section 16.3 "The SWIG runtime code"
        # http://www.swig.org/Doc3.0/SWIGDocumentation.html#Modules_nn2
        COMPILE_FLAGS "-DSWIG_TYPE_TABLE=WXPYTHON_TYPE_TABLE -Wno-delete-non-virtual-dtor"
        )

    if( MAKE_LINK_MAPS )
        set_target_properties( _kiway PROPERTIES
            LINK_FLAGS "-Wl,-cref,-Map=_kiway.so.map"
            )
    endif()

endif()
