# Add all the warnings to the files
if( COMPILER_SUPPORTS_WARNINGS )
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WARN_FLAGS_CXX}")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${WARN_FLAGS_C}")
endif()

add_compile_definitions( CVPCB )

include_directories( BEFORE ${INC_BEFORE} )
include_directories(
    ./dialogs
    ${CMAKE_SOURCE_DIR}/3d-viewer
    ${CMAKE_SOURCE_DIR}/pcbnew
    ${CMAKE_SOURCE_DIR}/pcbnew/dialogs
    ${CMAKE_SOURCE_DIR}/common
    ${CMAKE_SOURCE_DIR}/common/dialogs
    ${INC_AFTER}
    )


set( CVPCB_DIALOGS
    dialogs/fp_conflict_assignment_selector_base.cpp
    dialogs/fp_conflict_assignment_selector.cpp
    dialogs/dialog_config_equfiles_base.cpp
    dialogs/dialog_config_equfiles.cpp
    )

set( CVPCB_SRCS
    ${CMAKE_SOURCE_DIR}/pcbnew/board_stackup_manager/stackup_predefined_prms.cpp
    ${CMAKE_SOURCE_DIR}/pcbnew/footprint_info_impl.cpp
    auto_associate.cpp
    symbols_listbox.cpp
    display_footprints_frame.cpp
    footprints_listbox.cpp
    library_listbox.cpp
    cvpcb_mainframe.cpp
    listbox_base.cpp
    menubar.cpp
    readwrite_dlgs.cpp
    toolbars_cvpcb.cpp
    toolbars_display_footprints.cpp
    tools/cvpcb_actions.cpp
    tools/cvpcb_association_tool.cpp
    tools/cvpcb_control.cpp
    tools/cvpcb_fpviewer_selection_tool.cpp
    )

if( WIN32 )
    if( MINGW )
        # CVPCB_RESOURCES variable is set by the macro.
        mingw_resource_compiler( cvpcb-dll )
    else()
        set( CVPCB_RESOURCES ${CMAKE_SOURCE_DIR}/resources/msw/cvpcb-dll.rc )
    endif()
endif()


# the main cvpcb program, in DSO form.
add_library( cvpcb_kiface MODULE
    cvpcb.cpp
    ${CVPCB_SRCS}
    ${CVPCB_DIALOGS}
    ${CVPCB_RESOURCES}
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
    markdown_lib
    ${PCBNEW_IO_LIBRARIES}
    Boost::headers
    ${wxWidgets_LIBRARIES}
    )

# -lrt must follow Boost on Linux platforms
target_link_libraries( cvpcb_kiface $<$<BOOL:${UNIX_NOT_APPLE}>:rt> )

set_source_files_properties( cvpcb.cpp PROPERTIES
    # The KIFACE is in cvpcb.cpp, export it:
    COMPILE_DEFINITIONS     "BUILD_KIWAY_DLL;COMPILING_DLL"
    )

target_link_options( cvpcb_kiface PRIVATE
    $<$<BOOL:${KICAD_MAKE_LINK_MAPS}>:-Wl,--cref,-Map=_cvpcb.kiface.map>
    )

# these 2 binaries are a matched set, keep them together:
if( APPLE )
    # puts binaries into the *.app bundle while linking
    set_target_properties( cvpcb_kiface PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${OSX_BUNDLE_BUILD_KIFACE_DIR}
        )
    set_target_properties( cvpcb_kiface PROPERTIES INSTALL_RPATH
            "@executable_path/../Frameworks;@executable_path/../Frameworks/Python.framework" )
    set_target_properties( cvpcb_kiface PROPERTIES BUILD_WITH_INSTALL_RPATH 1 )
else()
    install( TARGETS cvpcb_kiface
        DESTINATION ${KICAD_KIFACE}
        COMPONENT binary
        )
endif()

if( KICAD_WIN32_INSTALL_PDBS )
    # Get the PDBs to copy over for MSVC
    install(FILES $<TARGET_PDB_FILE:cvpcb_kiface> DESTINATION ${KICAD_KIFACE})
endif()
