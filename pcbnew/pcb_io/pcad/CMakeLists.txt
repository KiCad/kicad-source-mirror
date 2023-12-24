
# Sources for the pcbnew PLUGIN called PCAD_PLUGIN

# This CMakeLists.txt is included from pcbnew, so we are inheriting its include_directories(),

set( PCAD2PCBNEW_SRCS
    pcb_io_pcad.cpp
    pcad2kicad_common.cpp
    pcad_pcb.cpp
        pcad_arc.cpp
    pcad_pcb_component.cpp
        pcad_copper_pour.cpp
        pcad_cutout.cpp
        pcad_keepout.cpp
        pcad_line.cpp
    pcad_footprint.cpp
        pcad_nets.cpp
        pcad_pad.cpp
        pcad_pad_shape.cpp
        pcad_plane.cpp
        pcad_polygon.cpp
    pcad_text.cpp
        pcad_via.cpp
        pcad_via_shape.cpp
    s_expr_loader.cpp
    )

add_library( pcad2kicadpcb STATIC ${PCAD2PCBNEW_SRCS} )

target_link_libraries( pcad2kicadpcb pcbcommon )

# Make all headers be prefixed by pcad/ to differentiate them
target_include_directories( pcad2kicadpcb PRIVATE
                            "${CMAKE_CURRENT_SOURCE_DIR}/../" )
