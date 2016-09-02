include_directories(
        pcb
        ${CMAKE_CURRENT_SOURCE_DIR}
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

install( TARGETS kicad2step
        DESTINATION bin
        COMPONENT binary )
