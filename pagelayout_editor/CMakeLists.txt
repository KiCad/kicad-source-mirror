add_definitions(-DPL_EDITOR)

###
# Includes
###

include_directories(BEFORE ${INC_BEFORE})
include_directories(
    dialogs
    ../common/dialogs
    ${INC_AFTER}
    )


###
# Sources
###

set(PL_EDITOR_SRCS
    dialogs/properties_frame_base.cpp
    dialogs/dialogs_for_printing.cpp
    dialogs/dialog_new_dataitem_base.cpp
    dialogs/dialog_new_dataitem.cpp
    class_pl_editor_screen.cpp
    class_pl_editor_layout.cpp
    design_tree_frame.cpp
    events_functions.cpp
    controle.cpp
    files.cpp
    onleftclick.cpp
    onrightclick.cpp
    page_layout_writer.cpp
    pl_editor.cpp
    pl_editor_config.cpp
    pl_editor_frame.cpp
    properties_frame.cpp
    hotkeys.cpp
    menubar.cpp
    toolbars_pl_editor.cpp
    )

###
# We need some extra sources from common
###
set(PL_EDITOR_EXTRA_SRCS
    ../common/base_screen.cpp
    ../common/base_units.cpp
    ../common/eda_text.cpp
    ../common/class_page_info.cpp
    ../common/dialogs/dialog_page_settings.cpp
)

###
# Windows resource file
###
if(WIN32)
    if(MINGW)
        # PL_EDITOR_RESOURCES variable is set by the macro.
        mingw_resource_compiler(pl_editor)
    else(MINGW)
        set(PL_EDITOR_RESOURCES pl_editor.rc)
    endif(MINGW)
endif(WIN32)

###
# Apple resource files
###
if(APPLE)
    set(PL_EDITOR_RESOURCES pl_editor.icns pl_editor_doc.icns)
    set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/pl_editor.icns"
                                PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/pl_editor_doc.icns"
                                PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    set(MACOSX_BUNDLE_ICON_FILE pl_editor.icns)
    set(MACOSX_BUNDLE_GUI_IDENTIFIER org.kicad-eda.pl_editor)
endif(APPLE)

###
# Create the pl_editor executable
###
add_executable(pl_editor WIN32 MACOSX_BUNDLE
               ${PL_EDITOR_SRCS}
               ${DIALOGS_SRCS}
               ${PL_EDITOR_EXTRA_SRCS}
               ${PL_EDITOR_RESOURCES})

###
# Set properties for APPLE on pl_editor target
###
if(APPLE)
    set_target_properties(pl_editor PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist)
endif(APPLE)

###
# Link executable target pl_editor with correct libraries
###
target_link_libraries(pl_editor common polygon bitmaps
                      ${OPENGL_LIBRARIES}
                      ${wxWidgets_LIBRARIES}
                      ${GDI_PLUS_LIBRARIES})

###
# Add pl_editor as install target
###
install(TARGETS pl_editor
        DESTINATION ${KICAD_BIN}
        COMPONENT binary)
