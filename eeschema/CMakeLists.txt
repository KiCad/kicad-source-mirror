add_definitions(-DEESCHEMA)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}
                    ${CMAKE_SOURCE_DIR}/common
                    ${Boost_INCLUDE_DIR}
                   )

set(EESCHEMA_SRCS
    annotate.cpp
    annotate_dialog.cpp
    backanno.cpp
    block.cpp
    block_libedit.cpp
    build_BOM.cpp
    busentry.cpp
    bus-wire-junction.cpp
    class_BodyItem_Text.cpp
    class_drawsheet.cpp
    class_drawsheetpath.cpp
    class_drc_erc_item.cpp
    class_hierarchical_PIN_sheet.cpp
    class_libentry.cpp
    class_libentry_fields.cpp
    class_library.cpp
    class_marker_sch.cpp
    class_netlist_object.cpp
    class_pin.cpp
    class_sch_cmp_field.cpp
    class_sch_component.cpp
    class_schematic_items.cpp
    class_sch_screen.cpp
    class_text-label.cpp
    classes_body_items.cpp
    cleanup.cpp
    controle.cpp
    cross-probing.cpp
    dangling_ends.cpp
    database.cpp
    delete.cpp
    delsheet.cpp
    dialog_annotate_base.cpp
    dialog_bodygraphictext_properties_base.cpp
    dialog_build_BOM.cpp
    dialog_build_BOM_base.cpp
    dialog_edit_component_in_lib.cpp
    dialog_edit_component_in_lib_base.cpp
    dialog_edit_component_in_schematic_fbp.cpp
    dialog_edit_component_in_schematic.cpp
    dialog_edit_label.cpp
    dialog_edit_label_base.cpp
    dialog_edit_libentry_fields_in_lib.cpp
    dialog_edit_libentry_fields_in_lib_base.cpp
    dialog_eeschema_config.cpp
    dialog_eeschema_config_fbp.cpp
    dialog_eeschema_options_base.cpp
    dialog_eeschema_options.cpp
    dialog_erc.cpp
    dialog_erc_base.cpp
    dialog_lib_edit_draw_item.cpp
    dialog_lib_edit_draw_item_base.cpp
    dialog_lib_edit_pin.cpp
    dialog_lib_edit_pin_base.cpp
    dialog_lib_new_component.cpp
    dialog_lib_new_component_base.cpp
    dialog_print_using_printer_base.cpp
    dialog_print_using_printer.cpp
    dialog_sch_sheet_props.cpp
    dialog_sch_sheet_props_base.cpp
    dialog_schematic_find.cpp
    dialog_schematic_find_base.cpp
    dialog_SVG_print.cpp
    dialog_SVG_print_base.cpp
    edit_component_in_lib.cpp
    edit_component_in_schematic.cpp
    edit_label.cpp
    eelayer.cpp
    eelibs_draw_components.cpp
    eelibs_read_libraryfiles.cpp
    eeredraw.cpp
    eeschema.cpp
    eeschema_config.cpp
    erc.cpp
    events_called_functions_for_edit.cpp
    files-io.cpp
    find.cpp
    getpart.cpp
    hierarch.cpp
    hotkeys.cpp
    libarch.cpp
    libedit.cpp
    libeditframe.cpp
    libedit_onleftclick.cpp
    libedit_onrightclick.cpp
    libedit_undo_redo.cpp
    lib_export.cpp
    libfield.cpp
    load_one_schematic_file.cpp
    locate.cpp
    menubar.cpp
    netform.cpp
    netlist_control.cpp
    netlist.cpp
    onleftclick.cpp
    onrightclick.cpp
    operations_on_items_lists.cpp
    pinedit.cpp
    plot.cpp
    plothpgl.cpp
    plotps.cpp
    plotdxf.cpp
    read_from_file_schematic_items_descriptions.cpp
    save_schemas.cpp
    schedit.cpp
    schematic_undo_redo.cpp
    schframe.cpp
    selpart.cpp
    sheet.cpp
    sheetlab.cpp
    symbdraw.cpp
    symbedit.cpp
    edit_graphic_bodyitem_text.cpp
    tool_lib.cpp
    tool_sch.cpp
    tool_viewlib.cpp
    viewlib_frame.cpp
    viewlibs.cpp)

set(EESCHEMA_EXTRA_SRCS
    ../common/dialog_page_settings.cpp
    ../common/sch_item_struct.cpp
)

if(WIN32)
    if(MINGW)
        # EESCHEMA_RESOURCES variable is set by the macro.
        mingw_resource_compiler(eeschema)
    else(MINGW)
        set(EESCHEMA_RESOURCES eeschema.rc)
    endif(MINGW)
endif(WIN32)

if(APPLE)
    set(EESCHEMA_RESOURCES eeschema.icns eeschema_doc.icns)
    set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/eeschema.icns"
                                PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/eeschema_doc.icns"
                                PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    set(MACOSX_BUNDLE_ICON_FILE eeschema.icns)
    set(MACOSX_BUNDLE_GUI_IDENTIFIER org.kicad-eda.eeschema)
endif(APPLE)

add_executable(eeschema WIN32 MACOSX_BUNDLE ${EESCHEMA_SRCS} ${EESCHEMA_EXTRA_SRCS} ${EESCHEMA_RESOURCES})

if(APPLE)
    set_target_properties(eeschema PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist)
endif(APPLE)

target_link_libraries(eeschema common bitmaps polygon ${wxWidgets_LIBRARIES} ${GDI_PLUS_LIBRARIES})

install(TARGETS eeschema
        DESTINATION ${KICAD_BIN}
        COMPONENT binary)

add_subdirectory(plugins)
