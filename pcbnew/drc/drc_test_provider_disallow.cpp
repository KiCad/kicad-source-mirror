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

#include <atomic>
#include <common.h>
#include <board_design_settings.h>
#include <drc/drc_rtree.h>
#include <drc/drc_engine.h>
#include <drc/drc_item.h>
#include <drc/drc_rule.h>
#include <drc/drc_test_provider.h>
#include <pad.h>
#include <progress_reporter.h>
#include <thread_pool.h>
#include <zone.h>
#include <pcb_track.h>
#include <mutex>


/*
    "Disallow" test. Goes through all items, matching types/conditions drop errors.
    Errors generated:
    - DRCE_ALLOWED_ITEMS
    - DRCE_TEXT_ON_EDGECUTS
*/

class DRC_TEST_PROVIDER_DISALLOW : public DRC_TEST_PROVIDER
{
public:
    DRC_TEST_PROVIDER_DISALLOW()
    {}

    virtual ~DRC_TEST_PROVIDER_DISALLOW() = default;

    virtual bool Run() override;

    virtual const wxString GetName() const override { return wxT( "disallow" ); };
};


bool DRC_TEST_PROVIDER_DISALLOW::Run()
{
    if( !reportPhase( _( "Checking keepouts & disallow constraints..." ) ) )
        return false;   // DRC cancelled

    BOARD* board = m_drcEngine->GetBoard();
    int    epsilon = board->GetDesignSettings().GetDRCEpsilon();

    // First build out the board's cache of copper-keepout to copper-zone caches.  This is where
    // the bulk of the time is spent, and we can do this in parallel.
    //
    std::vector<ZONE*>                   antiCopperKeepouts;
    std::vector<ZONE*>                   copperZones;
    std::vector<std::pair<ZONE*, ZONE*>> toCache;
    std::atomic<size_t>                  done( 1 );
    int                                  totalCount = 0;
    std::unique_ptr<DRC_RTREE>           antiTrackKeepouts = std::make_unique<DRC_RTREE>();

    forEachGeometryItem( { PCB_ZONE_T }, LSET::AllLayersMask(),
            [&]( BOARD_ITEM* item ) -> bool
            {
                ZONE* zone = static_cast<ZONE*>( item );

                if( zone->GetIsRuleArea() && zone->GetDoNotAllowZoneFills() )
                {
                    antiCopperKeepouts.push_back( zone );
                }
                else if( zone->GetIsRuleArea() && zone->GetDoNotAllowTracks() )
                {
                    for( PCB_LAYER_ID layer : zone->GetLayerSet() )
                        antiTrackKeepouts->Insert( zone, layer );
                }
                else if( zone->IsOnCopperLayer() )
                {
                    copperZones.push_back( zone );
                }

                totalCount++;

                return true;
            } );

    for( ZONE* ruleArea : antiCopperKeepouts )
    {
        for( ZONE* copperZone : copperZones )
        {
            toCache.push_back( { ruleArea, copperZone } );
            totalCount++;
        }
    }

    auto query_areas =
            [&]( const int idx ) -> size_t
            {
                if( m_drcEngine->IsCancelled() )
                    return 0;
                const auto& areaZonePair = toCache[idx];
                ZONE* ruleArea = areaZonePair.first;
                ZONE* copperZone = areaZonePair.second;
                BOX2I areaBBox = ruleArea->GetBoundingBox();
                BOX2I copperBBox = copperZone->GetBoundingBox();
                bool  isInside = false;

                if( copperZone->IsFilled() && areaBBox.Intersects( copperBBox ) )
                {
                    // Collisions include touching, so we need to deflate outline by enough to
                    // exclude it.  This is particularly important for detecting copper fills as
                    // they will be exactly touching along the entire exclusion border.
                    SHAPE_POLY_SET areaPoly = ruleArea->Outline()->CloneDropTriangulation();
                    areaPoly.Fracture();
                    areaPoly.Deflate( epsilon, CORNER_STRATEGY::ALLOW_ACUTE_CORNERS, ARC_LOW_DEF );

                    DRC_RTREE* zoneRTree = board->m_CopperZoneRTreeCache[ copperZone ].get();

                    if( zoneRTree )
                    {
                        for( size_t ii = 0; ii < ruleArea->GetLayerSet().size(); ++ii )
                        {
                            if( ruleArea->GetLayerSet().test( ii ) )
                            {
                                PCB_LAYER_ID layer = PCB_LAYER_ID( ii );

                                if( zoneRTree->QueryColliding( areaBBox, &areaPoly, layer ) )
                                {
                                    isInside = true;
                                    break;
                                }

                                if( m_drcEngine->IsCancelled() )
                                    return 0;
                            }
                        }
                    }
                }

                if( m_drcEngine->IsCancelled() )
                    return 0;

                PTR_PTR_LAYER_CACHE_KEY key = { ruleArea, copperZone, UNDEFINED_LAYER };

                {
                    std::unique_lock<std::shared_mutex> writeLock( board->m_CachesMutex );
                    board->m_IntersectsAreaCache[ key ] = isInside;
                }

                done.fetch_add( 1 );

                return 1;
            };

    thread_pool& tp = GetKiCadThreadPool();
    auto futures = tp.submit_loop( 0, toCache.size(), query_areas );

    for( auto& ret : futures )
    {
        std::future_status status = ret.wait_for( std::chrono::milliseconds( 250 ) );

        while( status != std::future_status::ready )
        {
            reportProgress( done, toCache.size() );
            status = ret.wait_for( std::chrono::milliseconds( 250 ) );
        }
    }

    if( m_drcEngine->IsCancelled() )
        return false;

    // Now go through all the board objects calling the DRC_ENGINE to run the actual disallow
    // tests.  These should be reasonably quick using the caches generated above.
    //
    const int progressDelta = 250;
    int       ii = static_cast<int>( toCache.size() );

    auto checkTextOnEdgeCuts =
            [&]( BOARD_ITEM* item )
            {
                if( item->Type() == PCB_FIELD_T
                        || item->Type() == PCB_TEXT_T
                        || item->Type() == PCB_TEXTBOX_T
                        || BaseType( item->Type() ) == PCB_DIMENSION_T )
                {
                    if( item->GetLayer() == Edge_Cuts )
                    {
                        std::shared_ptr<DRC_ITEM> drc = DRC_ITEM::Create( DRCE_TEXT_ON_EDGECUTS );
                        drc->SetItems( item );
                        reportViolation( drc, item->GetPosition(), Edge_Cuts );
                    }
                }
            };

    auto checkAntiTrackKeepout =
            [&]( PCB_TRACK* track, ZONE* keepout )
            {
                std::shared_ptr<SHAPE> shape = track->GetEffectiveShape();
                int                    dummyActual;
                VECTOR2I               pos;

                if( keepout->Outline()->Collide( shape.get(), board->m_DRCMaxClearance,
                                                 &dummyActual, &pos ) )
                {
                    std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_ALLOWED_ITEMS );

                    drcItem->SetItems( track );
                    reportViolation( drcItem, pos, track->GetLayerSet().ExtractLayer() );
                }
            };

    auto checkDisallow =
            [&]( BOARD_ITEM* item )
            {
                DRC_CONSTRAINT constraint = m_drcEngine->EvalRules( DISALLOW_CONSTRAINT, item,
                                                                    nullptr, UNDEFINED_LAYER );

                if( constraint.m_DisallowFlags && constraint.GetSeverity() != RPT_SEVERITY_IGNORE )
                {
                    std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_ALLOWED_ITEMS );
                    PCB_LAYER_ID              layer = item->GetLayerSet().ExtractLayer();

                    // Implicit rules reported in checkAntiTrackKeepout
                    if( constraint.GetParentRule()->IsImplicit() )
                        return;

                    drcItem->SetErrorDetail( wxString::Format( wxS( "(%s)" ), constraint.GetName() ) );
                    drcItem->SetItems( item );
                    drcItem->SetViolatingRule( constraint.GetParentRule() );

                    reportViolation( drcItem, item->GetPosition(), layer );
                }
            };

    forEachGeometryItem( {}, LSET::AllLayersMask(),
            [&]( BOARD_ITEM* item ) -> bool
            {
                if( !m_drcEngine->IsErrorLimitExceeded( DRCE_TEXT_ON_EDGECUTS ) )
                    checkTextOnEdgeCuts( item );

                if( !m_drcEngine->IsErrorLimitExceeded( DRCE_ALLOWED_ITEMS ) )
                {
                    if( ZONE* zone = dynamic_cast<ZONE*>( item ) )
                    {
                        if( zone->GetIsRuleArea() && zone->HasKeepoutParametersSet() )
                            return true;
                    }

                    item->ClearFlags( HOLE_PROXY );     // Just in case

                    if( item->Type() == PCB_TRACE_T || item->Type() == PCB_ARC_T )
                    {
                        PCB_TRACK*   track = static_cast<PCB_TRACK*>( item );
                        PCB_LAYER_ID layer = track->GetLayer();

                        antiTrackKeepouts->QueryColliding( track, layer, layer,
                                // Filter:
                                [&]( BOARD_ITEM* other ) -> bool
                                {
                                    return true;
                                },
                                // Visitor:
                                [&]( BOARD_ITEM* other ) -> bool
                                {
                                    checkAntiTrackKeepout( track, static_cast<ZONE*>( other ) );
                                    return !m_drcEngine->IsCancelled();
                                },
                                board->m_DRCMaxPhysicalClearance );
                    }

                    checkDisallow( item );

                    if( item->HasHole() )
                    {
                        item->SetFlags( HOLE_PROXY );
                        checkDisallow( item );
                        item->ClearFlags( HOLE_PROXY );
                    }
                }

                if( !reportProgress( ii++, totalCount, progressDelta ) )
                    return false;

                return true;
            } );

    return !m_drcEngine->IsCancelled();
}


namespace detail
{
static DRC_REGISTER_TEST_PROVIDER<DRC_TEST_PROVIDER_DISALLOW> dummy;
}
