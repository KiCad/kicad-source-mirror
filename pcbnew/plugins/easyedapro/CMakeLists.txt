
# Sources for the pcbnew PLUGIN called EASYEDAPRO_PLUGIN

include_directories( ${CMAKE_CURRENT_SOURCE_DIR} )


set( EASYEDAPRO_SRCS
    pcb_easyedapro_plugin.cpp
    pcb_easyedapro_parser.cpp
    )

add_library( easyedapro STATIC ${EASYEDAPRO_SRCS} )

target_link_libraries( easyedapro pcbcommon )
