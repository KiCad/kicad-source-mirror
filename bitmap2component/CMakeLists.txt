# Add all the warnings to the files
if( COMPILER_SUPPORTS_WARNINGS )
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WARN_FLAGS_CXX}")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${WARN_FLAGS_C}")
endif()

add_definitions( -DBITMAP_2_CMP )

include_directories( BEFORE ${INC_BEFORE} )
include_directories( ${INC_AFTER} )

set( BITMAP2COMPONENT_SRCS
    ${CMAKE_SOURCE_DIR}/common/single_top.cpp
    bitmap2cmp_main.cpp
    bitmap2cmp_settings.cpp
    bitmap2component.cpp
    bitmap2cmp_gui_base.cpp
    bitmap2cmp_gui.cpp
    ../common/env_vars.cpp      # needed on MSW to avoid a link issue (a symbol not found)
    )

set_source_files_properties( ${CMAKE_SOURCE_DIR}/common/single_top.cpp PROPERTIES
    COMPILE_DEFINITIONS     "TOP_FRAME=FRAME_BM2CMP"
    )
set_source_files_properties( bitmap2cmp_gui.cpp PROPERTIES
    COMPILE_DEFINITIONS     "COMPILING_DLL"
    )

if( MINGW )
    # BITMAP2COMPONENT_RESOURCES variable is set by the macro.
    mingw_resource_compiler( bitmap2component )
else()
    set( BITMAP2COMPONENT_RESOURCES ${CMAKE_SOURCE_DIR}/resources/msw/bitmap2component.rc )
endif()

if( APPLE )
    # setup bundle
    set( BITMAP2COMPONENT_RESOURCES bitmap2component.icns )
    set_source_files_properties( "${CMAKE_CURRENT_SOURCE_DIR}/bitmap2component.icns" PROPERTIES
        MACOSX_PACKAGE_LOCATION Resources
        )
    set( MACOSX_BUNDLE_ICON_FILE bitmap2component.icns )
    set( MACOSX_BUNDLE_GUI_IDENTIFIER org.kicad-pcb.kicad )
    set( MACOSX_BUNDLE_NAME bitmap2component )
endif()

add_executable( bitmap2component WIN32 MACOSX_BUNDLE
    ${BITMAP2COMPONENT_SRCS}
    ${BITMAP2COMPONENT_RESOURCES}
    )

target_link_libraries( bitmap2component
    common
    ${wxWidgets_LIBRARIES}
    potrace
    )

if( APPLE )
    set_target_properties( bitmap2component PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST ${PROJECT_BINARY_DIR}/bitmap2component/Info.plist
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
        LINK_FLAGS "-Wl,-cref,-Map=bitmap2component.map"
        )
endif()
