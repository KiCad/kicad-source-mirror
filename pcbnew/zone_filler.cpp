/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014-2017 CERN
 * Copyright (C) 2014-2023 KiCad Developers, see AUTHORS.txt for contributors.
 * @author Tomasz Włostowski <tomasz.wlostowski@cern.ch>
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
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

#include <future>
#include <core/kicad_algo.h>
#include <advanced_config.h>
#include <board.h>
#include <board_design_settings.h>
#include <zone.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_target.h>
#include <pcb_track.h>
#include <pcb_text.h>
#include <pcb_textbox.h>
#include <connectivity/connectivity_data.h>
#include <convert_basic_shapes_to_polygon.h>
#include <board_commit.h>
#include <progress_reporter.h>
#include <geometry/shape_poly_set.h>
#include <geometry/convex_hull.h>
#include <geometry/geometry_utils.h>
#include <confirm.h>
#include <thread_pool.h>
#include <math/util.h>      // for KiROUND
#include "zone_filler.h"


ZONE_FILLER::ZONE_FILLER(  BOARD* aBoard, COMMIT* aCommit ) :
        m_board( aBoard ),
        m_brdOutlinesValid( false ),
        m_commit( aCommit ),
        m_progressReporter( nullptr ),
        m_maxError( ARC_HIGH_DEF ),
        m_worstClearance( 0 )
{
    // To enable add "DebugZoneFiller=1" to kicad_advanced settings file.
    m_debugZoneFiller = ADVANCED_CFG::GetCfg().m_DebugZoneFiller;
}


ZONE_FILLER::~ZONE_FILLER()
{
}


void ZONE_FILLER::SetProgressReporter( PROGRESS_REPORTER* aReporter )
{
    m_progressReporter = aReporter;
    wxASSERT_MSG( m_commit, wxT( "ZONE_FILLER must have a valid commit to call "
                                 "SetProgressReporter" ) );
}


/**
 * Fills the given list of zones.
 *
 * NB: Invalidates connectivity - it is up to the caller to obtain a lock on the connectivity
 * data before calling Fill to prevent access to stale data by other coroutines (for example,
 * ratsnest redraw).  This will generally be required if a UI-based progress reporter has been
 * installed.
 *
 * Caller is also responsible for re-building connectivity afterwards.
 */
bool ZONE_FILLER::Fill( std::vector<ZONE*>& aZones, bool aCheck, wxWindow* aParent )
{
    std::lock_guard<KISPINLOCK> lock( m_board->GetConnectivity()->GetLock() );

    std::vector<std::pair<ZONE*, PCB_LAYER_ID>>               toFill;
    std::map<std::pair<ZONE*, PCB_LAYER_ID>, MD5_HASH>        oldFillHashes;
    std::map<ZONE*, std::map<PCB_LAYER_ID, ISOLATED_ISLANDS>> isolatedIslandsMap;

    std::shared_ptr<CONNECTIVITY_DATA> connectivity = m_board->GetConnectivity();

    // Rebuild (from scratch, ignoring dirty flags) just in case. This really needs to be reliable.
    connectivity->ClearRatsnest();
    connectivity->Build( m_board, m_progressReporter );

    BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();

    m_worstClearance = bds.GetBiggestClearanceValue();

    if( m_progressReporter )
    {
        m_progressReporter->Report( aCheck ? _( "Checking zone fills..." )
                                           : _( "Building zone fills..." ) );
        m_progressReporter->SetMaxProgress( aZones.size() );
        m_progressReporter->KeepRefreshing();
    }

    // The board outlines is used to clip solid areas inside the board (when outlines are valid)
    m_boardOutline.RemoveAllContours();
    m_brdOutlinesValid = m_board->GetBoardPolygonOutlines( m_boardOutline );

    // Update and cache zone bounding boxes and pad effective shapes so that we don't have to
    // make them thread-safe.
    //
    for( ZONE* zone : m_board->Zones() )
    {
        zone->CacheBoundingBox();
        m_worstClearance = std::max( m_worstClearance, zone->GetLocalClearance() );
    }

    for( FOOTPRINT* footprint : m_board->Footprints() )
    {
        for( PAD* pad : footprint->Pads() )
        {
            if( pad->IsDirty() )
            {
                pad->BuildEffectiveShapes( UNDEFINED_LAYER );
                pad->BuildEffectivePolygon();
            }

            m_worstClearance = std::max( m_worstClearance, pad->GetLocalClearance() );
        }

        for( ZONE* zone : footprint->Zones() )
        {
            zone->CacheBoundingBox();
            m_worstClearance = std::max( m_worstClearance, zone->GetLocalClearance() );
        }

        // Rules may depend on insideCourtyard() or other expressions
        footprint->BuildCourtyardCaches();
    }

    LSET boardCuMask = m_board->GetEnabledLayers() & LSET::AllCuMask();

    auto findHighestPriorityZone = [&]( const BOX2I& aBBox, const PCB_LAYER_ID aItemLayer,
                                        const int                                aNetcode,
                                        const std::function<bool( const ZONE* )> aTestFn ) -> ZONE*
    {
        unsigned highestPriority = 0;
        ZONE*    highestPriorityZone = nullptr;

        for( ZONE* zone : m_board->Zones() )
        {
            // Rule areas are not filled
            if( zone->GetIsRuleArea() )
                continue;

            if( zone->GetAssignedPriority() < highestPriority )
                continue;

            if( !zone->IsOnLayer( aItemLayer ) )
                continue;

            // Degenerate zones will cause trouble; skip them
            if( zone->GetNumCorners() <= 2 )
                continue;

            if( !zone->GetBoundingBox().Intersects( aBBox ) )
                continue;

            if( !aTestFn( zone ) )
                continue;

            // Prefer highest priority and matching netcode
            if( zone->GetAssignedPriority() > highestPriority || zone->GetNetCode() == aNetcode )
            {
                highestPriority = zone->GetAssignedPriority();
                highestPriorityZone = zone;
            }
        }

        return highestPriorityZone;
    };

    auto isInPourKeepoutArea = [&]( const BOX2I& aBBox, const PCB_LAYER_ID aItemLayer,
                                    const VECTOR2I aTestPoint ) -> bool
    {
        for( ZONE* zone : m_board->Zones() )
        {
            if( !zone->GetIsRuleArea() )
                continue;

            if( !zone->GetDoNotAllowCopperPour() )
                continue;

            if( !zone->IsOnLayer( aItemLayer ) )
                continue;

            // Degenerate zones will cause trouble; skip them
            if( zone->GetNumCorners() <= 2 )
                continue;

            if( !zone->GetBoundingBox().Intersects( aBBox ) )
                continue;

            if( zone->Outline()->Contains( aTestPoint ) )
                return true;
        }

        return false;
    };

    // Determine state of conditional via flashing
    for( PCB_TRACK* track : m_board->Tracks() )
    {
        if( track->Type() == PCB_VIA_T )
        {
            PCB_VIA* via = static_cast<PCB_VIA*>( track );

            via->ClearZoneLayerOverrides();

            if( !via->GetRemoveUnconnected() )
                continue;

            BOX2I    bbox = via->GetBoundingBox();
            VECTOR2I center = via->GetPosition();
            int      testRadius = via->GetDrillValue() / 2 + 1;
            unsigned netcode = via->GetNetCode();
            LSET     layers = via->GetLayerSet() & boardCuMask;

            // Checking if the via hole touches the zone outline
            auto viaTestFn = [&]( const ZONE* aZone ) -> bool
            {
                return aZone->Outline()->Contains( center, -1, testRadius );
            };

            for( PCB_LAYER_ID layer : layers.Seq() )
            {
                if( !via->ConditionallyFlashed( layer ) )
                    continue;

                if( isInPourKeepoutArea( bbox, layer, center ) )
                {
                    via->SetZoneLayerOverride( layer, ZLO_FORCE_NO_ZONE_CONNECTION );
                }
                else
                {
                    ZONE* zone = findHighestPriorityZone( bbox, layer, netcode, viaTestFn );

                    if( zone && zone->GetNetCode() == via->GetNetCode() )
                        via->SetZoneLayerOverride( layer, ZLO_FORCE_FLASHED );
                    else
                        via->SetZoneLayerOverride( layer, ZLO_FORCE_NO_ZONE_CONNECTION );
                }
            }
        }
    }

    // Determine state of conditional pad flashing
    for( FOOTPRINT* footprint : m_board->Footprints() )
    {
        for( PAD* pad : footprint->Pads() )
        {
            pad->ClearZoneLayerOverrides();

            if( !pad->GetRemoveUnconnected() )
                continue;

            BOX2I    bbox = pad->GetBoundingBox();
            VECTOR2I center = pad->GetPosition();
            unsigned netcode = pad->GetNetCode();
            LSET     layers = pad->GetLayerSet() & boardCuMask;

            auto padTestFn = [&]( const ZONE* aZone ) -> bool
            {
                return aZone->Outline()->Contains( center );
            };

            for( PCB_LAYER_ID layer : layers.Seq() )
            {
                if( !pad->ConditionallyFlashed( layer ) )
                    continue;

                if( isInPourKeepoutArea( bbox, layer, center ) )
                {
                    pad->SetZoneLayerOverride( layer, ZLO_FORCE_NO_ZONE_CONNECTION );
                }
                else
                {
                    ZONE* zone = findHighestPriorityZone( bbox, layer, netcode, padTestFn );

                    if( zone && zone->GetNetCode() == pad->GetNetCode() )
                        pad->SetZoneLayerOverride( layer, ZLO_FORCE_FLASHED );
                    else
                        pad->SetZoneLayerOverride( layer, ZLO_FORCE_NO_ZONE_CONNECTION );
                }
            }
        }
    }

    for( ZONE* zone : aZones )
    {
        // Rule areas are not filled
        if( zone->GetIsRuleArea() )
            continue;

        // Degenerate zones will cause trouble; skip them
        if( zone->GetNumCorners() <= 2 )
            continue;

        if( m_commit )
            m_commit->Modify( zone );

        // calculate the hash value for filled areas. it will be used later to know if the
        // current filled areas are up to date
        for( PCB_LAYER_ID layer : zone->GetLayerSet().Seq() )
        {
            zone->BuildHashValue( layer );
            oldFillHashes[ { zone, layer } ] = zone->GetHashValue( layer );

            // Add the zone to the list of zones to test or refill
            toFill.emplace_back( std::make_pair( zone, layer ) );

            isolatedIslandsMap[ zone ][ layer ] = ISOLATED_ISLANDS();
        }

        // Remove existing fill first to prevent drawing invalid polygons on some platforms
        zone->UnFill();
    }

    auto check_fill_dependency =
            [&]( ZONE* aZone, PCB_LAYER_ID aLayer, ZONE* aOtherZone ) -> bool
            {
                // Check to see if we have to knock-out the filled areas of a higher-priority
                // zone.  If so we have to wait until said zone is filled before we can fill.

                // If the other zone is already filled on the requested layer then we're
                // good-to-go
                if( aOtherZone->GetFillFlag( aLayer ) )
                    return false;

                // Even if keepouts exclude copper pours, the exclusion is by outline rather than
                // filled area, so we're good-to-go here too
                if( aOtherZone->GetIsRuleArea() )
                    return false;

                // If the other zone is never going to be filled then don't wait for it
                if( aOtherZone->GetNumCorners() <= 2 )
                    return false;

                // If the zones share no common layers
                if( !aOtherZone->GetLayerSet().test( aLayer ) )
                    return false;

                if( aZone->HigherPriority( aOtherZone ) )
                    return false;

                // Same-net zones always use outlines to produce determinate results
                if( aOtherZone->SameNet( aZone ) )
                    return false;

                // A higher priority zone is found: if we intersect and it's not filled yet
                // then we have to wait.
                BOX2I inflatedBBox = aZone->GetBoundingBox();
                inflatedBBox.Inflate( m_worstClearance );

                if( !inflatedBBox.Intersects( aOtherZone->GetBoundingBox() ) )
                    return false;

                return aZone->Outline()->Collide( aOtherZone->Outline(), m_worstClearance );
            };

    auto fill_lambda =
            [&]( std::pair<ZONE*, PCB_LAYER_ID> aFillItem ) -> int
            {
                PCB_LAYER_ID layer = aFillItem.second;
                ZONE*        zone = aFillItem.first;
                bool         canFill = true;

                // Check for any fill dependencies.  If our zone needs to be clipped by
                // another zone then we can't fill until that zone is filled.
                for( ZONE* otherZone : aZones )
                {
                    if( otherZone == zone )
                        continue;

                    if( check_fill_dependency( zone, layer, otherZone ) )
                    {
                        canFill = false;
                        break;
                    }
                }

                if( m_progressReporter && m_progressReporter->IsCancelled() )
                    return 0;

                if( !canFill )
                    return 0;

                // Now we're ready to fill.
                {
                    std::unique_lock<std::mutex> zoneLock( zone->GetLock(), std::try_to_lock );

                    if( !zoneLock.owns_lock() )
                        return 0;

                    SHAPE_POLY_SET fillPolys;

                    if( !fillSingleZone( zone, layer, fillPolys ) )
                        return 0;

                    zone->SetFilledPolysList( layer, fillPolys );
                }

                if( m_progressReporter )
                    m_progressReporter->AdvanceProgress();

                return 1;
            };

    auto tesselate_lambda =
            [&]( std::pair<ZONE*, PCB_LAYER_ID> aFillItem ) -> int
            {
                if( m_progressReporter && m_progressReporter->IsCancelled() )
                    return 0;

                PCB_LAYER_ID layer = aFillItem.second;
                ZONE*        zone = aFillItem.first;

                {
                    std::unique_lock<std::mutex> zoneLock( zone->GetLock(), std::try_to_lock );

                    if( !zoneLock.owns_lock() )
                        return 0;

                    zone->CacheTriangulation( layer );
                    zone->SetFillFlag( layer, true );
                }

                return 1;
            };

    // Calculate the copper fills (NB: this is multi-threaded)
    //
    std::vector<std::pair<std::future<int>, int>> returns;
    returns.reserve( toFill.size() );
    size_t finished = 0;
    bool cancelled = false;

    thread_pool& tp = GetKiCadThreadPool();

    for( const std::pair<ZONE*, PCB_LAYER_ID>& fillItem : toFill )
        returns.emplace_back( std::make_pair( tp.submit( fill_lambda, fillItem ), 0 ) );

    while( !cancelled && finished != 2 * toFill.size() )
    {
        for( size_t ii = 0; ii < returns.size(); ++ii )
        {
            auto& ret = returns[ii];

            if( ret.second > 1 )
                continue;

            std::future_status status = ret.first.wait_for( std::chrono::seconds( 0 ) );

            if( status == std::future_status::ready )
            {
                if( ret.first.get() )   // lambda completed
                {
                    ++finished;
                    ret.second++;       // go to next step
                }

                if( !cancelled )
                {
                    // Queue the next step (will re-queue the existing step if it didn't complete)
                    if( ret.second == 0 )
                        returns[ii].first = tp.submit( fill_lambda, toFill[ii] );
                    else if( ret.second == 1 )
                        returns[ii].first = tp.submit( tesselate_lambda, toFill[ii] );
                }
            }
        }

        std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );


        if( m_progressReporter )
        {
            m_progressReporter->KeepRefreshing();

            if( m_progressReporter->IsCancelled() )
                cancelled = true;
        }
    }

    // Make sure that all futures have finished.
    // This can happen when the user cancels the above operation
    for( auto& ret : returns )
    {
        if( ret.first.valid() )
        {
            std::future_status status = ret.first.wait_for( std::chrono::seconds( 0 ) );

            while( status != std::future_status::ready )
            {
                if( m_progressReporter )
                    m_progressReporter->KeepRefreshing();

                status = ret.first.wait_for( std::chrono::milliseconds( 100 ) );
            }
        }
    }

    // Now update the connectivity to check for isolated copper islands
    // (NB: FindIsolatedCopperIslands() is multi-threaded)
    //
    if( m_progressReporter )
    {
        if( m_progressReporter->IsCancelled() )
            return false;

        m_progressReporter->AdvancePhase();
        m_progressReporter->Report( _( "Removing isolated copper islands..." ) );
        m_progressReporter->KeepRefreshing();
    }

    connectivity->SetProgressReporter( m_progressReporter );
    connectivity->FillIsolatedIslandsMap( isolatedIslandsMap );
    connectivity->SetProgressReporter( nullptr );

    if( m_progressReporter && m_progressReporter->IsCancelled() )
        return false;

    for( ZONE* zone : aZones )
    {
        // Keepout zones are not filled
        if( zone->GetIsRuleArea() )
            continue;

        zone->SetIsFilled( true );
    }

    // Now remove isolated copper islands according to the isolated islands strategy assigned
    // by the user (always, never, below-certain-size).
    //
    for( const auto& [ zone, zoneIslands ] : isolatedIslandsMap )
    {
        // If *all* the polygons are islands, do not remove any of them
        bool allIslands = true;

        for( const auto& [ layer, layerIslands ] : zoneIslands )
        {
            if( layerIslands.m_IsolatedOutlines.size()
                    != static_cast<size_t>( zone->GetFilledPolysList( layer )->OutlineCount() ) )
            {
                allIslands = false;
                break;
            }
        }

        if( allIslands )
            continue;

        for( const auto& [ layer, layerIslands ] : zoneIslands )
        {
            if( m_debugZoneFiller && LSET::InternalCuMask().Contains( layer ) )
                continue;

            if( layerIslands.m_IsolatedOutlines.empty() )
                continue;

            std::vector<int> islands = layerIslands.m_IsolatedOutlines;

            // The list of polygons to delete must be explored from last to first in list,
            // to allow deleting a polygon from list without breaking the remaining of the list
            std::sort( islands.begin(), islands.end(), std::greater<int>() );

            std::shared_ptr<SHAPE_POLY_SET> poly = zone->GetFilledPolysList( layer );
            long long int                   minArea = zone->GetMinIslandArea();
            ISLAND_REMOVAL_MODE             mode = zone->GetIslandRemovalMode();

            for( int idx : islands )
            {
                SHAPE_LINE_CHAIN& outline = poly->Outline( idx );

                if( mode == ISLAND_REMOVAL_MODE::ALWAYS )
                    poly->DeletePolygonAndTriangulationData( idx, false );
                else if ( mode == ISLAND_REMOVAL_MODE::AREA && outline.Area( true ) < minArea )
                    poly->DeletePolygonAndTriangulationData( idx, false );
                else
                    zone->SetIsIsland( layer, idx );
            }

            poly->UpdateTriangulationDataHash();
            zone->CalculateFilledArea();

            if( m_progressReporter && m_progressReporter->IsCancelled() )
                return false;
        }
    }

    // Now remove islands which are either outside the board edge or fail to meet the minimum
    // area requirements
    using island_check_return = std::vector<std::pair<std::shared_ptr<SHAPE_POLY_SET>, int>>;

    std::vector<std::pair<std::shared_ptr<SHAPE_POLY_SET>, double>> polys_to_check;

    // rough estimate to save re-allocation time
    polys_to_check.reserve( m_board->GetCopperLayerCount() * aZones.size() );

    for( ZONE* zone : aZones )
    {
        LSET   zoneCopperLayers = zone->GetLayerSet() & LSET::AllCuMask( MAX_CU_LAYERS );

        // Min-thickness is the web thickness.  On the other hand, a blob min-thickness by
        // min-thickness is not useful.  Since there's no obvious definition of web vs. blob, we
        // arbitrarily choose "at least 2X the area".
        double minArea = (double) zone->GetMinThickness() * zone->GetMinThickness() * 2;

        for( PCB_LAYER_ID layer : zoneCopperLayers.Seq() )
        {
            if( m_debugZoneFiller && LSET::InternalCuMask().Contains( layer ) )
                continue;

            polys_to_check.emplace_back( zone->GetFilledPolysList( layer ), minArea );
        }
    }

    auto island_lambda =
            [&]( int aStart, int aEnd ) -> island_check_return
            {
                island_check_return retval;

                for( int ii = aStart; ii < aEnd && !cancelled; ++ii )
                {
                    auto [poly, minArea] = polys_to_check[ii];

                    for( int jj = poly->OutlineCount() - 1; jj >= 0; jj-- )
                    {
                        SHAPE_POLY_SET island;
                        SHAPE_POLY_SET intersection;
                        const SHAPE_LINE_CHAIN& test_poly = poly->Polygon( jj ).front();
                        double island_area = test_poly.Area();

                        if( island_area < minArea )
                            continue;


                        island.AddOutline( test_poly );
                        intersection.BooleanIntersection( m_boardOutline, island,
                                                          SHAPE_POLY_SET::POLYGON_MODE::PM_FAST );

                        // Nominally, all of these areas should be either inside or outside the
                        // board outline.  So this test should be able to just compare areas (if
                        // they are equal, you are inside).  But in practice, we sometimes have
                        // slight overlap at the edges, so testing against half-size area acts as
                        // a fail-safe.
                        if( intersection.Area() < island_area / 2.0 )
                            retval.emplace_back( poly, jj );
                    }
                }

                return retval;
            };

    auto island_returns = tp.parallelize_loop( 0, polys_to_check.size(), island_lambda );
    cancelled = false;

    // Allow island removal threads to finish
    for( size_t ii = 0; ii < island_returns.size(); ++ii )
    {
        std::future<island_check_return>& ret = island_returns[ii];

        if( ret.valid() )
        {
            std::future_status status = ret.wait_for( std::chrono::seconds( 0 ) );

            while( status != std::future_status::ready )
            {
                if( m_progressReporter )
                {
                    m_progressReporter->KeepRefreshing();

                    if( m_progressReporter->IsCancelled() )
                        cancelled = true;
                }

                status = ret.wait_for( std::chrono::milliseconds( 100 ) );
            }
        }
    }

    if( cancelled )
        return false;

    for( size_t ii = 0; ii < island_returns.size(); ++ii )
    {
        std::future<island_check_return>& ret = island_returns[ii];

        if( ret.valid() )
        {
            for( auto& action_item : ret.get() )
                action_item.first->DeletePolygonAndTriangulationData( action_item.second, true );
        }
    }

    for( ZONE* zone : aZones )
        zone->CalculateFilledArea();


    if( aCheck )
    {
        bool outOfDate = false;

        for( ZONE* zone : aZones )
        {
            // Keepout zones are not filled
            if( zone->GetIsRuleArea() )
                continue;

            for( PCB_LAYER_ID layer : zone->GetLayerSet().Seq() )
            {
                zone->BuildHashValue( layer );

                if( oldFillHashes[ { zone, layer } ] != zone->GetHashValue( layer ) )
                    outOfDate = true;
            }
        }

        if( outOfDate )
        {
            KIDIALOG dlg( aParent, _( "Zone fills are out-of-date. Refill?" ),
                          _( "Confirmation" ), wxOK | wxCANCEL | wxICON_WARNING );
            dlg.SetOKCancelLabels( _( "Refill" ), _( "Continue without Refill" ) );
            dlg.DoNotShowCheckbox( __FILE__, __LINE__ );

            if( dlg.ShowModal() == wxID_CANCEL )
                return false;
        }
        else
        {
            // No need to commit something that hasn't changed (and committing will set
            // the modified flag).
            return false;
        }
    }

    if( m_progressReporter )
    {
        if( m_progressReporter->IsCancelled() )
            return false;

        m_progressReporter->AdvancePhase();
        m_progressReporter->KeepRefreshing();
    }

    return true;
}


/**
 * Add a knockout for a pad.  The knockout is 'aGap' larger than the pad (which might be
 * either the thermal clearance or the electrical clearance).
 */
void ZONE_FILLER::addKnockout( PAD* aPad, PCB_LAYER_ID aLayer, int aGap, SHAPE_POLY_SET& aHoles )
{
    if( aPad->GetShape() == PAD_SHAPE::CUSTOM )
    {
        SHAPE_POLY_SET poly;
        aPad->TransformShapeToPolygon( poly, aLayer, aGap, m_maxError, ERROR_OUTSIDE );

        // the pad shape in zone can be its convex hull or the shape itself
        if( aPad->GetCustomShapeInZoneOpt() == CUST_PAD_SHAPE_IN_ZONE_CONVEXHULL )
        {
            std::vector<VECTOR2I> convex_hull;
            BuildConvexHull( convex_hull, poly );

            aHoles.NewOutline();

            for( const VECTOR2I& pt : convex_hull )
                aHoles.Append( pt );
        }
        else
            aHoles.Append( poly );
    }
    else
    {
        aPad->TransformShapeToPolygon( aHoles, aLayer, aGap, m_maxError, ERROR_OUTSIDE );
    }
}


/**
 * Add a knockout for a pad's hole.
 */
void ZONE_FILLER::addHoleKnockout( PAD* aPad, int aGap, SHAPE_POLY_SET& aHoles )
{
    aPad->TransformHoleToPolygon( aHoles, aGap, m_maxError, ERROR_OUTSIDE );
}


/**
 * Add a knockout for a graphic item.  The knockout is 'aGap' larger than the item (which
 * might be either the electrical clearance or the board edge clearance).
 */
void ZONE_FILLER::addKnockout( BOARD_ITEM* aItem, PCB_LAYER_ID aLayer, int aGap,
                               bool aIgnoreLineWidth, SHAPE_POLY_SET& aHoles )
{
    EDA_TEXT* text = nullptr;

    switch( aItem->Type() )
    {
    case PCB_TEXT_T:    text = static_cast<PCB_TEXT*>( aItem );    break;
    case PCB_TEXTBOX_T: text = static_cast<PCB_TEXTBOX*>( aItem ); break;
    default:                                                       break;
    }

    if( text )
        aGap += GetKnockoutTextMargin( text->GetTextSize(), text->GetTextThickness() );

    switch( aItem->Type() )
    {
    case PCB_SHAPE_T:
    case PCB_TEXT_T:
    case PCB_TEXTBOX_T:
    case PCB_TARGET_T:
        if( !text || text->IsVisible() )
        {
            aItem->TransformShapeToPolygon( aHoles, aLayer, aGap, m_maxError, ERROR_OUTSIDE,
                                            aIgnoreLineWidth );
        }

        break;

    default:
        break;
    }
}


/**
 * Removes thermal reliefs from the shape for any pads connected to the zone.  Does NOT add
 * in spokes, which must be done later.
 */
void ZONE_FILLER::knockoutThermalReliefs( const ZONE* aZone, PCB_LAYER_ID aLayer,
                                          SHAPE_POLY_SET& aFill,
                                          std::vector<PAD*>& aThermalConnectionPads,
                                          std::vector<PAD*>& aNoConnectionPads )
{
    BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();
    ZONE_CONNECTION        connection;
    DRC_CONSTRAINT         constraint;
    int                    padClearance;
    int                    holeClearance;
    SHAPE_POLY_SET         holes;

    for( FOOTPRINT* footprint : m_board->Footprints() )
    {
        for( PAD* pad : footprint->Pads() )
        {
            BOX2I padBBox = pad->GetBoundingBox();
            padBBox.Inflate( m_worstClearance );

            if( !padBBox.Intersects( aZone->GetBoundingBox() ) )
                continue;

            if( pad->GetNetCode() != aZone->GetNetCode()
                || pad->GetNetCode() <= 0
                || pad->GetZoneLayerOverride( aLayer ) == ZLO_FORCE_NO_ZONE_CONNECTION )
            {
                // collect these for knockout in buildCopperItemClearances()
                aNoConnectionPads.push_back( pad );
                continue;
            }

            if( aZone->IsTeardropArea() )
            {
                connection = ZONE_CONNECTION::FULL;
            }
            else
            {
                constraint = bds.m_DRCEngine->EvalZoneConnection( pad, aZone, aLayer );
                connection = constraint.m_ZoneConnection;
            }

            switch( connection )
            {
            case ZONE_CONNECTION::THERMAL:
                constraint = bds.m_DRCEngine->EvalRules( THERMAL_RELIEF_GAP_CONSTRAINT, pad, aZone,
                                                         aLayer );
                padClearance = constraint.GetValue().Min();

                if( pad->CanFlashLayer( aLayer ) )
                {
                    aThermalConnectionPads.push_back( pad );
                    addKnockout( pad, aLayer, padClearance, holes );
                }
                else if( pad->GetDrillSize().x > 0 )
                {
                    pad->TransformHoleToPolygon( holes, padClearance, m_maxError, ERROR_OUTSIDE );
                }

                break;

            case ZONE_CONNECTION::NONE:
                constraint = bds.m_DRCEngine->EvalRules( PHYSICAL_CLEARANCE_CONSTRAINT, pad,
                                                         aZone, aLayer );

                if( constraint.GetValue().Min() > aZone->GetLocalClearance() )
                    padClearance = constraint.GetValue().Min();
                else
                    padClearance = aZone->GetLocalClearance();

                if( pad->FlashLayer( aLayer ) )
                {
                    addKnockout( pad, aLayer, padClearance, holes );
                }
                else if( pad->GetDrillSize().x > 0 )
                {
                    constraint = bds.m_DRCEngine->EvalRules( PHYSICAL_HOLE_CLEARANCE_CONSTRAINT,
                                                             pad, aZone, aLayer );

                    if( constraint.GetValue().Min() > padClearance )
                        holeClearance = constraint.GetValue().Min();
                    else
                        holeClearance = padClearance;

                    pad->TransformHoleToPolygon( holes, holeClearance, m_maxError, ERROR_OUTSIDE );
                }

                break;

            default:
                // No knockout
                continue;
            }
        }
    }

    aFill.BooleanSubtract( holes, SHAPE_POLY_SET::PM_FAST );
}


/**
 * Removes clearance from the shape for copper items which share the zone's layer but are
 * not connected to it.
 */
void ZONE_FILLER::buildCopperItemClearances( const ZONE* aZone, PCB_LAYER_ID aLayer,
                                             const std::vector<PAD*> aNoConnectionPads,
                                             SHAPE_POLY_SET& aHoles )
{
    BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();
    long                   ticker = 0;

    auto checkForCancel =
            [&ticker]( PROGRESS_REPORTER* aReporter ) -> bool
            {
                return aReporter && ( ticker++ % 50 ) == 0 && aReporter->IsCancelled();
            };

    // A small extra clearance to be sure actual track clearances are not smaller than
    // requested clearance due to many approximations in calculations, like arc to segment
    // approx, rounding issues, etc.
    BOX2I zone_boundingbox = aZone->GetBoundingBox();
    int   extra_margin = pcbIUScale.mmToIU( ADVANCED_CFG::GetCfg().m_ExtraClearance );

    // Items outside the zone bounding box are skipped, so it needs to be inflated by the
    // largest clearance value found in the netclasses and rules
    zone_boundingbox.Inflate( m_worstClearance + extra_margin );

    auto evalRulesForItems =
            [&bds]( DRC_CONSTRAINT_T aConstraint, const BOARD_ITEM* a, const BOARD_ITEM* b,
                    PCB_LAYER_ID aEvalLayer ) -> int
            {
                DRC_CONSTRAINT c = bds.m_DRCEngine->EvalRules( aConstraint, a, b, aEvalLayer );
                return c.GetValue().Min();
            };

    // Add non-connected pad clearances
    //
    auto knockoutPadClearance =
            [&]( PAD* aPad )
            {
                int  init_gap = evalRulesForItems( PHYSICAL_CLEARANCE_CONSTRAINT, aZone, aPad, aLayer );
                int  gap = init_gap;
                bool hasHole = aPad->GetDrillSize().x > 0;
                bool flashLayer = aPad->FlashLayer( aLayer );
                bool platedHole = hasHole && aPad->GetAttribute() == PAD_ATTRIB::PTH;

                if( flashLayer || platedHole )
                {
                    gap = std::max( gap, evalRulesForItems( CLEARANCE_CONSTRAINT,
                                                            aZone, aPad, aLayer ) );
                }

                if( flashLayer && gap > 0 )
                    addKnockout( aPad, aLayer, gap + extra_margin, aHoles );

                if( hasHole )
                {
                    // NPTH do not need copper clearance gaps to their holes
                    if( aPad->GetAttribute() == PAD_ATTRIB::NPTH )
                        gap = init_gap;

                    gap = std::max( gap, evalRulesForItems( PHYSICAL_HOLE_CLEARANCE_CONSTRAINT,
                                                            aZone, aPad, aLayer ) );

                    gap = std::max( gap, evalRulesForItems( HOLE_CLEARANCE_CONSTRAINT,
                                                            aZone, aPad, aLayer ) );

                    if( gap > 0 )
                        addHoleKnockout( aPad, gap + extra_margin, aHoles );
                }
            };

    for( PAD* pad : aNoConnectionPads )
    {
        if( checkForCancel( m_progressReporter ) )
            return;

        knockoutPadClearance( pad );
    }

    // Add non-connected track clearances
    //
    auto knockoutTrackClearance =
            [&]( PCB_TRACK* aTrack )
            {
                if( aTrack->GetBoundingBox().Intersects( zone_boundingbox ) )
                {
                    bool sameNet = aTrack->GetNetCode() == aZone->GetNetCode()
                                        && aZone->GetNetCode() != 0;

                    int  gap = evalRulesForItems( PHYSICAL_CLEARANCE_CONSTRAINT,
                                                  aZone, aTrack, aLayer );

                    if( aTrack->Type() == PCB_VIA_T )
                    {
                        PCB_VIA* via = static_cast<PCB_VIA*>( aTrack );

                        if( via->GetZoneLayerOverride( aLayer ) == ZLO_FORCE_NO_ZONE_CONNECTION )
                            sameNet = false;
                    }

                    if( !sameNet )
                    {
                        gap = std::max( gap, evalRulesForItems( CLEARANCE_CONSTRAINT,
                                                                aZone, aTrack, aLayer ) );
                    }

                    if( aTrack->Type() == PCB_VIA_T )
                    {
                        PCB_VIA* via = static_cast<PCB_VIA*>( aTrack );

                        if( via->FlashLayer( aLayer ) && gap > 0 )
                        {
                            via->TransformShapeToPolygon( aHoles, aLayer, gap + extra_margin,
                                                          m_maxError, ERROR_OUTSIDE );
                        }

                        gap = std::max( gap, evalRulesForItems( PHYSICAL_HOLE_CLEARANCE_CONSTRAINT,
                                                                aZone, via, aLayer ) );

                        if( !sameNet )
                        {
                            gap = std::max( gap, evalRulesForItems( HOLE_CLEARANCE_CONSTRAINT,
                                                                    aZone, via, aLayer ) );
                        }

                        if( gap > 0 )
                        {
                            int radius = via->GetDrillValue() / 2;

                            TransformCircleToPolygon( aHoles, via->GetPosition(),
                                                      radius + gap + extra_margin,
                                                      m_maxError, ERROR_OUTSIDE );
                        }
                    }
                    else
                    {
                        if( gap > 0 )
                        {
                            aTrack->TransformShapeToPolygon( aHoles, aLayer, gap + extra_margin,
                                                             m_maxError, ERROR_OUTSIDE );
                        }
                    }
                }
            };

    for( PCB_TRACK* track : m_board->Tracks() )
    {
        if( !track->IsOnLayer( aLayer ) )
            continue;

        if( checkForCancel( m_progressReporter ) )
            return;

        knockoutTrackClearance( track );
    }

    // Add graphic item clearances.  They are by definition unconnected, and have no clearance
    // definitions of their own.
    //
    auto knockoutGraphicClearance =
            [&]( BOARD_ITEM* aItem )
            {
                // A item on the Edge_Cuts or Margin is always seen as on any layer:
                if( aItem->IsOnLayer( aLayer )
                        || aItem->IsOnLayer( Edge_Cuts )
                        || aItem->IsOnLayer( Margin ) )
                {
                    if( aItem->GetBoundingBox().Intersects( zone_boundingbox ) )
                    {
                        bool ignoreLineWidths = false;
                        int  gap = evalRulesForItems( PHYSICAL_CLEARANCE_CONSTRAINT,
                                                      aZone, aItem, aLayer );

                        if( aItem->IsOnLayer( aLayer ) )
                        {
                            gap = std::max( gap, evalRulesForItems( CLEARANCE_CONSTRAINT,
                                                                    aZone, aItem, aLayer ) );
                        }
                        else if( aItem->IsOnLayer( Edge_Cuts ) )
                        {
                            gap = std::max( gap, evalRulesForItems( EDGE_CLEARANCE_CONSTRAINT,
                                                                    aZone, aItem, Edge_Cuts ) );
                            ignoreLineWidths = true;
                        }
                        else if( aItem->IsOnLayer( Margin ) )
                        {
                            gap = std::max( gap, evalRulesForItems( EDGE_CLEARANCE_CONSTRAINT,
                                                                    aZone, aItem, Margin ) );
                        }

                        addKnockout( aItem, aLayer, gap + extra_margin, ignoreLineWidths, aHoles );
                    }
                }
            };

    for( FOOTPRINT* footprint : m_board->Footprints() )
    {
        knockoutGraphicClearance( &footprint->Reference() );
        knockoutGraphicClearance( &footprint->Value() );

        std::set<PAD*> allowedNetTiePads;

        // Don't knock out holes for graphic items which implement a net-tie to the zone's net
        // on the layer being filled.
        if( footprint->IsNetTie() )
        {
            for( PAD* pad : footprint->Pads() )
            {
                if( pad->GetNetCode() == aZone->GetNetCode() )
                {
                    if( pad->IsOnLayer( aLayer ) )
                        allowedNetTiePads.insert( pad );

                    for( PAD* other : footprint->GetNetTiePads( pad ) )
                    {
                        if( other->IsOnLayer( aLayer ) )
                            allowedNetTiePads.insert( other );
                    }
                }
            }
        }

        for( BOARD_ITEM* item : footprint->GraphicalItems() )
        {
            if( checkForCancel( m_progressReporter ) )
                return;

            BOX2I itemBBox = item->GetBoundingBox();

            if( !zone_boundingbox.Intersects( itemBBox ) )
                continue;

            bool skipItem = false;

            if( item->IsOnLayer( aLayer ) )
            {
                std::shared_ptr<SHAPE> itemShape = item->GetEffectiveShape();

                for( PAD* pad : allowedNetTiePads )
                {
                    if( pad->GetBoundingBox().Intersects( itemBBox )
                            && pad->GetEffectiveShape()->Collide( itemShape.get() ) )
                    {
                        skipItem = true;
                        break;
                    }
                }
            }

            if( !skipItem )
                knockoutGraphicClearance( item );
        }
    }

    for( BOARD_ITEM* item : m_board->Drawings() )
    {
        if( checkForCancel( m_progressReporter ) )
            return;

        knockoutGraphicClearance( item );
    }

    // Add non-connected zone clearances
    //
    auto knockoutZoneClearance =
            [&]( ZONE* aKnockout )
            {
                // If the zones share no common layers
                if( !aKnockout->GetLayerSet().test( aLayer ) )
                    return;

                if( aKnockout->GetBoundingBox().Intersects( zone_boundingbox ) )
                {
                    if( aKnockout->GetIsRuleArea() )
                    {
                        // Keepouts use outline with no clearance
                        aKnockout->TransformSmoothedOutlineToPolygon( aHoles, 0, m_maxError,
                                                                      ERROR_OUTSIDE, nullptr );
                    }
                    else
                    {
                        int gap = evalRulesForItems( PHYSICAL_CLEARANCE_CONSTRAINT, aZone,
                                                     aKnockout, aLayer );

                        gap = std::max( gap, evalRulesForItems( CLEARANCE_CONSTRAINT, aZone,
                                                                aKnockout, aLayer ) );

                        SHAPE_POLY_SET poly;
                        aKnockout->TransformShapeToPolygon( poly, aLayer, gap + extra_margin,
                                                            m_maxError, ERROR_OUTSIDE );
                        aHoles.Append( poly );
                    }
                }
            };

    for( ZONE* otherZone : m_board->Zones() )
    {
        if( checkForCancel( m_progressReporter ) )
            return;

        // Negative clearance permits zones to short
        if( evalRulesForItems( CLEARANCE_CONSTRAINT, aZone, otherZone, aLayer ) < 0 )
            continue;

        if( otherZone->GetIsRuleArea() )
        {
            if( otherZone->GetDoNotAllowCopperPour() && !aZone->IsTeardropArea() )
                knockoutZoneClearance( otherZone );
        }
        else if( otherZone->HigherPriority( aZone ) )
        {
            if( !otherZone->SameNet( aZone ) )
                knockoutZoneClearance( otherZone );
        }
    }

    for( FOOTPRINT* footprint : m_board->Footprints() )
    {
        for( ZONE* otherZone : footprint->Zones() )
        {
            if( checkForCancel( m_progressReporter ) )
                return;

            if( otherZone->GetIsRuleArea() )
            {
                if( otherZone->GetDoNotAllowCopperPour() && !aZone->IsTeardropArea() )
                    knockoutZoneClearance( otherZone );
            }
            else if( otherZone->HigherPriority( aZone ) )
            {
                if( !otherZone->SameNet( aZone ) )
                    knockoutZoneClearance( otherZone );
            }
        }
    }

    aHoles.Simplify( SHAPE_POLY_SET::PM_FAST );
}


/**
 * Removes the outlines of higher-proirity zones with the same net.  These zones should be
 * in charge of the fill parameters within their own outlines.
 */
void ZONE_FILLER::subtractHigherPriorityZones( const ZONE* aZone, PCB_LAYER_ID aLayer,
                                               SHAPE_POLY_SET& aRawFill )
{
    BOX2I zoneBBox = aZone->GetBoundingBox();

    auto knockoutZoneOutline =
            [&]( ZONE* aKnockout )
            {
                // If the zones share no common layers
                if( !aKnockout->GetLayerSet().test( aLayer ) )
                    return;

                if( aKnockout->GetBoundingBox().Intersects( zoneBBox ) )
                {
                    // Processing of arc shapes in zones is not yet supported because Clipper
                    // can't do boolean operations on them.  The poly outline must be converted to
                    // segments first.
                    SHAPE_POLY_SET outline = aKnockout->Outline()->CloneDropTriangulation();
                    outline.ClearArcs();

                    aRawFill.BooleanSubtract( outline, SHAPE_POLY_SET::PM_FAST );
                }
            };

    for( ZONE* otherZone : m_board->Zones() )
    {
        // Don't use the `HigherPriority()` check here because we _only_ want to knock out zones
        // with explicitly higher priorities, not those with equal priorities
        if( otherZone->SameNet( aZone )
                && otherZone->GetAssignedPriority() > aZone->GetAssignedPriority() )
        {
            // Do not remove teardrop area: it is not useful and not good
            if( !otherZone->IsTeardropArea() )
                knockoutZoneOutline( otherZone );
        }
    }

    for( FOOTPRINT* footprint : m_board->Footprints() )
    {
        for( ZONE* otherZone : footprint->Zones() )
        {
            if( otherZone->SameNet( aZone ) && otherZone->HigherPriority( aZone ) )
            {
                // Do not remove teardrop area: it is not useful and not good
                if( !otherZone->IsTeardropArea() )
                    knockoutZoneOutline( otherZone );
            }
        }
    }
}


#define DUMP_POLYS_TO_COPPER_LAYER( a, b, c ) \
    { if( m_debugZoneFiller && aDebugLayer == b ) \
        { \
            m_board->SetLayerName( b, c ); \
            SHAPE_POLY_SET d = a; \
            d.Fracture( SHAPE_POLY_SET::PM_STRICTLY_SIMPLE ); \
            aFillPolys = d; \
            return false; \
        } \
    }


/**
 * 1 - Creates the main zone outline using a correction to shrink the resulting area by
 *     m_ZoneMinThickness / 2.  The result is areas with a margin of m_ZoneMinThickness / 2
 *     so that when drawing outline with segments having a thickness of m_ZoneMinThickness the
 *     outlines will match exactly the initial outlines
 * 2 - Knocks out thermal reliefs around thermally-connected pads
 * 3 - Builds a set of thermal spoke for the whole zone
 * 4 - Knocks out unconnected copper items, deleting any affected spokes
 * 5 - Removes unconnected copper islands, deleting any affected spokes
 * 6 - Adds in the remaining spokes
 */
bool ZONE_FILLER::fillCopperZone( const ZONE* aZone, PCB_LAYER_ID aLayer, PCB_LAYER_ID aDebugLayer,
                                  const SHAPE_POLY_SET& aSmoothedOutline,
                                  const SHAPE_POLY_SET& aMaxExtents, SHAPE_POLY_SET& aFillPolys )
{
    m_maxError = m_board->GetDesignSettings().m_MaxError;

    // Features which are min_width should survive pruning; features that are *less* than
    // min_width should not.  Therefore we subtract epsilon from the min_width when
    // deflating/inflating.
    int half_min_width = aZone->GetMinThickness() / 2;
    int epsilon = pcbIUScale.mmToIU( 0.001 );
    int numSegs = GetArcToSegmentCount( half_min_width, m_maxError, FULL_CIRCLE );

    // Solid polygons are deflated and inflated during calculations.  Deflating doesn't cause
    // issues, but inflate is tricky as it can create excessively long and narrow spikes for
    // acute angles.
    // ALLOW_ACUTE_CORNERS cannot be used due to the spike problem.
    // CHAMFER_ACUTE_CORNERS is tempting, but can still produce spikes in some unusual
    // circumstances (https://gitlab.com/kicad/code/kicad/-/issues/5581).
    // It's unclear if ROUND_ACUTE_CORNERS would have the same issues, but is currently avoided
    // as a "less-safe" option.
    // ROUND_ALL_CORNERS produces the uniformly nicest shapes, but also a lot of segments.
    // CHAMFER_ALL_CORNERS improves the segment count.
    SHAPE_POLY_SET::CORNER_STRATEGY fastCornerStrategy = SHAPE_POLY_SET::CHAMFER_ALL_CORNERS;
    SHAPE_POLY_SET::CORNER_STRATEGY cornerStrategy = SHAPE_POLY_SET::ROUND_ALL_CORNERS;

    std::vector<PAD*>            thermalConnectionPads;
    std::vector<PAD*>            noConnectionPads;
    std::deque<SHAPE_LINE_CHAIN> thermalSpokes;
    SHAPE_POLY_SET               clearanceHoles;

    aFillPolys = aSmoothedOutline;
    DUMP_POLYS_TO_COPPER_LAYER( aFillPolys, In1_Cu, wxT( "smoothed-outline" ) );

    if( m_progressReporter && m_progressReporter->IsCancelled() )
        return false;

    /* -------------------------------------------------------------------------------------
     * Knockout thermal reliefs.
     */

    knockoutThermalReliefs( aZone, aLayer, aFillPolys, thermalConnectionPads, noConnectionPads );
    DUMP_POLYS_TO_COPPER_LAYER( aFillPolys, In2_Cu, wxT( "minus-thermal-reliefs" ) );

    if( m_progressReporter && m_progressReporter->IsCancelled() )
        return false;

    /* -------------------------------------------------------------------------------------
     * Knockout electrical clearances.
     */

    buildCopperItemClearances( aZone, aLayer, noConnectionPads, clearanceHoles );
    DUMP_POLYS_TO_COPPER_LAYER( clearanceHoles, In3_Cu, wxT( "clearance-holes" ) );

    if( m_progressReporter && m_progressReporter->IsCancelled() )
        return false;

    /* -------------------------------------------------------------------------------------
     * Add thermal relief spokes.
     */

    buildThermalSpokes( aZone, aLayer, thermalConnectionPads, thermalSpokes );

    if( m_progressReporter && m_progressReporter->IsCancelled() )
        return false;

    // Create a temporary zone that we can hit-test spoke-ends against.  It's only temporary
    // because the "real" subtract-clearance-holes has to be done after the spokes are added.
    static const bool USE_BBOX_CACHES = true;
    SHAPE_POLY_SET testAreas = aFillPolys.CloneDropTriangulation();
    testAreas.BooleanSubtract( clearanceHoles, SHAPE_POLY_SET::PM_FAST );
    DUMP_POLYS_TO_COPPER_LAYER( testAreas, In4_Cu, wxT( "minus-clearance-holes" ) );

    // Prune features that don't meet minimum-width criteria
    if( half_min_width - epsilon > epsilon )
    {
        testAreas.Deflate( half_min_width - epsilon, numSegs, fastCornerStrategy );
        DUMP_POLYS_TO_COPPER_LAYER( testAreas, In5_Cu, wxT( "spoke-test-deflated" ) );

        testAreas.Inflate( half_min_width - epsilon, numSegs, fastCornerStrategy );
        DUMP_POLYS_TO_COPPER_LAYER( testAreas, In6_Cu, wxT( "spoke-test-reinflated" ) );
    }

    if( m_progressReporter && m_progressReporter->IsCancelled() )
        return false;

    // Spoke-end-testing is hugely expensive so we generate cached bounding-boxes to speed
    // things up a bit.
    testAreas.BuildBBoxCaches();
    int interval = 0;

    SHAPE_POLY_SET debugSpokes;

    for( const SHAPE_LINE_CHAIN& spoke : thermalSpokes )
    {
        const VECTOR2I& testPt = spoke.CPoint( 3 );

        // Hit-test against zone body
        if( testAreas.Contains( testPt, -1, 1, USE_BBOX_CACHES ) )
        {
            if( m_debugZoneFiller )
                debugSpokes.AddOutline( spoke );

            aFillPolys.AddOutline( spoke );
            continue;
        }

        if( interval++ > 400 )
        {
            if( m_progressReporter && m_progressReporter->IsCancelled() )
                return false;

            interval = 0;
        }

        // Hit-test against other spokes
        for( const SHAPE_LINE_CHAIN& other : thermalSpokes )
        {
            // Hit test in both directions to avoid interactions with round-off errors.
            // (See https://gitlab.com/kicad/code/kicad/-/issues/13316.)
            if( &other != &spoke
                && other.PointInside( testPt, 1, USE_BBOX_CACHES )
                && spoke.PointInside( other.CPoint( 3 ), 1, USE_BBOX_CACHES ) )
            {
                if( m_debugZoneFiller )
                    debugSpokes.AddOutline( spoke );

                aFillPolys.AddOutline( spoke );
                break;
            }
        }
    }

    DUMP_POLYS_TO_COPPER_LAYER( debugSpokes, In7_Cu, wxT( "spokes" ) );

    if( m_progressReporter && m_progressReporter->IsCancelled() )
        return false;

    aFillPolys.BooleanSubtract( clearanceHoles, SHAPE_POLY_SET::PM_FAST );
    DUMP_POLYS_TO_COPPER_LAYER( aFillPolys, In8_Cu, wxT( "after-spoke-trimming" ) );

    /* -------------------------------------------------------------------------------------
     * Prune features that don't meet minimum-width criteria
     */

    if( half_min_width - epsilon > epsilon )
        aFillPolys.Deflate( half_min_width - epsilon, numSegs, fastCornerStrategy );

    // Min-thickness is the web thickness.  On the other hand, a blob min-thickness by
    // min-thickness is not useful.  Since there's no obvious definition of web vs. blob, we
    // arbitrarily choose "at least 1/2 min-thickness on one axis".
    for( int ii = aFillPolys.OutlineCount() - 1; ii >= 0; ii-- )
    {
        std::vector<SHAPE_LINE_CHAIN>& island = aFillPolys.Polygon( ii );
        BOX2I                          islandExtents = island.front().BBox();

        if( islandExtents.GetSizeMax() < half_min_width )
            aFillPolys.DeletePolygon( ii );
    }

    DUMP_POLYS_TO_COPPER_LAYER( aFillPolys, In9_Cu, wxT( "deflated" ) );

    if( m_progressReporter && m_progressReporter->IsCancelled() )
        return false;

    /* -------------------------------------------------------------------------------------
     * Process the hatch pattern (note that we do this while deflated)
     */

    if( aZone->GetFillMode() == ZONE_FILL_MODE::HATCH_PATTERN )
    {
        if( !addHatchFillTypeOnZone( aZone, aLayer, aDebugLayer, aFillPolys ) )
            return false;
    }

    if( m_progressReporter && m_progressReporter->IsCancelled() )
        return false;

    /* -------------------------------------------------------------------------------------
     * Finish minimum-width pruning by re-inflating
     */

    if( half_min_width - epsilon > epsilon )
        aFillPolys.Inflate( half_min_width - epsilon, numSegs, cornerStrategy, true );

    DUMP_POLYS_TO_COPPER_LAYER( aFillPolys, In15_Cu, wxT( "after-reinflating" ) );

    /* -------------------------------------------------------------------------------------
     * Ensure additive changes (thermal stubs and inflating acute corners) do not add copper
     * outside the zone boundary, inside the clearance holes, or between otherwise isolated
     * islands
     */

    for( PAD* pad : thermalConnectionPads )
        addHoleKnockout( pad, 0, clearanceHoles );

    aFillPolys.BooleanIntersection( aMaxExtents, SHAPE_POLY_SET::PM_FAST );
    DUMP_POLYS_TO_COPPER_LAYER( aFillPolys, In16_Cu, wxT( "after-trim-to-outline" ) );
    aFillPolys.BooleanSubtract( clearanceHoles, SHAPE_POLY_SET::PM_FAST );
    DUMP_POLYS_TO_COPPER_LAYER( aFillPolys, In17_Cu, wxT( "after-trim-to-clearance-holes" ) );

    /* -------------------------------------------------------------------------------------
     * Lastly give any same-net but higher-priority zones control over their own area.
     */

    subtractHigherPriorityZones( aZone, aLayer, aFillPolys );
    DUMP_POLYS_TO_COPPER_LAYER( aFillPolys, In18_Cu, wxT( "minus-higher-priority-zones" ) );

    aFillPolys.Fracture( SHAPE_POLY_SET::PM_FAST );
    return true;
}


bool ZONE_FILLER::fillNonCopperZone( const ZONE* aZone, PCB_LAYER_ID aLayer,
                                     const SHAPE_POLY_SET& aSmoothedOutline,
                                     SHAPE_POLY_SET& aFillPolys )
{
    BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();
    BOX2I                  zone_boundingbox = aZone->GetBoundingBox();
    SHAPE_POLY_SET         clearanceHoles;
    long                   ticker = 0;

    auto checkForCancel =
            [&ticker]( PROGRESS_REPORTER* aReporter ) -> bool
            {
                return aReporter && ( ticker++ % 50 ) == 0 && aReporter->IsCancelled();
            };

    auto knockoutGraphicClearance =
            [&]( BOARD_ITEM* aItem )
            {
                if( aItem->IsKnockout() && aItem->IsOnLayer( aLayer )
                        && aItem->GetBoundingBox().Intersects( zone_boundingbox ) )
                {
                    DRC_CONSTRAINT cc = bds.m_DRCEngine->EvalRules( PHYSICAL_CLEARANCE_CONSTRAINT,
                                                                    aZone, aItem, aLayer );

                    addKnockout( aItem, aLayer, cc.GetValue().Min(), false, clearanceHoles );
                }
            };

    for( FOOTPRINT* footprint : m_board->Footprints() )
    {
        if( checkForCancel( m_progressReporter ) )
            return false;

        knockoutGraphicClearance( &footprint->Reference() );
        knockoutGraphicClearance( &footprint->Value() );

        for( BOARD_ITEM* item : footprint->GraphicalItems() )
            knockoutGraphicClearance( item );
    }

    for( BOARD_ITEM* item : m_board->Drawings() )
    {
        if( checkForCancel( m_progressReporter ) )
            return false;

        knockoutGraphicClearance( item );
    }

    aFillPolys = aSmoothedOutline;
    aFillPolys.BooleanSubtract( clearanceHoles, SHAPE_POLY_SET::PM_FAST );

    for( ZONE* keepout : m_board->Zones() )
    {
        if( !keepout->GetIsRuleArea() )
            continue;

        if( keepout->GetDoNotAllowCopperPour() && keepout->IsOnLayer( aLayer ) )
        {
            if( keepout->GetBoundingBox().Intersects( zone_boundingbox ) )
                aFillPolys.BooleanSubtract( *keepout->Outline(), SHAPE_POLY_SET::PM_FAST );
        }
    }

    // Features which are min_width should survive pruning; features that are *less* than
    // min_width should not.  Therefore we subtract epsilon from the min_width when
    // deflating/inflating.
    int half_min_width = aZone->GetMinThickness() / 2;
    int epsilon = pcbIUScale.mmToIU( 0.001 );
    int numSegs = GetArcToSegmentCount( half_min_width, m_maxError, FULL_CIRCLE );

    aFillPolys.Deflate( half_min_width - epsilon, numSegs );

    // Remove the non filled areas due to the hatch pattern
    if( aZone->GetFillMode() == ZONE_FILL_MODE::HATCH_PATTERN )
    {
        if( !addHatchFillTypeOnZone( aZone, aLayer, aLayer, aFillPolys ) )
            return false;
    }

    // Re-inflate after pruning of areas that don't meet minimum-width criteria
    if( half_min_width - epsilon > epsilon )
        aFillPolys.Inflate( half_min_width - epsilon, numSegs );

    aFillPolys.Fracture( SHAPE_POLY_SET::PM_STRICTLY_SIMPLE );
    return true;
}


/*
 * Build the filled solid areas data from real outlines (stored in m_Poly)
 * The solid areas can be more than one on copper layers, and do not have holes
 * ( holes are linked by overlapping segments to the main outline)
 */
bool ZONE_FILLER::fillSingleZone( ZONE* aZone, PCB_LAYER_ID aLayer, SHAPE_POLY_SET& aFillPolys )
{
    SHAPE_POLY_SET* boardOutline = m_brdOutlinesValid ? &m_boardOutline : nullptr;
    SHAPE_POLY_SET  maxExtents;
    SHAPE_POLY_SET  smoothedPoly;
    PCB_LAYER_ID    debugLayer = UNDEFINED_LAYER;

    if( m_debugZoneFiller && LSET::InternalCuMask().Contains( aLayer ) )
    {
        debugLayer = aLayer;
        aLayer = F_Cu;
    }

    if( !aZone->BuildSmoothedPoly( maxExtents, aLayer, boardOutline, &smoothedPoly ) )
        return false;

    if( m_progressReporter && m_progressReporter->IsCancelled() )
        return false;

    if( aZone->IsOnCopperLayer() )
    {
        if( fillCopperZone( aZone, aLayer, debugLayer, smoothedPoly, maxExtents, aFillPolys ) )
            aZone->SetNeedRefill( false );
    }
    else
    {
        if( fillNonCopperZone( aZone, aLayer, smoothedPoly, aFillPolys ) )
            aZone->SetNeedRefill( false );
    }

    return true;
}


/**
 * Function buildThermalSpokes
 */
void ZONE_FILLER::buildThermalSpokes( const ZONE* aZone, PCB_LAYER_ID aLayer,
                                      const std::vector<PAD*>& aSpokedPadsList,
                                      std::deque<SHAPE_LINE_CHAIN>& aSpokesList )
{
    BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();
    BOX2I                  zoneBB = aZone->GetBoundingBox();
    DRC_CONSTRAINT         constraint;

    zoneBB.Inflate( std::max( bds.GetBiggestClearanceValue(), aZone->GetLocalClearance() ) );

    // Is a point on the boundary of the polygon inside or outside?  This small epsilon lets
    // us avoid the question.
    int epsilon = KiROUND( pcbIUScale.IU_PER_MM * 0.04 ); // about 1.5 mil

    for( PAD* pad : aSpokedPadsList )
    {
        // We currently only connect to pads, not pad holes
        if( !pad->IsOnLayer( aLayer ) )
            continue;

        constraint = bds.m_DRCEngine->EvalRules( THERMAL_RELIEF_GAP_CONSTRAINT, pad, aZone, aLayer );
        int thermalReliefGap = constraint.GetValue().Min();

        constraint = bds.m_DRCEngine->EvalRules( THERMAL_SPOKE_WIDTH_CONSTRAINT, pad, aZone, aLayer );
        int spoke_w = constraint.GetValue().Opt();

        // Spoke width should ideally be smaller than the pad minor axis.
        // Otherwise the thermal shape is not really a thermal relief,
        // and the algo to count the actual number of spokes can fail
        int spoke_max_allowed_w = std::min( pad->GetSize().x, pad->GetSize().y );

        spoke_w = std::max( spoke_w, constraint.Value().Min() );
        spoke_w = std::min( spoke_w, constraint.Value().Max() );

        // ensure the spoke width is smaller than the pad minor size
        spoke_w = std::min( spoke_w, spoke_max_allowed_w );

        // Cannot create stubs having a width < zone min thickness
        if( spoke_w < aZone->GetMinThickness() )
            continue;

        int spoke_half_w = spoke_w / 2;

        // Quick test here to possibly save us some work
        BOX2I itemBB = pad->GetBoundingBox();
        itemBB.Inflate( thermalReliefGap + epsilon );

        if( !( itemBB.Intersects( zoneBB ) ) )
            continue;

        // Thermal spokes consist of square-ended segments from the pad center to points just
        // outside the thermal relief.  The outside end has an extra center point (which must be
        // at idx 3) which is used for testing whether or not the spoke connects to copper in the
        // parent zone.

        auto buildSpokesFromOrigin =
                [&]( const BOX2I& box )
                {
                    for( int i = 0; i < 4; i++ )
                    {
                        SHAPE_LINE_CHAIN spoke;

                        switch( i )
                        {
                        case 0:       // lower stub
                            spoke.Append( +spoke_half_w, -spoke_half_w );
                            spoke.Append( -spoke_half_w, -spoke_half_w );
                            spoke.Append( -spoke_half_w, box.GetBottom() );
                            spoke.Append( 0,             box.GetBottom() );  // test pt
                            spoke.Append( +spoke_half_w, box.GetBottom() );
                            break;

                        case 1:       // upper stub
                            spoke.Append( +spoke_half_w, +spoke_half_w );
                            spoke.Append( -spoke_half_w, +spoke_half_w );
                            spoke.Append( -spoke_half_w, box.GetTop() );
                            spoke.Append( 0,             box.GetTop() );     // test pt
                            spoke.Append( +spoke_half_w, box.GetTop() );
                            break;

                        case 2:       // right stub
                            spoke.Append( -spoke_half_w,  +spoke_half_w );
                            spoke.Append( -spoke_half_w,  -spoke_half_w );
                            spoke.Append( box.GetRight(), -spoke_half_w );
                            spoke.Append( box.GetRight(), 0             );   // test pt
                            spoke.Append( box.GetRight(), +spoke_half_w );
                            break;

                        case 3:       // left stub
                            spoke.Append( +spoke_half_w, +spoke_half_w );
                            spoke.Append( +spoke_half_w, -spoke_half_w );
                            spoke.Append( box.GetLeft(), -spoke_half_w );
                            spoke.Append( box.GetLeft(), 0             );    // test pt
                            spoke.Append( box.GetLeft(), +spoke_half_w );
                            break;
                        }

                        spoke.SetClosed( true );
                        aSpokesList.push_back( std::move( spoke ) );
                    }
                };

        // If the spokes are at a cardinal angle then we can generate them from a bounding box
        // without trig.
        if( ( pad->GetOrientation() + pad->GetThermalSpokeAngle() ).IsCardinal() )
        {
            BOX2I spokesBox = pad->GetBoundingBox();
            spokesBox.Inflate( thermalReliefGap + epsilon );

            // Spokes are from center of pad shape, not from hole.
            spokesBox.Offset( - pad->ShapePos() );

            buildSpokesFromOrigin( spokesBox );

            auto spokeIter = aSpokesList.rbegin();

            for( int ii = 0; ii < 4; ++ii, ++spokeIter )
                spokeIter->Move( pad->ShapePos() );
        }
        // Even if the spokes are rotated, we can fudge it for round and square pads by rotating
        // the bounding box to match the spokes.
        else if( pad->GetSizeX() == pad->GetSizeY() && pad->GetShape() != PAD_SHAPE::CUSTOM )
        {
            // Since the bounding-box needs to be correclty rotated we use a dummy pad to keep
            // from dirtying the real pad's cached shapes.
            PAD dummy_pad( *pad );
            dummy_pad.SetOrientation( pad->GetThermalSpokeAngle() );

            // Spokes are from center of pad shape, not from hole. So the dummy pad has no shape
            // offset and is at position 0,0
            dummy_pad.SetPosition( VECTOR2I( 0, 0 ) );
            dummy_pad.SetOffset( VECTOR2I( 0, 0 ) );

            BOX2I spokesBox = dummy_pad.GetBoundingBox();
            spokesBox.Inflate( thermalReliefGap + epsilon );

            buildSpokesFromOrigin( spokesBox );

            auto spokeIter = aSpokesList.rbegin();

            for( int ii = 0; ii < 4; ++ii, ++spokeIter )
            {
                spokeIter->Rotate( pad->GetOrientation() + pad->GetThermalSpokeAngle() );
                spokeIter->Move( pad->ShapePos() );
            }
        }
        // And lastly, even when we have to resort to trig, we can use it only in a post-process
        // after the rotated-bounding-box trick from above.
        else
        {
            // Since the bounding-box needs to be correclty rotated we use a dummy pad to keep
            // from dirtying the real pad's cached shapes.
            PAD dummy_pad( *pad );
            dummy_pad.SetOrientation( pad->GetThermalSpokeAngle() );

            // Spokes are from center of pad shape, not from hole. So the dummy pad has no shape
            // offset and is at position 0,0
            dummy_pad.SetPosition( VECTOR2I( 0, 0 ) );
            dummy_pad.SetOffset( VECTOR2I( 0, 0 ) );

            BOX2I spokesBox = dummy_pad.GetBoundingBox();

            // In this case make the box -big-; we're going to clip to the "real" bbox later.
            spokesBox.Inflate( thermalReliefGap + spokesBox.GetWidth() + spokesBox.GetHeight() );

            buildSpokesFromOrigin( spokesBox );

            BOX2I realBBox = pad->GetBoundingBox();
            realBBox.Inflate( thermalReliefGap + epsilon );

            auto spokeIter = aSpokesList.rbegin();

            for( int ii = 0; ii < 4; ++ii, ++spokeIter )
            {
                spokeIter->Rotate( pad->GetOrientation() + pad->GetThermalSpokeAngle() );
                spokeIter->Move( pad->ShapePos() );

                VECTOR2I origin_p = spokeIter->GetPoint( 0 );
                VECTOR2I origin_m = spokeIter->GetPoint( 1 );
                VECTOR2I origin = ( origin_p + origin_m ) / 2;
                VECTOR2I end_m = spokeIter->GetPoint( 2 );
                VECTOR2I end = spokeIter->GetPoint( 3 );
                VECTOR2I end_p = spokeIter->GetPoint( 4 );

                ClipLine( &realBBox, origin_p.x, origin_p.y, end_p.x, end_p.y );
                ClipLine( &realBBox, origin_m.x, origin_m.y, end_m.x, end_m.y );
                ClipLine( &realBBox, origin.x, origin.y, end.x, end.y );

                spokeIter->SetPoint( 2, end_m );
                spokeIter->SetPoint( 3, end );
                spokeIter->SetPoint( 4, end_p );
            }
        }
    }

    for( size_t ii = 0; ii < aSpokesList.size(); ++ii )
        aSpokesList[ii].GenerateBBoxCache();
}


bool ZONE_FILLER::addHatchFillTypeOnZone( const ZONE* aZone, PCB_LAYER_ID aLayer,
                                          PCB_LAYER_ID aDebugLayer, SHAPE_POLY_SET& aFillPolys )
{
    // Build grid:

    // obviously line thickness must be > zone min thickness.
    // It can happens if a board file was edited by hand by a python script
    // Use 1 micron margin to be *sure* there is no issue in Gerber files
    // (Gbr file unit = 1 or 10 nm) due to some truncation in coordinates or calculations
    // This margin also avoid problems due to rounding coordinates in next calculations
    // that can create incorrect polygons
    int thickness = std::max( aZone->GetHatchThickness(),
                              aZone->GetMinThickness() + pcbIUScale.mmToIU( 0.001 ) );

    int linethickness = thickness - aZone->GetMinThickness();
    int gridsize = thickness + aZone->GetHatchGap();

    SHAPE_POLY_SET filledPolys = aFillPolys.CloneDropTriangulation();
    // Use a area that contains the rotated bbox by orientation, and after rotate the result
    // by -orientation.
    if( !aZone->GetHatchOrientation().IsZero() )
        filledPolys.Rotate( - aZone->GetHatchOrientation() );

    BOX2I bbox = filledPolys.BBox( 0 );

    // Build hole shape
    // the hole size is aZone->GetHatchGap(), but because the outline thickness
    // is aZone->GetMinThickness(), the hole shape size must be larger
    SHAPE_LINE_CHAIN hole_base;
    int hole_size = aZone->GetHatchGap() + aZone->GetMinThickness();
    VECTOR2I corner( 0, 0 );;
    hole_base.Append( corner );
    corner.x += hole_size;
    hole_base.Append( corner );
    corner.y += hole_size;
    hole_base.Append( corner );
    corner.x = 0;
    hole_base.Append( corner );
    hole_base.SetClosed( true );

    // Calculate minimal area of a grid hole.
    // All holes smaller than a threshold will be removed
    double minimal_hole_area = hole_base.Area() * aZone->GetHatchHoleMinArea();

    // Now convert this hole to a smoothed shape:
    if( aZone->GetHatchSmoothingLevel() > 0 )
    {
        // the actual size of chamfer, or rounded corner radius is the half size
        // of the HatchFillTypeGap scaled by aZone->GetHatchSmoothingValue()
        // aZone->GetHatchSmoothingValue() = 1.0 is the max value for the chamfer or the
        // radius of corner (radius = half size of the hole)
        int smooth_value = KiROUND( aZone->GetHatchGap()
                                    * aZone->GetHatchSmoothingValue() / 2 );

        // Minimal optimization:
        // make smoothing only for reasonable smooth values, to avoid a lot of useless segments
        // and if the smooth value is small, use chamfer even if fillet is requested
        #define SMOOTH_MIN_VAL_MM 0.02
        #define SMOOTH_SMALL_VAL_MM 0.04

        if( smooth_value > pcbIUScale.mmToIU( SMOOTH_MIN_VAL_MM ) )
        {
            SHAPE_POLY_SET smooth_hole;
            smooth_hole.AddOutline( hole_base );
            int smooth_level = aZone->GetHatchSmoothingLevel();

            if( smooth_value < pcbIUScale.mmToIU( SMOOTH_SMALL_VAL_MM ) && smooth_level > 1 )
                smooth_level = 1;

            // Use a larger smooth_value to compensate the outline tickness
            // (chamfer is not visible is smooth value < outline thickess)
            smooth_value += aZone->GetMinThickness() / 2;

            // smooth_value cannot be bigger than the half size oh the hole:
            smooth_value = std::min( smooth_value, aZone->GetHatchGap() / 2 );

            // the error to approximate a circle by segments when smoothing corners by a arc
            int error_max = std::max( pcbIUScale.mmToIU( 0.01 ), smooth_value / 20 );

            switch( smooth_level )
            {
            case 1:
                // Chamfer() uses the distance from a corner to create a end point
                // for the chamfer.
                hole_base = smooth_hole.Chamfer( smooth_value ).Outline( 0 );
                break;

            default:
                if( aZone->GetHatchSmoothingLevel() > 2 )
                    error_max /= 2;    // Force better smoothing

                hole_base = smooth_hole.Fillet( smooth_value, error_max ).Outline( 0 );
                break;

            case 0:
                break;
            };
        }
    }

    // Build holes
    SHAPE_POLY_SET holes;

    for( int xx = 0; ; xx++ )
    {
        int xpos = xx * gridsize;

        if( xpos > bbox.GetWidth() )
            break;

        for( int yy = 0; ; yy++ )
        {
            int ypos = yy * gridsize;

            if( ypos > bbox.GetHeight() )
                break;

            // Generate hole
            SHAPE_LINE_CHAIN hole( hole_base );
            hole.Move( VECTOR2I( xpos, ypos ) );
            holes.AddOutline( hole );
        }
    }

    holes.Move( bbox.GetPosition() );

    if( !aZone->GetHatchOrientation().IsZero() )
        holes.Rotate( aZone->GetHatchOrientation() );

    DUMP_POLYS_TO_COPPER_LAYER( holes, In10_Cu, wxT( "hatch-holes" ) );

    int outline_margin = aZone->GetMinThickness() * 1.1;

    // Using GetHatchThickness() can look more consistent than GetMinThickness().
    if( aZone->GetHatchBorderAlgorithm() && aZone->GetHatchThickness() > outline_margin )
        outline_margin = aZone->GetHatchThickness();

    // The fill has already been deflated to ensure GetMinThickness() so we just have to
    // account for anything beyond that.
    SHAPE_POLY_SET deflatedFilledPolys = aFillPolys.CloneDropTriangulation();
    deflatedFilledPolys.Deflate( outline_margin - aZone->GetMinThickness(), 16 );
    holes.BooleanIntersection( deflatedFilledPolys, SHAPE_POLY_SET::PM_FAST );
    DUMP_POLYS_TO_COPPER_LAYER( holes, In11_Cu, wxT( "fill-clipped-hatch-holes" ) );

    SHAPE_POLY_SET deflatedOutline = aZone->Outline()->CloneDropTriangulation();
    deflatedOutline.Deflate( outline_margin, 16 );
    holes.BooleanIntersection( deflatedOutline, SHAPE_POLY_SET::PM_FAST );
    DUMP_POLYS_TO_COPPER_LAYER( holes, In12_Cu, wxT( "outline-clipped-hatch-holes" ) );

    if( aZone->GetNetCode() != 0 )
    {
        // Vias and pads connected to the zone must not be allowed to become isolated inside
        // one of the holes.  Effectively this means their copper outline needs to be expanded
        // to be at least as wide as the gap so that it is guaranteed to touch at least one
        // edge.
        BOX2I          zone_boundingbox = aZone->GetBoundingBox();
        SHAPE_POLY_SET aprons;
        int            min_apron_radius = ( aZone->GetHatchGap() * 10 ) / 19;

        for( PCB_TRACK* track : m_board->Tracks() )
        {
            if( track->Type() == PCB_VIA_T )
            {
                PCB_VIA* via = static_cast<PCB_VIA*>( track );

                if( via->GetNetCode() == aZone->GetNetCode()
                    && via->IsOnLayer( aLayer )
                    && via->GetBoundingBox().Intersects( zone_boundingbox ) )
                {
                    int r = std::max( min_apron_radius,
                                      via->GetDrillValue() / 2 + outline_margin );

                    TransformCircleToPolygon( aprons, via->GetPosition(), r, ARC_HIGH_DEF,
                                              ERROR_OUTSIDE );
                }
            }
        }

        for( FOOTPRINT* footprint : m_board->Footprints() )
        {
            for( PAD* pad : footprint->Pads() )
            {
                if( pad->GetNetCode() == aZone->GetNetCode()
                    && pad->IsOnLayer( aLayer )
                    && pad->GetBoundingBox().Intersects( zone_boundingbox ) )
                {
                    // What we want is to bulk up the pad shape so that the narrowest bit of
                    // copper between the hole and the apron edge is at least outline_margin
                    // wide (and that the apron itself meets min_apron_radius.  But that would
                    // take a lot of code and math, and the following approximation is close
                    // enough.
                    int pad_width = std::min( pad->GetSize().x, pad->GetSize().y );
                    int slot_width = std::min( pad->GetDrillSize().x, pad->GetDrillSize().y );
                    int min_annular_ring_width = ( pad_width - slot_width ) / 2;
                    int clearance = std::max( min_apron_radius - pad_width / 2,
                                              outline_margin - min_annular_ring_width );

                    clearance = std::max( 0, clearance - linethickness / 2 );
                    pad->TransformShapeToPolygon( aprons, aLayer, clearance, ARC_HIGH_DEF,
                                                  ERROR_OUTSIDE );
                }
            }
        }

        holes.BooleanSubtract( aprons, SHAPE_POLY_SET::PM_FAST );
    }
    DUMP_POLYS_TO_COPPER_LAYER( holes, In13_Cu, wxT( "pad-via-clipped-hatch-holes" ) );

    // Now filter truncated holes to avoid small holes in pattern
    // It happens for holes near the zone outline
    for( int ii = 0; ii < holes.OutlineCount(); )
    {
        double area = holes.Outline( ii ).Area();

        if( area < minimal_hole_area ) // The current hole is too small: remove it
            holes.DeletePolygon( ii );
        else
            ++ii;
    }

    // create grid. Use SHAPE_POLY_SET::PM_STRICTLY_SIMPLE to
    // generate strictly simple polygons needed by Gerber files and Fracture()
    aFillPolys.BooleanSubtract( aFillPolys, holes, SHAPE_POLY_SET::PM_STRICTLY_SIMPLE );
    DUMP_POLYS_TO_COPPER_LAYER( aFillPolys, In14_Cu, wxT( "after-hatching" ) );

    return true;
}
