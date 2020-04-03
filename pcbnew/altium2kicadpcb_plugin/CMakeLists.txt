
# Sources for the pcbnew PLUGIN called ALTIUM_DESIGNER_PLUGIN, ALTIUM_CIRCUIT_STUDIO_PLUGIN and ALTIUM_CIRCUIT_MAKER_PLUGIN

set( ALTIUM2PCBNEW_SRCS
    altium_circuit_maker_plugin.cpp
    altium_circuit_studio_plugin.cpp
    altium_designer_plugin.cpp
    altium_pcb.cpp
    altium_parser.cpp
    altium_parser_pcb.cpp
    )

add_library( altium2kicadpcb STATIC ${ALTIUM2PCBNEW_SRCS} )

add_dependencies( altium2kicadpcb compoundfilereader )

target_link_libraries( altium2kicadpcb pcbcommon )

target_include_directories( altium2kicadpcb PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        $<TARGET_PROPERTY:compoundfilereader,INTERFACE_INCLUDE_DIRECTORIES>
        )