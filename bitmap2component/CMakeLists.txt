# Add all the warnings to the files
if( COMPILER_SUPPORTS_WARNINGS )
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WARN_FLAGS_CXX}")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${WARN_FLAGS_C}")
endif()

add_compile_definitions( BITMAP_2_CMP )

include_directories( BEFORE ${INC_BEFORE} )
include_directories( ${INC_AFTER} )

set( BITMAP2COMPONENT_SRCS
    ${CMAKE_SOURCE_DIR}/common/single_top.cpp
    bitmap2cmp_control.cpp
    bitmap2cmp_main.cpp
    bitmap2cmp_settings.cpp
    bitmap2component.cpp
    bitmap2cmp_panel_base.cpp
    bitmap2cmp_frame.cpp
    bitmap2cmp_panel.cpp
    bitmap2cmp_panel_base.cpp
    )

set_source_files_properties( ${CMAKE_SOURCE_DIR}/common/single_top.cpp PROPERTIES
    COMPILE_DEFINITIONS     "TOP_FRAME=FRAME_BM2CMP"
    )

if( WIN32 )
    if( MINGW )
        # BITMAP2COMPONENT_RESOURCES variable is set by the macro.
        mingw_resource_compiler( bitmap2component )
    else()
        set( BITMAP2COMPONENT_RESOURCES ${CMAKE_SOURCE_DIR}/resources/msw/bitmap2component.rc )
    endif()
endif()

if( APPLE )
    # setup bundle
    set( BITMAP2COMPONENT_RESOURCES bitmap2component.icns )
    set_source_files_properties( "${CMAKE_CURRENT_SOURCE_DIR}/bitmap2component.icns" PROPERTIES
        MACOSX_PACKAGE_LOCATION Resources
        )
    set( MACOSX_BUNDLE_ICON_FILE bitmap2component.icns )
    set( MACOSX_BUNDLE_GUI_IDENTIFIER org.kicad.kicad )
    set( MACOSX_BUNDLE_NAME bitmap2component )
endif()

add_executable( bitmap2component WIN32 MACOSX_BUNDLE
    ${BITMAP2COMPONENT_SRCS}
    ${BITMAP2COMPONENT_RESOURCES}
    )

target_link_libraries( bitmap2component
    common
    kicommon
    ${wxWidgets_LIBRARIES}
    potrace
    )

if( APPLE )
    set_target_properties( bitmap2component PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST ${PROJECT_BINARY_DIR}/bitmap2component/Info.plist
        )

    set_target_properties( bitmap2component PROPERTIES INSTALL_RPATH
            "@executable_path/../Frameworks;@executable_path/../Frameworks/Python.framework" )

    # put individual bundle outside of main bundle as a first step
    # will be pulled into the main bundle when creating main bundle
    install( TARGETS bitmap2component
        DESTINATION ${KICAD_BIN}
        COMPONENT binary
        )

    install( CODE "
        set( KICAD_CMAKE_MODULE_PATH \"${KICAD_CMAKE_MODULE_PATH}\" )
        set( KICAD_BIN \"${KICAD_BIN}\" )
        set( OSX_BUNDLE_INSTALL_BIN_DIR \"${OSX_BUNDLE_INSTALL_BIN_DIR}\" )
        set( OSX_BUNDLE_INSTALL_LIB_DIR \"${OSX_BUNDLE_INSTALL_LIB_DIR}\" )
    " )

    install( CODE [[
        include( ${KICAD_CMAKE_MODULE_PATH}/InstallSteps/InstallMacOS.cmake )

        # Install any dependencies (this will generally duplicate kicad.app but we can't be sure)
        install_runtime_deps( "${KICAD_BIN}/bitmap2component.app/Contents/MacOS/bitmap2component"
            ""
            ""
            )
    ]] )
else()
    install( TARGETS bitmap2component
        DESTINATION ${KICAD_BIN}
        COMPONENT binary
        )
endif()

if( KICAD_WIN32_INSTALL_PDBS )
    # Get the PDBs to copy over for MSVC
    install(FILES $<TARGET_PDB_FILE:bitmap2component> DESTINATION ${KICAD_BIN})
endif()

if( false )     # linker map with cross reference
    set_target_properties( bitmap2component PROPERTIES
        LINK_FLAGS "-Wl,--cref,-Map=bitmap2component.map"
        )
endif()
