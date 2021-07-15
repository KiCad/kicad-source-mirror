include_directories( SYSTEM
    ${OCE_INCLUDE_DIRS}
    ${OCC_INCLUDE_DIR}
)

set( KS2_LIB_FILES
    kicad2step.cpp
    kicad2step_frame_base.cpp
    pcb/3d_resolver.cpp
    pcb/base.cpp
    pcb/kicadmodel.cpp
    pcb/kicadfootprint.cpp
    pcb/kicadpad.cpp
    pcb/kicadpcb.cpp
    pcb/kicadcurve.cpp
    pcb/oce_utils.cpp
)

# Break the library out for re-use by both kicad2step and any qa that needs it
# In future, this could move for re-use by other programs needing s-expr support (?)
add_library( kicad2step_lib STATIC
    ${KS2_LIB_FILES}
)

target_include_directories( kicad2step_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include # for core
    ${Boost_INCLUDE_DIR}
)

target_link_libraries( kicad2step_lib
    sexpr
)

set( K2S_FILES
    kicad2step.cpp
)

if( MINGW )
    list( APPEND K2S_FILES ${CMAKE_SOURCE_DIR}/common/streamwrapper.cpp )
endif( MINGW )

add_executable( kicad2step WIN32 ${K2S_FILES} )

target_link_libraries( kicad2step
    kicad2step_lib
    ${wxWidgets_LIBRARIES}
    ${OCC_LIBRARIES}
    ${ZLIB_LIBRARIES} )

target_include_directories( kicad2step_lib PRIVATE
    $<TARGET_PROPERTY:gzip-hpp,INTERFACE_INCLUDE_DIRECTORIES>
    )

if( APPLE )
    # puts binaries into the *.app bundle while linking
    set_target_properties( kicad2step PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${OSX_BUNDLE_BUILD_BIN_DIR}
            )
else()
    install( TARGETS kicad2step
            DESTINATION ${KICAD_BIN}
            COMPONENT binary )
endif()

if( KICAD_WIN32_INSTALL_PDBS )
    # Get the PDBs to copy over for MSVC
    install(FILES $<TARGET_PDB_FILE:kicad2step> DESTINATION ${KICAD_BIN})
endif()