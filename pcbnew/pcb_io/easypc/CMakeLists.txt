# Sources for the pcbnew PLUGIN called EASYPC

# This CMakeLists.txt is included from pcbnew, so we inherit its include_directories().

set( EASYPC2PCBNEW_SRCS
    pcb_io_easypc.cpp
    easypc_pcb_items.cpp
    easypc_pcb_builder.cpp
    )

add_library( easypc2pcbnew STATIC ${EASYPC2PCBNEW_SRCS} )

target_link_libraries( easypc2pcbnew pcbcommon )

# Make all headers be prefixed by easypc/ to differentiate them
target_include_directories( easypc2pcbnew PRIVATE
                            "${CMAKE_CURRENT_SOURCE_DIR}/../" )
