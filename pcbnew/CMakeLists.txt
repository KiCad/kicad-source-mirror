# the map generation creates on Windows/gcc a lot of useless warnings
# so disable it on windows
if( WIN32 AND NOT CMAKE_CROSSCOMPILING )
    set( MAKE_LINK_MAPS false )
else()
    set( MAKE_LINK_MAPS true )
endif()

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
    ../utils/idftools
    ${INC_AFTER}
    )

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
    dialogs/dialog_enum_pads.cpp
    dialogs/dialog_enum_pads_base.cpp
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
    dialogs/dialog_pns_settings.cpp
    dialogs/dialog_pns_settings_base.cpp
    dialogs/dialog_non_copper_zones_properties.cpp
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
    dialogs/dialog_select_pretty_lib.cpp
    dialogs/dialog_select_pretty_lib_base.cpp
    dialogs/dialog_select_dirlist_base.cpp
    dialogs/dialog_set_grid.cpp
    dialogs/dialog_set_grid_base.cpp
    dialogs/dialog_target_properties_base.cpp
    dialogs/dialog_track_via_size.cpp
    dialogs/dialog_track_via_size_base.cpp
    footprint_wizard.cpp
    footprint_wizard_frame.cpp
    dialogs/dialog_footprint_wizard_list_base.cpp
    dialogs/dialog_footprint_wizard_list.cpp
    dialogs/wizard_add_fplib_base.cpp
    dialogs/wizard_add_fplib.cpp
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
    exporters/gen_drill_report_files.cpp
    exporters/gen_modules_placefile.cpp
    exporters/gendrill_Excellon_writer.cpp
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
    pcb_base_edit_frame.cpp
    append_board_to_current.cpp
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
    pcbnew_config.cpp
    pcbplot.cpp
    pcb_draw_panel_gal.cpp
    plot_board_layers.cpp
    plot_brditems_plotter.cpp
    print_board_functions.cpp
    printout_controler.cpp
    ratsnest.cpp
    ratsnest_viewitem.cpp
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
    zones_polygons_insulated_copper_islands.cpp
    zones_polygons_test_connections.cpp
    zones_test_and_combine_areas.cpp
    class_footprint_wizard.cpp

    tools/selection_tool.cpp
    tools/selection_area.cpp
    tools/selection_conditions.cpp
    tools/bright_box.cpp
    tools/edit_points.cpp
    tools/edit_constraints.cpp
    tools/point_editor.cpp
    tools/drawing_tool.cpp
    tools/edit_tool.cpp
    tools/pcbnew_control.cpp
    tools/pcb_editor_control.cpp
    tools/module_tools.cpp
    tools/placement_tool.cpp
    tools/common_actions.cpp
    )

set( PCBNEW_SRCS ${PCBNEW_AUTOROUTER_SRCS} ${PCBNEW_CLASS_SRCS} ${PCBNEW_DIALOGS} )


# extra sources from common
set( PCBNEW_COMMON_SRCS
    ../common/dialogs/dialog_page_settings.cpp
    ../common/base_units.cpp
    )

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


if( BUILD_GITHUB_PLUGIN )
    set( GITHUB_PLUGIN_LIBRARIES github_plugin )
endif()

if( UNIX AND NOT APPLE )
    list( APPEND PCBNEW_EXTRA_LIBS rt )
endif()

if( KICAD_USE_WEBKIT AND BUILD_GITHUB_PLUGIN )
    set( WEBVIEWER_WXLIB "webviewer" )
    add_definitions( -DKICAD_USE_WEBKIT )
endif()


if( KICAD_SCRIPTING_MODULES )

    #message( "building pcbnew scripting" )

    set( CMAKE_SWIG_FLAGS ${SWIG_FLAGS} )
    set_source_files_properties( scripting/pcbnew.i PROPERTIES CPLUSPLUS ON )

    swig_add_module( pcbnew
        python
        scripting/pcbnew.i
        ${PCBNEW_SCRIPTING_PYTHON_HELPERS}
        pcbnew.cpp
        ${PCBNEW_SRCS}
        ${PCBNEW_COMMON_SRCS}
        )

    swig_link_libraries( pcbnew
        3d-viewer
        pcbcommon
        pnsrouter
        common
        pcad2kicadpcb
        lib_dxf
        idf3
        ${GITHUB_PLUGIN_LIBRARIES}
        polygon
        ${WEBVIEWER_WXLIB}
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
        set_target_properties( _pcbnew PROPERTIES
            LINK_FLAGS "${TO_LINKER},-cref ${TO_LINKER},-Map=pcbnew.so.map"
            )
    endif()

    if( ${OPENMP_FOUND} )
        set_property( TARGET _pcbnew APPEND_STRING
            PROPERTY LINK_FLAGS " ${OpenMP_CXX_FLAGS}"
        )
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


if( MINGW )
    # PCBNEW_RESOURCES variable is set by the macro.
    mingw_resource_compiler( pcbnew )
endif()


if( APPLE )
    # setup bundle
    set( PCBNEW_RESOURCES pcbnew.icns pcbnew_doc.icns )
    set_source_files_properties( "${CMAKE_CURRENT_SOURCE_DIR}/pcbnew.icns" PROPERTIES
        MACOSX_PACKAGE_LOCATION Resources
        )
    set_source_files_properties( "${CMAKE_CURRENT_SOURCE_DIR}/pcbnew_doc.icns" PROPERTIES
        MACOSX_PACKAGE_LOCATION Resources
        )
    set( MACOSX_BUNDLE_ICON_FILE pcbnew.icns )
    set( MACOSX_BUNDLE_GUI_IDENTIFIER org.kicad-eda.kicad )
    set( MACOSX_BUNDLE_NAME pcbnew )
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

set_source_files_properties( dialogs/dialog_freeroute_exchange.cpp PROPERTIES
    OBJECT_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/dialogs/dialog_freeroute_exchange_help_html.h
    )

add_subdirectory( pcad2kicadpcb_plugin )

if( BUILD_GITHUB_PLUGIN )
    add_subdirectory( github )
    add_dependencies( github_plugin lib-dependencies )
endif()


# a very small program launcher for pcbnew_kiface
add_executable( pcbnew WIN32 MACOSX_BUNDLE
    ../common/single_top.cpp
    ${PCBNEW_RESOURCES}
    )
set_source_files_properties( ../common/single_top.cpp pcbnew.cpp PROPERTIES
    COMPILE_DEFINITIONS "TOP_FRAME=FRAME_PCB;PGM_DATA_FILE_EXT=\"kicad_pcb\";BUILD_KIWAY_DLL"
    )
target_link_libraries( pcbnew
    #singletop  # replaces common, giving us restrictive control and link warnings.
    # There's way too much crap coming in from common yet.
    common
    bitmaps
    ${wxWidgets_LIBRARIES}
    )

if( MAKE_LINK_MAPS )
    set_target_properties( pcbnew PROPERTIES
        LINK_FLAGS "${TO_LINKER},-cref ${TO_LINKER},-Map=pcbnew.map" )
endif()

# the main pcbnew program, in DSO form.
add_library( pcbnew_kiface MODULE
    pcbnew.cpp
    ${PCBNEW_SRCS}
    ${PCBNEW_COMMON_SRCS}
    ${PCBNEW_SCRIPTING_SRCS}
    )
set_target_properties( pcbnew_kiface PROPERTIES
    # Decorate OUTPUT_NAME with PREFIX and SUFFIX, creating something like
    # _pcbnew.so, _pcbnew.dll, or _pcbnew.kiface
    OUTPUT_NAME     pcbnew
    PREFIX          ${KIFACE_PREFIX}
    SUFFIX          ${KIFACE_SUFFIX}
    )

if( ${OPENMP_FOUND} )
    set_target_properties( pcbnew_kiface PROPERTIES
        COMPILE_FLAGS   ${OpenMP_CXX_FLAGS}
        )
endif()

target_link_libraries( pcbnew_kiface
    3d-viewer
    pcbcommon
    pnsrouter
    common
    pcad2kicadpcb
    polygon
    ${WEBVIEWER_WXLIB}    # before bitmaps, because it uses some bitmaps defined in bitmap
    bitmaps
    gal
    lib_dxf
    idf3
    ${GITHUB_PLUGIN_LIBRARIES}
    ${wxWidgets_LIBRARIES}
    ${OPENGL_LIBRARIES}
    ${GDI_PLUS_LIBRARIES}
    ${PYTHON_LIBRARIES}
    ${GLEW_LIBRARIES}
    ${CAIRO_LIBRARIES}
    ${PIXMAN_LIBRARY}
    ${Boost_LIBRARIES}      # must follow GITHUB
    ${PCBNEW_EXTRA_LIBS}    # -lrt must follow Boost
    ${OPENMP_LIBRARIES}
    )
set_source_files_properties( pcbnew.cpp PROPERTIES
    # The KIFACE is in pcbnew.cpp, export it:
    COMPILE_DEFINITIONS     "BUILD_KIWAY_DLL;COMPILING_DLL"
    )

if( MAKE_LINK_MAPS )
    set_target_properties( pcbnew_kiface PROPERTIES
        LINK_FLAGS "${TO_LINKER},-cref ${TO_LINKER},-Map=_pcbnew.kiface.map"
        )
    set_target_properties( pcbnew PROPERTIES
        LINK_FLAGS "${TO_LINKER},-cref ${TO_LINKER},-Map=pcbnew.map"
        )
endif()

# if building pcbnew, then also build pcbnew_kiface if out of date.
add_dependencies( pcbnew pcbnew_kiface )

# these 2 binaries are a matched set, keep them together:
if( APPLE )
    set_target_properties( pcbnew PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist
        )

    # puts binaries into the *.app bundle while linking
    set_target_properties( pcbnew_kiface PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${OSX_BUNDLE_BUILD_KIFACE_DIR}
        )
    # put individual bundle outside of main bundle as a first step
    # will be pulled into the main bundle when creating main bundle
    install( TARGETS pcbnew
        DESTINATION ${KICAD_BIN}
        COMPONENT binary
        )
    install( CODE "
        # override default embedded path settings
        ${OSX_BUNDLE_OVERRIDE_PATHS}

        # do all the work
        include( BundleUtilities )
        fixup_bundle( ${KICAD_BIN}/pcbnew.app/Contents/MacOS/pcbnew
            \"\"
            \"\"
            )
        " COMPONENT Runtime
        )
else()
    install( TARGETS pcbnew
        DESTINATION ${KICAD_BIN}
        COMPONENT binary
        )
    install( TARGETS pcbnew_kiface
        DESTINATION ${KICAD_BIN}
        COMPONENT binary
        )
endif()

add_dependencies( pcbnew lib-dependencies )


if( KICAD_SCRIPTING )
    if( NOT APPLE )
        install( FILES ${CMAKE_BINARY_DIR}/pcbnew/pcbnew.py DESTINATION ${PYTHON_DEST} )
    else()
        # put into bundle at build time, it is relocated at install
        add_custom_target( ScriptingPcbnewPyCopy ALL
            COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_BINARY_DIR}/pcbnew/pcbnew.py" "${PYTHON_DEST}/"
            DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/pcbnew.py
            COMMENT "Copying pcbnew.py into ${PYTHON_DEST}"
            )
        add_dependencies( ScriptingPcbnewPyCopy ScriptingWxpythonCopy )
        # scripting plugins
        install( DIRECTORY ${PROJECT_SOURCE_DIR}/pcbnew/scripting/plugins/
            DESTINATION ${KICAD_DATA}/scripting/plugins
            COMPONENT binary
            )
    endif()
endif()

if( KICAD_SCRIPTING_MODULES )
    add_custom_target( FixSwigImportsModuleScripting ALL
        COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_SOURCE_DIR}/scripting/build_tools/fix_swig_imports.py ${CMAKE_CURRENT_BINARY_DIR}/pcbnew.py
        DEPENDS _pcbnew
        COMMENT "Fixing swig_import_helper in Kicad scripting modules"
        )

    if( NOT APPLE )
        install( FILES ${CMAKE_BINARY_DIR}/pcbnew/pcbnew.py DESTINATION ${PYTHON_DEST} )
    else()
        # put everything into bundle at build time, it is relocated at install
        add_custom_target( ScriptingModulesPcbnewPyCopy ALL
            COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_BINARY_DIR}/pcbnew/pcbnew.py" "${PYTHON_DEST}/"
            DEPENDS FixSwigImportsModuleScripting
            COMMENT "Copying pcbnew.py into ${PYTHON_DEST}"
            )
        add_dependencies( ScriptingModulesPcbnewPyCopy ScriptingWxpythonCopy )
    endif()

    if( MINGW )
        install( FILES ${CMAKE_BINARY_DIR}/pcbnew/_pcbnew.pyd DESTINATION ${PYTHON_DEST} )
    elseif( APPLE )
         # put everything into bundle at build time, it is relocated at install
        add_custom_target( ScriptingModulesPcbnewSoCopy ALL
            COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_BINARY_DIR}/pcbnew/_pcbnew.so" "${PYTHON_DEST}/"
            DEPENDS _pcbnew
            COMMENT "Copying _pcbnew.so into ${PYTHON_DEST}"
            )
        add_dependencies( ScriptingModulesPcbnewSoCopy ScriptingWxpythonCopy )
    else()
        install( FILES ${CMAKE_BINARY_DIR}/pcbnew/_pcbnew.so DESTINATION ${PYTHON_DEST} )
    endif()
endif()

if( APPLE )
    if( KICAD_SCRIPTING OR KICAD_SCRIPTING_MODULES )
        # find wx-X.Y-osx_cocoa path below PYTHON_SITE_PACKAGE_PATH
        file( GLOB WXPYTHON_PATH ${PYTHON_SITE_PACKAGE_PATH}/wx-?.?-osx_cocoa )
        if( NOT WXPYTHON_PATH )
            message( FATAL_ERROR "Could not find 'wx-?.?-osx_cocoa' in '${PYTHON_SITE_PACKAGE_PATH}'" )
        endif()
        # copy contents
        add_custom_target( ScriptingWxpythonCopy ALL
            COMMAND ${CMAKE_COMMAND} -E copy_directory "${WXPYTHON_PATH}" "${PYTHON_DEST}"
            COMMENT "Copying wxPython into ${PYTHON_DEST}"
            )
    endif()
endif()


if( false )     # haven't been used in years.
    # This one gets made only when testing.
    add_executable( specctra_test EXCLUDE_FROM_ALL specctra_test.cpp specctra.cpp )
    target_link_libraries( specctra_test common ${wxWidgets_LIBRARIES} )


    # This one gets made only when testing.
    add_executable( layer_widget_test WIN32 EXCLUDE_FROM_ALL
        layer_widget.cpp
        )
    target_link_libraries( layer_widget_test common ${wxWidgets_LIBRARIES} )
endif()
