/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <algorithm>
#include <atomic>
#include <deque>
#include <optional>
#include <utility>

#include <wx/debug.h>

#include <board.h>
#include <board_connected_item.h>
#include <board_design_settings.h>
#include <drc/drc_item.h>
#include <drc/drc_test_provider.h>
#include <drc/drc_rtree.h>
#include <drc/drc_rule_condition.h>
#include <footprint.h>
#include <geometry/seg.h>
#include <geometry/shape_poly_set.h>
#include <geometry/vertex_set.h>
#include <math/box2.h>
#include <math/vector2d.h>
#include <pcb_shape.h>
#include <progress_reporter.h>
#include <thread_pool.h>
#include <pcb_track.h>
#include <pad.h>
#include <zone.h>
#include <advanced_config.h>

/*
    Checks for copper connections that are less than the specified minimum width

    Errors generated:
    - DRCE_CONNECTION_WIDTH
*/

struct NETCODE_LAYER_CACHE_KEY
{
    int          Netcode;
    PCB_LAYER_ID Layer;

    bool operator==(const NETCODE_LAYER_CACHE_KEY& other) const
    {
        return Netcode == other.Netcode && Layer == other.Layer;
    }
};


namespace std
{
    template <>
    struct hash<NETCODE_LAYER_CACHE_KEY>
    {
        std::size_t operator()( const NETCODE_LAYER_CACHE_KEY& k ) const
        {
            constexpr std::size_t prime = 19937;

            return hash<int>()( k.Netcode ) ^ ( hash<int>()( k.Layer ) * prime );
        }
    };
}


class DRC_TEST_PROVIDER_CONNECTION_WIDTH : public DRC_TEST_PROVIDER
{
public:
    DRC_TEST_PROVIDER_CONNECTION_WIDTH()
    {}

    virtual ~DRC_TEST_PROVIDER_CONNECTION_WIDTH() = default;

    virtual bool Run() override;

    virtual const wxString GetName() const override { return wxT( "copper width" ); };

private:
    wxString layerDesc( PCB_LAYER_ID aLayer );
};


class POLYGON_TEST : public VERTEX_SET
{
public:
    POLYGON_TEST( int aLimit ) :
        VERTEX_SET( 0 ),
        m_limit( aLimit )
    {};

    bool FindPairs( const SHAPE_LINE_CHAIN& aPoly )
    {
        m_hits.clear();
        m_vertices.clear();
        m_bbox = aPoly.BBox();

        createList( aPoly );

        m_vertices.front().updateList();

        VERTEX* p = m_vertices.front().next;
        std::set<VERTEX*> all_hits;

        while( p != &m_vertices.front() )
        {
            VERTEX* match = nullptr;

            // Only run the expensive search if we don't already have a match for the point
            if( ( all_hits.empty() || all_hits.count( p ) == 0 ) && ( match = getKink( p ) ) != nullptr )
            {
                if( !all_hits.count( match ) && m_hits.emplace( p->i, match->i ).second )
                {
                    all_hits.emplace( p );
                    all_hits.emplace( match );
                    all_hits.emplace( p->next );
                    all_hits.emplace( p->prev );
                    all_hits.emplace( match->next );
                    all_hits.emplace( match->prev );
                }
            }

            p = p->next;
        }

        return !m_hits.empty();
    }

    std::set<std::pair<int, int>>& GetVertices()
    {
        return m_hits;
    }

    /**
     * Checks to see if there is a "substantial" protrusion in each polygon produced by the cut from
     * aA to aB.  Substantial in this case means that the polygon bulges out to a wider cross-section
     * than the distance from aA to aB
     * @param aA Starting point in the polygon
     * @param aB Ending point in the polygon
     * @return True if the two polygons are both "substantial"
     */
    bool isSubstantial( const VERTEX* aA, const VERTEX* aB ) const
    {
        bool x_change = false;
        bool y_change = false;

        // This is a failsafe in case of invalid lists.  Never check
        // more than the total number of points in m_vertices
        size_t checked = 0;
        size_t total_pts = m_vertices.size();

        const VERTEX* p0 = aA;
        const VERTEX* p = getNextOutlineVertex( p0 );

        while( !same_point( p, aB )             // We've reached the other inflection point
                && !same_point( p, aA )         // We've gone around in a circle
                && checked < total_pts          // Fail-safe for invalid lists
                && !( x_change && y_change ) )  // We've found a substantial change in both directions
        {
            double diff_x = std::abs( p->x - p0->x );
            double diff_y = std::abs( p->y - p0->y );

            // Check for a substantial change in the x or y direction
            // This is measured by the set value of the minimum connection width
            if( diff_x > m_limit )
                x_change = true;

            if( diff_y > m_limit )
                y_change = true;

            p = getNextOutlineVertex( p );

            ++checked;
        }

        wxCHECK_MSG( checked < total_pts, false, wxT( "Invalid polygon detected.  Missing points to check" ) );

        if( !same_point( p, aA ) && ( !x_change || !y_change ) )
            return false;

        p = getPrevOutlineVertex( p0 );

        x_change = false;
        y_change = false;
        checked = 0;

        while( !same_point( p, aB )             // We've reached the other inflection point
                && !same_point( p, aA )         // We've gone around in a circle
                && checked < total_pts          // Fail-safe for invalid lists
                && !( x_change && y_change ) )  // We've found a substantial change in both directions
        {
            double diff_x = std::abs( p->x - p0->x );
            double diff_y = std::abs( p->y - p0->y );

            // Floating point zeros can have a negative sign, so we need to
            // ensure that only substantive diversions count for a direction
            // change
            if( diff_x > m_limit )
                x_change = true;

            if( diff_y > m_limit )
                y_change = true;

            p = getPrevOutlineVertex( p );

            ++checked;
        }

        wxCHECK_MSG( checked < total_pts, false, wxT( "Invalid polygon detected.  Missing points to check" ) );

        return ( same_point( p, aA ) || ( x_change && y_change ) );
    }

    VERTEX* getKink( VERTEX* aPt ) const
    {
        // The point needs to be at a concave surface
        if( locallyInside( aPt->prev, aPt->next ) )
            return nullptr;

        // z-order range for the current point ± limit bounding box
        const uint32_t    maxZ = zOrder( aPt->x + m_limit, aPt->y + m_limit );
        const uint32_t    minZ = zOrder( aPt->x - m_limit, aPt->y - m_limit );
        const SEG::ecoord limit2 = SEG::Square( m_limit );

        // first look for points in increasing z-order
        VERTEX* p = aPt->nextZ;
        SEG::ecoord min_dist = std::numeric_limits<SEG::ecoord>::max();
        VERTEX* retval = nullptr;

        while( p && p->z <= maxZ )
        {
            int delta_i = std::abs( p->i - aPt->i );
            VECTOR2D diff( p->x - aPt->x, p->y - aPt->y );
            SEG::ecoord dist2 = diff.SquaredEuclideanNorm();

            if( delta_i > 1 && dist2 < limit2 && dist2 < min_dist && dist2 > 0
                    && locallyInside( p, aPt ) && isSubstantial( p, aPt ) && isSubstantial( aPt, p ) )
            {
                min_dist = dist2;
                retval = p;
            }

            p = p->nextZ;
        }

        p = aPt->prevZ;

        while( p && p->z >= minZ )
        {
            int delta_i = std::abs( p->i - aPt->i );
            VECTOR2D diff( p->x - aPt->x, p->y - aPt->y );
            SEG::ecoord dist2 = diff.SquaredEuclideanNorm();

            if( delta_i > 1 && dist2 < limit2 && dist2 < min_dist && dist2 > 0
                    && locallyInside( p, aPt ) && isSubstantial( p, aPt ) && isSubstantial( aPt, p ) )
            {
                min_dist = dist2;
                retval = p;
            }

            p = p->prevZ;
        }
        return retval;
    }

private:
    int                             m_limit;
    std::set<std::pair<int, int>>   m_hits;
};


wxString DRC_TEST_PROVIDER_CONNECTION_WIDTH::layerDesc( PCB_LAYER_ID aLayer )
{
    return wxString::Format( wxT( "(%s)" ), m_drcEngine->GetBoard()->GetLayerName( aLayer ) );
}


bool DRC_TEST_PROVIDER_CONNECTION_WIDTH::Run()
{
    if( m_drcEngine->IsErrorLimitExceeded( DRCE_CONNECTION_WIDTH ) )
    {
        REPORT_AUX( wxT( "Connection width violations ignored. Tests not run." ) );
        return true;    // Continue with other tests
    }

    if( !reportPhase( _( "Checking nets for minimum connection width..." ) ) )
        return false;   // DRC cancelled

    BOARD* board = m_drcEngine->GetBoard();
    int    epsilon = board->GetDesignSettings().GetDRCEpsilon();

    // Zone knockouts can be approximated, and always have extra clearance built in
    epsilon += board->GetDesignSettings().m_MaxError + pcbIUScale.mmToIU( ADVANCED_CFG::GetCfg().m_ExtraClearance );

    // A neck in a zone fill can be between two knockouts. In this case it will be epsilon smaller
    // on -each- side.
    epsilon *= 2;

    /*
     * Build a set of distinct minWidths specified by various DRC rules.  We'll run a test for
     * each distinct minWidth, and then decide if any copper which failed that minWidth actually
     * was required to abide by it or not.
     */
    std::set<int> distinctMinWidths = m_drcEngine->QueryDistinctConstraints( CONNECTION_WIDTH_CONSTRAINT );

    if( m_drcEngine->IsCancelled() )
        return false;   // DRC cancelled

    struct ITEMS_POLY
    {
        std::set<BOARD_ITEM*> Items;
        SHAPE_POLY_SET        Poly;
    };

    std::unordered_map<NETCODE_LAYER_CACHE_KEY, ITEMS_POLY> dataset;
    std::atomic<size_t> done( 1 );

    auto calc_effort =
            [&]( const std::set<BOARD_ITEM*>& items, PCB_LAYER_ID aLayer ) -> size_t
            {
                size_t effort = 0;

                for( BOARD_ITEM* item : items )
                {
                    if( item->Type() == PCB_ZONE_T )
                    {
                        ZONE* zone = static_cast<ZONE*>( item );
                        effort += zone->GetFilledPolysList( aLayer )->FullPointCount();
                    }
                    else
                    {
                        effort += 4;
                    }
                }

                return effort;
            };

    /*
     * For each net, on each layer, build a polygonSet which contains all the copper associated
     * with that net on that layer.
     */
    auto build_netlayer_polys =
            [&]( int aNetcode, const PCB_LAYER_ID aLayer ) -> size_t
            {
                if( m_drcEngine->IsCancelled() )
                    return 0;

                ITEMS_POLY& itemsPoly = dataset[ { aNetcode, aLayer } ];

                for( BOARD_ITEM* item : itemsPoly.Items )
                    item->TransformShapeToPolygon( itemsPoly.Poly, aLayer, 0, ARC_HIGH_DEF, ERROR_OUTSIDE );

                itemsPoly.Poly.Fracture();

                done.fetch_add( calc_effort( itemsPoly.Items, aLayer ) );

                return 1;
            };

    /*
     * Examine all necks in a given polygonSet which fail a given minWidth.
     */
    auto min_checker =
            [&]( const ITEMS_POLY& aItemsPoly, const PCB_LAYER_ID aLayer, int aMinWidth ) -> size_t
            {
                if( m_drcEngine->IsCancelled() )
                    return 0;

                int testWidth = aMinWidth - epsilon;

                POLYGON_TEST test( testWidth );

                for( int ii = 0; ii < aItemsPoly.Poly.OutlineCount(); ++ii )
                {
                    const SHAPE_LINE_CHAIN& chain = aItemsPoly.Poly.COutline( ii );

                    test.FindPairs( chain );
                    auto& ret = test.GetVertices();

                    for( const std::pair<int, int>& pt : ret )
                    {
                        /*
                         * We've found a neck that fails the given aMinWidth.  We now need to know
                         * if the objects the produced the copper at this location are required to
                         * abide by said aMinWidth or not.  (If so, we have a violation.)
                         *
                         * We find the contributingItems by hit-testing at the choke point (the
                         * centre point of the neck), and then run the rules engine on those
                         * contributingItems.  If the reported constraint matches aMinWidth, then
                         * we've got a violation.
                         */
                        SEG      span( chain.CPoint( pt.first ), chain.CPoint( pt.second ) );
                        VECTOR2I location = ( span.A + span.B ) / 2;
                        int      dist = ( span.A - span.B ).EuclideanNorm();

                        std::vector<BOARD_ITEM*> contributingItems;

                        for( BOARD_ITEM* item : board->m_CopperItemRTreeCache->GetObjectsAt( location, aLayer,
                                                                                             aMinWidth ) )
                        {
                            if( item->HitTest( location, aMinWidth ) )
                                contributingItems.push_back( item );
                        }

                        for( auto& [ zone, rtree ] : board->m_CopperZoneRTreeCache )
                        {
                            if( !rtree.get() )
                                continue;

                            auto obj_list = rtree->GetObjectsAt( location, aLayer, aMinWidth );

                            if( !obj_list.empty() && zone->HitTestFilledArea( aLayer, location, aMinWidth ) )
                                contributingItems.push_back( zone );
                        }

                        if( !contributingItems.empty() )
                        {
                            BOARD_ITEM* item1 = contributingItems[0];
                            BOARD_ITEM* item2 = contributingItems.size() > 1 ? contributingItems[1]
                                                                             : nullptr;
                            DRC_CONSTRAINT c = m_drcEngine->EvalRules( CONNECTION_WIDTH_CONSTRAINT,
                                                                       item1, item2, aLayer );

                            if( c.Value().Min() == aMinWidth )
                            {
                                std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_CONNECTION_WIDTH );
                                wxString msg;

                                msg = formatMsg( _( "(%s minimum connection width %s; actual %s)" ),
                                                 c.GetName(),
                                                 c.Value().Min(),
                                                 dist );

                                msg += wxS( " " ) + layerDesc( aLayer );

                                drcItem->SetErrorDetail( msg );
                                drcItem->SetViolatingRule( c.GetParentRule() );

                                for( BOARD_ITEM* item : contributingItems )
                                    drcItem->AddItem( item );

                                reportTwoPointGeometry( drcItem, location, span.A, span.B, aLayer );
                            }
                        }
                    }
                }

                done.fetch_add( calc_effort( aItemsPoly.Items, aLayer ) );

                return 1;
            };

    for( PCB_LAYER_ID layer : LSET::AllCuMask( board->GetCopperLayerCount() ) )
    {
        for( ZONE* zone : board->m_DRCCopperZones )
        {
            if( !zone->GetIsRuleArea() && zone->IsOnLayer( layer ) )
                dataset[ { zone->GetNetCode(), layer } ].Items.emplace( zone );
        }

        for( PCB_TRACK* track : board->Tracks() )
        {
            if( PCB_VIA* via = dynamic_cast<PCB_VIA*>( track ) )
            {
                if( via->FlashLayer( static_cast<int>( layer ) ) )
                    dataset[ { via->GetNetCode(), layer } ].Items.emplace( via );
            }
            else if( track->IsOnLayer( layer ) )
            {
                dataset[ { track->GetNetCode(), layer } ].Items.emplace( track );
            }
        }

        for( FOOTPRINT* fp : board->Footprints() )
        {
            for( PAD* pad : fp->Pads() )
            {
                if( pad->FlashLayer( static_cast<int>( layer ) ) )
                    dataset[ { pad->GetNetCode(), layer } ].Items.emplace( pad );
            }

            // Footprint zones are also in the m_DRCCopperZones cache
        }
    }

    thread_pool&                     tp = GetKiCadThreadPool();
    size_t                           total_effort = 0;

    for( const auto& [ netLayer, itemsPoly ] : dataset )
        total_effort += calc_effort( itemsPoly.Items, netLayer.Layer );

    total_effort += std::max( (size_t) 1, total_effort ) * distinctMinWidths.size();

    std::vector<std::future<size_t>> returns;
    returns.reserve( dataset.size() );

    for( const auto& [ netLayer, itemsPoly ] : dataset )
    {
        int netcode = netLayer.Netcode;
        PCB_LAYER_ID layer = netLayer.Layer;
        returns.emplace_back( tp.submit_task(
                [&, netcode, layer]()
                {
                    return build_netlayer_polys( netcode, layer );
                } ) );
    }

    for( auto& ret : returns )
    {
        std::future_status status = ret.wait_for( std::chrono::milliseconds( 250 ) );

        while( status != std::future_status::ready )
        {
            reportProgress( done, total_effort );
            status = ret.wait_for( std::chrono::milliseconds( 250 ) );
        }
    }

    returns.clear();
    returns.reserve( dataset.size() * distinctMinWidths.size() );

    for( const auto& [ netLayer, itemsPoly ] : dataset )
    {
        for( int minWidth : distinctMinWidths )
        {
            if( minWidth - epsilon <= 0 )
                continue;

            returns.emplace_back( tp.submit_task(
                    [min_checker, &itemsPoly, &netLayer, minWidth]()
                    {
                        return min_checker( itemsPoly, netLayer.Layer, minWidth );
                    } ) );
        }
    }

    for( auto& ret : returns )
    {
        std::future_status status = ret.wait_for( std::chrono::milliseconds( 250 ) );

        while( status != std::future_status::ready )
        {
            reportProgress( done, total_effort );
            status = ret.wait_for( std::chrono::milliseconds( 250 ) );
        }
    }

    return true;
}


namespace detail
{
static DRC_REGISTER_TEST_PROVIDER<DRC_TEST_PROVIDER_CONNECTION_WIDTH> dummy;
}
