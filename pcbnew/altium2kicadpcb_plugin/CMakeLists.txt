
# Sources for the pcbnew PLUGIN called ALTIUM_PLUGIN

# This CMakeLists.txt is included from pcbnew, so we are inheriting its include_directories(),
# simply add the current source dir to inherited include_directories() for sources in here only.
include_directories( . )


set( ALTIUM2PCBNEW_SRCS
    altium_plugin.cpp
    altium_pcb.cpp
    altium_parser_binary.cpp
    )

add_library( altium2kicadpcb STATIC ${ALTIUM2PCBNEW_SRCS} )

add_dependencies( altium2kicadpcb compoundfilereader )

target_link_libraries( altium2kicadpcb pcbcommon )

target_include_directories( altium2kicadpcb PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        $<TARGET_PROPERTY:compoundfilereader,INTERFACE_INCLUDE_DIRECTORIES>
        )