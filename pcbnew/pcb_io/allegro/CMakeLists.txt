
# Sources for the ALLEGRO Pcbnew plugin

set( ALLEGRO2PCBNEW_SRCS
    allegro_builder.cpp
    allegro_parser.cpp
    pcb_io_allegro.cpp
)

add_library( allegro2pcbnew STATIC ${ALLEGRO2PCBNEW_SRCS} )

target_link_libraries( allegro2pcbnew
    PUBLIC
        pcbcommon
    PRIVATE
        magic_enum
)

target_include_directories( allegro2pcbnew
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)


add_executable( allegro_cli
    utils/cli/allegro_cli.cpp
)

target_link_libraries( allegro_cli PRIVATE
    allegro2pcbnew
    magic_enum
    argparse
)

add_subdirectory(utils/pycli)
