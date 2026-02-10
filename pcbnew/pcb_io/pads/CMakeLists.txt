
# Sources for the pcbnew PLUGIN called PADS_PLUGIN

set( PADS2PCBNEW_SRCS
    pads_layer_mapper.cpp
    pads_parser.cpp
    pcb_io_pads.cpp
    )

add_library( pads2pcbnew STATIC ${PADS2PCBNEW_SRCS} )

target_link_libraries( pads2pcbnew
    pcbcommon
    pads_common
)

# Make all headers be prefixed by pads/ to differentiate them
target_include_directories( pads2pcbnew PRIVATE
                            "${CMAKE_CURRENT_SOURCE_DIR}/../" )
