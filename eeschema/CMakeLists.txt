# the map generation creates on Windows/gcc a lot of useless warnings
# so disable it on windows
if( WIN32 AND NOT CMAKE_CROSSCOMPILING )
    set( MAKE_LINK_MAPS false )
else()
    set( MAKE_LINK_MAPS true )
endif()

add_definitions( -DEESCHEMA )


include_directories( BEFORE ${INC_BEFORE} )
include_directories(
    ./dialogs
    ../common
    ../common/dialogs
    ${INC_AFTER}
    )


set( EESCHEMA_DLGS
    dialogs/dialog_color_config.cpp
    dialogs/dialog_annotate.cpp
    dialogs/dialog_annotate_base.cpp
    dialogs/dialog_bom.cpp
    dialogs/dialog_bom_base.cpp
    dialogs/dialog_bom_cfg_keywords.cpp
    dialogs/dialog_choose_component.cpp
    dialogs/dialog_choose_component_base.cpp
    dialogs/dialog_lib_edit_text.cpp
    dialogs/dialog_lib_edit_text_base.cpp
    dialogs/dialog_edit_component_in_lib.cpp
    dialogs/dialog_edit_component_in_lib_base.cpp
    dialogs/dialog_edit_component_in_schematic_fbp.cpp
    dialogs/dialog_edit_component_in_schematic.cpp
    dialogs/dialog_edit_label.cpp
    dialogs/dialog_edit_label_base.cpp
    dialogs/dialog_edit_libentry_fields_in_lib.cpp
    dialogs/dialog_edit_libentry_fields_in_lib_base.cpp
    dialogs/dialog_edit_one_field.cpp
    dialogs/dialog_eeschema_config.cpp
    dialogs/dialog_eeschema_config_fbp.cpp
    dialogs/dialog_eeschema_options_base.cpp
    dialogs/dialog_eeschema_options.cpp
    dialogs/dialog_erc.cpp
    dialogs/dialog_erc_base.cpp
    dialogs/dialog_libedit_dimensions.cpp
    dialogs/dialog_libedit_dimensions_base.cpp
    dialogs/dialog_lib_edit_draw_item.cpp
    dialogs/dialog_lib_edit_draw_item_base.cpp
    dialogs/dialog_libedit_options_base.cpp
    dialogs/dialog_libedit_options.cpp
    dialogs/dialog_lib_edit_pin.cpp
    dialogs/dialog_lib_edit_pin_base.cpp
    dialogs/dialog_lib_new_component.cpp
    dialogs/dialog_lib_new_component_base.cpp
    dialogs/dialog_netlist.cpp
    dialogs/dialog_netlist_base.cpp
    dialogs/dialog_plot_schematic_base.cpp
    dialogs/dialog_plot_schematic.cpp
    dialogs/dialog_print_using_printer_base.cpp
    dialogs/dialog_print_using_printer.cpp
    dialogs/dialog_sch_edit_sheet_pin.cpp
    dialogs/dialog_sch_edit_sheet_pin_base.cpp
    dialogs/dialog_sch_sheet_props.cpp
    dialogs/dialog_sch_sheet_props_base.cpp
    dialogs/dialog_schematic_find.cpp
    dialogs/dialog_schematic_find_base.cpp
    )

set( EESCHEMA_SRCS
    annotate.cpp
    backanno.cpp
    block.cpp
    block_libedit.cpp
    busentry.cpp
    bus-wire-junction.cpp
    class_drc_erc_item.cpp
    class_libentry.cpp
    class_library.cpp
    class_netlist_object.cpp
    cmp_library_keywords.cpp
    cmp_library_lexer.cpp
    component_references_lister.cpp
    controle.cpp
    cross-probing.cpp
    ${EESCHEMA_DLGS}
    edit_component_in_schematic.cpp
    edit_bitmap.cpp
    edit_label.cpp
    eeredraw.cpp
    eeschema.cpp
    eeschema_config.cpp
    erc.cpp
    events_called_functions_for_edit.cpp
    files-io.cpp
    find.cpp
    getpart.cpp
    component_tree_search_container.cpp
    hierarch.cpp
    hotkeys.cpp
    libarch.cpp
    libedit.cpp
    libeditframe.cpp
    libedit_onleftclick.cpp
    libedit_onrightclick.cpp
    libedit_plot_component.cpp
    libedit_undo_redo.cpp
    lib_arc.cpp
    lib_bezier.cpp
    lib_circle.cpp
    lib_collectors.cpp
    lib_draw_item.cpp
    lib_export.cpp
    lib_field.cpp
    lib_pin.cpp
    lib_polyline.cpp
    lib_rectangle.cpp
    lib_text.cpp
    libfield.cpp
    load_one_schematic_file.cpp
    menubar.cpp
    menubar_libedit.cpp
    netform.cpp
    netlist.cpp
    onleftclick.cpp
    onrightclick.cpp
    operations_on_items_lists.cpp
    pinedit.cpp
    plot_schematic_DXF.cpp
    plot_schematic_HPGL.cpp
    plot_schematic_PS.cpp
    plot_schematic_PDF.cpp
    plot_schematic_SVG.cpp
    sch_base_frame.cpp
    sch_bitmap.cpp
    sch_bus_entry.cpp
    sch_collectors.cpp
    sch_component.cpp
    sch_field.cpp
    sch_item_struct.cpp
    sch_junction.cpp
    sch_line.cpp
    sch_marker.cpp
    sch_no_connect.cpp
    sch_screen.cpp
    sch_sheet.cpp
    sch_sheet_path.cpp
    sch_sheet_pin.cpp
    sch_text.cpp
    schedit.cpp
    schematic_undo_redo.cpp
    schframe.cpp
    selpart.cpp
    sheet.cpp
    sheetlab.cpp
    symbdraw.cpp
    symbedit.cpp
    template_fieldnames_keywords.cpp
    template_fieldnames.cpp
    tool_lib.cpp
    tool_sch.cpp
    tool_viewlib.cpp
    transform.cpp
    viewlib_frame.cpp
    viewlibs.cpp
    )


set( EESCHEMA_COMMON_SRCS
    ../common/dialogs/dialog_page_settings.cpp
    ../common/base_screen.cpp
    ../common/eda_text.cpp
    ../common/class_page_info.cpp
    ../common/base_units.cpp
    )


if( MINGW )
    # EESCHEMA_RESOURCES variable is set by the macro.
    mingw_resource_compiler( eeschema )
endif()

# Create a C++ compilable string initializer containing html text into a *.h file:
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/dialogs/dialog_bom_help_html.h
    COMMAND ${CMAKE_COMMAND}
        -DinputFile=${CMAKE_CURRENT_SOURCE_DIR}/dialogs/dialog_bom_help.html
        -DoutputFile=${CMAKE_CURRENT_SOURCE_DIR}/dialogs/dialog_bom_help_html.h
        -P ${CMAKE_MODULE_PATH}/Html2C.cmake
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/dialogs/dialog_bom_help.html
    COMMENT "creating ${CMAKE_CURRENT_SOURCE_DIR}/dialogs/dialog_bom_help_html.h
       from ${CMAKE_CURRENT_SOURCE_DIR}/dialogs/dialog_bom_help.html"
    )

set_source_files_properties( dialogs/dialog_bom.cpp
    PROPERTIES
        OBJECT_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/dialogs/dialog_bom_help_html.h
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
    set( MACOSX_BUNDLE_GUI_IDENTIFIER org.kicad-eda.kicad )
    set( MACOSX_BUNDLE_NAME eeschema )
endif()


add_executable( eeschema WIN32 MACOSX_BUNDLE
    ../common/single_top.cpp
    ../common/pgm_base.cpp
    ${EESCHEMA_RESOURCES}
    )
set_source_files_properties( ../common/single_top.cpp PROPERTIES
    COMPILE_DEFINITIONS "TOP_FRAME=FRAME_SCH;PGM_DATA_FILE_EXT=\"sch\";BUILD_KIWAY_DLL"
    )
target_link_libraries( eeschema
    #singletop  # replaces common, giving us restrictive control and link warnings.
    # There's way too much crap coming in from common yet.
    common
    bitmaps
    ${wxWidgets_LIBRARIES}
    )

# the DSO (KIFACE) housing the main eeschema code:
add_library( eeschema_kiface MODULE
    ${EESCHEMA_SRCS}
    ${EESCHEMA_COMMON_SRCS}
    )
target_link_libraries( eeschema_kiface
    common
    bitmaps
    polygon
    ${wxWidgets_LIBRARIES}
    ${GDI_PLUS_LIBRARIES}
    )
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
        LINK_FLAGS "${TO_LINKER},-cref ${TO_LINKER},-Map=${KIFACE_PREFIX}eeschema${KIFACE_SUFFIX}.map"
        )
    set_target_properties( eeschema PROPERTIES
        LINK_FLAGS "${TO_LINKER},-cref ${TO_LINKER},-Map=eeschema.map"
        )
endif()

# these 2 binaries are a matched set, keep them together:
if( APPLE )
    set_target_properties( eeschema PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist
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
    ${CMAKE_CURRENT_SOURCE_DIR}/cmp_library.keywords
    ${CMAKE_CURRENT_SOURCE_DIR}/cmp_library_lexer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/cmp_library_keywords.cpp
    TLIB_T
    )

add_custom_target(
    cmp_library_lexer_source_files ALL
    DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/cmp_library_lexer.h
        ${CMAKE_CURRENT_SOURCE_DIR}/cmp_library_keywords.cpp
    )

add_dependencies( eeschema_kiface cmp_library_lexer_source_files )

make_lexer(
    ${CMAKE_CURRENT_SOURCE_DIR}/template_fieldnames.keywords
    ${CMAKE_CURRENT_SOURCE_DIR}/template_fieldnames_lexer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/template_fieldnames_keywords.cpp
    TFIELD_T

    # Pass header file with dependency on *_lexer.h as extra_arg
    template_fieldnames.h
    )

add_custom_target(
    field_template_lexer_source_files ALL
    DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/template_fieldnames_lexer.h
        ${CMAKE_CURRENT_SOURCE_DIR}/template_fieldnames_keywords.cpp
    )

add_dependencies( eeschema_kiface field_template_lexer_source_files )

make_lexer(
    ${CMAKE_CURRENT_SOURCE_DIR}/dialogs/dialog_bom_cfg.keywords
    ${CMAKE_CURRENT_SOURCE_DIR}/dialogs/dialog_bom_cfg_lexer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/dialogs/dialog_bom_cfg_keywords.cpp
    T_BOMCFG_T

    # Pass header file with dependency on *_lexer.h as extra_arg
    dialogs/dialog_bom_cfg.h
    )

add_custom_target(
    dialog_bom_cfg_lexer_source_files ALL
    DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/dialogs/dialog_bom_cfg_lexer.h
        ${CMAKE_CURRENT_SOURCE_DIR}/dialogs/dialog_bom_cfg_keywords.cpp
    )

add_dependencies( eeschema_kiface dialog_bom_cfg_lexer_source_files )

add_subdirectory( plugins )
