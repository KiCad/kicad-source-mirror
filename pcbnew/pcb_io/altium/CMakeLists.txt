
# Sources for the pcbnew PLUGIN called ALTIUM_DESIGNER_PLUGIN, ALTIUM_CIRCUIT_STUDIO_PLUGIN and ALTIUM_CIRCUIT_MAKER_PLUGIN

set( ALTIUM2PCBNEW_SRCS
    altium_parser_pcb.cpp
    altium_pcb.cpp
    altium_pcb_compound_file.cpp
    altium_rule_transformer.cpp
    pcb_io_altium_circuit_maker.cpp
    pcb_io_altium_circuit_studio.cpp
    pcb_io_altium_designer.cpp
    pcb_io_solidworks.cpp
    )

add_library( altium2pcbnew STATIC ${ALTIUM2PCBNEW_SRCS} )

target_link_libraries( altium2pcbnew
    pcbcommon
    compoundfilereader )

target_include_directories( altium2pcbnew
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR} )

target_include_directories( altium2pcbnew PRIVATE
    $<TARGET_PROPERTY:magic_enum,INTERFACE_INCLUDE_DIRECTORIES>
    )
