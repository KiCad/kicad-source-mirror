/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 KiCad Developers.
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
#include <drc/drc_rule.h>
#include <drc/drc_item.h>
#include <drc/drc_test_provider.h>
#include <drc/drc_rtree.h>
#include <drc/drc_rule_condition.h>
#include <footprint.h>
#include <geometry/seg.h>
#include <geometry/shape_poly_set.h>
#include <math/box2.h>
#include <math/vector2d.h>
#include <pcb_shape.h>
#include <progress_reporter.h>
#include <thread_pool.h>
#include <pcb_track.h>
#include <pad.h>
#include <zone.h>

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
    {
    }

    virtual ~DRC_TEST_PROVIDER_CONNECTION_WIDTH()
    {
    }

    virtual bool Run() override;

    virtual const wxString GetName() const override
    {
        return wxT( "copper width" );
    };

    virtual const wxString GetDescription() const override
    {
        return wxT( "Checks copper nets for connections less than a specified minimum" );
    }

private:
    wxString layerDesc( PCB_LAYER_ID aLayer );
};


class POLYGON_TEST
{
public:
    POLYGON_TEST( int aLimit ) :
        m_limit( aLimit )
    {
    };

    bool FindPairs( const SHAPE_LINE_CHAIN& aPoly )
    {
        m_hits.clear();
        m_vertices.clear();
        m_bbox = aPoly.BBox();

        createList( aPoly );

        m_vertices.front().updateList();

        Vertex* p = m_vertices.front().next;
        std::set<Vertex*> all_hits;

        while( p != &m_vertices.front() )
        {
            Vertex* match = nullptr;

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


private:
    struct Vertex
    {
        Vertex( int aIndex, double aX, double aY, POLYGON_TEST* aParent ) :
                i( aIndex ),
                x( aX ),
                y( aY ),
                parent( aParent )
        {
        }

        Vertex& operator=( const Vertex& ) = delete;
        Vertex& operator=( Vertex&& ) = delete;

        bool operator==( const Vertex& rhs ) const
        {
            return this->x == rhs.x && this->y == rhs.y;
        }
        bool operator!=( const Vertex& rhs ) const { return !( *this == rhs ); }

        /**
         * Remove the node from the linked list and z-ordered linked list.
         */
        void remove()
        {
            next->prev = prev;
            prev->next = next;

            if( prevZ )
                prevZ->nextZ = nextZ;

            if( nextZ )
                nextZ->prevZ = prevZ;

            next = nullptr;
            prev = nullptr;
            nextZ = nullptr;
            prevZ = nullptr;
        }

        void updateOrder()
        {
            if( !z )
                z = parent->zOrder( x, y );
        }

        /**
         * After inserting or changing nodes, this function should be called to
         * remove duplicate vertices and ensure z-ordering is correct.
         */
        void updateList()
        {
            Vertex* p = next;

            while( p != this )
            {
                /**
                 * Remove duplicates
                 */
                if( *p == *p->next )
                {
                    p = p->prev;
                    p->next->remove();

                    if( p == p->next )
                        break;
                }

                p->updateOrder();
                p = p->next;
            };

            updateOrder();
            zSort();
        }

        /**
         * Sort all vertices in this vertex's list by their Morton code.
         */
        void zSort()
        {
            std::deque<Vertex*> queue;

            queue.push_back( this );

            for( Vertex* p = next; p && p != this; p = p->next )
                queue.push_back( p );

            std::sort( queue.begin(), queue.end(), []( const Vertex* a, const Vertex* b )
            {
                if( a->z != b->z )
                    return a->z < b->z;

                if( a->x != b->x )
                    return a->x < b->x;

                if( a->y != b->y )
                    return a->y < b->y;

                return a->i < b->i;
            } );

            Vertex* prev_elem = nullptr;

            for( Vertex* elem : queue )
            {
                if( prev_elem )
                    prev_elem->nextZ = elem;

                elem->prevZ = prev_elem;
                prev_elem = elem;
            }

            prev_elem->nextZ = nullptr;
        }

        const int    i;
        const double x;
        const double y;
        POLYGON_TEST* parent;

        // previous and next vertices nodes in a polygon ring
        Vertex* prev = nullptr;
        Vertex* next = nullptr;

        // z-order curve value
        int32_t z = 0;

        // previous and next nodes in z-order
        Vertex* prevZ = nullptr;
        Vertex* nextZ = nullptr;
    };

    /**
     * Calculate the Morton code of the Vertex
     * http://www.graphics.stanford.edu/~seander/bithacks.html#InterleaveBMN
     *
     */
    int32_t zOrder( const double aX, const double aY ) const
    {
        int32_t x = static_cast<int32_t>( 32767.0 * ( aX - m_bbox.GetX() ) / m_bbox.GetWidth() );
        int32_t y = static_cast<int32_t>( 32767.0 * ( aY - m_bbox.GetY() ) / m_bbox.GetHeight() );

        x = ( x | ( x << 8 ) ) & 0x00FF00FF;
        x = ( x | ( x << 4 ) ) & 0x0F0F0F0F;
        x = ( x | ( x << 2 ) ) & 0x33333333;
        x = ( x | ( x << 1 ) ) & 0x55555555;

        y = ( y | ( y << 8 ) ) & 0x00FF00FF;
        y = ( y | ( y << 4 ) ) & 0x0F0F0F0F;
        y = ( y | ( y << 2 ) ) & 0x33333333;
        y = ( y | ( y << 1 ) ) & 0x55555555;

        return x | ( y << 1 );
    }

    constexpr bool same_point( const Vertex* aA, const Vertex* aB ) const
    {
        return aA && aB && aA->x == aB->x && aA->y == aB->y;
    }

    Vertex* getNextOutlineVertex( const Vertex* aPt ) const
    {
        Vertex* nz = aPt->nextZ;
        Vertex* pz = aPt->prevZ;

        // If we hit a fracture point, we want to continue around the
        // edge we are working on and not switch to the pair edge
        // However, this will depend on which direction the initial
        // fracture hit is.  If we find that we skip directly to
        // a new fracture point, then we know that we are proceeding
        // in the wrong direction from the fracture and should
        // fall through to the next point
        if( same_point( aPt, nz )
                && aPt->y == aPt->next->y )
        {
            return nz->next;
        }

        if( same_point( aPt, pz )
                && aPt->y == aPt->next->y )
        {
            return pz->next;
        }

        return aPt->next;
    }

    Vertex* getPrevOutlineVertex( const Vertex* aPt ) const
    {
        Vertex* nz = aPt->nextZ;
        Vertex* pz = aPt->prevZ;

        // If we hit a fracture point, we want to continue around the
        // edge we are working on and not switch to the pair edge
        // However, this will depend on which direction the initial
        // fracture hit is.  If we find that we skip directly to
        // a new fracture point, then we know that we are proceeding
        // in the wrong direction from the fracture and should
        // fall through to the next point
        if( same_point( aPt, nz )
                && aPt->y == aPt->prev->y)
        {
            return nz->prev;
        }

        if( same_point( aPt, pz )
                && aPt->y == aPt->prev->y )
        {
            return pz->prev;
        }

        return aPt->prev;

    }

    /**
     * Checks to see if there is a "substantial" protrusion in each polygon produced by the cut from
     * aA to aB.  Substantial in this case means that the polygon bulges out to a wider cross-section
     * than the distance from aA to aB
     * @param aA Starting point in the polygon
     * @param aB Ending point in the polygon
     * @return True if the two polygons are both "substantial"
     */
    bool isSubstantial( const Vertex* aA, const Vertex* aB ) const
    {
        bool x_change = false;
        bool y_change = false;

        // This is a failsafe in case of invalid lists.  Never check
        // more than the total number of points in m_vertices
        size_t checked = 0;
        size_t total_pts = m_vertices.size();

        const Vertex* p0 = aA;
        const Vertex* p = getNextOutlineVertex( p0 );

        while( !same_point( p, aB ) && checked < total_pts && !( x_change && y_change ) )
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

            p = getNextOutlineVertex( p );

            ++checked;
        }

        wxCHECK_MSG( checked < total_pts, false, wxT( "Invalid polygon detected.  Missing points to check" ) );

        if( !x_change || !y_change )
            return false;

        p = getPrevOutlineVertex( p0 );

        x_change = false;
        y_change = false;
        checked = 0;

        while( !same_point( p, aB ) && checked < total_pts && !( x_change && y_change ) )
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

        return ( x_change && y_change );
    }

    /**
     * Take a #SHAPE_LINE_CHAIN and links each point into a circular, doubly-linked list.
     */
    Vertex* createList( const SHAPE_LINE_CHAIN& points )
    {
        Vertex* tail = nullptr;
        double sum = 0.0;

        // Check for winding order
        for( int i = 0; i < points.PointCount(); i++ )
        {
            VECTOR2D p1 = points.CPoint( i );
            VECTOR2D p2 = points.CPoint( i + 1 );

            sum += ( ( p2.x - p1.x ) * ( p2.y + p1.y ) );
        }

        if( sum > 0.0 )
        {
            for( int i = points.PointCount() - 1; i >= 0; i--)
                tail = insertVertex( i, points.CPoint( i ), tail );
        }
        else
        {
            for( int i = 0; i < points.PointCount(); i++ )
                tail = insertVertex( i, points.CPoint( i ), tail );
        }

        if( tail && ( *tail == *tail->next ) )
        {
            tail->next->remove();
        }

        return tail;
    }

    Vertex* getKink( Vertex* aPt ) const
    {
        // The point needs to be at a concave surface
        if( locallyInside( aPt->prev, aPt->next ) )
            return nullptr;

        // z-order range for the current point ± limit bounding box
        const int32_t     maxZ = zOrder( aPt->x + m_limit, aPt->y + m_limit );
        const int32_t     minZ = zOrder( aPt->x - m_limit, aPt->y - m_limit );

        // Subtract 1 to account for rounding inaccuracies in SquaredEuclideanNorm()
        // below.  We would usually test for rounding in the final value but since we
        // are working in squared integers here, we allow the 1nm slop rather than
        // force a separate calculation
        const SEG::ecoord limit2 = SEG::Square( m_limit - 1 );

        // first look for points in increasing z-order
        Vertex* p = aPt->nextZ;
        SEG::ecoord min_dist = std::numeric_limits<SEG::ecoord>::max();
        Vertex* retval = nullptr;

        while( p && p->z <= maxZ )
        {
            int delta_i = std::abs( p->i - aPt->i );
            VECTOR2D diff( p->x - aPt->x, p->y - aPt->y );
            SEG::ecoord dist2 = diff.SquaredEuclideanNorm();

            if( delta_i > 1 && dist2 < limit2 && dist2 < min_dist && dist2 > 0.0
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

            if( delta_i > 1 && dist2 < limit2 && dist2 < min_dist && dist2 > 0.0
                    && locallyInside( p, aPt ) && isSubstantial( p, aPt ) && isSubstantial( aPt, p ) )
            {
                min_dist = dist2;
                retval = p;
            }

            p = p->prevZ;
        }
        return retval;
    }


    /**
     * Return the twice the signed area of the triangle formed by vertices p, q, and r.
     */
    double area( const Vertex* p, const Vertex* q, const Vertex* r ) const
    {
        return ( q->y - p->y ) * ( r->x - q->x ) - ( q->x - p->x ) * ( r->y - q->y );
    }


    /**
     * Check whether the segment from vertex a -> vertex b is inside the polygon
     * around the immediate area of vertex a.
     *
     * We don't define the exact area over which the segment is inside but it is guaranteed to
     * be inside the polygon immediately adjacent to vertex a.
     *
     * @return true if the segment from a->b is inside a's polygon next to vertex a.
     */
    bool locallyInside( const Vertex* a, const Vertex* b ) const
    {
        const Vertex* an = getNextOutlineVertex( a );
        const Vertex* ap = getPrevOutlineVertex( a );

        if( area( ap, a, an ) < 0 )
            return area( a, b, an ) >= 0 && area( a, ap, b ) >= 0;
        else
            return area( a, b, ap ) < 0 || area( a, an, b ) < 0;
    }

    /**
     * Create an entry in the vertices lookup and optionally inserts the newly created vertex
     * into an existing linked list.
     *
     * @return a pointer to the newly created vertex.
     */
    Vertex* insertVertex( int aIndex, const VECTOR2I& pt, Vertex* last )
    {
        m_vertices.emplace_back( aIndex, pt.x, pt.y, this );

        Vertex* p = &m_vertices.back();

        if( !last )
        {
            p->prev = p;
            p->next = p;
        }
        else
        {
            p->next = last->next;
            p->prev = last;
            last->next->prev = p;
            last->next = p;
        }
        return p;
    }

private:
    int                             m_limit;
    BOX2I                           m_bbox;
    std::deque<Vertex>              m_vertices;
    std::set<std::pair<int, int>>   m_hits;
};


wxString DRC_TEST_PROVIDER_CONNECTION_WIDTH::layerDesc( PCB_LAYER_ID aLayer )
{
    return wxString::Format( wxT( "(%s)" ), m_drcEngine->GetBoard()->GetLayerName( aLayer ) );
}


bool DRC_TEST_PROVIDER_CONNECTION_WIDTH::Run()
{
    if( m_drcEngine->IsErrorLimitExceeded( DRCE_CONNECTION_WIDTH ) )
        return true;    // Continue with other tests

    if( !reportPhase( _( "Checking nets for minimum connection width..." ) ) )
        return false;   // DRC cancelled

    LSET          copperLayerSet = m_drcEngine->GetBoard()->GetEnabledLayers() & LSET::AllCuMask();
    LSEQ          copperLayers = copperLayerSet.Seq();
    BOARD*        board = m_drcEngine->GetBoard();

    /*
     * Build a set of distinct minWidths specified by various DRC rules.  We'll run a test for
     * each distinct minWidth, and then decide if any copper which failed that minWidth actually
     * was required to abide by it or not.
     */
    std::set<int> distinctMinWidths
                        = m_drcEngine->QueryDistinctConstraints( CONNECTION_WIDTH_CONSTRAINT );

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
                {
                    item->TransformShapeToPolygon( itemsPoly.Poly, aLayer, 0, ARC_HIGH_DEF,
                                                   ERROR_OUTSIDE );
                }

                itemsPoly.Poly.Fracture( SHAPE_POLY_SET::PM_FAST );

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

                POLYGON_TEST test( aMinWidth );

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

                        for( auto* item : board->m_CopperItemRTreeCache->GetObjectsAt( location,
                                                                                       aLayer,
                                                                                       aMinWidth ) )
                        {
                            if( item->HitTest( location, aMinWidth ) )
                                contributingItems.push_back( item );
                        }

                        for( auto& [ zone, rtree ] : board->m_CopperZoneRTreeCache )
                        {
                            if( !rtree->GetObjectsAt( location, aLayer, aMinWidth ).empty()
                                    && zone->HitTestFilledArea( aLayer, location, aMinWidth ) )
                            {
                                contributingItems.push_back( zone );
                            }
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
                                auto     drce = DRC_ITEM::Create( DRCE_CONNECTION_WIDTH );
                                wxString msg;

                                msg = formatMsg( _( "(%s minimum connection width %s; actual %s)" ),
                                                 c.GetName(),
                                                 aMinWidth,
                                                 dist );

                                msg += wxS( " " ) + layerDesc( aLayer );

                                drce->SetErrorMessage( drce->GetErrorText() + wxS( " " ) + msg );
                                drce->SetViolatingRule( c.GetParentRule() );

                                for( BOARD_ITEM* item : contributingItems )
                                    drce->AddItem( item );

                                reportViolation( drce, location, aLayer );
                            }
                        }
                    }
                }

                done.fetch_add( calc_effort( aItemsPoly.Items, aLayer ) );

                return 1;
            };

    for( PCB_LAYER_ID layer : copperLayers )
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
    std::vector<std::future<size_t>> returns;
    size_t                           total_effort = 0;

    for( const auto& [ netLayer, itemsPoly ] : dataset )
        total_effort += calc_effort( itemsPoly.Items, netLayer.Layer );

    total_effort += std::max( (size_t) 1, total_effort ) * distinctMinWidths.size();

    returns.reserve( dataset.size() );

    for( const auto& [ netLayer, itemsPoly ] : dataset )
    {
        returns.emplace_back( tp.submit( build_netlayer_polys, netLayer.Netcode, netLayer.Layer ) );
    }

    for( std::future<size_t>& ret : returns )
    {
        std::future_status status = ret.wait_for( std::chrono::milliseconds( 250 ) );

        while( status != std::future_status::ready )
        {
            m_drcEngine->ReportProgress( static_cast<double>( done ) / total_effort );
            status = ret.wait_for( std::chrono::milliseconds( 250 ) );
        }
    }

    returns.clear();
    returns.reserve( dataset.size() * distinctMinWidths.size() );

    for( const auto& [ netLayer, itemsPoly ] : dataset )
    {
        for( int minWidth : distinctMinWidths )
            returns.emplace_back( tp.submit( min_checker, itemsPoly, netLayer.Layer, minWidth ) );
    }

    for( std::future<size_t>& ret : returns )
    {
        std::future_status status = ret.wait_for( std::chrono::milliseconds( 250 ) );

        while( status != std::future_status::ready )
        {
            m_drcEngine->ReportProgress( static_cast<double>( done ) / total_effort );
            status = ret.wait_for( std::chrono::milliseconds( 250 ) );
        }
    }

    return true;
}


namespace detail
{
static DRC_REGISTER_TEST_PROVIDER<DRC_TEST_PROVIDER_CONNECTION_WIDTH> dummy;
}
