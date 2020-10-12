/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014-2017 CERN
 * Copyright (C) 2014-2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <thread>
#include <algorithm>
#include <future>

#include <advanced_config.h>
#include <class_board.h>
#include <class_zone.h>
#include <class_module.h>
#include <fp_shape.h>
#include <pcb_shape.h>
#include <pcb_text.h>
#include <class_pcb_target.h>
#include <class_track.h>
#include <connectivity/connectivity_data.h>
#include <convert_basic_shapes_to_polygon.h>
#include <board_commit.h>
#include <widgets/progress_reporter.h>
#include <geometry/shape_poly_set.h>
#include <geometry/convex_hull.h>
#include <geometry/geometry_utils.h>
#include <confirm.h>
#include <convert_to_biu.h>
#include <math/util.h>      // for KiROUND
#include "zone_filler.h"

static const double s_RoundPadThermalSpokeAngle = 450;      // in deci-degrees


ZONE_FILLER::ZONE_FILLER(  BOARD* aBoard, COMMIT* aCommit ) :
        m_board( aBoard ),
        m_brdOutlinesValid( false ),
        m_commit( aCommit ),
        m_progressReporter( nullptr ),
        m_maxError( ARC_HIGH_DEF )
{
    // To enable add "DebugZoneFiller=1" to kicad_advanced settings file.
    m_debugZoneFiller = ADVANCED_CFG::GetCfg().m_DebugZoneFiller;
}


ZONE_FILLER::~ZONE_FILLER()
{
}


void ZONE_FILLER::InstallNewProgressReporter( wxWindow* aParent, const wxString& aTitle,
                                              int aNumPhases )
{
    m_uniqueReporter = std::make_unique<WX_PROGRESS_REPORTER>( aParent, aTitle, aNumPhases );
    SetProgressReporter( m_uniqueReporter.get() );
}


void ZONE_FILLER::SetProgressReporter( PROGRESS_REPORTER* aReporter )
{
    m_progressReporter = aReporter;
    wxASSERT_MSG( m_commit, "ZONE_FILLER must have a valid commit to call SetProgressReporter" );
}


bool ZONE_FILLER::Fill( std::vector<ZONE_CONTAINER*>& aZones, bool aCheck, wxWindow* aParent )
{
    std::vector<std::pair<ZONE_CONTAINER*, PCB_LAYER_ID>> toFill;
    std::vector<CN_ZONE_ISOLATED_ISLAND_LIST> islandsList;

    std::shared_ptr<CONNECTIVITY_DATA> connectivity = m_board->GetConnectivity();
    std::unique_lock<std::mutex> lock( connectivity->GetLock(), std::try_to_lock );

    BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();
    int                    worstClearance = bds.GetBiggestClearanceValue();

    if( !lock )
        return false;

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

    // Update the bounding box and shape caches in the pads to prevent multi-threaded rebuilds.
    for( MODULE* module : m_board->Modules() )
    {
        for( D_PAD* pad : module->Pads() )
        {
            if( pad->IsDirty() )
                pad->BuildEffectiveShapes( UNDEFINED_LAYER );
        }
    }

    // Sort by priority to reduce deferrals waiting on higher priority zones.
    std::sort( aZones.begin(), aZones.end(),
               []( const ZONE_CONTAINER* lhs, const ZONE_CONTAINER* rhs )
               {
                   return lhs->GetPriority() > rhs->GetPriority();
               } );

    for( ZONE_CONTAINER* zone : aZones )
    {
        zone->CacheBoundingBox();

        // Rule areas are not filled
        if( zone->GetIsRuleArea() )
            continue;

        if( m_commit )
            m_commit->Modify( zone );

        // calculate the hash value for filled areas. it will be used later
        // to know if the current filled areas are up to date
        for( PCB_LAYER_ID layer : zone->GetLayerSet().Seq() )
        {
            zone->BuildHashValue( layer );

            // Add the zone to the list of zones to test or refill
            toFill.emplace_back( std::make_pair( zone, layer ) );
        }

        islandsList.emplace_back( CN_ZONE_ISOLATED_ISLAND_LIST( zone ) );

        // Remove existing fill first to prevent drawing invalid polygons
        // on some platforms
        zone->UnFill();

        zone->SetFillVersion( bds.m_ZoneFillVersion );
    }

    size_t cores = std::thread::hardware_concurrency();
    std::atomic<size_t> nextItem;

    auto check_fill_dependency =
            [&]( ZONE_CONTAINER* aZone, PCB_LAYER_ID aLayer, ZONE_CONTAINER* aOtherZone ) -> bool
            {
                // Check to see if we have to knock-out the filled areas of a higher-priority
                // zone.  If so we have to wait until said zone is filled before we can fill.

                // If the other zone is already filled then we're good-to-go
                if( aOtherZone->GetFillFlag( aLayer ) )
                    return false;

                // Even if keepouts exclude copper pours the exclusion is by outline, not by
                // filled area, so we're good-to-go here too.
                if( aOtherZone->GetIsRuleArea() )
                    return false;

                // If the zones share no common layers
                if( !aOtherZone->GetLayerSet().test( aLayer ) )
                    return false;

                if( aOtherZone->GetPriority() <= aZone->GetPriority() )
                    return false;

                // Same-net zones always use outline to produce predictable results
                if( aOtherZone->GetNetCode() == aZone->GetNetCode() )
                    return false;

                // A higher priority zone is found: if we intersect and it's not filled yet
                // then we have to wait.
                EDA_RECT inflatedBBox = aZone->GetCachedBoundingBox();
                inflatedBBox.Inflate( worstClearance );

                return inflatedBBox.Intersects( aOtherZone->GetCachedBoundingBox() );
            };

    auto fill_lambda =
            [&]( PROGRESS_REPORTER* aReporter )
            {
                size_t num = 0;

                for( size_t i = nextItem++; i < toFill.size(); i = nextItem++ )
                {
                    PCB_LAYER_ID    layer = toFill[i].second;
                    ZONE_CONTAINER* zone = toFill[i].first;
                    bool            canFill = true;

                    // Check for any fill dependencies.  If our zone needs to be clipped by
                    // another zone then we can't fill until that zone is filled.
                    for( ZONE_CONTAINER* otherZone : m_board->Zones() )
                    {
                        if( otherZone == zone )
                            continue;

                        if( check_fill_dependency( zone, layer, otherZone ) )
                        {
                            canFill = false;
                            break;
                        }
                    }

                    for( MODULE* module : m_board->Modules() )
                    {
                        for( ZONE_CONTAINER* otherZone : module->Zones() )
                        {
                            if( check_fill_dependency( zone, layer, otherZone ) )
                            {
                                canFill = false;
                                break;
                            }
                        }
                    }

                    if( m_progressReporter && m_progressReporter->IsCancelled() )
                        break;

                    if( !canFill )
                        continue;

                    // Now we're ready to fill.
                    SHAPE_POLY_SET rawPolys, finalPolys;
                    fillSingleZone( zone, layer, rawPolys, finalPolys );

                    std::unique_lock<std::mutex> zoneLock( zone->GetLock() );

                    zone->SetRawPolysList( layer, rawPolys );
                    zone->SetFilledPolysList( layer, finalPolys );
                    zone->SetFillFlag( layer, true );

                    if( m_progressReporter )
                        m_progressReporter->AdvanceProgress();

                    num++;
                }

                return num;
            };

    while( !toFill.empty() )
    {
        size_t parallelThreadCount = std::min( cores, toFill.size() );
        std::vector<std::future<size_t>> returns( parallelThreadCount );

        nextItem = 0;

        if( parallelThreadCount <= 1 )
            fill_lambda( m_progressReporter );
        else
        {
            for( size_t ii = 0; ii < parallelThreadCount; ++ii )
                returns[ii] = std::async( std::launch::async, fill_lambda, m_progressReporter );

            for( size_t ii = 0; ii < parallelThreadCount; ++ii )
            {
                // Here we balance returns with a 100ms timeout to allow UI updating
                std::future_status status;
                do
                {
                    if( m_progressReporter )
                        m_progressReporter->KeepRefreshing();

                    status = returns[ii].wait_for( std::chrono::milliseconds( 100 ) );
                } while( status != std::future_status::ready );
            }
        }

        toFill.erase( std::remove_if( toFill.begin(), toFill.end(),
                      [&] ( const std::pair<ZONE_CONTAINER*, PCB_LAYER_ID> pair ) -> bool
                      {
                          return pair.first->GetFillFlag( pair.second );
                      } ),
                      toFill.end() );

        if( m_progressReporter && m_progressReporter->IsCancelled() )
            break;
    }

    // Now update the connectivity to check for copper islands
    if( m_progressReporter )
    {
        if( m_progressReporter->IsCancelled() )
            return false;

        m_progressReporter->AdvancePhase();
        m_progressReporter->Report( _( "Removing insulated copper islands..." ) );
        m_progressReporter->KeepRefreshing();
    }

    connectivity->SetProgressReporter( m_progressReporter );
    connectivity->FindIsolatedCopperIslands( islandsList );
    connectivity->SetProgressReporter( nullptr );

    if( m_progressReporter && m_progressReporter->IsCancelled() )
        return false;

    for( ZONE_CONTAINER* zone : aZones )
    {
        // Keepout zones are not filled
        if( zone->GetIsRuleArea() )
            continue;

        zone->SetIsFilled( true );
    }

    // Now remove insulated copper islands
    for( CN_ZONE_ISOLATED_ISLAND_LIST& zone : islandsList )
    {
        for( PCB_LAYER_ID layer : zone.m_zone->GetLayerSet().Seq() )
        {
            if( m_debugZoneFiller && LSET::InternalCuMask().Contains( layer ) )
                continue;

            if( !zone.m_islands.count( layer ) )
                continue;

            std::vector<int>& islands = zone.m_islands.at( layer );

            // The list of polygons to delete must be explored from last to first in list,
            // to allow deleting a polygon from list without breaking the remaining of the list
            std::sort( islands.begin(), islands.end(), std::greater<int>() );

            SHAPE_POLY_SET      poly = zone.m_zone->GetFilledPolysList( layer );
            long long int       minArea = zone.m_zone->GetMinIslandArea();
            ISLAND_REMOVAL_MODE mode    = zone.m_zone->GetIslandRemovalMode();

            for( int idx : islands )
            {
                SHAPE_LINE_CHAIN& outline = poly.Outline( idx );

                if( mode == ISLAND_REMOVAL_MODE::ALWAYS )
                    poly.DeletePolygon( idx );
                else if ( mode == ISLAND_REMOVAL_MODE::AREA && outline.Area() < minArea )
                    poly.DeletePolygon( idx );
                else
                    zone.m_zone->SetIsIsland( layer, idx );
            }

            zone.m_zone->SetFilledPolysList( layer, poly );
            zone.m_zone->CalculateFilledArea();

            if( m_progressReporter && m_progressReporter->IsCancelled() )
                return false;
        }
    }

    // Now remove islands outside the board edge
    for( ZONE_CONTAINER* zone : aZones )
    {
        LSET zoneCopperLayers = zone->GetLayerSet() & LSET::AllCuMask( MAX_CU_LAYERS );

        for( PCB_LAYER_ID layer : zoneCopperLayers.Seq() )
        {
            if( m_debugZoneFiller && LSET::InternalCuMask().Contains( layer ) )
                continue;

            SHAPE_POLY_SET poly = zone->GetFilledPolysList( layer );

            for( int ii = poly.OutlineCount() - 1; ii >= 0; ii-- )
            {
                std::vector<SHAPE_LINE_CHAIN>& island = poly.Polygon( ii );

                if( island.empty() || !m_boardOutline.Contains( island.front().CPoint( 0 ) ) )
                    poly.DeletePolygon( ii );
            }

            zone->SetFilledPolysList( layer, poly );
            zone->CalculateFilledArea();

            if( m_progressReporter && m_progressReporter->IsCancelled() )
                return false;
        }
    }

    if( aCheck )
    {
        bool outOfDate = false;

        for( ZONE_CONTAINER* zone : aZones )
        {
            // Keepout zones are not filled
            if( zone->GetIsRuleArea() )
                continue;

            for( PCB_LAYER_ID layer : zone->GetLayerSet().Seq() )
            {
                MD5_HASH was = zone->GetHashValue( layer );
                zone->CacheTriangulation( layer );
                zone->BuildHashValue( layer );
                MD5_HASH is = zone->GetHashValue( layer );

                if( is != was )
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
    }

    if( m_progressReporter )
    {
        m_progressReporter->AdvancePhase();
        m_progressReporter->Report( _( "Performing polygon fills..." ) );
        m_progressReporter->SetMaxProgress( islandsList.size() );
    }

    nextItem = 0;

    auto tri_lambda =
            [&]( PROGRESS_REPORTER* aReporter ) -> size_t
            {
                size_t num = 0;

                for( size_t i = nextItem++; i < islandsList.size(); i = nextItem++ )
                {
                    islandsList[i].m_zone->CacheTriangulation();
                    num++;

                    if( m_progressReporter )
                    {
                        m_progressReporter->AdvanceProgress();

                        if( m_progressReporter->IsCancelled() )
                            break;
                    }
                }

                return num;
            };

    size_t parallelThreadCount = std::min( cores, islandsList.size() );
    std::vector<std::future<size_t>> returns( parallelThreadCount );

    if( parallelThreadCount <= 1 )
        tri_lambda( m_progressReporter );
    else
    {
        for( size_t ii = 0; ii < parallelThreadCount; ++ii )
            returns[ii] = std::async( std::launch::async, tri_lambda, m_progressReporter );

        for( size_t ii = 0; ii < parallelThreadCount; ++ii )
        {
            // Here we balance returns with a 100ms timeout to allow UI updating
            std::future_status status;
            do
            {
                if( m_progressReporter )
                {
                    m_progressReporter->KeepRefreshing();

                    if( m_progressReporter->IsCancelled() )
                        break;
                }

                status = returns[ii].wait_for( std::chrono::milliseconds( 100 ) );
            } while( status != std::future_status::ready );
        }
    }

    if( m_progressReporter )
    {
        if( m_progressReporter->IsCancelled() )
            return false;

        m_progressReporter->AdvancePhase();
        m_progressReporter->KeepRefreshing();
    }

    connectivity->SetProgressReporter( nullptr );
    return true;
}


/**
 * Return true if the given pad has a thermal connection with the given zone.
 */
bool hasThermalConnection( D_PAD* pad, const ZONE_CONTAINER* aZone )
{
    // Rejects non-standard pads with tht-only thermal reliefs
    if( aZone->GetPadConnection( pad ) == ZONE_CONNECTION::THT_THERMAL
            && pad->GetAttribute() != PAD_ATTRIB_PTH )
    {
        return false;
    }

    if( aZone->GetPadConnection( pad ) != ZONE_CONNECTION::THERMAL
            && aZone->GetPadConnection( pad ) != ZONE_CONNECTION::THT_THERMAL )
    {
        return false;
    }

    if( pad->GetNetCode() != aZone->GetNetCode() || pad->GetNetCode() <= 0 )
        return false;

    EDA_RECT item_boundingbox = pad->GetBoundingBox();
    int thermalGap = aZone->GetThermalReliefGap( pad );
    item_boundingbox.Inflate( thermalGap, thermalGap );

    return item_boundingbox.Intersects( aZone->GetCachedBoundingBox() );
}


/**
 * Setup aDummyPad to have the same size and shape of aPad's hole.  This allows us to create
 * thermal reliefs and clearances for holes using the pad code.
 */
static void setupDummyPadForHole( const D_PAD* aPad, D_PAD& aDummyPad )
{
    // People may author clearance rules that look at a bunch of different stuff so we need
    // to copy over pretty much everything except the shape info, which we fill from the drill
    // info.
    aDummyPad.SetLayerSet( aPad->GetLayerSet() );
    aDummyPad.SetAttribute( aPad->GetAttribute() );
    aDummyPad.SetProperty( aPad->GetProperty() );

    aDummyPad.SetNetCode( aPad->GetNetCode() );
    aDummyPad.SetLocalClearance( aPad->GetLocalClearance() );
    aDummyPad.SetLocalSolderMaskMargin( aPad->GetLocalSolderMaskMargin() );
    aDummyPad.SetLocalSolderPasteMargin( aPad->GetLocalSolderPasteMargin() );
    aDummyPad.SetLocalSolderPasteMarginRatio( aPad->GetLocalSolderPasteMarginRatio() );

    aDummyPad.SetZoneConnection( aPad->GetEffectiveZoneConnection() );
    aDummyPad.SetThermalSpokeWidth( aPad->GetEffectiveThermalSpokeWidth() );
    aDummyPad.SetThermalGap( aPad->GetEffectiveThermalGap() );

    aDummyPad.SetCustomShapeInZoneOpt( aPad->GetCustomShapeInZoneOpt() );

    // Note: drill size represents finish size, which means the actual holes size is the
    // plating thickness larger.
    int platingThickness = 0;

    if( aPad->GetAttribute() == PAD_ATTRIB_PTH )
        platingThickness = aPad->GetBoard()->GetDesignSettings().GetHolePlatingThickness();

    aDummyPad.SetOffset( wxPoint( 0, 0 ) );
    aDummyPad.SetSize( aPad->GetDrillSize() + wxSize( platingThickness, platingThickness ) );
    aDummyPad.SetShape( aPad->GetDrillShape() == PAD_DRILL_SHAPE_OBLONG ? PAD_SHAPE_OVAL
                                                                        : PAD_SHAPE_CIRCLE );
    aDummyPad.SetOrientation( aPad->GetOrientation() );
    aDummyPad.SetPosition( aPad->GetPosition() );
}


/**
 * Add a knockout for a pad.  The knockout is 'aGap' larger than the pad (which might be
 * either the thermal clearance or the electrical clearance).
 */
void ZONE_FILLER::addKnockout( D_PAD* aPad, PCB_LAYER_ID aLayer, int aGap, SHAPE_POLY_SET& aHoles )
{
    if( aPad->GetShape() == PAD_SHAPE_CUSTOM )
    {
        SHAPE_POLY_SET poly;
        aPad->TransformShapeWithClearanceToPolygon( poly, aLayer, aGap, m_maxError );

        // the pad shape in zone can be its convex hull or the shape itself
        if( aPad->GetCustomShapeInZoneOpt() == CUST_PAD_SHAPE_IN_ZONE_CONVEXHULL )
        {
            std::vector<wxPoint> convex_hull;
            BuildConvexHull( convex_hull, poly );

            aHoles.NewOutline();

            for( const wxPoint& pt : convex_hull )
                aHoles.Append( pt );
        }
        else
            aHoles.Append( poly );
    }
    else
    {
        aPad->TransformShapeWithClearanceToPolygon( aHoles, aLayer, aGap, m_maxError );
    }
}


/**
 * Add a knockout for a graphic item.  The knockout is 'aGap' larger than the item (which
 * might be either the electrical clearance or the board edge clearance).
 */
void ZONE_FILLER::addKnockout( BOARD_ITEM* aItem, PCB_LAYER_ID aLayer, int aGap,
                               bool aIgnoreLineWidth, SHAPE_POLY_SET& aHoles )
{
    switch( aItem->Type() )
    {
    case PCB_SHAPE_T:
    {
        PCB_SHAPE* shape = (PCB_SHAPE*) aItem;
        shape->TransformShapeWithClearanceToPolygon( aHoles, aLayer, aGap, m_maxError,
                                                     aIgnoreLineWidth );
        break;
    }
    case PCB_TEXT_T:
    {
        PCB_TEXT* text = (PCB_TEXT*) aItem;
        text->TransformBoundingBoxWithClearanceToPolygon( &aHoles, aGap );
        break;
    }
    case PCB_FP_SHAPE_T:
    {
        FP_SHAPE* shape = (FP_SHAPE*) aItem;
        shape->TransformShapeWithClearanceToPolygon( aHoles, aLayer, aGap, m_maxError,
                                                     aIgnoreLineWidth );
        break;
    }
    case PCB_FP_TEXT_T:
    {
        FP_TEXT* text = (FP_TEXT*) aItem;

        if( text->IsVisible() )
            text->TransformBoundingBoxWithClearanceToPolygon( &aHoles, aGap );

        break;
    }
    default:
        break;
    }
}


/**
 * Removes thermal reliefs from the shape for any pads connected to the zone.  Does NOT add
 * in spokes, which must be done later.
 */
void ZONE_FILLER::knockoutThermalReliefs( const ZONE_CONTAINER* aZone, PCB_LAYER_ID aLayer,
                                          SHAPE_POLY_SET& aFill )
{
    SHAPE_POLY_SET holes;

    // Use a dummy pad to calculate relief when a pad has a hole but is not on the zone's
    // copper layer.  The dummy pad has the size and shape of the original pad's hole. We have
    // to give it a parent because some functions expect a non-null parent to find clearance
    // data, etc.
    MODULE  dummymodule( m_board );
    D_PAD   dummypad( &dummymodule );

    for( auto module : m_board->Modules() )
    {
        for( auto pad : module->Pads() )
        {
            if( !hasThermalConnection( pad, aZone ) )
                continue;

            // If the pad isn't on the current layer but has a hole, knock out a thermal relief
            // for the hole.
            if( !pad->IsOnLayer( aLayer ) )
            {
                if( pad->GetDrillSize().x == 0 && pad->GetDrillSize().y == 0 )
                    continue;

                setupDummyPadForHole( pad, dummypad );
                pad = &dummypad;
            }

            addKnockout( pad, aLayer, aZone->GetThermalReliefGap( pad ), holes );
        }
    }

    aFill.BooleanSubtract( holes, SHAPE_POLY_SET::PM_FAST );
}


/**
 * Removes clearance from the shape for copper items which share the zone's layer but are
 * not connected to it.
 */
void ZONE_FILLER::buildCopperItemClearances( const ZONE_CONTAINER* aZone, PCB_LAYER_ID aLayer,
                                             SHAPE_POLY_SET& aHoles )
{
    static PCB_SHAPE dummyEdge;
    dummyEdge.SetParent( m_board );
    dummyEdge.SetLayer( Edge_Cuts );

    // A small extra clearance to be sure actual track clearances are not smaller than
    // requested clearance due to many approximations in calculations, like arc to segment
    // approx, rounding issues, etc.
    // 1 micron is a good value
    int extra_margin = Millimeter2iu( ADVANCED_CFG::GetCfg().m_ExtraClearance );

    BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();
    int                    zone_clearance = aZone->GetLocalClearance();
    EDA_RECT               zone_boundingbox = aZone->GetCachedBoundingBox();

    // Items outside the zone bounding box are skipped, so it needs to be inflated by the
    // largest clearance value found in the netclasses and rules
    int biggest_clearance = std::max( zone_clearance, bds.GetBiggestClearanceValue() );
    zone_boundingbox.Inflate( biggest_clearance + extra_margin );

    // Use a dummy pad to calculate hole clearance when a pad has a hole but is not on the
    // zone's copper layer.  The dummy pad has the size and shape of the original pad's hole.
    // We have to give it a parent because some functions expect a non-null parent to find
    // clearance data, etc.
    MODULE  dummymodule( m_board );
    D_PAD   dummypad( &dummymodule );

    auto evalRulesForItems =
            [&]( DRC_CONSTRAINT_TYPE_T aConstraint, const BOARD_ITEM* a, const BOARD_ITEM* b,
                 PCB_LAYER_ID aLayer ) -> int
            {
                DRC_CONSTRAINT c = bds.m_DRCEngine->EvalRulesForItems( aConstraint, a, b, aLayer );
                return c.Value().HasMin() ? c.Value().Min() : 0;
            };

    // Add non-connected pad clearances
    //
    for( MODULE* module : m_board->Modules() )
    {
        for( D_PAD* pad : module->Pads() )
        {
            if( !pad->FlashLayer( aLayer ) )
            {
                if( pad->GetDrillSize().x == 0 && pad->GetDrillSize().y == 0 )
                    continue;

                setupDummyPadForHole( pad, dummypad );
                pad = &dummypad;
            }

            if( pad->GetNetCode() != aZone->GetNetCode() || pad->GetNetCode() <= 0
                    || aZone->GetPadConnection( pad ) == ZONE_CONNECTION::NONE )
            {
                if( pad->GetBoundingBox().Intersects( zone_boundingbox ) )
                {
                    int gap;

                    // for pads having the same netcode as the zone, the net clearance has no
                    // meaning so use the greater of the zone clearance and the thermal relief
                    if( pad->GetNetCode() > 0 && pad->GetNetCode() == aZone->GetNetCode() )
                    {
                        gap = std::max( zone_clearance, aZone->GetThermalReliefGap( pad ) );
                    }
                    else
                    {
                        gap = evalRulesForItems( DRC_CONSTRAINT_TYPE_CLEARANCE, aZone, pad,
                                                 aLayer );
                    }

                    addKnockout( pad, aLayer, gap, aHoles );
                }
            }
        }
    }

    // Add non-connected track clearances
    //
    for( TRACK* track : m_board->Tracks() )
    {
        if( !track->IsOnLayer( aLayer ) )
            continue;

        if( track->GetNetCode() == aZone->GetNetCode()  && ( aZone->GetNetCode() != 0) )
            continue;

        if( track->GetBoundingBox().Intersects( zone_boundingbox ) )
        {
            int gap = evalRulesForItems( DRC_CONSTRAINT_TYPE_CLEARANCE, aZone, track, aLayer );

            gap += extra_margin;

            if( track->Type() == PCB_VIA_T )
            {
                VIA* via = static_cast<VIA*>( track );

                if( !via->FlashLayer( aLayer ) )
                {
                    int radius = via->GetDrillValue() / 2 + bds.GetHolePlatingThickness() + gap;
                    TransformCircleToPolygon( aHoles, via->GetPosition(), radius, m_maxError );
                }
                else
                {
                    via->TransformShapeWithClearanceToPolygon( aHoles, aLayer, gap, m_maxError );
                }
            }
            else
            {
                track->TransformShapeWithClearanceToPolygon( aHoles, aLayer, gap, m_maxError );
            }
        }
    }

    // Add graphic item clearances.  They are by definition unconnected, and have no clearance
    // definitions of their own.
    //
    auto doGraphicItem =
            [&]( BOARD_ITEM* aItem )
            {
                // A item on the Edge_Cuts is always seen as on any layer:
                if( !aItem->IsOnLayer( aLayer ) && !aItem->IsOnLayer( Edge_Cuts ) )
                    return;

                if( aItem->GetBoundingBox().Intersects( zone_boundingbox ) )
                {
                    int gap = evalRulesForItems( DRC_CONSTRAINT_TYPE_CLEARANCE, aZone, aItem,
                                                 aLayer );

                    if( aItem->IsOnLayer( Edge_Cuts ) )
                    {
                        gap = std::max( gap, evalRulesForItems( DRC_CONSTRAINT_TYPE_EDGE_CLEARANCE,
                                                                aZone, aItem, Edge_Cuts ) );
                    }

                    addKnockout( aItem, aLayer, gap, aItem->IsOnLayer( Edge_Cuts ), aHoles );
                }
            };

    for( MODULE* module : m_board->Modules() )
    {
        doGraphicItem( &module->Reference() );
        doGraphicItem( &module->Value() );

        for( BOARD_ITEM* item : module->GraphicalItems() )
            doGraphicItem( item );
    }

    for( BOARD_ITEM* item : m_board->Drawings() )
        doGraphicItem( item );

    // Add keepout zones and higher-priority zones
    //
    auto knockoutZone =
            [&]( ZONE_CONTAINER* aKnockout )
            {
                // If the zones share no common layers
                if( !aKnockout->GetLayerSet().test( aLayer ) )
                    return;

                if( aKnockout->GetBoundingBox().Intersects( zone_boundingbox ) )
                {
                    if( aKnockout->GetIsRuleArea()
                        || aZone->GetNetCode() == aKnockout->GetNetCode() )
                    {
                        // Keepouts and same-net zones use outline with no clearance
                        aKnockout->TransformSmoothedOutlineToPolygon( aHoles, 0, nullptr );
                    }
                    else
                    {
                        int gap = evalRulesForItems( DRC_CONSTRAINT_TYPE_CLEARANCE, aZone,
                                                     aKnockout, aLayer );

                        if( bds.m_ZoneFillVersion == 5 )
                        {
                            // 5.x used outline with clearance
                            aKnockout->TransformSmoothedOutlineToPolygon( aHoles, gap, nullptr );
                        }
                        else
                        {
                            // 6.0 uses filled areas with clearance
                            SHAPE_POLY_SET poly;
                            aKnockout->TransformShapeWithClearanceToPolygon( poly, aLayer, gap );
                            aHoles.Append( poly );
                        }
                    }
                }
            };

    for( ZONE_CONTAINER* otherZone : m_board->Zones() )
    {
        if( otherZone == aZone )
            continue;

        if( otherZone->GetIsRuleArea() )
        {
            if( otherZone->GetDoNotAllowCopperPour() )
                knockoutZone( otherZone );
        }
        else
        {
            if( otherZone->GetPriority() > aZone->GetPriority() )
                knockoutZone( otherZone );
        }
    }

    for( MODULE* module : m_board->Modules() )
    {
        for( ZONE_CONTAINER* otherZone : module->Zones() )
        {
            if( otherZone->GetIsRuleArea() )
            {
                if( otherZone->GetDoNotAllowCopperPour() )
                    knockoutZone( otherZone );
            }
            else
            {
                if( otherZone->GetPriority() > aZone->GetPriority() )
                    knockoutZone( otherZone );
            }
        }
    }

    aHoles.Simplify( SHAPE_POLY_SET::PM_FAST );
}


#define DUMP_POLYS_TO_COPPER_LAYER( a, b, c ) \
    { if( m_debugZoneFiller && dumpLayer == b ) \
        { \
            m_board->SetLayerName( b, c ); \
            a.Simplify( SHAPE_POLY_SET::PM_STRICTLY_SIMPLE ); \
            a.Fracture( SHAPE_POLY_SET::PM_STRICTLY_SIMPLE ); \
            aRawPolys = a; \
            aFinalPolys = a; \
            return; \
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
void ZONE_FILLER::computeRawFilledArea( const ZONE_CONTAINER* aZone, PCB_LAYER_ID aLayer,
                                        const SHAPE_POLY_SET& aSmoothedOutline,
                                        SHAPE_POLY_SET& aRawPolys,
                                        SHAPE_POLY_SET& aFinalPolys )
{
    PCB_LAYER_ID dumpLayer = aLayer;

    if( m_debugZoneFiller && LSET::InternalCuMask().Contains( aLayer ) )
        aLayer = F_Cu;

    m_maxError = m_board->GetDesignSettings().m_MaxError;

    // Features which are min_width should survive pruning; features that are *less* than
    // min_width should not.  Therefore we subtract epsilon from the min_width when
    // deflating/inflating.
    int half_min_width = aZone->GetMinThickness() / 2;
    int epsilon = Millimeter2iu( 0.001 );
    int numSegs = GetArcToSegmentCount( half_min_width, m_maxError, 360.0 );

    // Solid polygons are deflated and inflated during calculations.  Deflating doesn't cause
    // issues, but inflate is tricky as it can create excessively long and narrow spikes for
    // acute angles.
    // ALLOW_ACUTE_CORNERS cannot be used due to the spike problem.
    // CHAMFER_ACUTE_CORNERS is tempting, but can still produce spikes in some unusual
    // circumstances (https://gitlab.com/kicad/code/kicad/-/issues/5581).
    // It's unclear if ROUND_ACUTE_CORNERS would have the same issues, but is currently avoided
    // as a "less-safe" option.
    // ROUND_ALL_CORNERS produces the uniformly nicest shapes, but also a lot of segments.
    // CHAMFER_ALL_CORNERS improves the segement count.
    SHAPE_POLY_SET::CORNER_STRATEGY fastCornerStrategy = SHAPE_POLY_SET::CHAMFER_ALL_CORNERS;
    SHAPE_POLY_SET::CORNER_STRATEGY cornerStrategy = SHAPE_POLY_SET::ROUND_ALL_CORNERS;

    std::deque<SHAPE_LINE_CHAIN> thermalSpokes;
    SHAPE_POLY_SET clearanceHoles;

    aRawPolys = aSmoothedOutline;
    DUMP_POLYS_TO_COPPER_LAYER( aRawPolys, In1_Cu, "smoothed-outline" );

    if( m_progressReporter && m_progressReporter->IsCancelled() )
        return;

    knockoutThermalReliefs( aZone, aLayer, aRawPolys );
    DUMP_POLYS_TO_COPPER_LAYER( aRawPolys, In2_Cu, "minus-thermal-reliefs" );

    if( m_progressReporter && m_progressReporter->IsCancelled() )
        return;

    buildCopperItemClearances( aZone, aLayer, clearanceHoles );

    if( m_progressReporter && m_progressReporter->IsCancelled() )
        return;

    buildThermalSpokes( aZone, aLayer, thermalSpokes );

    if( m_progressReporter && m_progressReporter->IsCancelled() )
        return;

    // Create a temporary zone that we can hit-test spoke-ends against.  It's only temporary
    // because the "real" subtract-clearance-holes has to be done after the spokes are added.
    static const bool USE_BBOX_CACHES = true;
    SHAPE_POLY_SET testAreas = aRawPolys;
    testAreas.BooleanSubtract( clearanceHoles, SHAPE_POLY_SET::PM_FAST );
    DUMP_POLYS_TO_COPPER_LAYER( testAreas, In3_Cu, "minus-clearance-holes" );

    // Prune features that don't meet minimum-width criteria
    if( half_min_width - epsilon > epsilon )
    {
        testAreas.Deflate( half_min_width - epsilon, numSegs, fastCornerStrategy );
        DUMP_POLYS_TO_COPPER_LAYER( testAreas, In4_Cu, "spoke-test-deflated" );

        testAreas.Inflate( half_min_width - epsilon, numSegs, fastCornerStrategy );
        DUMP_POLYS_TO_COPPER_LAYER( testAreas, In5_Cu, "spoke-test-reinflated" );
    }

    if( m_progressReporter && m_progressReporter->IsCancelled() )
        return;

    // Spoke-end-testing is hugely expensive so we generate cached bounding-boxes to speed
    // things up a bit.
    testAreas.BuildBBoxCaches();
    int interval = 0;

    for( const SHAPE_LINE_CHAIN& spoke : thermalSpokes )
    {
        const VECTOR2I& testPt = spoke.CPoint( 3 );

        // Hit-test against zone body
        if( testAreas.Contains( testPt, -1, 1, USE_BBOX_CACHES ) )
        {
            aRawPolys.AddOutline( spoke );
            continue;
        }

        if( interval++ > 400 )
        {
            if( m_progressReporter && m_progressReporter->IsCancelled() )
                return;

            interval = 0;
        }

        // Hit-test against other spokes
        for( const SHAPE_LINE_CHAIN& other : thermalSpokes )
        {
            if( &other != &spoke && other.PointInside( testPt, 1, USE_BBOX_CACHES  ) )
            {
                aRawPolys.AddOutline( spoke );
                break;
            }
        }
    }

    DUMP_POLYS_TO_COPPER_LAYER( aRawPolys, In6_Cu, "plus-spokes" );

    if( m_progressReporter && m_progressReporter->IsCancelled() )
        return;

    aRawPolys.BooleanSubtract( clearanceHoles, SHAPE_POLY_SET::PM_FAST );
    DUMP_POLYS_TO_COPPER_LAYER( aRawPolys, In7_Cu, "trimmed-spokes" );

    // Prune features that don't meet minimum-width criteria
    if( half_min_width - epsilon > epsilon )
        aRawPolys.Deflate( half_min_width - epsilon, numSegs, cornerStrategy );

    DUMP_POLYS_TO_COPPER_LAYER( aRawPolys, In8_Cu, "deflated" );

    if( m_progressReporter && m_progressReporter->IsCancelled() )
        return;

    // Now remove the non filled areas due to the hatch pattern
    if( aZone->GetFillMode() == ZONE_FILL_MODE::HATCH_PATTERN )
        addHatchFillTypeOnZone( aZone, aLayer, aRawPolys );

    DUMP_POLYS_TO_COPPER_LAYER( aRawPolys, In9_Cu, "after-hatching" );

    if( m_progressReporter && m_progressReporter->IsCancelled() )
        return;

    // Re-inflate after pruning of areas that don't meet minimum-width criteria
    if( aZone->GetFilledPolysUseThickness() )
    {
        // If we're stroking the zone with a min_width stroke then this will naturally inflate
        // the zone by half_min_width
    }
    else if( half_min_width - epsilon > epsilon )
    {
        aRawPolys.Inflate( half_min_width - epsilon, numSegs, cornerStrategy );
    }

    DUMP_POLYS_TO_COPPER_LAYER( aRawPolys, In10_Cu, "after-reinflating" );

    // Ensure additive changes (thermal stubs and particularly inflating acute corners) do not
    // add copper outside the zone boundary or inside the clearance holes
    aRawPolys.BooleanIntersection( aSmoothedOutline, SHAPE_POLY_SET::PM_FAST );
    aRawPolys.BooleanSubtract( clearanceHoles, SHAPE_POLY_SET::PM_FAST );

    aRawPolys.Fracture( SHAPE_POLY_SET::PM_FAST );

    aFinalPolys = aRawPolys;
}


/*
 * Build the filled solid areas data from real outlines (stored in m_Poly)
 * The solid areas can be more than one on copper layers, and do not have holes
 * ( holes are linked by overlapping segments to the main outline)
 */
bool ZONE_FILLER::fillSingleZone( ZONE_CONTAINER* aZone, PCB_LAYER_ID aLayer,
                                  SHAPE_POLY_SET& aRawPolys, SHAPE_POLY_SET& aFinalPolys )
{
    SHAPE_POLY_SET* boardOutline = m_brdOutlinesValid ? &m_boardOutline : nullptr;
    SHAPE_POLY_SET  smoothedPoly;

    /*
     * convert outlines + holes to outlines without holes (adding extra segments if necessary)
     * m_Poly data is expected normalized, i.e. NormalizeAreaOutlines was used after building
     * this zone
     */
    if ( !aZone->BuildSmoothedPoly( smoothedPoly, aLayer, boardOutline ) )
        return false;

    if( m_progressReporter && m_progressReporter->IsCancelled() )
        return false;

    if( aZone->IsOnCopperLayer() )
    {
        computeRawFilledArea( aZone, aLayer, smoothedPoly, aRawPolys, aFinalPolys );
    }
    else
    {
        // Features which are min_width should survive pruning; features that are *less* than
        // min_width should not.  Therefore we subtract epsilon from the min_width when
        // deflating/inflating.
        int half_min_width = aZone->GetMinThickness() / 2;
        int epsilon = Millimeter2iu( 0.001 );
        int numSegs = GetArcToSegmentCount( half_min_width, m_maxError, 360.0 );

        smoothedPoly.Deflate( half_min_width - epsilon, numSegs );

        // Remove the non filled areas due to the hatch pattern
        if( aZone->GetFillMode() == ZONE_FILL_MODE::HATCH_PATTERN )
            addHatchFillTypeOnZone( aZone, aLayer, smoothedPoly );

        // Re-inflate after pruning of areas that don't meet minimum-width criteria
        if( aZone->GetFilledPolysUseThickness() )
        {
            // If we're stroking the zone with a min_width stroke then this will naturally
            // inflate the zone by half_min_width
        }
        else if( half_min_width - epsilon > epsilon )
            smoothedPoly.Deflate( -( half_min_width - epsilon ), numSegs );

        aRawPolys = smoothedPoly;
        aFinalPolys = smoothedPoly;

        aFinalPolys.Fracture( SHAPE_POLY_SET::PM_STRICTLY_SIMPLE );
    }

    aZone->SetNeedRefill( false );
    return true;
}


/**
 * Function buildThermalSpokes
 */
void ZONE_FILLER::buildThermalSpokes( const ZONE_CONTAINER* aZone, PCB_LAYER_ID aLayer,
                                      std::deque<SHAPE_LINE_CHAIN>& aSpokesList )
{
    auto zoneBB = aZone->GetCachedBoundingBox();
    int  zone_clearance = aZone->GetLocalClearance();
    int  biggest_clearance = m_board->GetDesignSettings().GetBiggestClearanceValue();
    biggest_clearance = std::max( biggest_clearance, zone_clearance );
    zoneBB.Inflate( biggest_clearance );

    // Is a point on the boundary of the polygon inside or outside?  This small epsilon lets
    // us avoid the question.
    int epsilon = KiROUND( IU_PER_MM * 0.04 );  // about 1.5 mil

    for( auto module : m_board->Modules() )
    {
        for( auto pad : module->Pads() )
        {
            if( !hasThermalConnection( pad, aZone ) )
                continue;

            // We currently only connect to pads, not pad holes
            if( !pad->IsOnLayer( aLayer ) )
                continue;

            int thermalReliefGap = aZone->GetThermalReliefGap( pad );

            // Calculate thermal bridge half width
            int spoke_w = aZone->GetThermalReliefSpokeWidth( pad );
            // Avoid spoke_w bigger than the smaller pad size, because
            // it is not possible to create stubs bigger than the pad.
            // Possible refinement: have a separate size for vertical and horizontal stubs
            spoke_w = std::min( spoke_w, pad->GetSize().x );
            spoke_w = std::min( spoke_w, pad->GetSize().y );

            // Cannot create stubs having a width < zone min thickness
            if( spoke_w < aZone->GetMinThickness() )
                continue;

            int spoke_half_w = spoke_w / 2;

            // Quick test here to possibly save us some work
            BOX2I itemBB = pad->GetBoundingBox();
            itemBB.Inflate( thermalReliefGap + epsilon );

            if( !( itemBB.Intersects( zoneBB ) ) )
                continue;

            // Thermal spokes consist of segments from the pad center to points just outside
            // the thermal relief.
            //
            // We use the bounding-box to lay out the spokes, but for this to work the
            // bounding box has to be built at the same rotation as the spokes.
            // We have to use a dummy pad to avoid dirtying the cached shapes
            wxPoint shapePos = pad->ShapePos();
            double  padAngle = pad->GetOrientation();
            D_PAD   dummy_pad( *pad );
            dummy_pad.SetOrientation( 0.0 );
            dummy_pad.SetPosition( { 0, 0 } );

            BOX2I reliefBB = dummy_pad.GetBoundingBox();
            reliefBB.Inflate( thermalReliefGap + epsilon );

            // For circle pads, the thermal spoke orientation is 45 deg
            if( pad->GetShape() == PAD_SHAPE_CIRCLE )
                padAngle = s_RoundPadThermalSpokeAngle;

            for( int i = 0; i < 4; i++ )
            {
                SHAPE_LINE_CHAIN spoke;
                switch( i )
                {
                case 0:       // lower stub
                    spoke.Append( +spoke_half_w,       -spoke_half_w );
                    spoke.Append( -spoke_half_w,       -spoke_half_w );
                    spoke.Append( -spoke_half_w,       reliefBB.GetBottom() );
                    spoke.Append( 0,                   reliefBB.GetBottom() );  // test pt
                    spoke.Append( +spoke_half_w,       reliefBB.GetBottom() );
                    break;

                case 1:       // upper stub
                    spoke.Append( +spoke_half_w,       spoke_half_w );
                    spoke.Append( -spoke_half_w,       spoke_half_w );
                    spoke.Append( -spoke_half_w,       reliefBB.GetTop() );
                    spoke.Append( 0,                   reliefBB.GetTop() );     // test pt
                    spoke.Append( +spoke_half_w,       reliefBB.GetTop() );
                    break;

                case 2:       // right stub
                    spoke.Append( -spoke_half_w,       spoke_half_w );
                    spoke.Append( -spoke_half_w,       -spoke_half_w );
                    spoke.Append( reliefBB.GetRight(), -spoke_half_w );
                    spoke.Append( reliefBB.GetRight(), 0 );                     // test pt
                    spoke.Append( reliefBB.GetRight(), spoke_half_w );
                    break;

                case 3:       // left stub
                    spoke.Append( spoke_half_w,        spoke_half_w );
                    spoke.Append( spoke_half_w,        -spoke_half_w );
                    spoke.Append( reliefBB.GetLeft(),  -spoke_half_w );
                    spoke.Append( reliefBB.GetLeft(),  0 );                     // test pt
                    spoke.Append( reliefBB.GetLeft(),  spoke_half_w );
                    break;
                }

                spoke.Rotate( -DECIDEG2RAD( padAngle ) );
                spoke.Move( shapePos );

                spoke.SetClosed( true );
                spoke.GenerateBBoxCache();
                aSpokesList.push_back( std::move( spoke ) );
            }
        }
    }
}


void ZONE_FILLER::addHatchFillTypeOnZone( const ZONE_CONTAINER* aZone, PCB_LAYER_ID aLayer,
                                          SHAPE_POLY_SET& aRawPolys )
{
    // Build grid:

    // obviously line thickness must be > zone min thickness.
    // It can happens if a board file was edited by hand by a python script
    // Use 1 micron margin to be *sure* there is no issue in Gerber files
    // (Gbr file unit = 1 or 10 nm) due to some truncation in coordinates or calculations
    // This margin also avoid problems due to rounding coordinates in next calculations
    // that can create incorrect polygons
    int thickness = std::max( aZone->GetHatchThickness(),
                              aZone->GetMinThickness() + Millimeter2iu( 0.001 ) );

    int linethickness = thickness - aZone->GetMinThickness();
    int gridsize = thickness + aZone->GetHatchGap();
    double orientation = aZone->GetHatchOrientation();

    SHAPE_POLY_SET filledPolys = aRawPolys;
    // Use a area that contains the rotated bbox by orientation,
    // and after rotate the result by -orientation.
    if( orientation != 0.0 )
        filledPolys.Rotate( M_PI/180.0 * orientation, VECTOR2I( 0,0 ) );

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
        // make smoothing only for reasonnable smooth values, to avoid a lot of useless segments
        // and if the smooth value is small, use chamfer even if fillet is requested
        #define SMOOTH_MIN_VAL_MM 0.02
        #define SMOOTH_SMALL_VAL_MM 0.04

        if( smooth_value > Millimeter2iu( SMOOTH_MIN_VAL_MM ) )
        {
            SHAPE_POLY_SET smooth_hole;
            smooth_hole.AddOutline( hole_base );
            int smooth_level = aZone->GetHatchSmoothingLevel();

            if( smooth_value < Millimeter2iu( SMOOTH_SMALL_VAL_MM ) && smooth_level > 1 )
                smooth_level = 1;

            // Use a larger smooth_value to compensate the outline tickness
            // (chamfer is not visible is smooth value < outline thickess)
            smooth_value += aZone->GetMinThickness() / 2;

            // smooth_value cannot be bigger than the half size oh the hole:
            smooth_value = std::min( smooth_value, aZone->GetHatchGap() / 2 );

            // the error to approximate a circle by segments when smoothing corners by a arc
            int error_max = std::max( Millimeter2iu( 0.01 ), smooth_value / 20 );

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

    // We must buffer holes by at least aZone->GetMinThickness() to guarantee that thermal
    // reliefs can be built (and to give the zone a solid outline).  However, it looks more
    // visually consistent if the buffer width is the same as the hatch width.
    int outline_margin = KiROUND( aZone->GetMinThickness() * 1.1 );

    if( aZone->GetHatchBorderAlgorithm() )
        outline_margin = std::max( outline_margin, aZone->GetHatchThickness() );

    if( outline_margin > linethickness / 2 )
        filledPolys.Deflate( outline_margin - linethickness / 2, 16 );

    holes.BooleanIntersection( filledPolys, SHAPE_POLY_SET::PM_FAST );

    if( orientation != 0.0 )
        holes.Rotate( -M_PI/180.0 * orientation, VECTOR2I( 0,0 ) );

    if( aZone->GetNetCode() != 0 )
    {
        // Vias and pads connected to the zone must not be allowed to become isolated inside
        // one of the holes.  Effectively this means their copper outline needs to be expanded
        // to be at least as wide as the gap so that it is guaranteed to touch at least one
        // edge.
        EDA_RECT       zone_boundingbox = aZone->GetCachedBoundingBox();
        SHAPE_POLY_SET aprons;
        int            min_apron_radius = ( aZone->GetHatchGap() * 10 ) / 19;

        for( TRACK* track : m_board->Tracks() )
        {
            if( track->Type() == PCB_VIA_T )
            {
                VIA* via = static_cast<VIA*>( track );

                if( via->GetNetCode() == aZone->GetNetCode()
                    && via->IsOnLayer( aLayer )
                    && via->GetBoundingBox().Intersects( zone_boundingbox ) )
                {
                    int r = std::max( min_apron_radius,
                                      via->GetDrillValue() / 2 + outline_margin );

                    TransformCircleToPolygon( aprons, via->GetPosition(), r, ARC_HIGH_DEF );
                }
            }
        }

        for( MODULE* module : m_board->Modules() )
        {
            for( D_PAD* pad : module->Pads() )
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
                    int min_annulus = ( pad_width - slot_width ) / 2;
                    int clearance = std::max( min_apron_radius - pad_width / 2,
                                              outline_margin - min_annulus );

                    clearance = std::max( 0, clearance - linethickness / 2 );
                    pad->TransformShapeWithClearanceToPolygon( aprons, aLayer, clearance,
                                                               ARC_HIGH_DEF );
                }
            }
        }

        holes.BooleanSubtract( aprons, SHAPE_POLY_SET::PM_FAST );
    }

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
    aRawPolys.BooleanSubtract( aRawPolys, holes, SHAPE_POLY_SET::PM_STRICTLY_SIMPLE );
}
