#
# CMake build file for the Allegro extract utility
#

add_executable( allegro_extract
    allegro_ascii.cpp
    allegro_extract.cpp
    data_dictionary.cpp
    extract_spec_parser.cpp
)

target_link_libraries( allegro_extract PRIVATE
    allegro_convert
    magic_enum
    argparse
)
