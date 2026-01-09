/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <board.h>
#include <board_design_settings.h>
#include <connectivity/connectivity_data.h>
#include <zone.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_shape.h>
#include <pcb_track.h>
#include <thread_pool.h>

#include <geometry/shape_line_chain.h>
#include <geometry/shape_poly_set.h>
#include <drc/drc_item.h>
#include <drc/drc_test_provider.h>


/*
    This loads some rule resolvers for the ZONE_FILLER, and checks that pad thermal relief
    connections have at least the required number of spokes.

    Errors generated:
    - DRCE_STARVED_THERMAL
*/

class DRC_TEST_PROVIDER_ZONE_CONNECTIONS : public DRC_TEST_PROVIDER
{
public:
    DRC_TEST_PROVIDER_ZONE_CONNECTIONS()
    {}

    virtual ~DRC_TEST_PROVIDER_ZONE_CONNECTIONS() = default;

    virtual bool Run() override;

    virtual const wxString GetName() const override { return wxT( "zone connections" ); };

private:
    void testZoneLayer( ZONE* aZone, PCB_LAYER_ID aLayer );
};


void DRC_TEST_PROVIDER_ZONE_CONNECTIONS::testZoneLayer( ZONE* aZone, PCB_LAYER_ID aLayer )
{
    BOARD*                             board = m_drcEngine->GetBoard();
    BOARD_DESIGN_SETTINGS&             bds = board->GetDesignSettings();
    std::shared_ptr<CONNECTIVITY_DATA> connectivity = board->GetConnectivity();
    DRC_CONSTRAINT                     constraint;

    const std::shared_ptr<SHAPE_POLY_SET>& zoneFill = aZone->GetFilledPolysList( aLayer );
    ISOLATED_ISLANDS                       isolatedIslands;

    auto zoneIter = board->m_ZoneIsolatedIslandsMap.find( aZone );

    if( zoneIter != board->m_ZoneIsolatedIslandsMap.end() )
    {
        auto layerIter = zoneIter->second.find( aLayer );

        if( layerIter != zoneIter->second.end() )
            isolatedIslands = layerIter->second;
    }

    for( FOOTPRINT* footprint : board->Footprints() )
    {
        for( PAD* pad : footprint->Pads() )
        {
            if( m_drcEngine->IsErrorLimitExceeded( DRCE_STARVED_THERMAL ) )
                return;

            if( m_drcEngine->IsCancelled() )
                return;

            //
            // Quick tests for "connected":
            //

            if( pad->GetNetCode() != aZone->GetNetCode() || pad->GetNetCode() <= 0 )
                continue;

            BOX2I item_bbox = pad->GetBoundingBox();

            if( !item_bbox.Intersects( aZone->GetBoundingBox() ) )
                continue;

            if( !pad->FlashLayer( aLayer ) )
                continue;

            //
            // If those passed, do a thorough test:
            //

            constraint = bds.m_DRCEngine->EvalZoneConnection( pad, aZone, aLayer );
            ZONE_CONNECTION conn = constraint.m_ZoneConnection;

            if( conn != ZONE_CONNECTION::THERMAL )
                continue;

            constraint = bds.m_DRCEngine->EvalRules( MIN_RESOLVED_SPOKES_CONSTRAINT, pad, aZone, aLayer );
            int minCount = constraint.m_Value.Min();

            if( constraint.GetSeverity() == RPT_SEVERITY_IGNORE || minCount <= 0 )
                continue;

            constraint = bds.m_DRCEngine->EvalRules( THERMAL_RELIEF_GAP_CONSTRAINT, pad, aZone, aLayer );
            int mid_gap = constraint.m_Value.Min() / 2;

            SHAPE_POLY_SET padPoly;
            pad->TransformShapeToPolygon( padPoly, aLayer, mid_gap, ARC_LOW_DEF, ERROR_OUTSIDE );

            SHAPE_LINE_CHAIN& padOutline = padPoly.Outline( 0 );
            BOX2I             padBBox( padOutline.BBox() );
            int               spokes = 0;
            int               ignoredSpokes = 0;
            VECTOR2I          ignoredSpokePos;

            for( int jj = 0; jj < zoneFill->OutlineCount(); ++jj )
            {
                std::vector<SHAPE_LINE_CHAIN::INTERSECTION> intersections;

                zoneFill->Outline( jj ).Intersect( padOutline, intersections, true, &padBBox );

                std::vector<SHAPE_LINE_CHAIN::INTERSECTION> unique_intersections;

                for( const SHAPE_LINE_CHAIN::INTERSECTION& i : intersections )
                {
                    const auto found = std::find_if(
                            std::begin( unique_intersections ), std::end( unique_intersections ),
                            [&]( const SHAPE_LINE_CHAIN::INTERSECTION& j ) -> bool
                            {
                                return ( j.p == i.p );
                            } );

                    if( found == std::end( unique_intersections ) )
                        unique_intersections.emplace_back( i );
                }

                // If we connect to an island that only connects to a single item then we *are*
                // that item.  Thermal spokes to this (otherwise isolated) island don't provide
                // electrical connectivity to anything, so we don't count them.
                if( unique_intersections.size() >= 2 )
                {
                    if( alg::contains( isolatedIslands.m_SingleConnectionOutlines, jj ) )
                    {
                        ignoredSpokes += (int) unique_intersections.size() / 2;
                        ignoredSpokePos = ( unique_intersections[0].p + unique_intersections[1].p ) / 2;
                    }
                    else
                    {
                        spokes += (int) unique_intersections.size() / 2;
                    }
                }
            }

            if( spokes == 0 && ignoredSpokes == 0 )     // Not connected at all
                continue;

            int customSpokes = 0;

            if( pad->GetShape( aLayer ) == PAD_SHAPE::CUSTOM )
            {
                for( const std::shared_ptr<PCB_SHAPE>& primitive : pad->GetPrimitives( aLayer ) )
                {
                    if( primitive->IsProxyItem() && primitive->GetShape() == SHAPE_T::SEGMENT )
                        customSpokes++;
                }
            }

            if( customSpokes > 0 )
            {
                if( spokes < customSpokes )
                {
                    std::shared_ptr<DRC_ITEM> drce = DRC_ITEM::Create( DRCE_STARVED_THERMAL );
                    VECTOR2I                  pos;

                    if( ignoredSpokes )
                    {
                        drce->SetErrorDetail( wxString::Format( _( "(layer %s; %d spokes connected to isolated island)" ),
                                                                board->GetLayerName( aLayer ),
                                                                ignoredSpokes ) );
                        pos = ignoredSpokePos;
                    }
                    else
                    {
                        drce->SetErrorDetail( wxString::Format( _( "(layer %s; %s custom spoke count %d; actual %d)" ),
                                                                board->GetLayerName( aLayer ),
                                                                constraint.GetName(),
                                                                customSpokes,
                                                                spokes ) );
                        pos = pad->GetPosition();
                    }

                    drce->SetItems( aZone, pad );
                    drce->SetViolatingRule( constraint.GetParentRule() );

                    reportViolation( drce, pos, aLayer );
                }

                continue;
            }

            if( spokes >= minCount )                    // We already have enough
                continue;

            //
            // See if there are any other manual spokes added:
            //

            for( PCB_TRACK* track : connectivity->GetConnectedTracks( pad ) )
            {
                if( padOutline.PointInside( track->GetStart() ) )
                {
                    if( aZone->GetFilledPolysList( aLayer )->Collide( track->GetEnd() ) )
                        spokes++;
                }
                else if( padOutline.PointInside( track->GetEnd() ) )
                {
                    if( aZone->GetFilledPolysList( aLayer )->Collide( track->GetStart() ) )
                        spokes++;
                }
            }

            for( BOARD_CONNECTED_ITEM* item : connectivity->GetConnectedItems( pad, EXCLUDE_ZONES ) )
            {
                PCB_SHAPE* shape = dynamic_cast<PCB_SHAPE*>( item );

                if( !shape || !shape->IsOnLayer( aLayer ) )
                    continue;

                std::vector<VECTOR2I> connectionPts = shape->GetConnectionPoints();

                for( const VECTOR2I& pt : connectionPts )
                {
                    if( padOutline.PointInside( pt ) )
                    {
                        for( const VECTOR2I& other : connectionPts )
                        {
                            if( other != pt && zoneFill->Collide( other ) )
                            {
                                spokes++;
                                break;
                            }
                        }

                        break;
                    }
                }
            }

            //
            // If we're *only* connected to isolated islands, then ignore the fact that they're
            // isolated.  (We leave that for the connectivity tester, which checks connections on
            // all layers.)
            //

            if( spokes == 0 )
            {
                spokes += ignoredSpokes;
                ignoredSpokes = 0;
            }

            //
            // And finally report it if there aren't enough:
            //

            if( spokes < minCount )
            {
                std::shared_ptr<DRC_ITEM> drce = DRC_ITEM::Create( DRCE_STARVED_THERMAL );
                VECTOR2I                  pos;

                if( ignoredSpokes )
                {
                    drce->SetErrorDetail( wxString::Format( _( "(layer %s; %d spokes connected to isolated island)" ),
                                                            board->GetLayerName( aLayer ),
                                                            ignoredSpokes ) );
                    pos = ignoredSpokePos;
                }
                else
                {
                    drce->SetErrorDetail( wxString::Format( _( "(layer %s; %s min spoke count %d; actual %d)" ),
                                                            board->GetLayerName( aLayer ),
                                                            constraint.GetName(),
                                                            minCount,
                                                            spokes ) );
                    pos = pad->GetPosition();
                }

                drce->SetItems( aZone, pad );
                drce->SetViolatingRule( constraint.GetParentRule() );

                reportViolation( drce, pos, aLayer );
            }
        }
    }
}


bool DRC_TEST_PROVIDER_ZONE_CONNECTIONS::Run()
{
    BOARD*                             board = m_drcEngine->GetBoard();
    std::shared_ptr<CONNECTIVITY_DATA> connectivity = board->GetConnectivity();
    DRC_CONSTRAINT                     constraint;

    if( !reportPhase( _( "Checking thermal reliefs..." ) ) )
        return false;   // DRC cancelled

    std::vector< std::pair<ZONE*, PCB_LAYER_ID> > zoneLayers;
    std::atomic<size_t> done( 1 );
    size_t total_effort = 0;

    for( ZONE* zone : board->m_DRCCopperZones )
    {
        if( !zone->IsTeardropArea() )
        {
            for( PCB_LAYER_ID layer : zone->GetLayerSet() )
            {
                zoneLayers.push_back( { zone, layer } );
                total_effort += zone->GetFilledPolysList( layer )->FullPointCount();
            }
        }
    }

    total_effort = std::max( (size_t) 1, total_effort );

    thread_pool& tp = GetKiCadThreadPool();
    auto returns = tp.submit_loop( 0, zoneLayers.size(),
                            [&]( const int ii )
                            {
                                if( !m_drcEngine->IsCancelled() )
                                {
                                    testZoneLayer( zoneLayers[ii].first, zoneLayers[ii].second );
                                    done.fetch_add( zoneLayers[ii].first->GetFilledPolysList( zoneLayers[ii].second )->FullPointCount() );
                                }
                            } );

    for( auto& ret : returns )
    {
        std::future_status status = ret.wait_for( std::chrono::milliseconds( 250 ) );

        while( status != std::future_status::ready )
        {
            reportProgress( done, total_effort );
            status = ret.wait_for( std::chrono::milliseconds( 250 ) );
        }
    }

    return !m_drcEngine->IsCancelled();
}


namespace detail
{
static DRC_REGISTER_TEST_PROVIDER<DRC_TEST_PROVIDER_ZONE_CONNECTIONS> dummy;
}
