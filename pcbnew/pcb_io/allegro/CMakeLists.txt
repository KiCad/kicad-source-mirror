
# Sources for the ALLEGRO Pcbnew plugin

set( ALLEGRO_CONVERT_SRCS
    convert/allegro_db.cpp
    convert/allegro_parser.cpp
)

add_library( allegro_convert STATIC ${ALLEGRO_CONVERT_SRCS} )

target_include_directories( allegro_convert
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries( allegro_convert
   PUBLIC
        kicommon
)


set( ALLEGRO2PCBNEW_SRCS
    allegro_builder.cpp
    pcb_io_allegro.cpp
        convert/allegro_padstack.h
)

add_library( allegro2pcbnew STATIC ${ALLEGRO2PCBNEW_SRCS} )

target_link_libraries( allegro2pcbnew
    PUBLIC
        pcbcommon
        allegro_convert
    PRIVATE
        magic_enum
)

target_include_directories( allegro2pcbnew
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

add_subdirectory(utils/extract)
add_subdirectory(utils/pycli)
