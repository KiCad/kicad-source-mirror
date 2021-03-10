# Add all the warnings to the files
if( COMPILER_SUPPORTS_WARNINGS )
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WARN_FLAGS_CXX}")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${WARN_FLAGS_C}")
endif()

add_definitions( -DCVPCB )

include_directories( BEFORE ${INC_BEFORE} )
include_directories(
    ./dialogs
    ${CMAKE_SOURCE_DIR}/3d-viewer
    ${CMAKE_SOURCE_DIR}/pcbnew
    ${CMAKE_SOURCE_DIR}/pcbnew/dialogs
    ${CMAKE_SOURCE_DIR}/common
    ${INC_AFTER}
    )


set( CVPCB_DIALOGS
    dialogs/fp_conflict_assignment_selector_base.cpp
    dialogs/fp_conflict_assignment_selector.cpp
    dialogs/dialog_config_equfiles_base.cpp
    dialogs/dialog_config_equfiles.cpp
    )

set( CVPCB_SRCS
    ${CMAKE_SOURCE_DIR}/common/base_units.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/board_stackup_manager/stackup_predefined_prms.cpp
    auto_associate.cpp
    components_listbox.cpp
    display_footprints_frame.cpp
    footprints_listbox.cpp
    library_listbox.cpp
    cvpcb_mainframe.cpp
    cvpcb_settings.cpp
    listbox_base.cpp
    menubar.cpp
    readwrite_dlgs.cpp
    toolbars_cvpcb.cpp
    tools/cvpcb_actions.cpp
    tools/cvpcb_association_tool.cpp
    tools/cvpcb_control.cpp
    tools/cvpcb_fpviewer_selection_tool.cpp
    )


if( MINGW )
    # CVPCB_RESOURCES variable is set by the macro.
    mingw_resource_compiler( cvpcb )
else()
    set( CVPCB_RESOURCES ${CMAKE_SOURCE_DIR}/resources/msw/cvpcb.rc )
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
    3d-viewer
    gal
    common
    kimath
    ${PCBNEW_IO_LIBRARIES}
    ${wxWidgets_LIBRARIES}
    ${GDI_PLUS_LIBRARIES}
    )

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
