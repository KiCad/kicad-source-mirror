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

#include <common.h>
#include <board_design_settings.h>
#include <pcb_board_outline.h>
#include <footprint.h>
#include <thread_pool.h>
#include <zone.h>
#include <connectivity/connectivity_data.h>
#include <drc/drc_engine.h>
#include <drc/drc_rtree.h>
#include <drc/drc_cache_generator.h>
#include <mutex>

bool DRC_CACHE_GENERATOR::Run()
{
    m_board = m_drcEngine->GetBoard();

    int&           largestClearance = m_board->m_DRCMaxClearance;
    int&           largestPhysicalClearance = m_board->m_DRCMaxPhysicalClearance;
    DRC_CONSTRAINT worstConstraint;
    LSET           boardCopperLayers = LSET::AllCuMask( m_board->GetCopperLayerCount() );
    thread_pool&   tp = GetKiCadThreadPool();

    largestClearance = std::max( largestClearance, m_board->GetMaxClearanceValue() );

    if( m_drcEngine->QueryWorstConstraint( PHYSICAL_CLEARANCE_CONSTRAINT, worstConstraint ) )
        largestPhysicalClearance = worstConstraint.GetValue().Min();

    if( m_drcEngine->QueryWorstConstraint( PHYSICAL_HOLE_CLEARANCE_CONSTRAINT, worstConstraint ) )
        largestPhysicalClearance = std::max( largestPhysicalClearance, worstConstraint.GetValue().Min() );

    // Ensure algorithmic safety
    largestClearance = std::min( largestClearance, INT_MAX / 3 );
    largestPhysicalClearance = std::min( largestPhysicalClearance, INT_MAX / 3 );

    std::set<ZONE*> allZones;

    auto cacheBBoxes =
            []( ZONE* zone, const LSET& copperLayers )
            {
                zone->Outline()->BuildBBoxCaches();

                for( PCB_LAYER_ID layer : copperLayers )
                {
                    if( SHAPE_POLY_SET* fill = zone->GetFill( layer ) )
                        fill->BuildBBoxCaches();
                }
            };

    for( ZONE* zone : m_board->Zones() )
    {
        allZones.insert( zone );

        if( !zone->GetIsRuleArea() )
        {
            m_board->m_DRCZones.push_back( zone );

            LSET zoneCopperLayers = zone->GetLayerSet() & boardCopperLayers;

            if( zoneCopperLayers.any() )
            {
                cacheBBoxes( zone, zoneCopperLayers );
                m_board->m_DRCCopperZones.push_back( zone );
            }
        }
    }

    for( FOOTPRINT* footprint : m_board->Footprints() )
    {
        for( ZONE* zone : footprint->Zones() )
        {
            allZones.insert( zone );

            if( !zone->GetIsRuleArea() )
            {
                m_board->m_DRCZones.push_back( zone );

                LSET zoneCopperLayers = zone->GetLayerSet() & boardCopperLayers;

                if( zoneCopperLayers.any() )
                {
                    cacheBBoxes( zone, zoneCopperLayers );
                    m_board->m_DRCCopperZones.push_back( zone );
                }
            }
        }
    }

    size_t              count = 0;
    std::atomic<size_t> done( 1 );

    auto countItems =
            [&]( BOARD_ITEM* item ) -> bool
            {
                ++count;
                return true;
            };

    auto addToCopperTree =
            [&]( BOARD_ITEM* item ) -> bool
            {
                if( m_drcEngine->IsCancelled() )
                    return false;

                LSET copperLayers = item->GetLayerSet() & boardCopperLayers;

                // Special-case pad holes which pierce all the copper layers
                if( item->Type() == PCB_PAD_T )
                {
                    PAD* pad = static_cast<PAD*>( item );

                    if( pad->HasHole() )
                        copperLayers = boardCopperLayers;
                }

                copperLayers.RunOnLayers(
                        [&]( PCB_LAYER_ID layer )
                        {
                            m_board->m_CopperItemRTreeCache->Insert( item, layer, largestClearance );
                        } );

                done.fetch_add( 1 );
                return true;
            };

    if( !reportPhase( _( "Gathering copper items..." ) ) )
        return false;   // DRC cancelled

    static const std::vector<KICAD_T> itemTypes = {
        PCB_TRACE_T, PCB_ARC_T, PCB_VIA_T,
        PCB_PAD_T,
        PCB_SHAPE_T,
        PCB_FIELD_T, PCB_TEXT_T, PCB_TEXTBOX_T,
        PCB_TABLE_T, PCB_TABLECELL_T,
        PCB_DIMENSION_T,
        PCB_BARCODE_T
    };

    forEachGeometryItem( itemTypes, boardCopperLayers, countItems );

    std::future<void> retn = tp.submit_task(
            [&]()
            {
                std::unique_lock<std::shared_mutex> writeLock( m_board->m_CachesMutex );

                if( !m_board->m_CopperItemRTreeCache )
                    m_board->m_CopperItemRTreeCache = std::make_shared<DRC_RTREE>();

                forEachGeometryItem( itemTypes, boardCopperLayers, addToCopperTree );
            } );

    std::future_status status = retn.wait_for( std::chrono::milliseconds( 250 ) );

    while( status != std::future_status::ready )
    {
        reportProgress( done, count );
        status = retn.wait_for( std::chrono::milliseconds( 250 ) );
    }

    if( !reportPhase( _( "Tessellating copper zones..." ) ) )
        return false;   // DRC cancelled

    // Cache zone bounding boxes, triangulation, copper zone rtrees, and footprint courtyards
    // before we start.

    for( FOOTPRINT* footprint : m_board->Footprints() )
    {
        footprint->BuildCourtyardCaches();
        footprint->BuildNetTieCache();
    }

    std::vector<std::future<size_t>> returns;

    returns.reserve( allZones.size() );

    auto cache_zones =
            [this, &done]( ZONE* aZone ) -> size_t
            {
                if( m_drcEngine->IsCancelled() )
                    return 0;

                aZone->CacheBoundingBox();
                aZone->CacheTriangulation();

                if( !aZone->GetIsRuleArea() && aZone->IsOnCopperLayer() )
                {
                   std::unique_ptr<DRC_RTREE> rtree = std::make_unique<DRC_RTREE>();

                   aZone->GetLayerSet().RunOnLayers(
                           [&]( PCB_LAYER_ID layer )
                           {
                               if( IsCopperLayer( layer ) )
                                   rtree->Insert( aZone, layer );
                           } );

                   {
                       std::unique_lock<std::shared_mutex> writeLock( m_board->m_CachesMutex );
                       m_board->m_CopperZoneRTreeCache[ aZone ] = std::move( rtree );
                   }

                   done.fetch_add( 1 );
                }

                return 1;
            };

    for( ZONE* zone : allZones )
    {
        returns.emplace_back( tp.submit_task(
                [cache_zones, zone]
                {
                    return cache_zones( zone );
                } ) );
    }

    done.store( 1 );

    for( const std::future<size_t>& ret : returns )
    {
        status = ret.wait_for( std::chrono::milliseconds( 250 ) );

        while( status != std::future_status::ready )
        {
            reportProgress( done, allZones.size() );
            status = ret.wait_for( std::chrono::milliseconds( 250 ) );
        }
    }

    m_board->m_ZoneIsolatedIslandsMap.clear();

    for( ZONE* zone : m_board->Zones() )
    {
        if( !zone->GetIsRuleArea() && !zone->IsTeardropArea() )
        {
            zone->GetLayerSet().RunOnLayers(
                    [&]( PCB_LAYER_ID layer )
                    {
                        m_board->m_ZoneIsolatedIslandsMap[ zone ][ layer ] = ISOLATED_ISLANDS();
                    } );
        }
    }

    m_board->UpdateBoardOutline();

    if( m_board->BoardOutline() )
        m_board->BoardOutline()->GetOutline().BuildBBoxCaches();

    std::shared_ptr<CONNECTIVITY_DATA> connectivity = m_board->GetConnectivity();

    connectivity->ClearRatsnest();
    connectivity->Build( m_board, m_drcEngine->GetProgressReporter() );
    connectivity->FillIsolatedIslandsMap( m_board->m_ZoneIsolatedIslandsMap, true );

    return !m_drcEngine->IsCancelled();
}

