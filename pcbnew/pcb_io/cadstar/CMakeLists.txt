# Sources for the pcbnew PLUGIN called CADSTAR_PCB_ARCHIVE_PLUGIN
include_directories( . )

set( CADSTAR2PCBNEW_SRCS
    cadstar_pcb_archive_plugin.cpp
    cadstar_pcb_archive_parser.cpp
    cadstar_pcb_archive_loader.cpp
    )

add_library( cadstar2pcbnew STATIC ${CADSTAR2PCBNEW_SRCS} )

target_link_libraries( cadstar2pcbnew pcbcommon )
