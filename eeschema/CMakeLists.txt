add_definitions(-DEESCHEMA)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}
                    ${Boost_INCLUDE_DIR}
                   )

set(EESCHEMA_SRCS
    affiche.cpp
    annotate.cpp
    annotate_dialog.cpp
    backanno.cpp
    block.cpp
    block_libedit.cpp
    build_BOM.cpp
    busentry.cpp
    bus-wire-junction.cpp
    class_drawsheet.cpp
    class_hierarchical_PIN_sheet.cpp
    class_library.cpp
    class_pin.cpp
    class_sch_cmp_field.cpp
    class_schematic_items.cpp
    class_screen.cpp
    class_text-label.cpp
    classes_body_items.cpp
    cleanup.cpp
    component_class.cpp
    controle.cpp
    cross-probing.cpp
    dangling_ends.cpp
    database.cpp
    delete.cpp
    delsheet.cpp
#   dialog_backanno.cpp
    dialog_build_BOM.cpp
#   dialog_cmp_graphic_properties.cpp
    dialog_create_component.cpp
#   dialog_edit_component_in_lib.cpp
    dialog_edit_component_in_schematic_fbp.cpp
    dialog_edit_component_in_schematic.cpp
#   dialog_edit_label.cpp
    dialog_eeschema_config.cpp
    dialog_erc.cpp
#   dialog_find.cpp
    dialog_options.cpp
    edit_component_in_lib.cpp
    edit_component_in_schematic.cpp
    edit_label.cpp
    eeconfig.cpp
#   eecreate.cpp
    eelayer.cpp
    eelibs_draw_components.cpp
    eelibs_read_libraryfiles.cpp
    eeredraw.cpp
    eeschema.cpp
    erc.cpp
    files-io.cpp
    find.cpp
    getpart.cpp
    hierarch.cpp
    hotkeys.cpp
    libalias.cpp
    libarch.cpp
    libedit.cpp
    libedit_onleftclick.cpp
    libedit_onrightclick.cpp
    libedit_undo_redo.cpp
    lib_export.cpp
    libfield.cpp
    libframe.cpp
    load_one_schematic_file.cpp
    locate.cpp
    menubar.cpp
    netform.cpp
    netlist_control.cpp
    netlist.cpp
    onleftclick.cpp
    onrightclick.cpp
    pinedit.cpp
#   pinedit-dialog.cpp
    plot.cpp
    plothpgl.cpp
    plotps.cpp
    priorque.cpp
    savelib.cpp
    save_schemas.cpp
    schedit.cpp
    schematic_undo_redo.cpp
    schframe.cpp
    selpart.cpp
    sheet.cpp
    sheetlab.cpp
    symbdraw.cpp
    symbedit.cpp
    symbtext.cpp
    tool_lib.cpp
    tool_sch.cpp
    tool_viewlib.cpp
    viewlib_frame.cpp
    viewlibs.cpp)

set(EESCHEMA_EXTRA_SRCS
    ../share/setpage.cpp
    ../share/svg_print.cpp
    ../share/wxprint.cpp)

if(WIN32)
    if(MINGW)
        # EESCHEMA_RESOURCES variable is set by the macro.
        mingw_resource_compiler(eeschema)
    else(MINGW)
        set(EESCHEMA_RESOURCES eeschema.rc)
    endif(MINGW)
endif(WIN32)

if(APPLE)
    set(EESCHEMA_RESOURCES eeschema.icns)
    set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/eeschema.icns"
                                PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    set(MACOSX_BUNDLE_ICON_FILE eeschema.icns)
    set(MACOSX_BUNDLE_GUI_IDENTIFIER org.kicad-eda.eeschema)
    set(MACOSX_BUNDLE_NAME eeschema)
endif(APPLE)

add_executable(eeschema WIN32 MACOSX_BUNDLE ${EESCHEMA_SRCS} ${EESCHEMA_EXTRA_SRCS} ${EESCHEMA_RESOURCES})

target_link_libraries(eeschema common bitmaps ${wxWidgets_LIBRARIES})

install(TARGETS eeschema
        DESTINATION ${KICAD_BIN}
        COMPONENT binary)

add_subdirectory(plugins)
