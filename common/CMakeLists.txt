include_directories( BEFORE ${INC_BEFORE} )
include_directories(
    ./dialogs
    ./dialog_about
    ${CAIRO_INCLUDE_DIR}
    ${GLEW_INCLUDE_DIR}
    ../3d-viewer
    ../pcbnew
    ../polygon
    ${INC_AFTER}
    )

# Generate header files containing shader programs
# Order of input files is significant
add_custom_command(
    OUTPUT gal/opengl/shader_src.h
    COMMAND ${CMAKE_COMMAND}
        -DinputFiles="${PROJECT_SOURCE_DIR}/common/gal/opengl/shader.vert\\;${PROJECT_SOURCE_DIR}/common/gal/opengl/shader.frag"
        -DoutputFile="shader_src.h"
        -P ${CMAKE_MODULE_PATH}/Shaders.cmake
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/common/gal/opengl
    COMMENT "Generating headers containing GLSL source code"
    )

add_custom_target(
    shader_headers ALL
    DEPENDS gal/opengl/shader_src.h
    )

set( GAL_SRCS
    # Common part
    draw_panel_gal.cpp
    painter.cpp
    worksheet_viewitem.cpp
    gal/graphics_abstraction_layer.cpp
    gal/stroke_font.cpp
    gal/color4d.cpp
    view/wx_view_controls.cpp
    geometry/hetriang.cpp

    # OpenGL GAL
    gal/opengl/opengl_gal.cpp
    gal/opengl/shader.cpp
    gal/opengl/vertex_item.cpp
    gal/opengl/vertex_container.cpp
    gal/opengl/cached_container.cpp
    gal/opengl/noncached_container.cpp
    gal/opengl/vertex_manager.cpp
    gal/opengl/gpu_manager.cpp
    gal/opengl/opengl_compositor.cpp

    # Cairo GAL
    gal/cairo/cairo_gal.cpp
    gal/cairo/cairo_compositor.cpp
    )

add_library( gal STATIC ${GAL_SRCS} )
add_dependencies( gal shader_headers )

add_dependencies( gal lib-dependencies )
add_dependencies( shader_headers lib-dependencies )


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


set( COMMON_ABOUT_DLG_SRCS
    dialog_about/AboutDialog_main.cpp
    dialog_about/dialog_about.cpp
    dialog_about/dialog_about_base.cpp
    dialogs/dialog_display_info_HTML_base.cpp
    dialogs/dialog_exit_base.cpp
    dialogs/dialog_image_editor.cpp
    dialogs/dialog_image_editor_base.cpp
    dialogs/dialog_get_component.cpp
    dialogs/dialog_get_component_base.cpp
    dialogs/dialog_hotkeys_editor.cpp
    dialogs/dialog_hotkeys_editor_base.cpp
    dialogs/dialog_list_selector_base.cpp
    dialogs/dialog_page_settings_base.cpp
    )

set( COMMON_PAGE_LAYOUT_SRCS
    page_layout/title_block_shapes.cpp
    page_layout/class_worksheet_dataitem.cpp
    page_layout/class_worksheet_layout.cpp
    page_layout/page_layout_default_description.cpp
    page_layout/page_layout_graphic_items.cpp
    page_layout/page_layout_reader_keywords.cpp
    page_layout/page_layout_reader.cpp
    )

set( COMMON_SRCS
    ${LIB_KICAD_SRCS}
    ${COMMON_ABOUT_DLG_SRCS}
    ${COMMON_PAGE_LAYOUT_SRCS}
    base_struct.cpp
    basicframe.cpp
    bezier_curves.cpp
    bin_mod.cpp
    bitmap.cpp
    block_commande.cpp
    build_version.cpp
    class_bitmap_base.cpp
    class_colors_design_settings.cpp
    class_layer_box_selector.cpp
    class_marker_base.cpp
    class_plotter.cpp
    class_undoredo_container.cpp
    colors.cpp
    common.cpp
    common_plot_functions.cpp
    common_plotHPGL_functions.cpp
    common_plotPS_functions.cpp
    common_plotPDF_functions.cpp
    common_plotGERBER_functions.cpp
    common_plotDXF_functions.cpp
    common_plotSVG_functions.cpp
    config_params.cpp
    confirm.cpp
    copy_to_clipboard.cpp
    convert_basic_shapes_to_polygon.cpp
    dialog_shim.cpp
    displlst.cpp
    draw_frame.cpp
    draw_panel.cpp
    drawtxt.cpp
    dsnlexer.cpp
    eda_dde.cpp
    eda_doc.cpp
    filter_reader.cpp
#    findkicadhelppath.cpp.notused      deprecated, use searchhelpfilefullpath.cpp
    gestfich.cpp
    getrunningmicrosecs.cpp
    grid_tricks.cpp
    gr_basic.cpp
    hotkeys_basic.cpp
    html_messagebox.cpp
    kiface_i.cpp
    kiway.cpp
    kiway_express.cpp
    kiway_holder.cpp
    kiway_player.cpp
    lockfile.cpp
    msgpanel.cpp
    netlist_keywords.cpp
    newstroke_font.cpp
    prependpath.cpp
    project.cpp
    ptree.cpp
    reporter.cpp
    richio.cpp
    searchhelpfilefullpath.cpp
    search_stack.cpp
    selcolor.cpp
    systemdirsappend.cpp
    trigo.cpp
    utf8.cpp
    validators.cpp
    wildcards_and_files_ext.cpp
    worksheet.cpp
    wxwineda.cpp
    wx_unit_binder.cpp
    wx_status_popup.cpp
    xnode.cpp
    zoom.cpp
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


set( COMMON_SRCS
    ${COMMON_SRCS}
    view/view.cpp
    view/view_item.cpp
    view/view_group.cpp

    math/math_util.cpp

    tool/tool_action.cpp
    tool/tool_base.cpp
    tool/tool_manager.cpp
    tool/tool_dispatcher.cpp
    tool/tool_event.cpp
    tool/tool_interactive.cpp
    tool/action_manager.cpp
    tool/context_menu.cpp

    geometry/seg.cpp
    geometry/shape_line_chain.cpp
    geometry/shape_collisions.cpp
    )
add_library( common STATIC ${COMMON_SRCS} )
add_dependencies( common lib-dependencies )


set( PCB_COMMON_SRCS
    base_screen.cpp
    eda_text.cpp
    class_page_info.cpp
    pcbcommon.cpp
    lset.cpp
    footprint_info.cpp
    ../pcbnew/basepcbframe.cpp
    ../pcbnew/class_board.cpp
    ../pcbnew/class_board_connected_item.cpp
    ../pcbnew/class_board_design_settings.cpp
    ../pcbnew/class_board_item.cpp
    ../pcbnew/class_dimension.cpp
    ../pcbnew/class_drawsegment.cpp
    ../pcbnew/class_drc_item.cpp
    ../pcbnew/class_edge_mod.cpp
    ../pcbnew/class_netclass.cpp
    ../pcbnew/class_netinfo_item.cpp
    ../pcbnew/class_netinfolist.cpp
    ../pcbnew/class_marker_pcb.cpp
    ../pcbnew/class_mire.cpp
    ../pcbnew/class_module.cpp
    ../pcbnew/class_pad.cpp
    ../pcbnew/class_pad_draw_functions.cpp
    ../pcbnew/class_pcb_text.cpp
    ../pcbnew/class_text_mod.cpp
    ../pcbnew/class_track.cpp
    ../pcbnew/class_zone.cpp
    ../pcbnew/class_zone_settings.cpp
    ../pcbnew/classpcb.cpp
    ../pcbnew/ratsnest_data.cpp
    ../pcbnew/ratsnest_viewitem.cpp
    ../pcbnew/collectors.cpp
    ../pcbnew/netlist_reader.cpp
    ../pcbnew/legacy_netlist_reader.cpp
    ../pcbnew/kicad_netlist_reader.cpp
    ../pcbnew/sel_layer.cpp
    ../pcbnew/pcb_plot_params.cpp
    ../pcbnew/io_mgr.cpp
    ../pcbnew/plugin.cpp
    ../pcbnew/eagle_plugin.cpp
    ../pcbnew/legacy_plugin.cpp
    ../pcbnew/kicad_plugin.cpp
    ../pcbnew/gpcb_plugin.cpp
    ../pcbnew/pcb_netlist.cpp
    ../pcbnew/specctra.cpp
    ../pcbnew/specctra_export.cpp
    ../pcbnew/specctra_keywords.cpp
    pcb_plot_params_keywords.cpp
    pcb_keywords.cpp
    ../pcbnew/pcb_parser.cpp
    fp_lib_table_keywords.cpp
    fpid.cpp
    fp_lib_table.cpp
)

set( PCB_COMMON_SRCS
    ${PCB_COMMON_SRCS}
    ../pcbnew/pcb_painter.cpp
    )

# add -DPCBNEW to compilation of these PCBNEW sources
set_source_files_properties( ${PCB_COMMON_SRCS} PROPERTIES
    COMPILE_DEFINITIONS "PCBNEW"
    )

add_library( pcbcommon STATIC ${PCB_COMMON_SRCS} )
add_dependencies( pcbcommon lib-dependencies )

# auto-generate specctra_lexer.h and specctra_keywords.cpp
make_lexer(
    ${PROJECT_SOURCE_DIR}/pcbnew/specctra.keywords
    ${PROJECT_SOURCE_DIR}/pcbnew/specctra_lexer.h
    ${PROJECT_SOURCE_DIR}/pcbnew/specctra_keywords.cpp
    DSN

    # Pass header file with dependency on *_lexer.h as extra_arg
    specctra.h
    )

add_custom_target(
    specctra_lexer_source_files ALL
    DEPENDS
        ${PROJECT_SOURCE_DIR}/pcbnew/specctra_lexer.h
        ${PROJECT_SOURCE_DIR}/pcbnew/specctra_keywords.cpp
    )

add_dependencies( pcbcommon specctra_lexer_source_files )

# auto-generate netlist_lexer.h and netlist_keywords.cpp
make_lexer(
    ${CMAKE_CURRENT_SOURCE_DIR}/netlist.keywords
    ${PROJECT_SOURCE_DIR}/include/netlist_lexer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/netlist_keywords.cpp
    NL_T

    # Pass header file with dependency on *_lexer.h as extra_arg
    ${CMAKE_PROJECT_SOURCE_DIR}/pcbnew/netlist_reader.h
    )

add_custom_target(
    netlist_lexer_source_files ALL
    DEPENDS
        ${PROJECT_SOURCE_DIR}/include/netlist_lexer.h
        ${CMAKE_CURRENT_SOURCE_DIR}/netlist_keywords.cpp
    )

add_dependencies( common netlist_lexer_source_files )

# auto-generate pcb_plot_params_lexer.h and pcb_plot_params_keywords.cpp
make_lexer(
    ${CMAKE_CURRENT_SOURCE_DIR}/pcb_plot_params.keywords
    ${PROJECT_SOURCE_DIR}/include/pcb_plot_params_lexer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/pcb_plot_params_keywords.cpp
    PCBPLOTPARAMS_T

    # Pass header file with dependencies on *_lexer.h as extra_arg
    ${PROJECT_SOURCE_DIR}/pcbnew/pcb_plot_params.h
    )

add_custom_target(
    pcb_plot_lexer_source_files ALL
    DEPENDS
        ${PROJECT_SOURCE_DIR}/include/pcb_plot_params_lexer.h
        ${CMAKE_CURRENT_SOURCE_DIR}/pcb_plot_params_keywords.cpp
    )

add_dependencies( pcbcommon pcb_plot_lexer_source_files )

# auto-generate pcbnew_sexpr.h and pcbnew_sexpr.cpp
make_lexer(
    ${CMAKE_CURRENT_SOURCE_DIR}/pcb.keywords
    ${PROJECT_SOURCE_DIR}/include/pcb_lexer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/pcb_keywords.cpp
    PCB_KEYS_T

    # Pass header file with dependency on *_lexer.h as extra_arg
    ${PROJECT_SOURCE_DIR}/pcbnew/pcb_parser.h
    )

add_custom_target(
    pcb_lexer_source_files ALL
    DEPENDS
        ${PROJECT_SOURCE_DIR}/include/pcb_lexer.h
        ${CMAKE_CURRENT_SOURCE_DIR}/pcb_keywords.cpp
    )

add_dependencies( pcbcommon pcb_lexer_source_files )

# auto-generate pcbnew s-expression footprint library table code.
make_lexer(
    ${CMAKE_CURRENT_SOURCE_DIR}/fp_lib_table.keywords
    ${PROJECT_SOURCE_DIR}/include/fp_lib_table_lexer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/fp_lib_table_keywords.cpp
    FP_LIB_TABLE_T
    )

add_custom_target(
    fp_lib_table_lexer_source_files ALL
    DEPENDS
        ${PROJECT_SOURCE_DIR}/include/fp_lib_table_lexer.h
        ${CMAKE_CURRENT_SOURCE_DIR}/fp_lib_table_keywords.cpp
    )

add_dependencies( pcbcommon fp_lib_table_lexer_source_files )

# auto-generate page layout reader s-expression page_layout_reader_lexer.h
# and title_block_reader_keywords.cpp.
make_lexer(
    ${CMAKE_CURRENT_SOURCE_DIR}/page_layout/page_layout_reader.keywords
    ${PROJECT_SOURCE_DIR}/include/page_layout_reader_lexer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/page_layout/page_layout_reader_keywords.cpp
    TB_READER_T
    )

add_custom_target(
    page_layout_lexer_source_files ALL
    DEPENDS
        ${PROJECT_SOURCE_DIR}/include/page_layout_reader_lexer.h
        ${CMAKE_CURRENT_SOURCE_DIR}/page_layout/page_layout_reader_keywords.cpp
    )

add_dependencies( common page_layout_lexer_source_files )

# This one gets made only when testing.
# to build it, first enable #define STAND_ALONE at top of dsnlexer.cpp
add_executable( dsntest EXCLUDE_FROM_ALL dsnlexer.cpp )
target_link_libraries( dsntest common ${wxWidgets_LIBRARIES} rt )

add_dependencies( dsntest lib-dependencies )

