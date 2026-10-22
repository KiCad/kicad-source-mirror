# Sources for the pcbnew PLUGIN called TARGET3001

# This CMakeLists.txt is included from pcbnew, so we inherit its include_directories().

set( TARGET30012PCBNEW_SRCS
    pcb_io_target3001.cpp
    target3001_pcb_builder.cpp
    )

add_library( target30012pcbnew STATIC ${TARGET30012PCBNEW_SRCS} )

target_link_libraries( target30012pcbnew pcbcommon )

target_include_directories( target30012pcbnew PRIVATE
                            "${CMAKE_CURRENT_SOURCE_DIR}/../" )
