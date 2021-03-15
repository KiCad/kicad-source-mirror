set( KIPYTHON_SRCS
    kipython_settings.cpp
    python_scripting.cpp
)

add_library( scripting STATIC
    ${KIPYTHON_SRCS}
)

target_link_libraries( scripting
    ${wxWidgets_LIBRARIES}      # wxLogDebug, wxASSERT
    ${Boost_LIBRARIES}          # Because of the OPT types
    common
)

target_include_directories( scripting PUBLIC
    ${PYTHON_INCLUDE_DIRS}
    ${PROJECT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_include_directories( scripting PRIVATE
    $<TARGET_PROPERTY:nlohmann_json,INTERFACE_INCLUDE_DIRECTORIES>
    ${PROJECT_SOURCE_DIR}/bitmaps_png/include
    ${PROJECT_SOURCE_DIR}/include
    ${wxWidgets_LIBRARIES}
    ${Boost_INCLUDE_DIR}
)

# Setup the KIFACE
add_library( scripting_kiface MODULE
    kicad_scripting_main.cpp
    kipython_frame.cpp
    ${KIPYTHON_SRCS}
    )

set_source_files_properties( kicad_scripting_main.cpp PROPERTIES
    # The KIFACE is in kicad_scripting_main.cpp, export it:
    COMPILE_DEFINITIONS     "BUILD_KIWAY_DLL;COMPILING_DLL"
    )

target_include_directories( scripting_kiface PRIVATE
    ${PROJECT_SOURCE_DIR}/bitmaps_png/include
    ${PROJECT_SOURCE_DIR}/include
    ${wxWidgets_LIBRARIES}
    ${Boost_INCLUDE_DIR}
)
    
target_link_libraries( scripting_kiface
    scripting
    )

set_target_properties( scripting_kiface PROPERTIES
    OUTPUT_NAME     kipython
    PREFIX          ${KIFACE_PREFIX}
    SUFFIX          ${KIFACE_SUFFIX}
    )
    
if( MAKE_LINK_MAPS )
    set_target_properties( scripting_kiface PROPERTIES
        LINK_FLAGS "-Wl,-cref,-Map=_scripting.kiface.map" )
endif()

if( APPLE )
    set_target_properties( scripting_kiface PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${OSX_BUNDLE_BUILD_KIFACE_DIR}
        )
else()
    install( TARGETS scripting_kiface
        DESTINATION ${KICAD_BIN}
        COMPONENT binary
        )
endif()
# python shell installation
install( DIRECTORY ${PROJECT_SOURCE_DIR}/scripting/kicad_pyshell/
    DESTINATION ${KICAD_DATA}/scripting/kicad_pyshell
    FILE_PERMISSIONS OWNER_EXECUTE OWNER_READ OWNER_WRITE GROUP_EXECUTE GROUP_READ WORLD_EXECUTE WORLD_READ
)