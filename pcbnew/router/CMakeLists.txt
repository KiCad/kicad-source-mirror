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
    time_limit.h
    time_limit.cpp
    trace.h

    pns_algo_base.h
    pns_algo_base.cpp
    pns_dragger.cpp
    pns_dragger.h
    pns_index.h
    pns_item.h
    pns_item.cpp
    pns_itemset.h
    pns_itemset.cpp
    pns_joint.h
    pns_layerset.h
    pns_line.h
    pns_line.cpp
    pns_line_placer.h
    pns_line_placer.cpp
    pns_logger.h
    pns_logger.cpp
    pns_node.h
    pns_node.cpp
    pns_optimizer.h
    pns_optimizer.cpp
    pns_router.h
    pns_router.cpp
    pns_routing_settings.h
    pns_routing_settings.cpp
    pns_segment.h
    pns_shove.h
    pns_shove.cpp
    pns_solid.h
    pns_solid.cpp
    pns_utils.h
    pns_utils.cpp
    pns_via.h
    pns_via.cpp
    pns_walkaround.h
    pns_walkaround.cpp
    router_preview_item.cpp
    router_preview_item.h
    router_tool.cpp
    router_tool.h
)

add_library( pnsrouter STATIC ${PCBNEW_PNS_SRCS} )
