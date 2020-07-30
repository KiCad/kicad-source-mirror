
# Sources for the pcbnew PLUGIN called CADSTAR_ARCHIVE_PLUGIN
include_directories( . )

set( CADSTAR2PCBNEW_SRCS
    cadstar_pcb_archive_plugin.cpp
    cadstar_pcb_archive_parser.cpp
    cadstar_archive_common.cpp
    cadstar_pcb.cpp
    )

add_library( cadstar2kicadpcb STATIC ${CADSTAR2PCBNEW_SRCS} )

target_link_libraries( cadstar2kicadpcb pcbcommon )
