
if( UNIX AND NOT APPLE )
    # Setting this ON slows down linking and is a advanced (=hidden) developer option for
    # linux, not a user option.
    option( PCBNEW_LINK_MAPS
        "Developer: create linker map files for pcbnew binaries, not typical for Debug builds"
        )
    mark_as_advanced( PCBNEW_LINK_MAPS )
endif()

add_definitions( -DPCBNEW )

add_subdirectory(connectivity)

add_subdirectory(router)

# psnrouter depends on make_lexer outputs in common (bug #1285878 )
add_dependencies( pnsrouter pcbcommon )

if( KICAD_SCRIPTING OR KICAD_SCRIPTING_MODULES )
    file( MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/swig )

    # Infrequently needed headers go at end of search paths, append to INC_AFTER
    set( INC_AFTER ${INC_AFTER} swig )
    set( INC_AFTER ${INC_AFTER} ../common/swig )

    #message( STATUS "pcbnew INC_AFTER:${INC_AFTER}" )
endif()


if( BUILD_GITHUB_PLUGIN )
    set( GITHUB_3DLIBRARIES_WIZARD
        dialogs/wizard_3DShape_Libs_downloader_base.cpp
        dialogs/wizard_3DShape_Libs_downloader.cpp
        )
endif()

include_directories( BEFORE ${INC_BEFORE} )
include_directories(
    ./dialogs
    ./autorouter
    ../3d-viewer
    ../common
    ../common/dialogs
    ./exporters
    ../dxflib_qcad
    ../utils/idftools
    ${GLM_INCLUDE_DIR}
    ./specctra_import_export
    ${INC_AFTER}
    )

set( PCBNEW_DIALOGS
    dialogs/dialog_block_options.cpp
    dialogs/dialog_block_options_base.cpp
    dialogs/dialog_board_setup.cpp
    dialogs/dialog_board_statistics.cpp
    dialogs/dialog_board_statistics_base.cpp
    dialogs/dialog_choose_footprint.cpp
    dialogs/dialog_cleanup_tracks_and_vias.cpp
    dialogs/dialog_cleanup_tracks_and_vias_base.cpp
    dialogs/dialog_copper_zones.cpp
    dialogs/dialog_copper_zones_base.cpp
    dialogs/dialog_create_array.cpp
    dialogs/dialog_create_array_base.cpp
    dialogs/dialog_drc.cpp
    dialogs/dialog_drc_base.cpp
    dialogs/dialog_edit_footprint_for_BoardEditor.cpp
    dialogs/dialog_edit_footprint_for_BoardEditor_base.cpp
    dialogs/dialog_edit_footprint_for_fp_editor.cpp
    dialogs/dialog_edit_footprint_for_fp_editor_base.cpp
    dialogs/dialog_enum_pads.cpp
    dialogs/dialog_enum_pads_base.cpp
    dialogs/dialog_exchange_footprints.cpp
    dialogs/dialog_exchange_footprints_base.cpp
    dialogs/dialog_export_idf.cpp
    dialogs/dialog_export_idf_base.cpp
    dialogs/dialog_export_step.cpp
    dialogs/dialog_export_step_base.cpp
    dialogs/dialog_export_svg.cpp
    dialogs/dialog_export_svg_base.cpp
    dialogs/dialog_export_vrml.cpp
    dialogs/dialog_export_vrml_base.cpp
    dialogs/dialog_find.cpp
    dialogs/dialog_find_base.cpp
    dialogs/dialog_footprint_wizard_list.cpp
    dialogs/dialog_footprint_wizard_list_base.cpp
    dialogs/dialog_fp_browser_display_options.cpp
    dialogs/dialog_fp_browser_display_options_base.cpp
    dialogs/dialog_fp_plugin_options.cpp
    dialogs/dialog_fp_plugin_options_base.cpp
    dialogs/dialog_gen_footprint_position_file_base.cpp
    dialogs/dialog_gencad_export_options.cpp
    dialogs/dialog_gendrill.cpp
    dialogs/dialog_gendrill_base.cpp
    dialogs/dialog_get_footprint_by_name_base.cpp
    dialogs/dialog_global_deletion.cpp
    dialogs/dialog_global_deletion_base.cpp
    dialogs/dialog_global_edit_tracks_and_vias.cpp
    dialogs/dialog_global_edit_tracks_and_vias_base.cpp
    dialogs/dialog_global_edit_text_and_graphics.cpp
    dialogs/dialog_global_edit_text_and_graphics_base.cpp
    dialogs/dialog_global_fp_lib_table_config.cpp
    dialogs/dialog_push_pad_properties.cpp
    dialogs/dialog_push_pad_properties_base.cpp
    dialogs/dialog_graphic_item_properties.cpp
    dialogs/dialog_graphic_item_properties_base.cpp
    dialogs/dialog_import_settings.cpp
    dialogs/dialog_import_settings_base.cpp
    dialogs/dialog_keepout_area_properties.cpp
    dialogs/dialog_keepout_area_properties_base.cpp
    dialogs/dialog_layer_selection_base.cpp
    dialogs/dialog_move_exact.cpp
    dialogs/dialog_move_exact_base.cpp
    dialogs/dialog_netlist.cpp
    dialogs/dialog_netlist_base.cpp
    dialogs/dialog_non_copper_zones_properties.cpp
    dialogs/dialog_non_copper_zones_properties_base.cpp
    dialogs/dialog_pad_basicshapes_properties.cpp
    dialogs/dialog_pad_properties.cpp
    dialogs/dialog_pad_properties_base.cpp
    dialogs/dialog_plot.cpp
    dialogs/dialog_plot_base.cpp
    dialogs/dialog_pns_diff_pair_dimensions.cpp
    dialogs/dialog_pns_diff_pair_dimensions_base.cpp
    dialogs/dialog_pns_length_tuning_settings.cpp
    dialogs/dialog_pns_length_tuning_settings_base.cpp
    dialogs/dialog_pns_settings.cpp
    dialogs/dialog_pns_settings_base.cpp
    dialogs/dialog_position_relative.cpp
    dialogs/dialog_position_relative_base.cpp
    dialogs/dialog_print_pcbnew.cpp
    dialogs/dialog_select_net_from_list.cpp
    dialogs/dialog_select_net_from_list_base.cpp
    dialogs/dialog_set_grid.cpp
    dialogs/dialog_set_grid_base.cpp
    dialogs/dialog_swap_layers.cpp
    dialogs/dialog_swap_layers_base.cpp
    dialogs/dialog_target_properties.cpp
    dialogs/dialog_target_properties_base.cpp
    dialogs/dialog_text_properties.cpp
    dialogs/dialog_text_properties_base.cpp
    dialogs/dialog_track_via_properties.cpp
    dialogs/dialog_track_via_properties_base.cpp
    dialogs/dialog_track_via_size.cpp
    dialogs/dialog_track_via_size_base.cpp
    dialogs/dialog_update_pcb.cpp
    dialogs/dialog_update_pcb_base.cpp
    dialogs/panel_fp_lib_table.cpp
    dialogs/panel_fp_lib_table_base.cpp
    dialogs/panel_modedit_defaults.cpp
    dialogs/panel_modedit_defaults_base.cpp
    dialogs/panel_modedit_display_options.cpp
    dialogs/panel_modedit_settings.cpp
    dialogs/panel_modedit_settings_base.cpp
    dialogs/panel_pcbnew_display_options.cpp
    dialogs/panel_pcbnew_display_options_base.cpp
    dialogs/panel_pcbnew_settings.cpp
    dialogs/panel_pcbnew_settings_base.cpp
    dialogs/panel_setup_mask_and_paste.cpp
    dialogs/panel_setup_mask_and_paste_base.cpp
    dialogs/panel_setup_feature_constraints.cpp
    dialogs/panel_setup_feature_constraints_base.cpp
    dialogs/panel_setup_layers.cpp
    dialogs/panel_setup_layers_base.cpp
    dialogs/panel_setup_netclasses.cpp
    dialogs/panel_setup_netclasses_base.cpp
    dialogs/panel_setup_text_and_graphics.cpp
    dialogs/panel_setup_text_and_graphics_base.cpp
    dialogs/panel_setup_tracks_and_vias.cpp
    dialogs/panel_setup_tracks_and_vias_base.cpp
    footprint_wizard.cpp
    footprint_wizard_frame.cpp
    footprint_wizard_frame_functions.cpp
    ${GITHUB_3DLIBRARIES_WIZARD}
    )

if( KICAD_SCRIPTING AND KICAD_SCRIPTING_ACTION_MENU )
    set( PCBNEW_DIALOGS ${PCBNEW_DIALOGS}
        dialogs/panel_pcbnew_action_plugins.cpp
        dialogs/panel_pcbnew_action_plugins_base.cpp
        )
endif()

set( PCBNEW_BRDSTACKUP_MGR
    board_stackup_manager/stackup_predefined_prms.cpp
    )

set( PCBNEW_IMPORT_GFX
    import_gfx/dialog_import_gfx_base.cpp
    import_gfx/dialog_import_gfx.cpp
    import_gfx/graphics_import_mgr.cpp
    import_gfx/graphics_importer.cpp
    import_gfx/graphics_importer_pcbnew.cpp
    import_gfx/graphics_importer_buffer.cpp
    import_gfx/dxf_import_plugin.cpp
    import_gfx/svg_import_plugin.cpp
    import_gfx/nanosvg.cpp
    )


set( PCBNEW_EXPORTERS
    exporters/export_hyperlynx.cpp
    exporters/export_d356.cpp
    exporters/export_footprint_associations.cpp
    exporters/export_gencad.cpp
    exporters/export_idf.cpp
    exporters/export_vrml.cpp
    exporters/export_footprints_placefile.cpp
    exporters/gen_drill_report_files.cpp
    exporters/gen_footprints_placefile.cpp
    exporters/gendrill_Excellon_writer.cpp
    exporters/gendrill_file_writer_base.cpp
    exporters/gendrill_gerber_writer.cpp
    exporters/gerber_jobfile_writer.cpp
    )

set( PCBNEW_MICROWAVE_SRCS
    microwave/microwave_inductor.cpp
    )

set( PCBNEW_DRC_SRCS
    drc/courtyard_overlap.cpp
    drc/drc_marker_factory.cpp
    drc/drc_provider.cpp
    )

set( PCBNEW_CLASS_SRCS
    ${PCBNEW_DIALOGS}
    ${PCBNEW_EXPORTERS}
    ${PCBNEW_IMPORT_GFX}
    ${PCBNEW_DRC_SRCS}
    ${PCBNEW_BRDSTACKUP_MGR}

    autorouter/rect_placement/rect_placement.cpp
    autorouter/spread_footprints.cpp
    autorouter/ar_autoplacer.cpp
    autorouter/ar_matrix.cpp
    autorouter/autoplacer_tool.cpp

    action_plugin.cpp
    array_creator.cpp
    array_pad_name_provider.cpp
    board_netlist_updater.cpp
    build_BOM_from_board.cpp
    cross-probing.cpp
    tools/drc.cpp
    tools/drc_clearance_test_functions.cpp
    edit.cpp
    edit_track_width.cpp
    files.cpp
    footprint_info_impl.cpp
    footprint_wizard.cpp
    footprint_editor_utils.cpp
    footprint_editor_options.cpp
    fp_tree_synchronizing_adapter.cpp
    footprint_edit_frame.cpp
    footprint_libraries_utils.cpp
    footprint_viewer_frame.cpp
    fp_tree_model_adapter.cpp
    generate_footprint_info.cpp
    grid_layer_box_helpers.cpp
    grid_layer_box_helpers.h
    initpcb.cpp
    layer_widget.cpp
    load_select_footprint.cpp
    menubar_footprint_editor.cpp
    menubar_pcb_editor.cpp
    microwave.cpp
    netlist.cpp
    pad_naming.cpp
    pcb_base_edit_frame.cpp
    pcb_layer_box_selector.cpp
    pcb_layer_widget.cpp
#    pcb_draw_panel_gal.cpp
#    pcb_view.cpp
    pcb_edit_frame.cpp
    pcbnew_config.cpp
    pcbnew_printout.cpp
    pcb_legacy_draw_utils.cpp
    pcbplot.cpp
    plot_board_layers.cpp
    plot_brditems_plotter.cpp
    ratsnest.cpp
    specctra_import_export/specctra.cpp
    specctra_import_export/specctra_export.cpp
    specctra_import_export/specctra_import.cpp
    text_mod_grid_table.cpp
    toolbars_footprint_editor.cpp
    toolbars_footprint_viewer.cpp
    toolbars_pcb_editor.cpp
    tracks_cleaner.cpp
    undo_redo.cpp
    zone_filler.cpp
    zones_by_polygon.cpp
    zones_functions_for_undo_redo.cpp
    zones_test_and_combine_areas.cpp

    tools/drawing_tool.cpp
    tools/edit_tool.cpp
    tools/global_edit_tool.cpp
    tools/grid_helper.cpp
    tools/microwave_tool.cpp
    tools/footprint_editor_tools.cpp
    tools/pad_tool.cpp
    tools/pcb_actions.cpp
    tools/pcb_bright_box.cpp
    tools/pcb_editor_control.cpp
    tools/pcb_inspection_tool.cpp
    tools/pcb_selection_conditions.cpp
    tools/pcb_tool_base.cpp
    tools/pcbnew_control.cpp
    tools/pcbnew_picker_tool.cpp
    tools/pcbnew_selection.cpp
    tools/placement_tool.cpp
    tools/point_editor.cpp
    tools/position_relative_tool.cpp
    tools/selection_tool.cpp
    tools/tool_event_utils.cpp
    tools/zone_create_helper.cpp
    tools/zone_filler_tool.cpp

    footprint_preview_panel.cpp
    footprint_tree_pane.cpp
    )

set( PCBNEW_SRCS
    ${PCBNEW_MICROWAVE_SRCS}
    ${PCBNEW_CLASS_SRCS}
    ${PCBNEW_DIALOGS}
    )

# extra sources from common
set( PCBNEW_COMMON_SRCS
    ../common/dialogs/dialog_page_settings.cpp
    ../common/base_units.cpp
    ../common/hotkeys_basic.cpp
    )

set( PCBNEW_SCRIPTING_DIALOGS
    dialogs/dialog_scripting.cpp
    dialogs/dialog_scripting_base.cpp
    )

set( PCBNEW_SCRIPTING_PYTHON_HELPERS
    ../common/swig/wx_python_helpers.cpp
    swig/pcbnew_action_plugins.cpp
    swig/pcbnew_footprint_wizards.cpp
    swig/pcbnew_scripting_helpers.cpp
    swig/python_scripting.cpp
    )

if( COMPILER_SUPPORTS_WSHADOW )
    # .cpp files are compiled with extra ${WSHADOW_FLAGS}, but not .cxx files
    set_source_files_properties(
        ${PCBNEW_SRCS} ${PCBNEW_COMMON_SRCS} ${PCBNEW_SCRIPTING_DIALOGS} ${PCBNEW_SCRIPTING_PYTHON_HELPERS}
        PROPERTIES COMPILE_FLAGS ${WSHADOW_FLAGS}
    )

    # There is a lot of 'local variable shadowed' warnings,
    # but we have no control over the generated code
    set_source_files_properties( pcbnew_wrap.cxx pcbnewPYTHON_wrap.cxx
        PROPERTIES COMPILE_FLAGS -Wno-shadow
    )
endif()


if( KICAD_SCRIPTING )
    set( PCBNEW_SCRIPTING_SRCS
        ${PCBNEW_SCRIPTING_DIALOGS}
        pcbnew_wrap.cxx
        ${PCBNEW_SCRIPTING_PYTHON_HELPERS}
        )


    # Swig generated files do not use the override specifier, therefore
    # disable suggest-override warnings
    if( COMPILER_SUPPORTS_WSUGGEST_OVERRIDE )
        set_source_files_properties( pcbnew_wrap.cxx pcbnewPYTHON_wrap.cxx
            PROPERTIES COMPILE_FLAGS -Wno-suggest-override
        )
    endif()
endif()


if( KICAD_SCRIPTING OR KICAD_SCRIPTING_MODULES )

    set( SWIG_FLAGS
        -I${CMAKE_CURRENT_SOURCE_DIR}
        -I${CMAKE_CURRENT_SOURCE_DIR}/../include
        -I${CMAKE_CURRENT_SOURCE_DIR}/../scripting
        -I${CMAKE_CURRENT_SOURCE_DIR}/../common/swig
        -I${WXPYTHON_SWIG_DIR}
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


if( KICAD_SCRIPTING )   # Generate pcbnew.py and pcbnew_wrap.cxx using swig

    # We deliberately do not use the CMake support for swig here,
    # i.e. swig_add_footprint()) because we want full control.

    set( SWIG_OPTS -python -c++ -outdir ${CMAKE_CURRENT_BINARY_DIR} ${SWIG_FLAGS} )

    if( EXISTS ${CMAKE_CURRENT_BINARY_DIR}/doxygen-xml )
        set( SWIG_OPTS ${SWIG_OPTS} -DENABLE_DOCSTRINGS_FROM_DOXYGEN )
        set( SWIG_OPTS ${SWIG_OPTS} -I${CMAKE_CURRENT_BINARY_DIR}/docstrings )
    endif()

    add_custom_command( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/pcbnew_wrap.cxx
                        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/pcbnew.py

        DEPENDS pcbcommon
        DEPENDS plotcontroller.h
        DEPENDS exporters/gendrill_Excellon_writer.h
        DEPENDS swig/pcbnew.i
        DEPENDS swig/board.i
        DEPENDS swig/board_connected_item.i
        DEPENDS swig/board_design_settings.i
        DEPENDS swig/board_item.i
        DEPENDS swig/board_item_container.i
        DEPENDS swig/connectivity.i
        DEPENDS swig/dimension.i
        DEPENDS swig/drawsegment.i
        DEPENDS swig/edge_mod.i
        DEPENDS swig/marker_pcb.i
        DEPENDS swig/pcb_target.i
        DEPENDS swig/pcb_plot_params.i
        DEPENDS swig/footprint.i
        DEPENDS swig/netclass.i
        DEPENDS swig/netinfo.i
        DEPENDS swig/pad.i
        DEPENDS swig/pcb_text.i
        DEPENDS swig/plugins.i
        DEPENDS swig/text_mod.i
        DEPENDS swig/track.i
        DEPENDS swig/units.i
        DEPENDS swig/typeinfo.i
        DEPENDS swig/zone.i
        DEPENDS swig/zone_settings.i

        DEPENDS ../common/swig/kicad.i
        DEPENDS ../common/swig/wx.i
        DEPENDS ../common/swig/ki_exception.i
        DEPENDS ../scripting/kicadplugins.i

        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/docstrings

        # Make docstrings.i available if it doesn't exist
        COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_CURRENT_BINARY_DIR}/docstrings/docstrings.i

        COMMAND ${SWIG_EXECUTABLE}
            ${SWIG_OPTS} -o ${CMAKE_CURRENT_BINARY_DIR}/pcbnew_wrap.cxx swig/pcbnew.i

        COMMAND ${PYTHON_EXECUTABLE}
            ${CMAKE_SOURCE_DIR}/scripting/build_tools/fix_swig_imports.py
            ${CMAKE_CURRENT_BINARY_DIR}/pcbnew.py

        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        )
endif()


if( BUILD_GITHUB_PLUGIN )
    set( GITHUB_PLUGIN_LIBRARIES github_plugin )
endif()

if( UNIX AND NOT APPLE )
    list( APPEND PCBNEW_EXTRA_LIBS rt )
endif()


###
# Doxygen python documentation
###

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

        # The sources to give to the Python Doxygen target
        set( DOXYGEN_PYTHON_SOURCES
            ${CMAKE_CURRENT_BINARY_DIR}/pcbnew.py
            ${CMAKE_CURRENT_SOURCE_DIR}/python/plugins/FootprintWizardBase.py
            ${CMAKE_CURRENT_SOURCE_DIR}/python/plugins/PadArray.py )

        # The Doxyfile expects a space-separated list in the env var
        string(REPLACE ";" " " DOXYGEN_PYTHON_SOURCES_STR "${DOXYGEN_PYTHON_SOURCES}")

        # Create doxygen-python html
        add_custom_target( doxygen-python
            ${CMAKE_COMMAND} -E remove_directory doxygen-python
            COMMAND ${CMAKE_COMMAND} -E env
                PYTHON_SOURCES_TO_DOC=${DOXYGEN_PYTHON_SOURCES_STR}
                CMAKE_SOURCE_DIR=${CMAKE_SOURCE_DIR}
                ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile_python
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            DEPENDS Doxyfile_python
            DEPENDS xml-to-docstrings
            DEPENDS ${DOXYGEN_PYTHON_SOURCES}
            COMMENT "building doxygen docs into directory doxygen-python/html"
            )
    endif()
endif()


if( MINGW )
    # PCBNEW_RESOURCES variable is set by the macro.
    mingw_resource_compiler( pcbnew )
else()
    set( PCBNEW_RESOURCES pcbnew.rc )
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
    set( MACOSX_BUNDLE_GUI_IDENTIFIER org.kicad-pcb.kicad )
    set( MACOSX_BUNDLE_NAME pcbnew )
endif()


add_subdirectory( pcad2kicadpcb_plugin )

if( BUILD_GITHUB_PLUGIN )
    add_subdirectory( github )
    # github_plugin depends on make_lexer outputs in common
    add_dependencies( github_plugin pcbcommon )
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
    gal
    ${wxWidgets_LIBRARIES}
    )

if( PCBNEW_LINK_MAPS )
    set_target_properties( pcbnew PROPERTIES
        LINK_FLAGS "-Wl,-cref,-Map=pcbnew.map" )
endif()

# the main pcbnew program, in DSO form.
add_library( pcbnew_kiface_objects OBJECT
    pcbnew.cpp
    ${PCBNEW_SRCS}
    ${PCBNEW_COMMON_SRCS}
    ${PCBNEW_SCRIPTING_SRCS}
    )

# auto-generate specctra_lexer.h and specctra_keywords.cpp
make_lexer(
    pcbnew_kiface_objects
    specctra_import_export/specctra.keywords
    specctra_import_export/specctra_lexer.h
    specctra_import_export/specctra_keywords.cpp
    DSN
    )

# CMake <3.9 can't link anything to object libraries,
# but we only need include directories, as we will link the kiface MODULE
target_include_directories( pcbnew_kiface_objects PRIVATE
   $<TARGET_PROPERTY:common,INTERFACE_INCLUDE_DIRECTORIES>
)

# Since we're not using target_link_libraries, we need to explicitly
# declare the dependency
add_dependencies( pcbnew_kiface_objects common )

add_library( pcbnew_kiface MODULE $<TARGET_OBJECTS:pcbnew_kiface_objects> )

set_target_properties( pcbnew_kiface PROPERTIES
    # Decorate OUTPUT_NAME with PREFIX and SUFFIX, creating something like
    # _pcbnew.so, _pcbnew.dll, or _pcbnew.kiface
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
    3d-viewer
    connectivity
    pcbcommon
    pnsrouter
    pcad2kicadpcb
    common
    gal
    lib_dxf
    idf3
    ${wxWidgets_LIBRARIES}
    ${GITHUB_PLUGIN_LIBRARIES}
    ${GDI_PLUS_LIBRARIES}
    ${PYTHON_LIBRARIES}
    ${Boost_LIBRARIES}      # must follow GITHUB
    ${PCBNEW_EXTRA_LIBS}    # -lrt must follow Boost
    )


target_link_libraries( pcbnew_kiface ${PCBNEW_KIFACE_LIBRARIES} )

set_source_files_properties( pcbnew.cpp PROPERTIES
    # The KIFACE is in pcbnew.cpp, export it:
    COMPILE_DEFINITIONS     "BUILD_KIWAY_DLL;COMPILING_DLL"
    )

if( PCBNEW_LINK_MAPS )
    set_target_properties( pcbnew_kiface PROPERTIES
        LINK_FLAGS "-Wl,-cref,-Map=_pcbnew.kiface.map"
        )
    set_target_properties( pcbnew PROPERTIES
        LINK_FLAGS "-Wl,-cref,-Map=pcbnew.map"
        )
endif()

# if building pcbnew, then also build pcbnew_kiface if out of date.
add_dependencies( pcbnew pcbnew_kiface )

# these 2 binaries are a matched set, keep them together:
if( APPLE )
    include( ${CMAKE_MODULE_PATH}/KiCadVersion.cmake )
    configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist @ONLY )

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
    endif()

    # python plugins
    install( DIRECTORY ${PROJECT_SOURCE_DIR}/pcbnew/python/plugins/
        DESTINATION ${KICAD_DATA}/scripting/plugins
        FILE_PERMISSIONS OWNER_EXECUTE OWNER_READ OWNER_WRITE GROUP_EXECUTE GROUP_READ WORLD_EXECUTE WORLD_READ
    )

    # python shell
    if ( KICAD_SCRIPTING_WXPYTHON )
        install( DIRECTORY ${PROJECT_SOURCE_DIR}/pcbnew/python/kicad_pyshell/
            DESTINATION ${KICAD_DATA}/scripting/kicad_pyshell
            FILE_PERMISSIONS OWNER_EXECUTE OWNER_READ OWNER_WRITE GROUP_EXECUTE GROUP_READ WORLD_EXECUTE WORLD_READ
        )
    endif()
endif()


if( KICAD_SCRIPTING_MODULES )

    # 1) KICAD_SCRIPTING enables python inside _pcbnew.kiface.
    # 2) KICAD_SCRIPTING_MODULES enables python from the OS command line for pcbnew.
    # When python is running within _pcbnew.kiface (case 1 above) it uses said
    # kiface for the native part of the pcbnew python module.  This is a kind of
    # circular dependency that works well.  When running python from
    # the command line (case 2 above) then python needs a native portion of the pcbnew
    # python module also, and this is _pcbnew.{so,pyd}.  It turns out that the
    # kiface file is built adequately to serve the needs of 2) for now if it is
    # merely renamed. This is phase 1 of a 2 step plan.
    # In phase 2 we will use the _pcbnew.kiface file without renaming, by doctoring
    # what the python portion of the pcbnew python module wants to load when run
    # from the command line, case 2 above.

    # Here is built the _pcbnew.{so,pyd} which is the native part of the pcbnew Python library
    # when Python is used from the command line.

    if( MINGW )
        install( FILES ${CMAKE_BINARY_DIR}/pcbnew/_pcbnew.pyd DESTINATION ${PYTHON_DEST} )
        set( PYMOD_EXT "pyd" )
    elseif( APPLE )
         # put everything into bundle at build time, it is relocated at install
        add_custom_target( ScriptingModulesPcbnewSoCopy ALL
            COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_BINARY_DIR}/pcbnew/_pcbnew.so" "${PYTHON_DEST}/"
            DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/_pcbnew.so
            COMMENT "Copying _pcbnew.so into ${PYTHON_DEST}"
            )
        add_dependencies( ScriptingModulesPcbnewSoCopy ScriptingWxpythonCopy )
        set( PYMOD_EXT "so" )
    else()  # only linux remains among supported platforms
        install( FILES ${CMAKE_CURRENT_BINARY_DIR}/_pcbnew.kiface DESTINATION ${PYTHON_DEST} COMPONENT binary RENAME "_pcbnew.so" )
        set( PYMOD_EXT "so" )
    endif()

    if( APPLE )
         add_custom_command( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/_pcbnew.${PYMOD_EXT}
            DEPENDS pcbnew_kiface
            COMMAND ${CMAKE_COMMAND} -E copy  ${OSX_BUNDLE_BUILD_KIFACE_DIR}/_pcbnew.kiface _pcbnew.${PYMOD_EXT}
            COMMENT "Creating python's pcbnew native module _pcbnew.${PYMOD_EXT} for command line use."
            )
        add_custom_target(
            pcbnew_python_module ALL
            DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/_pcbnew.${PYMOD_EXT}
            )
    else()
        # For phase 1, copy _pcbnew.kiface to the python module.
        add_custom_command( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/_pcbnew.${PYMOD_EXT}
            DEPENDS pcbnew_kiface
            COMMAND ${CMAKE_COMMAND} -E copy _pcbnew.kiface _pcbnew.${PYMOD_EXT}
            COMMENT "Creating python's pcbnew native module _pcbnew.${PYMOD_EXT} for command line use."
            )
        add_custom_target(
            pcbnew_python_module ALL
            DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/_pcbnew.${PYMOD_EXT}
            )
    endif()

endif()


if( APPLE )
    if( KICAD_SCRIPTING OR KICAD_SCRIPTING_MODULES )
        # find wx-X.Y-osx_cocoa path below PYTHON_SITE_PACKAGE_PATH
        file( GLOB WXPYTHON_DIR RELATIVE ${PYTHON_SITE_PACKAGE_PATH} ${PYTHON_SITE_PACKAGE_PATH}/wx-?.?-osx_cocoa )
        if( NOT WXPYTHON_DIR )
            message( FATAL_ERROR "Could not find 'wx-?.?-osx_cocoa' in '${PYTHON_SITE_PACKAGE_PATH}'" )
        endif()
        # copy contents
        add_custom_target( ScriptingWxpythonCopy ALL
            COMMAND ${CMAKE_COMMAND} -E copy_directory "${PYTHON_SITE_PACKAGE_PATH}/${WXPYTHON_DIR}" "${PYTHON_DEST}/${WXPYTHON_DIR}"
            COMMAND ${CMAKE_COMMAND} -E copy "${PYTHON_SITE_PACKAGE_PATH}/wxversion.py" "${PYTHON_DEST}"
            COMMENT "Copying wxPython into ${PYTHON_DEST}"
            )
    endif()
endif()
