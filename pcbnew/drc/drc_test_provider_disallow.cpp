/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2022 KiCad Developers.
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
#include <thread>
#include <common.h>
#include <board_design_settings.h>
#include <drc/drc_rtree.h>
#include <drc/drc_engine.h>
#include <drc/drc_item.h>
#include <drc/drc_rule.h>
#include <drc/drc_test_provider.h>
#include <pad.h>
#include <zone.h>


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
    {
    }

    virtual ~DRC_TEST_PROVIDER_DISALLOW()
    {
    }

    virtual bool Run() override;

    virtual const wxString GetName() const override
    {
        return wxT( "disallow" );
    };

    virtual const wxString GetDescription() const override
    {
        return wxT( "Tests for disallowed items (e.g. keepouts)" );
    }
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
    int                                  totalCount = 0;

    forEachGeometryItem( {}, LSET::AllLayersMask(),
            [&]( BOARD_ITEM* item ) -> bool
            {
                ZONE* zone = dynamic_cast<ZONE*>( item );

                if( zone && zone->GetIsRuleArea() && zone->GetDoNotAllowCopperPour() )
                    antiCopperKeepouts.push_back( zone );
                else if( zone && zone->IsOnCopperLayer() )
                    copperZones.push_back( zone );

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

    std::atomic<size_t> next( 0 );
    std::atomic<size_t> done( 0 );
    std::atomic<size_t> threads_finished( 0 );
    size_t parallelThreadCount = std::max<size_t>( std::thread::hardware_concurrency(), 2 );

    for( size_t ii = 0; ii < parallelThreadCount; ++ii )
    {
        std::thread t = std::thread(
                [&]()
                {
                    for( size_t i = next.fetch_add( 1 ); i < toCache.size(); i = next.fetch_add( 1 ) )
                    {
                        ZONE*    ruleArea = toCache[i].first;
                        ZONE*    copperZone = toCache[i].second;
                        EDA_RECT areaBBox = ruleArea->GetCachedBoundingBox();
                        EDA_RECT copperBBox = copperZone->GetCachedBoundingBox();
                        bool     isInside = false;

                        if( copperZone->IsFilled() && areaBBox.Intersects( copperBBox ) )
                        {
                            // Collisions include touching, so we need to deflate outline by
                            // enough to exclude it.  This is particularly important for detecting
                            // copper fills as they will be exactly touching along the entire
                            // exclusion border.
                            SHAPE_POLY_SET areaPoly = ruleArea->Outline()->CloneDropTriangulation();
                            areaPoly.Deflate( epsilon, 0, SHAPE_POLY_SET::ALLOW_ACUTE_CORNERS );

                            DRC_RTREE* zoneRTree = board->m_CopperZoneRTrees[ copperZone ].get();

                            if( zoneRTree )
                            {
                                for( PCB_LAYER_ID layer : ruleArea->GetLayerSet().Seq() )
                                {
                                    if( zoneRTree->QueryColliding( areaBBox, &areaPoly, layer ) )
                                    {
                                        isInside = true;
                                        break;
                                    }

                                    if( m_drcEngine->IsCancelled() )
                                        break;
                                }
                            }
                        }

                        if( m_drcEngine->IsCancelled() )
                            break;

                        std::pair<BOARD_ITEM*, BOARD_ITEM*> key( ruleArea, copperZone );
                        {
                            std::unique_lock<std::mutex> cacheLock( board->m_CachesMutex );
                            board->m_InsideAreaCache[ key ] = isInside;
                        }
                        done.fetch_add( 1 );
                    }

                    threads_finished.fetch_add( 1 );
                } );

        t.detach();
    }

    while( threads_finished < parallelThreadCount )
    {
        m_drcEngine->ReportProgress( (double) done / (double) totalCount );
        std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
    }

    if( m_drcEngine->IsCancelled() )
        return false;

    // Now go through all the board objects calling the DRC_ENGINE to run the actual dissallow
    // tests.  These should be reasonably quick using the caches generated above.
    //
    int    delta = 100;
    int    ii = done;

    auto checkTextOnEdgeCuts =
            [&]( BOARD_ITEM* item )
            {
                if( item->Type() == PCB_TEXT_T || item->Type() == PCB_TEXTBOX_T
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

    auto checkDisallow =
            [&]( BOARD_ITEM* item )
            {
                DRC_CONSTRAINT constraint = m_drcEngine->EvalRules( DISALLOW_CONSTRAINT, item,
                                                                    nullptr, UNDEFINED_LAYER );

                if( constraint.m_DisallowFlags && constraint.GetSeverity() != RPT_SEVERITY_IGNORE )
                {
                    std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_ALLOWED_ITEMS );

                    m_msg.Printf( drcItem->GetErrorText() + wxS( " (%s)" ), constraint.GetName() );

                    drcItem->SetErrorMessage( m_msg );
                    drcItem->SetItems( item );
                    drcItem->SetViolatingRule( constraint.GetParentRule() );

                    PCB_LAYER_ID   layer = UNDEFINED_LAYER;

                    if( item->GetLayerSet().count() )
                        layer = item->GetLayerSet().Seq().front();

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
                    ZONE* zone = dynamic_cast<ZONE*>( item );
                    PAD*  pad = dynamic_cast<PAD*>( item );

                    if( zone && zone->GetIsRuleArea() )
                        return true;

                    item->ClearFlags( HOLE_PROXY );     // Just in case

                    checkDisallow( item );

                    bool hasHole;

                    switch( item->Type() )
                    {
                    case PCB_VIA_T: hasHole = true;                     break;
                    case PCB_PAD_T: hasHole = pad->GetDrillSizeX() > 0; break;
                    default:        hasHole = false;                    break;
                    }

                    if( hasHole )
                    {
                        item->SetFlags( HOLE_PROXY );
                        {
                            checkDisallow( item );
                        }
                        item->ClearFlags( HOLE_PROXY );
                    }
                }

                if( !reportProgress( ii++, totalCount, delta ) )
                    return false;

                return true;
            } );

    reportRuleStatistics();

    return !m_drcEngine->IsCancelled();
}


namespace detail
{
static DRC_REGISTER_TEST_PROVIDER<DRC_TEST_PROVIDER_DISALLOW> dummy;
}
