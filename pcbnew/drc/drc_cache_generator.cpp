/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 KiCad Developers.
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
#include <footprint.h>
#include <thread_pool.h>
#include <zone.h>

#include <drc/drc_engine.h>
#include <drc/drc_rtree.h>
#include <drc/drc_cache_generator.h>

bool DRC_CACHE_GENERATOR::Run()
{
    m_board = m_drcEngine->GetBoard();

    int&           m_largestClearance = m_board->m_DRCMaxClearance;
    int&           m_largestPhysicalClearance = m_board->m_DRCMaxPhysicalClearance;
    DRC_CONSTRAINT worstConstraint;

    if( m_drcEngine->QueryWorstConstraint( CLEARANCE_CONSTRAINT, worstConstraint ) )
        m_largestClearance = worstConstraint.GetValue().Min();

    if( m_drcEngine->QueryWorstConstraint( HOLE_CLEARANCE_CONSTRAINT, worstConstraint ) )
        m_largestClearance = std::max( m_largestClearance, worstConstraint.GetValue().Min() );

    if( m_drcEngine->QueryWorstConstraint( PHYSICAL_CLEARANCE_CONSTRAINT, worstConstraint ) )
        m_largestPhysicalClearance = worstConstraint.GetValue().Min();

    if( m_drcEngine->QueryWorstConstraint( PHYSICAL_HOLE_CLEARANCE_CONSTRAINT, worstConstraint ) )
        m_largestPhysicalClearance = std::max( m_largestPhysicalClearance, worstConstraint.GetValue().Min() );

    std::set<ZONE*> allZones;

    for( ZONE* zone : m_board->Zones() )
    {
        allZones.insert( zone );

        if( !zone->GetIsRuleArea() )
        {
            m_board->m_DRCZones.push_back( zone );

            if( ( zone->GetLayerSet() & LSET::AllCuMask() ).any() )
            {
                m_board->m_DRCCopperZones.push_back( zone );
                m_largestClearance = std::max( m_largestClearance, zone->GetLocalClearance() );
            }
        }
    }

    for( FOOTPRINT* footprint : m_board->Footprints() )
    {
        for( PAD* pad : footprint->Pads() )
            m_largestClearance = std::max( m_largestClearance, pad->GetLocalClearance() );

        for( ZONE* zone : footprint->Zones() )
        {
            allZones.insert( zone );

            if( !zone->GetIsRuleArea() )
            {
                m_board->m_DRCZones.push_back( zone );

                if( ( zone->GetLayerSet() & LSET::AllCuMask() ).any() )
                {
                    m_board->m_DRCCopperZones.push_back( zone );
                    m_largestClearance = std::max( m_largestClearance, zone->GetLocalClearance() );
                }
            }
        }
    }

    // This is the number of tests between 2 calls to the progress bar
    size_t delta = 50;
    size_t count = 0;
    size_t ii = 0;

    auto countItems =
            [&]( BOARD_ITEM* item ) -> bool
            {
                ++count;
                return true;
            };

    auto addToCopperTree =
            [&]( BOARD_ITEM* item ) -> bool
            {
                if( !reportProgress( ii++, count, delta ) )
                    return false;

                LSET layers = item->GetLayerSet();

                // Special-case pad holes which pierce all the copper layers
                if( item->Type() == PCB_PAD_T )
                {
                    PAD* pad = static_cast<PAD*>( item );

                    if( pad->HasHole() )
                        layers |= LSET::AllCuMask();
                }

                for( PCB_LAYER_ID layer : layers.Seq() )
                {
                    if( IsCopperLayer( layer ) )
                        m_board->m_CopperItemRTreeCache->Insert( item, layer, m_largestClearance );
                }

                return true;
            };

    if( !reportPhase( _( "Gathering copper items..." ) ) )
        return false;   // DRC cancelled

    static const std::vector<KICAD_T> itemTypes = {
        PCB_TRACE_T, PCB_ARC_T, PCB_VIA_T,
        PCB_PAD_T,
        PCB_SHAPE_T, PCB_FP_SHAPE_T,
        PCB_TEXT_T, PCB_FP_TEXT_T, PCB_TEXTBOX_T, PCB_FP_TEXTBOX_T,
        PCB_DIMENSION_T
    };

    forEachGeometryItem( itemTypes, LSET::AllCuMask(), countItems );
    forEachGeometryItem( itemTypes, LSET::AllCuMask(), addToCopperTree );

    if( !reportPhase( _( "Tessellating copper zones..." ) ) )
        return false;   // DRC cancelled

    // Cache zone bounding boxes, triangulation, copper zone rtrees, and footprint courtyards
    // before we start.

    m_drcEngine->SetMaxProgress( allZones.size() );

    for( FOOTPRINT* footprint : m_board->Footprints() )
        footprint->BuildCourtyardCaches();

    thread_pool&                     tp = GetKiCadThreadPool();
    std::vector<std::future<size_t>> returns;

    returns.reserve( allZones.size() );

    auto cache_zones = [this]( ZONE* aZone ) -> size_t
            {
                if( m_drcEngine->IsCancelled() )
                    return 0;

                aZone->CacheBoundingBox();
                aZone->CacheTriangulation();

                if( !aZone->GetIsRuleArea() && aZone->IsOnCopperLayer() )
                {
                   std::unique_ptr<DRC_RTREE> rtree = std::make_unique<DRC_RTREE>();

                   for( PCB_LAYER_ID layer : aZone->GetLayerSet().Seq() )
                   {
                       if( IsCopperLayer( layer ) )
                           rtree->Insert( aZone, layer );
                   }

                   std::unique_lock<std::mutex> cacheLock( m_board->m_CachesMutex );
                   m_board->m_CopperZoneRTreeCache[ aZone ] = std::move( rtree );
                   m_drcEngine->AdvanceProgress();
                }

                return 1;
            };

    for( ZONE* zone : allZones )
        returns.emplace_back( tp.submit( cache_zones, zone ) );

    for( auto& retval : returns )
    {
        std::future_status status;

        do
        {
            m_drcEngine->KeepRefreshing();
            status = retval.wait_for( std::chrono::milliseconds( 100 ) );
        } while( status != std::future_status::ready );
    }

    return !m_drcEngine->IsCancelled();
}

