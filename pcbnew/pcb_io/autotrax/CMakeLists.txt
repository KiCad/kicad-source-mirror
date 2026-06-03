# Sources for the pcbnew PLUGIN called PCB_IO_AUTOTRAX

include_directories( ${CMAKE_CURRENT_SOURCE_DIR} )

set( AUTOTRAX_SRCS
    pcb_io_autotrax.cpp
    autotrax_parser.cpp
    )

add_library( autotrax2pcbnew STATIC ${AUTOTRAX_SRCS} )

target_link_libraries( autotrax2pcbnew pcbcommon )
