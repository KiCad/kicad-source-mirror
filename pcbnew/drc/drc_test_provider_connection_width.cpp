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

#include <advanced_config.h>
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
    POLYGON_TEST( int aLimit, int aErrorLimit ) :
        m_limit( aLimit ), m_max_error( aErrorLimit )
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
            if( ( all_hits.empty() || all_hits.count( p ) == 0 ) && ( match = getKink( p ) ) )
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
                return a->z < b->z;
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

    /**
     * Checks to see if there is a "substantial" protrusion in each polygon produced by the cut from
     * aA to aB.  Substantial in this case means that the polygon bulges out to a wider cross-section
     * than the distance from aA to aB
     * @param aA Starting point in the polygon
     * @param aB Ending point in the polygon
     * @return True if the two polygons are both "substantial"
     */
    bool isSubstantial( Vertex* aA, Vertex* aB ) const
    {
        // `directions` is a bitfield where
        // bit 0 = pos y
        // bit 1 = neg y
        // bit 2 = pos x
        // bit 3 = neg x
        // So, once directions = 15, we have all directions
        int directions = 0;

        // This is a failsafe in case of invalid lists.  Never check
        // more than the total number of points in m_vertices
        size_t checked = 0;
        size_t total_pts = m_vertices.size();

        Vertex* p0 = aA;
        Vertex* p;
        Vertex* nz = p0->nextZ;
        Vertex* pz = p0->prevZ;

        auto same_point =
                []( const Vertex* a, const Vertex* b ) -> bool
                {
                    return a && b && a->x == b->x && a->y == b->y;
                };

        // If we hit a fracture point, we want to continue around the
        // edge we are working on and not switch to the pair edge
        // However, this will depend on which direction the initial
        // fracture hit is.  If we find that we skip directly to
        // a new fracture point, then we know that we are proceeding
        // in the wrong direction from the fracture and should
        // fall through to the next point
        if( same_point( p0, nz )
                && !( same_point( nz->next, nz->next->prevZ ) || same_point( nz->next, nz->next->nextZ ) ) )
        {
            p = nz->next;
        }
        else if( same_point( p0, pz )
                && !( same_point( pz->next, pz->next->prevZ ) || same_point( pz->next, pz->next->nextZ ) ) )
        {
            p = pz->next;
        }
        else
        {
            p = p0->next;
        }

        while( p0 != aB && checked < total_pts && directions != 0b1111 )
        {
            double diff_x = std::abs( p->x - p0->x );
            double diff_y = std::abs( p->y - p0->y );

            // Floating point zeros can have a negative sign, so we need to
            // ensure that only substantive diversions count for a direction
            // change
            if( diff_x > m_max_error )
                directions |= ( 1 << ( 2 + std::signbit( p->x - p0->x ) ) );

            if( diff_y > m_max_error )
                directions |= ( 1 << std::signbit( p->y - p0->y ) );

            // In the case of a circle, we need to eventually get the direction
            // so keep the p0 at the same point
            if( diff_x > m_max_error || diff_y > m_max_error || p == aB )
                p0 = p;

            if( same_point( p, p->nextZ ) )
                p = p->nextZ->next;
            else if( same_point( p, p->prevZ ) )
                p = p->prevZ->next;
            else
                p = p->next;

            ++checked;
        }

        wxCHECK_MSG( checked < total_pts, false, wxT( "Invalid polygon detected.  Missing points to check" ) );

        if( directions != 15 )
            return false;

        p0 = aA;
        nz = p0->nextZ;
        pz = p0->prevZ;

        if( nz && same_point( p0, nz )
                && !( same_point( nz->prev, nz->prev->nextZ ) || same_point( nz->prev, nz->prev->prevZ ) ) )
        {
            p = nz->prev;
        }
        else if( pz && same_point( p0, pz )
                && !( same_point( pz->prev, pz->prev->nextZ ) || same_point( pz->prev, pz->prev->prevZ ) ) )
        {
            p = pz->prev;
        }
        else
        {
            p = p0->prev;
        }

        directions = 0;
        checked = 0;

        while( p0 != aB && checked < total_pts && directions != 0b1111 )
        {
            double diff_x = std::abs( p->x - p0->x );
            double diff_y = std::abs( p->y - p0->y );

            // Floating point zeros can have a negative sign, so we need to
            // ensure that only substantive diversions count for a direction
            // change
            if( diff_x > m_max_error )
                directions |= ( 1 << ( 2 + std::signbit( p->x - p0->x ) ) );

            if( diff_y > m_max_error )
                directions |= ( 1 << std::signbit( p->y - p0->y ) );

            // In the case of a circle, we need to eventually get the direction
            // so keep the p0 at the same point
            if( diff_x > m_max_error || diff_y > m_max_error || p == aB )
                p0 = p;

            if( same_point( p, p->nextZ ) )
                p = p->nextZ->prev;
            else if( same_point( p, p->prevZ ) )
                p = p->prevZ->prev;
            else
                p = p->prev;

            ++checked;
        }

        wxCHECK_MSG( checked < total_pts, false, wxT( "Invalid polygon detected.  Missing points to check" ) );

        return ( directions == 15 );
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
        const SEG::ecoord limit2 = SEG::Square( m_limit );

        // first look for points in increasing z-order
        Vertex* p = aPt->nextZ;
        int64_t min_dist = std::numeric_limits<int64_t>::max();
        Vertex* retval = nullptr;

        while( p && p->z <= maxZ )
        {
            int delta_i = std::abs( p->i - aPt->i );
            VECTOR2D diff( p->x - aPt->x, p->y - aPt->y );
            SEG::ecoord dist2 = diff.SquaredEuclideanNorm();

            if( delta_i > 1 && dist2 < limit2 && dist2 < min_dist && dist2 > 0.0
                    && locallyInside( p, aPt ) && isSubstantial( p, aPt ) )
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
                    && locallyInside( p, aPt ) && isSubstantial( p, aPt ) )
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
        if( area( a->prev, a, a->next ) < 0 )
            return area( a, b, a->next ) >= 0 && area( a, a->prev, b ) >= 0;
        else
            return area( a, b, a->prev ) < 0 || area( a, a->next, b ) < 0;
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
    double                          m_max_error;
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
    DRC_RTREE*    tree = board->m_CopperItemRTreeCache.get();
    std::set<int> distinctMinWidths
                        = m_drcEngine->QueryDistinctConstraints( CONNECTION_WIDTH_CONSTRAINT );

    if( m_drcEngine->IsCancelled() )
        return false;   // DRC cancelled

    struct ITEMS_POLY
    {
        std::set<BOARD_ITEM*> items;
        SHAPE_POLY_SET        poly;
    };

    std::map< std::pair<int, PCB_LAYER_ID>, ITEMS_POLY > dataset;
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

    auto build_netlayer_polys =
            [&]( int aNetcode, const PCB_LAYER_ID aLayer ) -> size_t
            {
                if( m_drcEngine->IsCancelled() )
                    return 0;

                ITEMS_POLY& data = dataset[ { aNetcode, aLayer } ];

                for( BOARD_ITEM* item : data.items )
                {
                    item->TransformShapeWithClearanceToPolygon( data.poly, aLayer, 0,
                                                                ARC_HIGH_DEF, ERROR_OUTSIDE );
                }

                data.poly.Fracture( SHAPE_POLY_SET::PM_FAST );

                done.fetch_add( calc_effort( data.items, aLayer ) );

                return 1;
            };

    auto min_checker =
            [&]( const ITEMS_POLY& aDataset, const PCB_LAYER_ID aLayer, int aMinWidth ) -> size_t
            {
                if( m_drcEngine->IsCancelled() )
                    return 0;

                POLYGON_TEST test( aMinWidth, m_drcEngine->GetDesignSettings()->m_MaxError );

                for( int ii = 0; ii < aDataset.poly.OutlineCount(); ++ii )
                {
                    const SHAPE_LINE_CHAIN& chain = aDataset.poly.COutline( ii );

                    test.FindPairs( chain );
                    auto& ret = test.GetVertices();

                    for( const std::pair<int, int>& pt : ret )
                    {
                        SEG span( chain.CPoint( pt.first ), chain.CPoint( pt.second ) );
                        VECTOR2I location = ( span.A + span.B ) / 2;
                        int dist = ( span.A - span.B ).EuclideanNorm();
                        std::set<BOARD_ITEM*> nearbyItems = tree->GetObjectsAt( location, aLayer,
                                                                                aMinWidth );

                        std::vector<BOARD_ITEM*> items;

                        for( BOARD_ITEM* item : nearbyItems )
                        {
                            if( item->HitTest( location, aMinWidth ) )
                                items.push_back( item );
                        }

                        for( auto& [ zone, rtree ] : board->m_CopperZoneRTreeCache )
                        {
                            if( !rtree->GetObjectsAt( location, aLayer, aMinWidth ).empty() &&
                                    zone->HitTestFilledArea( aLayer, location, aMinWidth ) )
                                items.push_back( zone );
                        }

                        if( !items.empty() )
                        {
                            DRC_CONSTRAINT c = m_drcEngine->EvalRules( CONNECTION_WIDTH_CONSTRAINT,
                                                                       items[0],
                                                                       items.size() > 1 ? items[1]
                                                                                        : nullptr,
                                                                       aLayer );

                            if( c.Value().Min() == aMinWidth )
                            {
                                auto     drce = DRC_ITEM::Create( DRCE_CONNECTION_WIDTH );
                                wxString msg;

                                msg.Printf( _( "Minimum connection width %s; actual %s" ),
                                              MessageTextFromValue( userUnits(), aMinWidth ),
                                              MessageTextFromValue( userUnits(), dist ) );

                                drce->SetErrorMessage( msg + wxS( " " ) + layerDesc( aLayer ) );
                                drce->SetViolatingRule( c.GetParentRule() );

                                for( BOARD_ITEM* item : items )
                                    drce->AddItem( item );

                                reportViolation( drce, location, aLayer );
                            }
                        }
                    }
                }

                done.fetch_add( calc_effort( aDataset.items, aLayer ) );

                return 1;
            };

    for( PCB_LAYER_ID layer : copperLayers )
    {
        for( ZONE* zone : board->m_DRCCopperZones )
        {
            if( !zone->GetIsRuleArea() && zone->IsOnLayer( layer ) )
                dataset[ { zone->GetNetCode(), layer } ].items.emplace( zone );
        }

        for( PCB_TRACK* track : board->Tracks() )
        {
            if( PCB_VIA* via = dynamic_cast<PCB_VIA*>( track ) )
            {
                if( via->FlashLayer( static_cast<int>( layer ) ) )
                    dataset[ { via->GetNetCode(), layer } ].items.emplace( via );
            }
            else if( track->IsOnLayer( layer ) )
            {
                dataset[ { track->GetNetCode(), layer } ].items.emplace( track );
            }
        }

        for( FOOTPRINT* fp : board->Footprints() )
        {
            for( PAD* pad : fp->Pads() )
            {
                if( pad->FlashLayer( static_cast<int>( layer ) ) )
                    dataset[ { pad->GetNetCode(), layer } ].items.emplace( pad );
            }

            // Footprint zones are also in the m_DRCCopperZones cache
        }
    }

    thread_pool&                     tp = GetKiCadThreadPool();
    std::vector<std::future<size_t>> returns;
    size_t                           total_effort = 0;

    for( const std::pair<const std::pair<int, PCB_LAYER_ID>, ITEMS_POLY>& netLayer : dataset )
        total_effort += calc_effort( netLayer.second.items, netLayer.first.second );

    total_effort += std::max( (size_t) 1, total_effort ) * distinctMinWidths.size();

    returns.reserve( dataset.size() );

    for( const std::pair<const std::pair<int, PCB_LAYER_ID>, ITEMS_POLY>& netLayer : dataset )
    {
        returns.emplace_back( tp.submit( build_netlayer_polys, netLayer.first.first,
                                         netLayer.first.second ) );
    }

    for( std::future<size_t>& retval : returns )
    {
        std::future_status status;

        do
        {
            m_drcEngine->ReportProgress( static_cast<double>( done ) / total_effort );
            status = retval.wait_for( std::chrono::milliseconds( 250 ) );
        }
        while( status != std::future_status::ready );
    }

    returns.clear();
    returns.reserve( dataset.size() * distinctMinWidths.size() );

    for( const std::pair<const std::pair<int, PCB_LAYER_ID>, ITEMS_POLY>& netLayer : dataset )
    {
        for( int minWidth : distinctMinWidths )
        {
            returns.emplace_back( tp.submit( min_checker, netLayer.second, netLayer.first.second,
                                             minWidth ) );
        }
    }

    for( std::future<size_t>& retval : returns )
    {
        std::future_status status;

        do
        {
            m_drcEngine->ReportProgress( static_cast<double>( done ) / total_effort );
            status = retval.wait_for( std::chrono::milliseconds( 250 ) );
        }
        while( status != std::future_status::ready );
    }

    return true;
}


namespace detail
{
static DRC_REGISTER_TEST_PROVIDER<DRC_TEST_PROVIDER_CONNECTION_WIDTH> dummy;
}
