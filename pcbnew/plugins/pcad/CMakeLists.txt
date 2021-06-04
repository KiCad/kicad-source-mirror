
# Sources for the pcbnew PLUGIN called PCAD_PLUGIN

# This CMakeLists.txt is included from pcbnew, so we are inheriting its include_directories(),

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
    pcb_footprint.cpp
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

target_link_libraries( pcad2kicadpcb pcbcommon )

# Make all headers be prefixed by pcad/ to differentiate them
target_include_directories( pcad2kicadpcb PRIVATE
                            "${CMAKE_CURRENT_SOURCE_DIR}/../" )
