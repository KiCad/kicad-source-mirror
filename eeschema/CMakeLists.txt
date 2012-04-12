add_definitions(-DEESCHEMA)


include_directories(BEFORE ${INC_BEFORE})
include_directories(
    ./dialogs
    ../common
    ../common/dialogs
    ${Boost_INCLUDE_DIR}
    ${INC_AFTER}
    )

set(EESCHEMA_SRCS
    annotate.cpp
    backanno.cpp
    block.cpp
    block_libedit.cpp
    build_BOM.cpp
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
    dangling_ends.cpp
    database.cpp
    dialogs/dialog_color_config.cpp
    dialogs/dialog_plot_schematic_DXF.cpp
    dialogs/dialog_plot_schematic_DXF_base.cpp
    dialogs/dialog_plot_schematic_HPGL.cpp
    dialogs/dialog_plot_schematic_HPGL_base.cpp
    dialogs/dialog_plot_schematic_PS.cpp
    dialogs/dialog_plot_schematic_PS_base.cpp
    dialogs/annotate_dialog.cpp
    dialogs/dialog_annotate_base.cpp
    dialogs/dialog_lib_edit_text.cpp
    dialogs/dialog_lib_edit_text_base.cpp
    dialogs/dialog_build_BOM.cpp
    dialogs/dialog_build_BOM_base.cpp
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
    dialogs/dialog_lib_edit_pin.cpp
    dialogs/dialog_lib_edit_pin_base.cpp
    dialogs/dialog_lib_new_component.cpp
    dialogs/dialog_lib_new_component_base.cpp
    dialogs/dialog_print_using_printer_base.cpp
    dialogs/dialog_print_using_printer.cpp
    dialogs/dialog_sch_edit_sheet_pin.cpp
    dialogs/dialog_sch_edit_sheet_pin_base.cpp
    dialogs/dialog_sch_sheet_props.cpp
    dialogs/dialog_sch_sheet_props_base.cpp
    dialogs/dialog_schematic_find.cpp
    dialogs/dialog_schematic_find_base.cpp
    dialogs/dialog_SVG_print.cpp
    dialogs/dialog_SVG_print_base.cpp
    edit_component_in_schematic.cpp
    edit_bitmap.cpp
    edit_label.cpp
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
    netlist_control.cpp
    netlist.cpp
    onleftclick.cpp
    onrightclick.cpp
    operations_on_items_lists.cpp
    pinedit.cpp
    sch_base_frame.cpp
    sch_bitmap.cpp
    sch_bus_entry.cpp
    sch_collectors.cpp
    sch_component.cpp
    sch_field.cpp
    sch_junction.cpp
    sch_line.cpp
    sch_marker.cpp
    sch_no_connect.cpp
    sch_polyline.cpp
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

    # This file does not look common.
    # Keep it after template_fieldnames_keywords.cpp
    ../common/sch_item_struct.cpp
    )


set(EESCHEMA_COMMON_SRCS
    ../common/dialogs/dialog_page_settings.cpp
    ../common/base_screen.cpp
    ../common/eda_text.cpp
    ../common/class_page_info.cpp
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
    set(MACOSX_BUNDLE_ICON_FILE eeschema.icns)
    set(MACOSX_BUNDLE_GUI_IDENTIFIER org.kicad-eda.eeschema)
endif(APPLE)


# auto-generate cmp_library_lexer.h and cmp_library_keywords.cpp for the component
# library format.
make_lexer(
   ${CMAKE_CURRENT_SOURCE_DIR}/cmp_library.keywords
   ${CMAKE_CURRENT_SOURCE_DIR}/cmp_library_lexer.h
   ${CMAKE_CURRENT_SOURCE_DIR}/cmp_library_keywords.cpp
   TLIB_T
   )

make_lexer(
    ${CMAKE_CURRENT_SOURCE_DIR}/template_fieldnames.keywords
    ${CMAKE_CURRENT_SOURCE_DIR}/template_fieldnames_lexer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/template_fieldnames_keywords.cpp
    TFIELD_T
    )

add_executable(eeschema WIN32 MACOSX_BUNDLE
    ${EESCHEMA_SRCS}
    ${EESCHEMA_COMMON_SRCS}
    ${EESCHEMA_RESOURCES}
    )

if(APPLE)
    set_target_properties(eeschema PROPERTIES MACOSX_BUNDLE_INFO_PLIST
        ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist)
endif(APPLE)

target_link_libraries(eeschema
    common
    bitmaps
    polygon
    kbool
    ${wxWidgets_LIBRARIES}
    ${GDI_PLUS_LIBRARIES}
    )

install(TARGETS eeschema
        DESTINATION ${KICAD_BIN}
        COMPONENT binary)

add_subdirectory(plugins)
