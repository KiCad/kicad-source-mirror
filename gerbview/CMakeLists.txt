# Add all the warnings to the files
if( COMPILER_SUPPORTS_WARNINGS )
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WARN_FLAGS_CXX}")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${WARN_FLAGS_C}")
endif()

add_definitions(-DGERBVIEW)

include_directories( BEFORE ${INC_BEFORE} )
include_directories(
    dialogs
    ${CMAKE_SOURCE_DIR}/common
    ${CMAKE_SOURCE_DIR}/3d-viewer
    ${INC_AFTER}
    )


set( DIALOGS_SRCS
    dialogs/panel_gerbview_display_options.cpp
    dialogs/panel_gerbview_display_options_base.cpp
    dialogs/panel_gerbview_settings.cpp
    dialogs/panel_gerbview_settings_base.cpp
    dialogs/dialog_layers_select_to_pcb.cpp
    dialogs/dialog_layers_select_to_pcb_base.cpp
    dialogs/dialog_print_gerbview.cpp
    dialogs/dialog_select_one_pcb_layer.cpp
    )

set( WIDGET_SRCS
    widgets/dcode_selection_box.cpp
    widgets/gbr_layer_box_selector.cpp
    widgets/gerbview_layer_widget.cpp
    widgets/layer_widget.cpp
    )

set( GERBVIEW_SRCS
    am_param.cpp
    am_primitive.cpp
    gbr_layout.cpp
    gerber_file_image.cpp
    gerber_file_image_list.cpp
    gerber_draw_item.cpp
    gerbview_printout.cpp
    X2_gerber_attributes.cpp
    clear_gbr_drawlayers.cpp
    dcode.cpp
    evaluate.cpp
    events_called_functions.cpp
    excellon_read_drill_file.cpp
    export_to_pcbnew.cpp
    files.cpp
    gerbview_settings.cpp
    gerbview_frame.cpp
    job_file_reader.cpp
    menubar.cpp
    readgerb.cpp
    rs274_read_XY_and_IJ_coordinates.cpp
    rs274d.cpp
    rs274x.cpp
    toolbars_gerber.cpp

    gerbview_draw_panel_gal.cpp
    gerbview_painter.cpp

    tools/gerbview_actions.cpp
    tools/gerbview_inspection_tool.cpp
    tools/gerbview_selection.cpp
    tools/gerbview_selection_tool.cpp
    tools/gerbview_control.cpp
    gerber_collectors.cpp
    )

set( GERBVIEW_EXTRA_SRCS
    ${CMAKE_SOURCE_DIR}/common/base_screen.cpp
    ${CMAKE_SOURCE_DIR}/common/base_units.cpp
    ${CMAKE_SOURCE_DIR}/common/eda_text.cpp
    ${CMAKE_SOURCE_DIR}/common/widgets/layer_box_selector.cpp
    ${CMAKE_SOURCE_DIR}/common/lset.cpp
    ${CMAKE_SOURCE_DIR}/common/page_info.cpp
)

if( MINGW )
    # GERBVIEW_RESOURCES variable is set by the macro.
    mingw_resource_compiler( gerbview )
else()
    set( GERBVIEW_RESOURCES ${CMAKE_SOURCE_DIR}/resources/msw/gerbview.rc )
endif()

if( APPLE )
    # setup bundle
    set( GERBVIEW_RESOURCES gerbview.icns gerbview_doc.icns )
    set_source_files_properties( "${CMAKE_CURRENT_SOURCE_DIR}/gerbview.icns" PROPERTIES
        MACOSX_PACKAGE_LOCATION Resources
        )
    set_source_files_properties( "${CMAKE_CURRENT_SOURCE_DIR}/gerbview_doc.icns" PROPERTIES
        MACOSX_PACKAGE_LOCATION Resources
        )
    set( MACOSX_BUNDLE_ICON_FILE gerbview.icns )
    set( MACOSX_BUNDLE_GUI_IDENTIFIER org.kicad-pcb.kicad )
    set( MACOSX_BUNDLE_NAME gerbview )
endif()

add_executable( gerbview WIN32 MACOSX_BUNDLE
    ${CMAKE_SOURCE_DIR}/common/single_top.cpp
    ${CMAKE_SOURCE_DIR}/common/pgm_base.cpp
    ${GERBVIEW_RESOURCES}
    )
set_source_files_properties( ${CMAKE_SOURCE_DIR}/common/single_top.cpp PROPERTIES
    COMPILE_DEFINITIONS "TOP_FRAME=FRAME_GERBER;BUILD_KIWAY_DLL"
    )
target_link_libraries( gerbview
    #singletop  # replaces common, giving us restrictive control and link warnings.
    # There's way too much crap coming in from common yet.
    gal
    common
    nlohmann_json
    ${wxWidgets_LIBRARIES}
    )

if( MAKE_LINK_MAPS )
    set_target_properties( gerbview PROPERTIES
        LINK_FLAGS "-Wl,-cref,-Map=gerbview.map" )
endif()


# The objects for the main gerbview program
add_library( gerbview_kiface_objects OBJECT
    gerbview.cpp
    ${GERBVIEW_SRCS}
    ${DIALOGS_SRCS}
    ${WIDGET_SRCS}
    ${GERBVIEW_EXTRA_SRCS}
    )

# Since we're not using target_link_libraries, we need to explicitly
# declare the dependency
add_dependencies( gerbview_kiface_objects common )
add_dependencies( gerbview_kiface_objects gal )

# CMake <3.9 can't link anything to object libraries,
# but we only need include directories, as we will link the kiface MODULE
target_include_directories( gerbview_kiface_objects PRIVATE
    $<TARGET_PROPERTY:common,INTERFACE_INCLUDE_DIRECTORIES>
    )

# the main gerbview program, in DSO form.
add_library( gerbview_kiface MODULE $<TARGET_OBJECTS:gerbview_kiface_objects> )
set_target_properties( gerbview_kiface PROPERTIES
    OUTPUT_NAME     gerbview
    PREFIX          ${KIFACE_PREFIX}
    SUFFIX          ${KIFACE_SUFFIX}
    )
target_link_libraries( gerbview_kiface
    gal
    common
    ${wxWidgets_LIBRARIES}
    ${GDI_PLUS_LIBRARIES}
    )
set_source_files_properties( gerbview.cpp PROPERTIES
    # The KIFACE is in gerbview.cpp, export it:
    COMPILE_DEFINITIONS     "BUILD_KIWAY_DLL;COMPILING_DLL"
    )

if( MAKE_LINK_MAPS )
    set_target_properties( gerbview_kiface PROPERTIES
        LINK_FLAGS "-Wl,-cref,-Map=_gerbview.kiface.map" )
endif()

target_include_directories( gerbview_kiface PRIVATE
    $<TARGET_PROPERTY:nlohmann_json,INTERFACE_INCLUDE_DIRECTORIES>
    )

# if building gerbview, then also build gerbview_kiface if out of date.
add_dependencies( gerbview gerbview_kiface )

# these 2 binaries are a matched set, keep them together
if( APPLE )
    set_target_properties( gerbview PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST ${PROJECT_BINARY_DIR}/gerbview/Info.plist
        )

    # puts binaries into the *.app bundle while linking
    set_target_properties( gerbview_kiface PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${OSX_BUNDLE_BUILD_KIFACE_DIR}
        )
    # put individual bundle outside of main bundle as a first step
    # will be pulled into the main bundle when creating main bundle
    install( TARGETS gerbview
        DESTINATION ${KICAD_BIN}
        COMPONENT binary
        )
    install( CODE "
            # override default embedded path settings
            ${OSX_BUNDLE_OVERRIDE_PATHS}

            # do all the work
            include( BundleUtilities )
            fixup_bundle( ${KICAD_BIN}/gerbview.app/Contents/MacOS/gerbview
                \"\"
                \"\"
                )
            " COMPONENT Runtime
        )
else()
    install( TARGETS gerbview
        DESTINATION ${KICAD_BIN}
        COMPONENT binary
        )
    install( TARGETS gerbview_kiface
        DESTINATION ${KICAD_BIN}
        COMPONENT binary
        )
endif()


if( KICAD_WIN32_INSTALL_PDBS )
    # Get the PDBs to copy over for MSVC
    install(FILES $<TARGET_PDB_FILE:gerbview> DESTINATION ${KICAD_BIN})
    install(FILES $<TARGET_PDB_FILE:gerbview_kiface> DESTINATION ${KICAD_BIN})
endif()