
# Sources for the pcbnew PLUGIN called EASYEDA_PLUGIN

include_directories( ${CMAKE_CURRENT_SOURCE_DIR} )


set( EASYEDA_SRCS
    pcb_io_easyeda_plugin.cpp
    pcb_io_easyeda_parser.cpp
    )

add_library( easyeda STATIC ${EASYEDA_SRCS} )

target_link_libraries( easyeda pcbcommon )
