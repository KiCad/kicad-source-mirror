# Add all the warnings to the files
if( COMPILER_SUPPORTS_WARNINGS )
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WARN_FLAGS_CXX}")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${WARN_FLAGS_C}")
endif()

add_compile_definitions( GERBVIEW )

include_directories( BEFORE ${INC_BEFORE} )
include_directories(
    dialogs
    ${CMAKE_SOURCE_DIR}/common
    ${CMAKE_SOURCE_DIR}/common/dialogs
    ${CMAKE_SOURCE_DIR}/3d-viewer
    ${INC_AFTER}
    )


set( DIALOGS_SRCS
    dialogs/panel_gerbview_color_settings.cpp
    dialogs/panel_gerbview_display_options.cpp
    dialogs/panel_gerbview_display_options_base.cpp
    dialogs/panel_gerbview_excellon_settings.cpp
    dialogs/panel_gerbview_excellon_settings_base.cpp
    dialogs/dialog_draw_layers_settings.cpp
    dialogs/dialog_draw_layers_settings_base.cpp
    dialogs/dialog_map_gerber_layers_to_pcb.cpp
    dialogs/dialog_map_gerber_layers_to_pcb_base.cpp
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
    aperture_macro.cpp
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
)

if( WIN32 )
    if( MINGW )
        # GERBVIEW_RESOURCES variable is set by the macro.
        mingw_resource_compiler( gerbview )
    else()
        set( GERBVIEW_RESOURCES ${CMAKE_SOURCE_DIR}/resources/msw/gerbview.rc )
    endif()
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
    set( MACOSX_BUNDLE_GUI_IDENTIFIER org.kicad.kicad )
    set( MACOSX_BUNDLE_NAME gerbview )
endif()

add_executable( gerbview WIN32 MACOSX_BUNDLE
    ${CMAKE_SOURCE_DIR}/common/single_top.cpp
    ${GERBVIEW_RESOURCES}
    )

set_source_files_properties( ${CMAKE_SOURCE_DIR}/common/single_top.cpp PROPERTIES
    COMPILE_DEFINITIONS "TOP_FRAME=FRAME_GERBER;BUILD_KIWAY_DLL"
    )
target_link_libraries( gerbview
    core
    kicommon
    nlohmann_json
    ${wxWidgets_LIBRARIES}
    )

target_link_options( gerbview PRIVATE
    $<$<BOOL:${KICAD_MAKE_LINK_MAPS}>:-Wl,--cref,-Map=gerbview.map>
    )

# The objects for the main gerbview program
add_library( gerbview_kiface_objects OBJECT
    gerbview.cpp
    ${GERBVIEW_SRCS}
    ${DIALOGS_SRCS}
    ${WIDGET_SRCS}
    ${GERBVIEW_EXTRA_SRCS}
    )

target_link_libraries( gerbview_kiface_objects
    PUBLIC
        common
        gal
        core
    )

message( STATUS "Including 3Dconnexion SpaceMouse navigation support in gerbview" )
add_subdirectory( navlib )

target_link_libraries( gerbview_kiface_objects PUBLIC gerbview_navlib)

add_dependencies( gerbview_kiface_objects gerbview_navlib )

# the main gerbview program, in DSO form.
add_library( gerbview_kiface MODULE )

set_target_properties( gerbview_kiface PROPERTIES
    OUTPUT_NAME     gerbview
    PREFIX          ${KIFACE_PREFIX}
    SUFFIX          ${KIFACE_SUFFIX}
    )

target_link_libraries( gerbview_kiface
    PRIVATE
        nlohmann_json
        gal
        kicommon
        common
        core
        gerbview_kiface_objects
        ${wxWidgets_LIBRARIES}
    )

if( MINGW )
    target_link_libraries( gerbview_kiface PUBLIC gerbview_navlib)
endif()

set_source_files_properties( gerbview.cpp PROPERTIES
    # The KIFACE is in gerbview.cpp, export it:
    COMPILE_DEFINITIONS     "BUILD_KIWAY_DLL;COMPILING_DLL"
    )

target_link_options( gerbview_kiface PRIVATE
    $<$<BOOL:${KICAD_MAKE_LINK_MAPS}>:-Wl,--cref,-Map=_gerbview.kiface.map>
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

    set_target_properties( gerbview PROPERTIES INSTALL_RPATH
            "@executable_path/../Frameworks;@executable_path/../Frameworks/Python.framework" )

    set_target_properties( gerbview_kiface PROPERTIES INSTALL_RPATH
            "@executable_path/../Frameworks;@executable_path/../Frameworks/Python.framework" )
    set_target_properties( gerbview_kiface PROPERTIES BUILD_WITH_INSTALL_RPATH 1 )

    # put individual bundle outside of main bundle as a first step
    # will be pulled into the main bundle when creating main bundle
    install( TARGETS gerbview
        DESTINATION ${KICAD_BIN}
        COMPONENT binary
        )

    install( CODE "
        set( KICAD_CMAKE_MODULE_PATH \"${KICAD_CMAKE_MODULE_PATH}\" )
        set( KICAD_BIN \"${KICAD_BIN}\" )
        set( OSX_BUNDLE_INSTALL_BIN_DIR \"${OSX_BUNDLE_INSTALL_BIN_DIR}\" )
        set( OSX_BUNDLE_INSTALL_LIB_DIR \"${OSX_BUNDLE_INSTALL_LIB_DIR}\" )
        set( OSX_BUNDLE_BUILD_KIFACE_DIR \"${OSX_BUNDLE_BUILD_KIFACE_DIR}\" )
    " )

    install( CODE [[
        include( ${KICAD_CMAKE_MODULE_PATH}/InstallSteps/InstallMacOS.cmake )

        # Install any dependencies (this will generally duplicate kicad.app but we can't be sure)
        install_runtime_deps( "${KICAD_BIN}/gerbview.app/Contents/MacOS/gerbview"
            "${OSX_BUNDLE_BUILD_KIFACE_DIR}/_gerbview.kiface"
            ""
            )
    ]] )
else()
    if( MSVC )
        target_sources( gerbview_kiface PRIVATE ${CMAKE_SOURCE_DIR}/resources/msw/gerbview-dll.rc )
    endif()

    install( TARGETS gerbview
        DESTINATION ${KICAD_BIN}
        COMPONENT binary
        )
    install( TARGETS gerbview_kiface
        DESTINATION ${KICAD_KIFACE}
        COMPONENT binary
        )
endif()


if( KICAD_WIN32_INSTALL_PDBS )
    # Get the PDBs to copy over for MSVC
    install(FILES $<TARGET_PDB_FILE:gerbview> DESTINATION ${KICAD_BIN})
    install(FILES $<TARGET_PDB_FILE:gerbview_kiface> DESTINATION ${KICAD_KIFACE})
endif()
