include_directories( BEFORE
        pcb
        ${CMAKE_CURRENT_SOURCE_DIR}
)

include_directories( SYSTEM 
    ${OCE_INCLUDE_DIRS}
)

add_executable( kicad2step
    kicad2step.cpp
    pcb/3d_resolver.cpp
    pcb/base.cpp
    pcb/kicadmodel.cpp
    pcb/kicadmodule.cpp
    pcb/kicadpad.cpp
    pcb/kicadpcb.cpp
    pcb/kicadcurve.cpp
    pcb/oce_utils.cpp
    sexpr/sexpr.cpp
    sexpr/sexpr_parser.cpp
)

target_link_libraries( kicad2step ${wxWidgets_LIBRARIES} ${LIBS_OCE} )

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
