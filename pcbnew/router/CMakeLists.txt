include_directories( BEFORE ${INC_BEFORE} )

include_directories(
    ./
    ../
    ../../include
    ../../pcbnew
    ../../polygon
    ${INC_AFTER}
)

set( PCBNEW_PNS_SRCS
    direction.h
    pns_via.h
    pns_routing_settings.h
    pns_shove.cpp
    pns_line.cpp
    pns_utils.h
    pns_layerset.h
    trace.h
    pns_line.h
    pns_walkaround.cpp
    pns_node.h
    pns_line_placer.cpp
    pns_utils.cpp
    pns_solid.h
    pns_item.cpp
    pns_via.cpp
    pns_node.cpp
    pns_solid.cpp
    pns_line_placer.h
    pns_optimizer.h
    pns_walkaround.h
    pns_shove.h
    pns_router.h
    pns_router.cpp
    pns_index.h
    pns_item.h
    pns_optimizer.cpp
    pns_joint.h
    pns_segment.h
    pns_itemset.h
    pns_itemset.cpp
    router_tool.cpp
    router_tool.h
    router_preview_item.cpp
    router_preview_item.h
)

add_library( pnsrouter STATIC ${PCBNEW_PNS_SRCS} )
