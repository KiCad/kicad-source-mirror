# Sources for the pcbnew PLUGIN called CADSTAR_PCB_ARCHIVE_PLUGIN
include_directories( . )

set( CADSTAR2PCBNEW_SRCS
    cadstar_pcb_archive_parser.cpp
    cadstar_pcb_archive_loader.cpp
    pcb_io_cadstar_archive.cpp
    )

add_library( cadstar2pcbnew STATIC ${CADSTAR2PCBNEW_SRCS} )

target_link_libraries( cadstar2pcbnew pcbcommon )
