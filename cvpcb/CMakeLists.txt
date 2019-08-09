# .cpp files are compiled with extra ${WSHADOW_FLAGS}
if( COMPILER_SUPPORTS_WSHADOW )
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WSHADOW_FLAGS}")
endif()

add_definitions( -DCVPCB )

include_directories( BEFORE ${INC_BEFORE} )
include_directories(
    ./dialogs
    ../3d-viewer
    ../pcbnew
    ../pcbnew/dialogs
    ../common
    ${GLM_INCLUDE_DIR}
    ${INC_AFTER}
    )


set( CVPCB_DIALOGS
    dialogs/fp_conflict_assignment_selector_base.cpp
    dialogs/fp_conflict_assignment_selector.cpp
    dialogs/dialog_display_options.cpp
    dialogs/dialog_display_options_base.cpp
    dialogs/dialog_config_equfiles_base.cpp
    dialogs/dialog_config_equfiles.cpp
    )

set( CVPCB_SRCS
    ../common/base_units.cpp
    ../pcbnew/tools/grid_helper.cpp
    auto_associate.cpp
    cfg.cpp
    components_listbox.cpp
    display_footprints_frame.cpp
    footprints_listbox.cpp
    library_listbox.cpp
    cvpcb_mainframe.cpp
    listbox_base.cpp
    menubar.cpp
    readwrite_dlgs.cpp
    toolbars_cvpcb.cpp
    tools/cvpcb_actions.cpp
    tools/cvpcb_association_tool.cpp
    tools/cvpcb_control.cpp
    tools/cvpcb_fpviewer_control.cpp
    tools/cvpcb_fpviewer_selection_tool.cpp
    )


if( MINGW )
    # CVPCB_RESOURCES variable is set by the macro.
    mingw_resource_compiler( cvpcb )
else()
    set( CVPCB_RESOURCES cvpcb.rc )
endif()


if( false )
    # setup bundle
    set( CVPCB_RESOURCES cvpcb.icns cvpcb_doc.icns )
    set_source_files_properties( "${CMAKE_CURRENT_SOURCE_DIR}/cvpcb.icns" PROPERTIES
        MACOSX_PACKAGE_LOCATION Resources
        )
    set_source_files_properties( "${CMAKE_CURRENT_SOURCE_DIR}/cvpcb_doc.icns" PROPERTIES
        MACOSX_PACKAGE_LOCATION Resources
        )
    set( MACOSX_BUNDLE_ICON_FILE cvpcb.icns )
    set( MACOSX_BUNDLE_GUI_IDENTIFIER org.kicad-pcb.kicad )
    set( MACOSX_BUNDLE_NAME cvpcb )
endif()


# the main cvpcb program, in DSO form.
add_library( cvpcb_kiface MODULE
    cvpcb.cpp
    ${CVPCB_SRCS}
    ${CVPCB_DIALOGS}
    )

set_target_properties( cvpcb_kiface PROPERTIES
    OUTPUT_NAME     cvpcb
    PREFIX          ${KIFACE_PREFIX}
    SUFFIX          ${KIFACE_SUFFIX}
    )
target_link_libraries( cvpcb_kiface
    pcbcommon
    pcad2kicadpcb
    3d-viewer
    gal
    common
    ${wxWidgets_LIBRARIES}
    ${GDI_PLUS_LIBRARIES}
    )

if( BUILD_GITHUB_PLUGIN )
    target_link_libraries( cvpcb_kiface github_plugin )
endif()

# Must follow github_plugin
target_link_libraries( cvpcb_kiface ${Boost_LIBRARIES} )

if( UNIX AND NOT APPLE )
    # -lrt must follow Boost
    target_link_libraries( cvpcb_kiface rt )
endif()

set_source_files_properties( cvpcb.cpp PROPERTIES
    # The KIFACE is in cvpcb.cpp, export it:
    COMPILE_DEFINITIONS     "BUILD_KIWAY_DLL;COMPILING_DLL"
    )

if( MAKE_LINK_MAPS )
    set_target_properties( cvpcb_kiface PROPERTIES
        LINK_FLAGS "-Wl,-cref,-Map=_cvpcb.kiface.map" )
endif()

# these 2 binaries are a matched set, keep them together:
if( APPLE )
    # puts binaries into the *.app bundle while linking
    set_target_properties( cvpcb_kiface PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${OSX_BUNDLE_BUILD_KIFACE_DIR}
        )
else()
    install( TARGETS cvpcb_kiface
        DESTINATION ${KICAD_BIN}
        COMPONENT binary
        )
endif()
