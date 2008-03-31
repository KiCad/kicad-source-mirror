add_definitions(-DEESCHEMA)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    bitmaps)

set(EESCHEMA_SRCS
    affiche.cpp
    annotate.cpp
    annotate_dialog.cpp
    block.cpp
    block_libedit.cpp
    busentry.cpp
    bus-wire-junction.cpp
    class_drawsheet.cpp
    class_screen.cpp
    class_text-label.cpp
    cleanup.cpp
    cmpclass.cpp
    component_class.cpp
    controle.cpp
    cross-probing.cpp
    dangling_ends.cpp
    database.cpp
    delete.cpp
    delsheet.cpp
    dialog_build_BOM.cpp
#   dialog_cmp_graphic_properties.cpp
    dialog_create_component.cpp
#   dialog_edit_component_in_lib.cpp
#   dialog_edit_component_in_schematic.cpp
#   dialog_edit_label.cpp
    dialog_eeschema_config.cpp
    dialog_erc.cpp
#   dialog_find.cpp
    dialog_options.cpp
#   divers.cpp
    edit_component_in_lib.cpp
    edit_component_in_schematic.cpp
    edit_label.cpp
    eeconfig.cpp
#   eecreate.cpp
    eelayer.cpp
    eelibs_draw_components.cpp
    eelibs_read_libraryfiles.cpp
    eeload.cpp
    eeredraw.cpp
    eeschema.cpp
    eestring.cpp
    erc.cpp
    files-io.cpp
    find.cpp
    getpart.cpp
    hierarch.cpp
    hotkeys.cpp
    libalias.cpp
    libarch.cpp
    libclass.cpp
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
    ../share/drawframe.cpp
    ../share/drawpanel.cpp
    ../share/setpage.cpp
    ../share/svg_print.cpp
    ../share/wxprint.cpp
    ../share/zoom.cpp)

if(WIN32)
    if(MINGW)
        # resource compilation for mingw (http://www.cmake.org/Bug/view.php?id=4068)
        add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/eeschema_rc.o
            COMMAND windres.exe -I ${CMAKE_CURRENT_SOURCE_DIR} -I ${wxWidgets_ROOT_DIR}/include
                        -i ${CMAKE_CURRENT_SOURCE_DIR}/eeschema.rc
                        -o ${CMAKE_CURRENT_BINARY_DIR}/eeschema_rc.o)
        set(EESCHEMA_RESOURCES ${CMAKE_CURRENT_BINARY_DIR}/eeschema_rc.o)
    else(MINGW)
        set(EESCHEMA_RESOURCES eeschema.rc)
    endif(MINGW)
endif(WIN32)
if(APPLE)
    set(EESCHEMA_RESOURCES eeschema.r)
endif(APPLE)

add_executable(eeschema WIN32 MACOSX_BUNDLE ${EESCHEMA_SRCS} ${EESCHEMA_EXTRA_SRCS} ${EESCHEMA_RESOURCES})

target_link_libraries(eeschema common ${wxWidgets_LIBRARIES})

install(TARGETS eeschema RUNTIME DESTINATION ${KICAD_BIN}
    COMPONENT binary)

add_subdirectory(plugins)
