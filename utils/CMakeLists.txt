# Add all the warnings to the files
if( COMPILER_SUPPORTS_WARNINGS )
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WARN_FLAGS_CXX}")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${WARN_FLAGS_C}")
endif()

if( KICAD_IDF_TOOLS )
    add_subdirectory( idftools )
endif()

# The Quick Look extensions are Objective-C++ app extensions, so they build with the rest of the
# tree under whichever generator KiCad is configured with.
set( KICAD_BUILD_QUICKLOOK OFF CACHE INTERNAL "Quick Look model preview extensions are buildable" FORCE )

if( APPLE AND KICAD_BUILD_NATIVE_MODEL_PREVIEW )
    set( KICAD_BUILD_QUICKLOOK ON CACHE INTERNAL "Quick Look model preview extensions are buildable" FORCE )
endif()

if( KICAD_BUILD_QUICKLOOK )
    include( ${KICAD_CMAKE_MODULE_PATH}/KiCadVersion.cmake )
    set( SRC_PATH ${CMAKE_SOURCE_DIR} )
    include( ${KICAD_CMAKE_MODULE_PATH}/KiCadFullVersion.cmake )

    set( KICAD_PREVIEW_BUNDLE_ID "${KICAD_BUNDLE_ID}.ModelPreview"
        CACHE STRING "Bundle identifier for the KiCad Quick Look preview extension" )
    set( KICAD_THUMBNAIL_BUNDLE_ID "${KICAD_BUNDLE_ID}.ModelThumbnail"
        CACHE STRING "Bundle identifier for the KiCad Quick Look thumbnail extension" )

    add_subdirectory( quicklook )

    add_dependencies( kicad kicad_model_quicklook )

    install( DIRECTORY "${KICAD_QUICKLOOK_PREVIEW_APPEX}"
        DESTINATION "${OSX_BUNDLE_INSTALL_DIR}/Contents/PlugIns"
        USE_SOURCE_PERMISSIONS
        COMPONENT Runtime )
    install( DIRECTORY "${KICAD_QUICKLOOK_THUMBNAIL_APPEX}"
        DESTINATION "${OSX_BUNDLE_INSTALL_DIR}/Contents/PlugIns"
        USE_SOURCE_PERMISSIONS
        COMPONENT Runtime )
    install( FILES
        "$<TARGET_FILE:kicad_3d_import>"
        "$<TARGET_FILE:kicad_3dsg>"
        DESTINATION "${OSX_BUNDLE_INSTALL_DIR}/Contents/Frameworks"
        COMPONENT Runtime )

    install( CODE "
        set( KICAD_CMAKE_MODULE_PATH \"${KICAD_CMAKE_MODULE_PATH}\" )
        set( OSX_BUNDLE_INSTALL_LIB_DIR \"${OSX_BUNDLE_INSTALL_LIB_DIR}\" )
        include( \"${KICAD_CMAKE_MODULE_PATH}/InstallSteps/InstallMacOS.cmake\" )
        install_runtime_deps( \"\" \"$<TARGET_FILE:kicad_3d_import>;$<TARGET_FILE:kicad_3dsg>\" \"\" )
    " COMPONENT Runtime )
endif()

if( WIN32 AND KICAD_BUILD_NATIVE_MODEL_PREVIEW )
    target_sources( kicommon PRIVATE
        ${CMAKE_SOURCE_DIR}/3d-viewer/dialogs/model_preview_manager_windows.cpp )
    target_link_libraries( kicommon Version )
    add_subdirectory( preview_handlers )
endif()
