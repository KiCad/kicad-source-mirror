
# Sources for the pcbnew PLUGIN called DIPTRACE

# This CMakeLists.txt is included from pcbnew, so we inherit its include_directories().

set( DIPTRACE2PCBNEW_SRCS
    pcb_io_diptrace.cpp
    diptrace_pcb_parser.cpp
    )

add_library( diptrace2pcbnew STATIC ${DIPTRACE2PCBNEW_SRCS} )

target_link_libraries( diptrace2pcbnew pcbcommon )

# Make all headers be prefixed by diptrace/ to differentiate them
target_include_directories( diptrace2pcbnew PRIVATE
                            "${CMAKE_CURRENT_SOURCE_DIR}/../" )
