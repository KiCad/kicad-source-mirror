include_directories( BEFORE ${INC_BEFORE} )

include_directories(
    ./
    ../
    ../../include
    ../../3d-viewer
    ../../pcbnew
    ../../libs/kimath/include/
    ../../libs/core/include/
    ../../thirdparty/clipper2/Clipper2Lib/include
    ../../thirdparty/nlohmann_json
    ../../thirdparty/rtree
    ../../thirdparty/dynamic_bitset/
    ../../thirdparty/expected/include/
    ../../libs/kiplatform/include/
    ../../common

    ${INC_AFTER}
)

set( PCBNEW_PNS_SRCS
    time_limit.cpp
    pns_kicad_iface.cpp
    pns_algo_base.cpp
    pns_arc.cpp
    pns_component_dragger.cpp
    pns_diff_pair.cpp
    pns_diff_pair_placer.cpp
    pns_dp_meander_placer.cpp
    pns_dragger.cpp
    pns_hole.cpp
    pns_index.cpp
    pns_item.cpp
    pns_itemset.cpp
    pns_line.cpp
    pns_line_placer.cpp
    pns_logger.cpp
    pns_meander.cpp
    pns_meander_placer.cpp
    pns_meander_placer_base.cpp
    pns_meander_skew_placer.cpp
    pns_mouse_trail_tracer.cpp
    pns_node.cpp
    pns_optimizer.cpp
    pns_router.cpp
    pns_routing_settings.cpp
    pns_shove.cpp
    pns_sizes_settings.cpp
    pns_solid.cpp
    pns_tool_base.cpp
    pns_topology.cpp
    pns_utils.cpp
    pns_via.cpp
    pns_walkaround.cpp
    pns_multi_dragger.cpp
    router_preview_item.cpp
    router_status_view_item.cpp
    router_tool.cpp
)


set( PCBNEW_PNS_QA_SRCS
    time_limit.cpp
    pns_kicad_iface.cpp
    pns_algo_base.cpp
    pns_arc.cpp
    pns_component_dragger.cpp
    pns_diff_pair.cpp
    pns_diff_pair_placer.cpp
    pns_dp_meander_placer.cpp
    pns_dragger.cpp
    pns_hole.cpp
    pns_index.cpp
    pns_item.cpp
    pns_itemset.cpp
    pns_line.cpp
    pns_line_placer.cpp
    pns_logger.cpp
    pns_meander.cpp
    pns_meander_placer.cpp
    pns_meander_placer_base.cpp
    pns_meander_skew_placer.cpp
    pns_mouse_trail_tracer.cpp
    pns_node.cpp
    pns_optimizer.cpp
    pns_router.cpp
    pns_routing_settings.cpp
    pns_shove.cpp
    pns_sizes_settings.cpp
    pns_solid.cpp
    pns_topology.cpp
    pns_utils.cpp
    pns_via.cpp
    pns_walkaround.cpp
    pns_multi_dragger.cpp
    router_preview_item.cpp
)


add_library( pnsrouter STATIC ${PCBNEW_PNS_SRCS} )
add_library( pnsrouter_qa STATIC ${PCBNEW_PNS_QA_SRCS} )

if( KICAD_USE_PCH )
    target_precompile_headers( pnsrouter
        PRIVATE
            <board.h>
            <clipper2/clipper.h>
            <kiid.h>
            <pgm_base.h>
            <wx/wx.h>
            )

    target_precompile_headers( pnsrouter_qa
        PRIVATE
            <board.h>
            <clipper2/clipper.h>
            <kiid.h>
            <pgm_base.h>
            <wx/wx.h>
            )
endif()

target_link_libraries( pnsrouter PRIVATE
    common
)
