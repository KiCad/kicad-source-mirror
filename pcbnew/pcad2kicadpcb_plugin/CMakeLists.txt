
# Sources for the pcbnew PLUGIN called PCAD_PLUGIN

# This CMakeLists.txt is included from pcbnew, so we are inheriting its include_directories(),
# simply add the current source dir to inherited include_directories() for sources in here only.
include_directories( . )


set( PCAD2PCBNEW_SRCS
    pcad2kicad_common.cpp
    pcad_plugin.cpp
    pcb.cpp
    pcb_arc.cpp
    pcb_component.cpp
    pcb_copper_pour.cpp
    pcb_cutout.cpp
    pcb_keepout.cpp
    pcb_line.cpp
    pcb_module.cpp
    pcb_net.cpp
    pcb_pad.cpp
    pcb_pad_shape.cpp
    pcb_plane.cpp
    pcb_polygon.cpp
    pcb_text.cpp
    pcb_via.cpp
    pcb_via_shape.cpp
    s_expr_loader.cpp
    )

add_library( pcad2kicadpcb STATIC ${PCAD2PCBNEW_SRCS} )
add_dependencies( pcad2kicadpcb lib-dependencies )

