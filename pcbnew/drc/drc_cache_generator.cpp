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
#include <atomic>
#include <thread>
#include <board_design_settings.h>
#include <footprint.h>
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

    std::vector<ZONE*> allZones;

    for( ZONE* zone : m_board->Zones() )
    {
        allZones.push_back( zone );

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
            allZones.push_back( zone );

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

                    if( pad->GetDrillSizeX() > 0 && pad->GetDrillSizeY() > 0 )
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
    //
    for( FOOTPRINT* footprint : m_board->Footprints() )
        footprint->BuildPolyCourtyards();

    count = allZones.size();
    std::atomic<size_t> next( 0 );
    std::atomic<size_t> done( 0 );
    std::atomic<size_t> threads_finished( 0 );
    size_t parallelThreadCount = std::max<size_t>( std::thread::hardware_concurrency(), 2 );

    for( ii = 0; ii < parallelThreadCount; ++ii )
    {
        std::thread t = std::thread(
                [ this, &allZones, &done, &threads_finished, &next, count ]( )
                {
                    for( size_t i = next.fetch_add( 1 ); i < count; i = next.fetch_add( 1 ) )
                    {
                        ZONE* zone = allZones[ i ];

                        zone->CacheBoundingBox();
                        zone->CacheTriangulation();

                        if( !zone->GetIsRuleArea() && zone->IsOnCopperLayer() )
                        {
                            std::unique_ptr<DRC_RTREE> rtree = std::make_unique<DRC_RTREE>();

                            for( PCB_LAYER_ID layer : zone->GetLayerSet().Seq() )
                            {
                                if( IsCopperLayer( layer ) )
                                    rtree->Insert( zone, layer );
                            }

                            std::unique_lock<std::mutex> cacheLock( m_board->m_CachesMutex );
                            m_board->m_CopperZoneRTreeCache[ zone ] = std::move( rtree );
                        }

                        if( m_drcEngine->IsCancelled() )
                            break;

                        done.fetch_add( 1 );
                    }

                    threads_finished.fetch_add( 1 );
                } );

        t.detach();
    }

    while( threads_finished < parallelThreadCount )
    {
        reportProgress( done, count, 1 );
        std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
    }

    return !m_drcEngine->IsCancelled();
}

