
include_directories( BEFORE ${INC_BEFORE} )
include_directories(
    ../potrace
    ../common
    ${INC_AFTER}
    )

set( BITMAP2COMPONENT_SRCS
    ../common/single_top.cpp
    bitmap2component.cpp
    bitmap2cmp_gui_base
    bitmap2cmp_gui
    )

set_source_files_properties( ../common/single_top.cpp PROPERTIES
    COMPILE_DEFINITIONS     "TOP_FRAME=FRAME_BM2CMP"
    )
set_source_files_properties( bitmap2cmp_gui.cpp PROPERTIES
    COMPILE_DEFINITIONS     "COMPILING_DLL"
    )

if( APPLE )
    # setup bundle
    set( BITMAP2COMPONENT_RESOURCES bitmap2component.icns )
    set_source_files_properties( "${CMAKE_CURRENT_SOURCE_DIR}/bitmap2component.icns" PROPERTIES
        MACOSX_PACKAGE_LOCATION Resources
        )
    set( MACOSX_BUNDLE_ICON_FILE bitmap2component.icns )
    set( MACOSX_BUNDLE_GUI_IDENTIFIER org.kicad-eda.kicad )
    set( MACOSX_BUNDLE_NAME bitmap2component )
endif()

add_executable( bitmap2component WIN32 MACOSX_BUNDLE
    ${BITMAP2COMPONENT_SRCS}
    ${BITMAP2COMPONENT_RESOURCES}
    )

target_link_libraries( bitmap2component
    common
    polygon
    bitmaps
    ${wxWidgets_LIBRARIES}
    potrace
    )

if( APPLE )
    set_target_properties( bitmap2component PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist
        )

    # put individual bundle outside of main bundle as a first step
    # will be pulled into the main bundle when creating main bundle
    install( TARGETS bitmap2component
        DESTINATION ${KICAD_BIN}
        COMPONENT binary
        )
    install( CODE "
        # override default embedded path settings
        ${OSX_BUNDLE_OVERRIDE_PATHS}

        # do all the work
        include( BundleUtilities )
        fixup_bundle( ${KICAD_BIN}/bitmap2component.app/Contents/MacOS/bitmap2component
            \"\"
            \"\"
            )
        " COMPONENT Runtime
        )
else()
    install( TARGETS bitmap2component
        DESTINATION ${KICAD_BIN}
        COMPONENT binary
        )
endif()


if( false )     # linker map with cross reference
    set_target_properties( bitmap2component PROPERTIES
        LINK_FLAGS "${TO_LINKER},-cref ${TO_LINKER},-Map=bitmap2component.map"
        )
endif()


if( MINGW )
    # BITMAP2COMPONENT_RESOURCES variable is set by the macro.
    mingw_resource_compiler( bitmap2component )
else()
    set( BITMAP2COMPONENT_RESOURCES bitmap2component.rc )
endif()
