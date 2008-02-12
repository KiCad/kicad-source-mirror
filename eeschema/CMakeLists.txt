ADD_DEFINITIONS(-DEESCHEMA)

INCLUDE_DIRECTORIES(
    ${CMAKE_CURRENT_SOURCE_DIR}
    bitmaps)

SET(EESCHEMA_SRCS
    affiche.cpp
    annotate.cpp
#   annotate_dialog.cpp
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

SET(EESCHEMA_EXTRA_SRCS
    ../share/drawframe.cpp
    ../share/drawpanel.cpp
    ../share/setpage.cpp
    ../share/svg_print.cpp
    ../share/wxprint.cpp
    ../share/zoom.cpp)

IF(WIN32)
    IF(MINGW)
        # resource compilation for mingw (http://www.cmake.org/Bug/view.php?id=4068)
        ADD_CUSTOM_COMMAND(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/eeschema_rc.o
            COMMAND windres.exe -I ${CMAKE_CURRENT_SOURCE_DIR} -I ${wxWidgets_ROOT_DIR}/include
                        -i ${CMAKE_CURRENT_SOURCE_DIR}/eeschema.rc
                        -o ${CMAKE_CURRENT_BINARY_DIR}/eeschema_rc.o)
        SET(EESCHEMA_RESOURCES ${CMAKE_CURRENT_BINARY_DIR}/eeschema_rc.o)
    ELSE(MINGW)
        SET(EESCHEMA_RESOURCES eeschema.rc)
    ENDIF(MINGW)
ENDIF(WIN32)
IF(APPLE)
    SET(EESCHEMA_RESOURCES eeschema.r)
ENDIF(APPLE)

ADD_EXECUTABLE(eeschema WIN32 MACOSX_BUNDLE ${EESCHEMA_SRCS} ${EESCHEMA_EXTRA_SRCS} ${EESCHEMA_RESOURCES})

TARGET_LINK_LIBRARIES(eeschema common ${wxWidgets_LIBRARIES})

INSTALL(TARGETS eeschema RUNTIME DESTINATION ${KICAD_BIN} COMPONENT binary)

ADD_SUBDIRECTORY(plugins)
