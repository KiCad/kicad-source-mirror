
# Sources for the pcbnew PLUGIN called FABMASTER_PLUGIN

include_directories( ${CMAKE_CURRENT_SOURCE_DIR} )


set( FABMASTER_SRCS
    pcb_io_fabmaster.cpp
    import_fabmaster.cpp
    )

add_library( fabmaster STATIC ${FABMASTER_SRCS} )

target_link_libraries( fabmaster pcbcommon )
