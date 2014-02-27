
set( MAKE_LINK_MAPS false )

add_definitions( -DPCBNEW )
add_subdirectory(router)

# psnrouter depends on make_lexer outputs in common (bug # 1285878 )
add_dependencies( pnsrouter pcbcommon )


if( KICAD_SCRIPTING OR KICAD_SCRIPTING_MODULES )
    file( MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/scripting )
    find_package( SWIG REQUIRED )
    include( ${SWIG_USE_FILE} )

    # Infrequently needed headers go at end of search paths, append to INC_AFTER
    set( INC_AFTER ${INC_AFTER} ${CMAKE_CURRENT_SOURCE_DIR}/scripting )

    #message( STATUS "pcbnew INC_AFTER:${INC_AFTER}" )
endif()


###
# Includes
###
include_directories( BEFORE ${INC_BEFORE} )
include_directories(
    ./dialogs
    ./autorouter
    ../3d-viewer
    ../common
    ../polygon
    ../common/dialogs
    ./exporters
    ../lib_dxf
    ./import_dxf
    ${INC_AFTER}
    )

###
# Sources
#
set( PCBNEW_DIALOGS
    dialogs/dialog_block_options_base.cpp
    dialogs/dialog_cleaning_options_base.cpp
    dialogs/dialog_cleaning_options.cpp
    dialogs/dialog_copper_zones.cpp
    dialogs/dialog_copper_zones_base.cpp
    dialogs/dialog_design_rules.cpp
    dialogs/dialog_design_rules_base.cpp
    dialogs/dialog_dimension_editor_base.cpp
    dialogs/dialog_display_options.cpp
    dialogs/dialog_display_options_base.cpp
    dialogs/dialog_drc_base.cpp
    dialogs/dialog_drc.cpp
    dialogs/dialog_edit_module_for_BoardEditor.cpp
    dialogs/dialog_edit_module_for_BoardEditor_base.cpp
    dialogs/dialog_edit_module_for_Modedit_base.cpp
    dialogs/dialog_edit_module_for_Modedit.cpp
    dialogs/dialog_edit_module_text.cpp
    dialogs/dialog_edit_module_text_base.cpp
    dialogs/dialog_exchange_modules_base.cpp
    dialogs/dialog_export_idf.cpp
    dialogs/dialog_export_idf_base.cpp
    dialogs/dialog_export_vrml_base.cpp
    dialogs/dialog_export_vrml.cpp
    dialogs/dialog_find_base.cpp
    dialogs/dialog_find.cpp
    dialogs/dialog_fp_lib_table_base.cpp
    dialogs/dialog_fp_lib_table.cpp
    dialogs/dialog_fp_plugin_options_base.cpp
    dialogs/dialog_fp_plugin_options.cpp
    dialogs/dialog_freeroute_exchange.cpp
    dialogs/dialog_freeroute_exchange_base.cpp
    dialogs/dialog_gendrill.cpp
    dialogs/dialog_gendrill_base.cpp
    dialogs/dialog_gen_module_position_file_base.cpp
    dialogs/dialog_general_options.cpp
    dialogs/dialog_general_options_BoardEditor_base.cpp
    dialogs/dialog_global_edit_tracks_and_vias.cpp
    dialogs/dialog_global_edit_tracks_and_vias_base.cpp
    dialogs/dialog_global_modules_fields_edition.cpp
    dialogs/dialog_global_modules_fields_edition_base.cpp
    dialogs/dialog_global_pads_edition_base.cpp
    dialogs/dialog_graphic_items_options.cpp
    dialogs/dialog_graphic_items_options_base.cpp
    dialogs/dialog_graphic_item_properties.cpp
    dialogs/dialog_graphic_item_properties_base.cpp
    dialogs/dialog_graphic_item_properties_for_Modedit.cpp
    dialogs/dialog_global_deletion.cpp
    dialogs/dialog_global_deletion_base.cpp
    dialogs/dialog_keepout_area_properties.cpp
    dialogs/dialog_keepout_area_properties_base.cpp
    dialogs/dialog_layer_selection_base.cpp
    dialogs/dialog_layers_setup.cpp
    dialogs/dialog_layers_setup_base.cpp
    dialogs/dialog_netlist.cpp
    dialogs/dialog_netlist_fbp.cpp
    dialogs/dialog_pcb_text_properties.cpp
    dialogs/dialog_pcb_text_properties_base.cpp
    dialogs/dialog_non_copper_zones_properties_base.cpp
    dialogs/dialog_pad_properties.cpp
    dialogs/dialog_pad_properties_base.cpp
    dialogs/dialog_plot_base.cpp
    dialogs/dialog_plot.cpp
    dialogs/dialog_print_for_modedit.cpp
    dialogs/dialog_print_for_modedit_base.cpp
    dialogs/dialog_print_using_printer.cpp
    dialogs/dialog_print_using_printer_base.cpp
    dialogs/dialog_orient_footprints.cpp
    dialogs/dialog_orient_footprints_base.cpp
    dialogs/dialog_mask_clearance.cpp
    dialogs/dialog_mask_clearance_base.cpp
    dialogs/dialog_SVG_print.cpp
    dialogs/dialog_SVG_print_base.cpp
    dialogs/dialog_set_grid.cpp
    dialogs/dialog_set_grid_base.cpp
    dialogs/dialog_target_properties_base.cpp
    footprint_wizard.cpp
    footprint_wizard_frame.cpp
    dialogs/dialog_footprint_wizard_list_base.cpp
    dialogs/dialog_footprint_wizard_list.cpp
    )

set( PCBNEW_IMPORT_DXF
    import_dxf/dialog_dxf_import_base.cpp
    import_dxf/dialog_dxf_import.cpp
    import_dxf/dxf2brd_items.cpp
    )

set( PCBNEW_EXPORTERS
    exporters/export_d356.cpp
    exporters/export_gencad.cpp
    exporters/export_idf.cpp
    exporters/export_vrml.cpp
    exporters/idf_common.cpp
    exporters/idf.cpp
    exporters/gen_drill_report_files.cpp
    exporters/gen_modules_placefile.cpp
    exporters/gendrill_Excellon_writer.cpp
    exporters/vrml_board.cpp
    )

set( PCBNEW_AUTOROUTER_SRCS
    autorouter/rect_placement/rect_placement.cpp
    autorouter/move_and_route_event_functions.cpp
    autorouter/auto_place_footprints.cpp
    autorouter/autorout.cpp
    autorouter/routing_matrix.cpp
    autorouter/dist.cpp
    autorouter/queue.cpp
    autorouter/spread_footprints.cpp
    autorouter/solve.cpp
    autorouter/graphpcb.cpp
    autorouter/work.cpp
    )

set( PCBNEW_CLASS_SRCS
    tool_modview.cpp
    modview_frame.cpp
    pcbframe.cpp
    attribut.cpp
    board_items_to_polygon_shape_transform.cpp
    board_undo_redo.cpp
    block.cpp
    block_module_editor.cpp
    build_BOM_from_board.cpp
    class_pcb_layer_widget.cpp
    class_pcb_layer_box_selector.cpp
    clean.cpp
    connect.cpp
    controle.cpp
    dimension.cpp
    cross-probing.cpp
    deltrack.cpp
    ${PCBNEW_DIALOGS}
    ${PCBNEW_IMPORT_DXF}
    ${PCBNEW_EXPORTERS}
    dragsegm.cpp
    drc.cpp
    drc_clearance_test_functions.cpp
    drc_marker_functions.cpp
    edgemod.cpp
    edit.cpp
    editedge.cpp
    editmod.cpp
    editrack.cpp
    editrack-part2.cpp
    edit_pcb_text.cpp
    edit_track_width.cpp
    edtxtmod.cpp
    event_handlers_tracks_vias_sizes.cpp
    files.cpp
    globaleditpad.cpp
    highlight.cpp
    hotkeys.cpp
    hotkeys_board_editor.cpp
    hotkeys_module_editor.cpp
    initpcb.cpp
    layer_widget.cpp
    librairi.cpp
    loadcmp.cpp
    magnetic_tracks_functions.cpp
    menubar_modedit.cpp
    menubar_pcbframe.cpp
    minimun_spanning_tree.cpp
    modedit.cpp
    modedit_onclick.cpp
    modeditoptions.cpp
    modedit_undo_redo.cpp
    moduleframe.cpp
    modules.cpp
    move-drag_pads.cpp
    move_or_drag_track.cpp
    muonde.cpp
    muwave_command.cpp
    netlist.cpp
    onleftclick.cpp
    onrightclick.cpp
    pad_edition_functions.cpp
    pcbnew.cpp
    pcbnew_config.cpp
    pcbplot.cpp
    plot_board_layers.cpp
    plot_brditems_plotter.cpp
    print_board_functions.cpp
    printout_controler.cpp
    ratsnest.cpp
    ratsnest_data.cpp
    ratsnest_viewitem.cpp
#    specctra.cpp           #moved in pcbcommon lib
#    specctra_export.cpp
#    specctra_keywords.cpp
    specctra_import.cpp
    swap_layers.cpp
    target_edit.cpp
    tool_modedit.cpp
    tool_onrightclick.cpp
    tool_pcb.cpp
    toolbars_update_user_interface.cpp
    tracepcb.cpp
    tr_modif.cpp
    xchgmod.cpp
    zones_convert_brd_items_to_polygons_with_Boost.cpp
    zones_convert_to_polygons_aux_functions.cpp
    zones_by_polygon.cpp
    zones_by_polygon_fill_functions.cpp
    zone_filling_algorithm.cpp
    zones_functions_for_undo_redo.cpp
    zones_non_copper_type_functions.cpp
    zones_polygons_insulated_copper_islands.cpp
    zones_polygons_test_connections.cpp
    zones_test_and_combine_areas.cpp
    class_footprint_wizard.cpp

    tools/selection_tool.cpp
    tools/selection_area.cpp
    tools/bright_box.cpp
    tools/move_tool.cpp
    tools/pcb_tools.cpp
    tools/common_actions.cpp
    )

set( PCBNEW_SRCS ${PCBNEW_AUTOROUTER_SRCS} ${PCBNEW_CLASS_SRCS} ${PCBNEW_DIALOGS} )

###
# We need some extra sources from common
###
set( PCBNEW_COMMON_SRCS
    ../common/dialogs/dialog_page_settings.cpp
    ../common/base_units.cpp
    )

##
# Scripting sources
##
set( PCBNEW_SCRIPTING_DIALOGS
    dialogs/dialog_scripting_base.cpp
    dialogs/dialog_scripting.cpp
    )


set( PCBNEW_SCRIPTING_PYTHON_HELPERS
    ../scripting/wx_python_helpers.cpp
    ../scripting/python_scripting.cpp
    scripting/pcbnew_scripting_helpers.cpp
    scripting/pcbnew_footprint_wizards.cpp
    )

if( KICAD_SCRIPTING )
    set( PCBNEW_SCRIPTING_SRCS
        ${PCBNEW_SCRIPTING_DIALOGS}
        pcbnew_wrap.cxx
        ${PCBNEW_SCRIPTING_PYTHON_HELPERS}
    )
endif()

##
# Scripting build
##

if( KICAD_SCRIPTING OR KICAD_SCRIPTING_MODULES )

    set( SWIG_FLAGS
        -I${CMAKE_CURRENT_SOURCE_DIR}/../..
        -I${CMAKE_CURRENT_SOURCE_DIR}
        -I${CMAKE_CURRENT_SOURCE_DIR}/../include
        -I${CMAKE_CURRENT_SOURCE_DIR}/../scripting
        )

    if( DEBUG )
        set( SWIG_FLAGS ${SWIG_FLAGS} -DDEBUG )
    endif()

    # collect CFLAGS , and pass them to swig later

    get_directory_property( DirDefs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} COMPILE_DEFINITIONS )
    foreach( d ${DirDefs} )
        set( SWIG_FLAGS ${SWIG_FLAGS} -D${d} )
    endforeach()

endif()


if( KICAD_SCRIPTING )

    set( SWIG_OPTS -python -c++ -outdir ${CMAKE_CURRENT_BINARY_DIR} ${SWIG_FLAGS} )

    if( EXISTS ${CMAKE_CURRENT_BINARY_DIR}/doxygen-xml )
        set( SWIG_OPTS ${SWIG_OPTS} -DENABLE_DOCSTRINGS_FROM_DOXYGEN )
        set( SWIG_OPTS ${SWIG_OPTS} -I${CMAKE_CURRENT_BINARY_DIR}/docstrings )
    endif()

    add_custom_command( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/pcbnew_wrap.cxx
                        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/pcbnew.py

        DEPENDS pcbcommon
        DEPENDS scripting/pcbnew.i
        DEPENDS scripting/board.i
        DEPENDS scripting/board_item.i
        DEPENDS scripting/module.i
        DEPENDS scripting/plugins.i
        DEPENDS scripting/units.i
        DEPENDS ../scripting/dlist.i
        DEPENDS ../scripting/kicad.i
        DEPENDS ../scripting/wx.i
        DEPENDS ../scripting/kicadplugins.i

        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/docstrings
        COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_CURRENT_BINARY_DIR}/docstrings/docstrings.i # this makes docstrings.i available if it doesn't exist

        COMMAND ${SWIG_EXECUTABLE} ${SWIG_OPTS} -o ${CMAKE_CURRENT_BINARY_DIR}/pcbnew_wrap.cxx  scripting/pcbnew.i
        COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_SOURCE_DIR}/scripting/build_tools/fix_swig_imports.py ${CMAKE_CURRENT_BINARY_DIR}/pcbnew.py
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )

endif()

###
# _pcbnew DLL/DSO file creation
###

if( BUILD_GITHUB_PLUGIN )
    set( GITHUB_PLUGIN_LIBRARIES github_plugin )
endif()

if( ( KICAD_SCRIPTING OR KICAD_SCRIPTING_MODULES ) AND NOT WIN32 AND NOT APPLE )
    list( APPEND PCBNEW_EXTRA_LIBS rt )
endif()


if( KICAD_SCRIPTING_MODULES )

    #message( "building pcbnew scripting" )

    set( CMAKE_SWIG_FLAGS ${SWIG_FLAGS} )
    set_source_files_properties( scripting/pcbnew.i PROPERTIES CPLUSPLUS ON )
    swig_add_module( pcbnew python scripting/pcbnew.i ${PCBNEW_SCRIPTING_PYTHON_HELPERS} ${PCBNEW_SRCS} ${PCBNEW_COMMON_SRCS} )
    swig_link_libraries( pcbnew
        3d-viewer
        pcbcommon
        pnsrouter
        common
        pcad2kicadpcb
        lib_dxf
        ${GITHUB_PLUGIN_LIBRARIES}
        polygon
        bitmaps
        gal
        ${GLEW_LIBRARIES}
        ${CAIRO_LIBRARIES}
        ${PIXMAN_LIBRARY}
        ${wxWidgets_LIBRARIES}
        ${OPENGL_LIBRARIES}
        ${GDI_PLUS_LIBRARIES}
        ${PYTHON_LIBRARIES}
        ${PCBNEW_EXTRA_LIBS}
        ${Boost_LIBRARIES}      # must follow GITHUB
        )

    if( MAKE_LINK_MAPS )
        # generate a link map with cross reference
        set_target_properties( _pcbnew PROPERTIES LINK_FLAGS "-Wl,-cref -Wl,-Map=_pcbnew.map" )
    endif()

endif()


###
# Doxygen python documentation
###

find_package( Doxygen )
if( DOXYGEN_FOUND )
    if( KICAD_SCRIPTING )

        # create XML files from doxygen parsing
        add_custom_target( doxygen-python-xml
            ${CMAKE_COMMAND} -E remove_directory doxygen-python-xml
            COMMAND SOURCES_DIR=${CMAKE_SOURCE_DIR} ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile_xml
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            DEPENDS Doxyfile_xml
            COMMENT "building doxygen docs into directory doxygen-python/html"
        )

        # create .i files from XML doxygen parsing, docstrings.i will include all of them
        add_custom_target( xml-to-docstrings
            COMMAND ${CMAKE_COMMAND} -E remove_directory docstrings
            COMMAND ${CMAKE_COMMAND} -E make_directory docstrings
            COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_SOURCE_DIR}/scripting/build_tools/extract_docstrings.py pcbnew.py doxygen-xml/xml docstrings

            COMMAND ${CMAKE_COMMAND} -E remove pcbnew.py # force removal so it will be recreated later with the new docstrings
            COMMENT "building docstring files"
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            DEPENDS pcbnew.py
            DEPENDS doxygen-python-xml
            )

        # create doxygen-python html
        add_custom_target( doxygen-python
            ${CMAKE_COMMAND} -E remove_directory doxygen-python
            COMMAND PYTHON_SOURCES_TO_DOC=${CMAKE_CURRENT_BINARY_DIR}/pcbnew.py ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile_python
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            DEPENDS Doxyfile_python
            DEPENDS xml-to-docstrings
            DEPENDS pcbnew.py

            COMMENT "building doxygen docs into directory doxygen-python/html"
        )
    endif()
else()
    message( STATUS "WARNING: Doxygen not found - doxygen-docs (Source Docs) target not created" )
endif()




###
# Windows resource file
###
if( WIN32 )
    if( MINGW )
        # PCBNEW_RESOURCES variable is set by the macro.
        mingw_resource_compiler( pcbnew )
    else()
        set( PCBNEW_RESOURCES pcbnew.rc )
    endif()
endif()

###
# Set properties for APPLE on pcbnew target
###
if( APPLE )
    set( PCBNEW_RESOURCES pcbnew.icns pcbnew_doc.icns )
    set_source_files_properties( "${CMAKE_CURRENT_SOURCE_DIR}/pcbnew.icns"
                                PROPERTIES MACOSX_PACKAGE_LOCATION Resources )
    set_source_files_properties( "${CMAKE_CURRENT_SOURCE_DIR}/pcbnew_doc.icns"
                                PROPERTIES MACOSX_PACKAGE_LOCATION Resources )
    set( MACOSX_BUNDLE_ICON_FILE pcbnew.icns )
    set( MACOSX_BUNDLE_GUI_IDENTIFIER org.kicad-eda.pcbnew )
    set( MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist )
endif()

# Create a C++ compilable string initializer containing html text into a *.h file:
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/dialogs/dialog_freeroute_exchange_help_html.h
    COMMAND ${CMAKE_COMMAND}
        -DinputFile=${CMAKE_CURRENT_SOURCE_DIR}/dialogs/dialog_freeroute_exchange_help.html
        -DoutputFile=${CMAKE_CURRENT_SOURCE_DIR}/dialogs/dialog_freeroute_exchange_help_html.h
        -P ${CMAKE_MODULE_PATH}/Html2C.cmake
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/dialogs/dialog_freeroute_exchange_help.html
    COMMENT "creating ${CMAKE_CURRENT_SOURCE_DIR}/dialogs/dialog_freeroute_exchange_help_html.h
       from ${CMAKE_CURRENT_SOURCE_DIR}/dialogs/dialog_freeroute_exchange_help.html"
    )

set_source_files_properties( dialogs/dialog_freeroute_exchange.cpp
    PROPERTIES
        OBJECT_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/dialogs/dialog_freeroute_exchange_help_html.h
    )


# auto-generate specctra_lexer.h and specctra_keywords.cpp
make_lexer(
    ${CMAKE_CURRENT_SOURCE_DIR}/specctra.keywords
    ${CMAKE_CURRENT_SOURCE_DIR}/specctra_lexer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/specctra_keywords.cpp
    DSN

    # Pass header file with dependency on *_lexer.h as extra_arg
    specctra.h
    )


###
# subcomponents of pcbnew
###
add_subdirectory( pcad2kicadpcb_plugin )
if( BUILD_GITHUB_PLUGIN )
    add_subdirectory( github )
    add_dependencies( github_plugin lib-dependencies )
endif()


###
# Create the pcbnew executable
###
add_executable( pcbnew WIN32 MACOSX_BUNDLE
    ${PCBNEW_SRCS}
    ${PCBNEW_COMMON_SRCS}
    ${PCBNEW_SCRIPTING_SRCS}
    ${PCBNEW_RESOURCES}
    )

add_dependencies( pcbnew lib-dependencies )
###
# Set properties for APPLE on pcbnew target
###
if( APPLE )
    set_target_properties( pcbnew PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist )
endif()

###
# Link executable target pcbnew with correct libraries
###

target_link_libraries( pcbnew
    3d-viewer
    pcbcommon
    pnsrouter
    common
    pcad2kicadpcb
    polygon
    bitmaps
    gal
    lib_dxf
    ${GITHUB_PLUGIN_LIBRARIES}
    ${wxWidgets_LIBRARIES}
    ${OPENGL_LIBRARIES}
    ${GDI_PLUS_LIBRARIES}
    ${PYTHON_LIBRARIES}
    ${PCBNEW_EXTRA_LIBS}
    ${GLEW_LIBRARIES}
    ${CAIRO_LIBRARIES}
    ${PIXMAN_LIBRARY}
    ${Boost_LIBRARIES}      # must follow GITHUB
    )

# Only for win32 cross compilation using MXE
if( WIN32 AND MSYS AND CMAKE_CROSSCOMPILING )
target_link_libraries( pcbnew
    opengl32
    glu32
    pixman-1
    fontconfig
    freetype
    bz2
    )
endif()

if( MAKE_LINK_MAPS )
    # generate a link map with cross reference
    set_target_properties( pcbnew PROPERTIES LINK_FLAGS "-Wl,-cref -Wl,-Map=pcbnew.map" )
endif()


###
# Add pcbnew as install target
###
install( TARGETS pcbnew
    DESTINATION ${KICAD_BIN}
    COMPONENT binary )


if( KICAD_SCRIPTING )
    add_custom_target( FixSwigImportsScripting ALL
        COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_SOURCE_DIR}/scripting/build_tools/fix_swig_imports.py ${CMAKE_CURRENT_BINARY_DIR}/pcbnew.py
        DEPENDS pcbnew
        COMMENT "Fixing swig_import_helper in Kicad scripting"
        )

    install( FILES ${CMAKE_BINARY_DIR}/pcbnew/pcbnew.py DESTINATION ${PYTHON_DEST} )

    if( APPLE ) 
        # copies all into PYTHON_DEST then all into the bundle !
        add_custom_target( _pcbnew_py_copy ALL
            COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/pcbnew/pcbnew.py "${PYTHON_DEST}/wx-3.0-osx_cocoa/"
            DEPENDS FixSwigImportsScripting
            COMMENT "Copying pcbnew.py into PYTHON_DEST/wx-3.0-osx_cocoa/"
        )

        add_custom_target( pcbnew_copy_wxpython_scripting ALL
            COMMAND ${CMAKE_COMMAND} -E copy_directory ${LIBWXPYTHON_ROOT}/wxPython/  ${CMAKE_SOURCE_DIR}/pcbnew/pcbnew.app/Contents/Frameworks/wxPython/
            DEPENDS FixSwigImportsScripting _pcbnew_py_copy
            COMMENT "Copying wxPython into pcbnew.app Framework"
        )

        add_custom_target( pcbnew_copy_plugins ALL
            COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/pcbnew/scripting/plugins ${PROJECT_SOURCE_DIR}/pcbnew/pcbnew.app/Contents/PlugIns/scripting/plugins
            DEPENDS pcbnew_copy_wxpython_scripting 
            COMMENT "Copying plugins into bundle"
        )
 
        # fix bundle after copying wxpython, fixing and copying
        add_dependencies( osx_fix_bundles pcbnew_copy_wxpython_scripting )

        if ( KICAD_SCRIPTING_MODULES )
            #they do more or less the same job, avoid race between them
            #Cmake copy goes in error otherwise
            add_dependencies( pcbnew_copy_wxpython_scripting  pcbnew_copy_wxpython_module )
        endif()

    endif()
endif()

if( KICAD_SCRIPTING_MODULES )
    add_custom_target( FixSwigImportsModuleScripting ALL
        COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_SOURCE_DIR}/scripting/build_tools/fix_swig_imports.py ${CMAKE_CURRENT_BINARY_DIR}/pcbnew.py
        DEPENDS _pcbnew
        COMMENT "Fixing swig_import_helper in Kicad scripting modules"
        )

    install( FILES ${CMAKE_BINARY_DIR}/pcbnew/pcbnew.py DESTINATION ${PYTHON_DEST} )

    if( WIN32 )
        install( FILES ${CMAKE_BINARY_DIR}/pcbnew/_pcbnew.pyd DESTINATION ${PYTHON_DEST} )
    else()
        install( FILES ${CMAKE_BINARY_DIR}/pcbnew/_pcbnew.so DESTINATION ${PYTHON_DEST} )
    endif()

    if( APPLE ) 
        # copies needed files into PYTHON_DEST then copy all into the bundle !
        add_custom_target( _pcbnew_so_copy ALL
            COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/pcbnew/_pcbnew.so "${PYTHON_DEST}"
            DEPENDS _pcbnew FixSwigImportsModuleScripting
            COMMENT "Copying _pcbnew.so into PYTHON_DEST"
        )

        add_custom_target( pcbnew_copy_wxpython_module ALL
            COMMAND ${CMAKE_COMMAND} -E copy_directory ${LIBWXPYTHON_ROOT}/wxPython/  ${PROJECT_SOURCE_DIR}/pcbnew/pcbnew.app/Contents/Frameworks/wxPython/
            DEPENDS FixSwigImportsModuleScripting _pcbnew_so_copy
            COMMENT "Copying wxPython into pcbnew.app Frameworks"
        )

        # Tell that we have to run osx_fix_bundles fix after building _pcbnew and migrating wxPython
        add_dependencies( osx_fix_bundles pcbnew_copy_wxpython_module )
        add_dependencies( osx_fix_bundles _pcbnew )
    endif()
endif()


# This one gets made only when testing.
add_executable( specctra_test EXCLUDE_FROM_ALL specctra_test.cpp specctra.cpp )
target_link_libraries( specctra_test common ${wxWidgets_LIBRARIES} )


# This one gets made only when testing.
add_executable( layer_widget_test WIN32 EXCLUDE_FROM_ALL
    layer_widget.cpp
    )
target_link_libraries( layer_widget_test common ${wxWidgets_LIBRARIES} )

