
# Sources for the pcbnew PLUGIN called IPC2581_PLUGIN

include_directories( ${CMAKE_CURRENT_SOURCE_DIR} )


set( IPC2581_SRCS
    pcb_io_ipc2581.cpp
    )

add_library( ipc2581 STATIC ${IPC2581_SRCS} )

target_link_libraries( ipc2581 pcbcommon )
