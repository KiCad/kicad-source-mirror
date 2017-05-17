include_directories( BEFORE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
    ${OPENGL_INCLUDE_DIR}
    ${GLEW_INCLUDE_DIR}
    ${GLM_INCLUDE_DIR}
    ${Boost_INCLUDE_DIR}
)

include_directories(
    ../../include
    ../../common
    ${INC_AFTER}
)

set( OGLTEST_FILES
    kicad-ogltest.cpp
)

add_executable( kicad-ogltest WIN32
    ${OGLTEST_FILES} )

target_link_libraries( kicad-ogltest
    gal
    common
    bitmaps
    polygon
    ${wxWidgets_LIBRARIES} )

if( APPLE )
    # puts binary into the main kicad.app bundle while linking
    set_target_properties( kicad-ogltest PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${OSX_BUNDLE_BUILD_BIN_DIR}
            )
    # bundle dependencies, rewrite binary to use bundled libraries
    install( CODE "
        # override default embedded path settings
        ${OSX_BUNDLE_OVERRIDE_PATHS}

        # do all the work
        include( BundleUtilities )
        fixup_bundle( ${OSX_BUNDLE_BUILD_BIN_DIR}/kicad-ogltest
            \"\"
            \"\"
            )
        " COMPONENT Runtime
        )
else()
    install( TARGETS kicad-ogltest
            DESTINATION ${KICAD_BIN}
            COMPONENT binary )
endif()
