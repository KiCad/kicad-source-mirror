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
    time_limit.cpp

    pns_algo_base.cpp
    pns_diff_pair.cpp
    pns_diff_pair_placer.cpp
    pns_dp_meander_placer.cpp
    pns_dragger.cpp
    pns_item.cpp
    pns_itemset.cpp
    pns_line.cpp
    pns_line_placer.cpp
    pns_logger.cpp
    pns_meander.cpp
    pns_meander_placer.cpp
    pns_meander_placer_base.cpp
    pns_meander_skew_placer.cpp
    pns_node.cpp
    pns_optimizer.cpp
    pns_router.cpp
    pns_routing_settings.cpp
    pns_shove.cpp
    pns_sizes_settings.cpp
    pns_solid.cpp
    pns_tool_base.cpp
    pns_topology.cpp
    pns_tune_status_popup.cpp
    pns_utils.cpp
    pns_via.cpp
    pns_walkaround.cpp
    router_preview_item.cpp
    router_tool.cpp
    length_tuner_tool.cpp
)

add_library( pnsrouter STATIC ${PCBNEW_PNS_SRCS} )
