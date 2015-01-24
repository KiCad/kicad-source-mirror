# the map generation creates on Windows/gcc a lot of useless warnings
# so disable it on windows
if( WIN32 AND NOT CMAKE_CROSSCOMPILING )
    set( MAKE_LINK_MAPS false )
else()
    set( MAKE_LINK_MAPS true )
endif()

add_definitions( -DCVPCB )

if( KICAD_USE_WEBKIT AND BUILD_GITHUB_PLUGIN )
    set( WEBVIEWER_WXLIB "webviewer" )
endif()


include_directories( BEFORE ${INC_BEFORE} )
include_directories(
    ./dialogs
    ../3d-viewer
    ../pcbnew
    ../pcbnew/dialogs
    ../polygon
    ../common
    ${INC_AFTER}
    )


set( CVPCB_DIALOGS
    dialogs/fp_conflict_assignment_selector_base.cpp
    dialogs/fp_conflict_assignment_selector.cpp
    dialogs/dialog_display_options.cpp
    dialogs/dialog_display_options_base.cpp
    dialogs/dialog_config_equfiles_base.cpp
    dialogs/dialog_config_equfiles.cpp
    ../pcbnew/dialogs/dialog_fp_lib_table.cpp
    ../pcbnew/dialogs/dialog_fp_lib_table_base.cpp
    ../pcbnew/dialogs/dialog_fp_plugin_options.cpp
    ../pcbnew/dialogs/dialog_fp_plugin_options_base.cpp
    ../pcbnew/dialogs/wizard_add_fplib_base.cpp
    ../pcbnew/dialogs/wizard_add_fplib.cpp
    ../pcbnew/dialogs/dialog_select_dirlist_base.cpp
    )

set( CVPCB_SRCS
    ../common/base_units.cpp
    ../pcbnew/board_items_to_polygon_shape_transform.cpp
    ../pcbnew/class_drc_item.cpp
    autosel.cpp
    cfg.cpp
    class_components_listbox.cpp
    class_DisplayFootprintsFrame.cpp
    class_footprints_listbox.cpp
    class_library_listbox.cpp
    cvframe.cpp
    listboxes.cpp
    menubar.cpp
    readwrite_dlgs.cpp
    tool_cvpcb.cpp
    )


if( MINGW )
    # CVPCB_RESOURCES variable is set by the macro.
    mingw_resource_compiler( cvpcb )
endif()


if( APPLE )
    # setup bundle
    set( CVPCB_RESOURCES cvpcb.icns cvpcb_doc.icns )
    set_source_files_properties( "${CMAKE_CURRENT_SOURCE_DIR}/cvpcb.icns" PROPERTIES
        MACOSX_PACKAGE_LOCATION Resources
        )
    set_source_files_properties( "${CMAKE_CURRENT_SOURCE_DIR}/cvpcb_doc.icns" PROPERTIES
        MACOSX_PACKAGE_LOCATION Resources
        )
    set( MACOSX_BUNDLE_ICON_FILE cvpcb.icns )
    set( MACOSX_BUNDLE_GUI_IDENTIFIER org.kicad-eda.kicad )
    set( MACOSX_BUNDLE_NAME cvpcb )
endif()


add_executable( cvpcb WIN32 MACOSX_BUNDLE
    ../common/single_top.cpp
    ../common/pgm_base.cpp
    ${CVPCB_RESOURCES}
    )

set_source_files_properties( ../common/single_top.cpp PROPERTIES
    COMPILE_DEFINITIONS "TOP_FRAME=FRAME_CVPCB;PGM_DATA_FILE_EXT=\"net\";BUILD_KIWAY_DLL"
    )

target_link_libraries( cvpcb
    #singletop  # replaces common, giving us restrictive control and link warnings.
    # There's way too much crap coming in from common yet.
    common
    bitmaps
    ${wxWidgets_LIBRARIES}
    )

if( MAKE_LINK_MAPS )
    set_target_properties( cvpcb PROPERTIES
        LINK_FLAGS "${TO_LINKER},-cref ${TO_LINKER},-Map=cvpcb.map" )
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
    3d-viewer
    pcbcommon
    pcad2kicadpcb
    common
    ${WEBVIEWER_WXLIB}   # before bitmaps, because it uses some bitmaps defined in bitmaps
    bitmaps
    polygon
    gal
    ${wxWidgets_LIBRARIES}
    ${OPENGL_LIBRARIES}
    ${GDI_PLUS_LIBRARIES}
    ${GLEW_LIBRARIES}
    ${CAIRO_LIBRARIES}
    ${PIXMAN_LIBRARY}
    ${OPENMP_LIBRARIES}
    )

# Only for win32 cross compilation using MXE
if( WIN32 AND MSYS AND CMAKE_CROSSCOMPILING )
    target_link_libraries( cvpcb_kiface
        opengl32
        glu32
        pixman-1
        fontconfig
        freetype
        bz2
        )
endif()

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
        LINK_FLAGS "${TO_LINKER},-cref ${TO_LINKER},-Map=_cvpcb.kiface.map" )
endif()

# if building cvpcb, then also build cvpcb_kiface if out of date.
add_dependencies( cvpcb cvpcb_kiface )

# these 2 binaries are a matched set, keep them together:
if( APPLE )
    set_target_properties( cvpcb PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist
        )

    # puts binaries into the *.app bundle while linking
    set_target_properties( cvpcb_kiface PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${OSX_BUNDLE_BUILD_KIFACE_DIR}
        )
    # put individual bundle outside of main bundle as a first step
    # will be pulled into the main bundle when creating main bundle
    install( TARGETS cvpcb
        DESTINATION ${KICAD_BIN}
        COMPONENT binary
        )
    install( CODE "
            # override default embedded path settings
            ${OSX_BUNDLE_OVERRIDE_PATHS}

            # do all the work
            include( BundleUtilities )
            fixup_bundle( ${KICAD_BIN}/cvpcb.app/Contents/MacOS/cvpcb
                \"\"
                \"\"
                )
            " COMPONENT Runtime
        )
else()
    install( TARGETS cvpcb
        DESTINATION ${KICAD_BIN}
        COMPONENT binary
        )
    install( TARGETS cvpcb_kiface
        DESTINATION ${KICAD_BIN}
        COMPONENT binary
        )
endif()
