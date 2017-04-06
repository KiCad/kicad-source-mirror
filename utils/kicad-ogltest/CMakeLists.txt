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

add_executable( kicad-ogltest WIN32 MACOSX_BUNDLE
    ${OGLTEST_FILES} )

target_link_libraries( kicad-ogltest
    gal
    common
    bitmaps
    polygon
    ${wxWidgets_LIBRARIES} )

if( APPLE )
    # puts binaries into the *.app bundle while linking
    set_target_properties( kicad-ogltest PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${OSX_BUNDLE_BUILD_BIN_DIR}
            )
else()
    install( TARGETS kicad-ogltest
            DESTINATION ${KICAD_BIN}
            COMPONENT binary )
endif()
