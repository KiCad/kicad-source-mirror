include_directories( BEFORE ${INC_BEFORE} )

include_directories(
    ./
    ../
    ../../include
    ${INC_AFTER}
)

set( PCBNEW_CONN_SRCS
    connectivity_algo.cpp
    connectivity_data.cpp
    connectivity_items.cpp
    from_to_cache.cpp
    topo_match.cpp
)

add_library( connectivity STATIC ${PCBNEW_CONN_SRCS} )

target_link_libraries( connectivity PRIVATE
    common
)
