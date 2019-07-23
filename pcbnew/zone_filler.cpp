/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014-2017 CERN
 * Copyright (C) 2014-2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <cstdint>
#include <thread>
#include <mutex>
#include <algorithm>
#include <future>

#include <class_board.h>
#include <class_zone.h>
#include <class_module.h>
#include <class_edge_mod.h>
#include <class_drawsegment.h>
#include <class_track.h>
#include <class_pcb_text.h>
#include <class_pcb_target.h>

#include <connectivity/connectivity_data.h>
#include <board_commit.h>

#include <widgets/progress_reporter.h>

#include <geometry/shape_poly_set.h>
#include <geometry/shape_file_io.h>
#include <geometry/convex_hull.h>
#include <geometry/geometry_utils.h>
#include <confirm.h>

#include "zone_filler.h"

#include <advanced_config.h>        // To be removed later, when the zone fill option will be always allowed

class PROGRESS_REPORTER_HIDER
{
public:
    PROGRESS_REPORTER_HIDER( WX_PROGRESS_REPORTER* aReporter )
    {
        m_reporter = aReporter;

        if( aReporter )
            aReporter->Hide();
    }

    ~PROGRESS_REPORTER_HIDER()
    {
        if( m_reporter )
            m_reporter->Show();
    }

private:
    WX_PROGRESS_REPORTER* m_reporter;
};


static const double s_RoundPadThermalSpokeAngle = 450;
static const bool s_DumpZonesWhenFilling = false;


ZONE_FILLER::ZONE_FILLER(  BOARD* aBoard, COMMIT* aCommit ) :
    m_board( aBoard ), m_brdOutlinesValid( false ), m_commit( aCommit ),
    m_progressReporter( nullptr )
{
}


ZONE_FILLER::~ZONE_FILLER()
{
}


void ZONE_FILLER::InstallNewProgressReporter( wxWindow* aParent, const wxString& aTitle,
                                              int aNumPhases )
{
    m_uniqueReporter = std::make_unique<WX_PROGRESS_REPORTER>( aParent, aTitle, aNumPhases );
    m_progressReporter = m_uniqueReporter.get();
}


bool ZONE_FILLER::Fill( const std::vector<ZONE_CONTAINER*>& aZones, bool aCheck )
{
    std::vector<CN_ZONE_ISOLATED_ISLAND_LIST> toFill;
    auto connectivity = m_board->GetConnectivity();
    bool filledPolyWithOutline = not m_board->GetDesignSettings().m_ZoneUseNoOutlineInFill;

    if( ADVANCED_CFG::GetCfg().m_forceThickOutlinesInZones )
        filledPolyWithOutline = true;

    std::unique_lock<std::mutex> lock( connectivity->GetLock(), std::try_to_lock );

    if( !lock )
        return false;

    if( m_progressReporter )
    {
        m_progressReporter->Report( aCheck ? _( "Checking zone fills..." ) : _( "Building zone fills..." ) );
        m_progressReporter->SetMaxProgress( toFill.size() );
    }

    // The board outlines is used to clip solid areas inside the board (when outlines are valid)
    m_boardOutline.RemoveAllContours();
    m_brdOutlinesValid = m_board->GetBoardPolygonOutlines( m_boardOutline );

    for( auto zone : aZones )
    {
        // Keepout zones are not filled
        if( zone->GetIsKeepout() )
            continue;

        if( m_commit )
            m_commit->Modify( zone );

        // calculate the hash value for filled areas. it will be used later
        // to know if the current filled areas are up to date
        zone->BuildHashValue();

        // Add the zone to the list of zones to test or refill
        toFill.emplace_back( CN_ZONE_ISOLATED_ISLAND_LIST(zone) );

        // Remove existing fill first to prevent drawing invalid polygons
        // on some platforms
        zone->UnFill();
    }

    std::atomic<size_t> nextItem( 0 );
    size_t              parallelThreadCount =
            std::min<size_t>( std::thread::hardware_concurrency(), aZones.size() );
    std::vector<std::future<size_t>> returns( parallelThreadCount );

    auto fill_lambda = [&] ( PROGRESS_REPORTER* aReporter ) -> size_t
    {
        size_t num = 0;

        for( size_t i = nextItem++; i < toFill.size(); i = nextItem++ )
        {
            ZONE_CONTAINER* zone = toFill[i].m_zone;
            zone->SetFilledPolysUseThickness( filledPolyWithOutline );
            SHAPE_POLY_SET rawPolys, finalPolys;
            fillSingleZone( zone, rawPolys, finalPolys );

            zone->SetRawPolysList( rawPolys );
            zone->SetFilledPolysList( finalPolys );
            zone->SetIsFilled( true );

            if( m_progressReporter )
                m_progressReporter->AdvanceProgress();

            num++;
        }

        return num;
    };

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

    // Now update the connectivity to check for copper islands
    if( m_progressReporter )
    {
        m_progressReporter->AdvancePhase();
        m_progressReporter->Report( _( "Removing insulated copper islands..." ) );
        m_progressReporter->KeepRefreshing();
    }

    connectivity->SetProgressReporter( m_progressReporter );
    connectivity->FindIsolatedCopperIslands( toFill );

    // Now remove insulated copper islands and islands outside the board edge
    bool outOfDate = false;

    for( auto& zone : toFill )
    {
        std::sort( zone.m_islands.begin(), zone.m_islands.end(), std::greater<int>() );
        SHAPE_POLY_SET poly = zone.m_zone->GetFilledPolysList();

        // Remove solid areas outside the board cutouts and the insulated islands
        // only zones with net code > 0 can have insulated islands by definition
        if( zone.m_zone->GetNetCode() > 0 )
        {
            // solid areas outside the board cutouts are also removed, because they are usually
            // insulated islands
            for( auto idx : zone.m_islands )
            {
                poly.DeletePolygon( idx );
            }
        }
        // Zones with no net can have areas outside the board cutouts.
        // By definition, Zones with no net have no isolated island
        // (in fact all filled areas are isolated islands)
        // but they can have some areas outside the board cutouts.
        // A filled area outside the board cutouts has all points outside cutouts,
        // so we only need to check one point for each filled polygon.
        // Note also non copper zones are already clipped
        else if( m_brdOutlinesValid && zone.m_zone->IsOnCopperLayer() )
        {
            for( int idx = 0; idx < poly.OutlineCount(); )
            {
                if( poly.Polygon( idx ).empty() ||
                    !m_boardOutline.Contains( poly.Polygon( idx ).front().CPoint( 0 ) ) )
                {
                    poly.DeletePolygon( idx );
                }
                else
                     idx++;
            }
        }

        zone.m_zone->SetFilledPolysList( poly );

        if( aCheck && zone.m_zone->GetHashValue() != poly.GetHash() )
            outOfDate = true;
    }

    if( aCheck && outOfDate )
    {
        PROGRESS_REPORTER_HIDER raii( m_progressReporter );
        KIDIALOG dlg( m_progressReporter->GetParent(),
                      _( "Zone fills are out-of-date. Refill?" ),
                      _( "Confirmation" ), wxOK | wxCANCEL | wxICON_WARNING );
        dlg.SetOKCancelLabels( _( "Refill" ), _( "Continue without Refill" ) );
        dlg.DoNotShowCheckbox( __FILE__, __LINE__ );

        if( dlg.ShowModal() == wxID_CANCEL )
        {
            if( m_commit )
                m_commit->Revert();

            connectivity->SetProgressReporter( nullptr );
            return false;
        }
    }

    if( m_progressReporter )
    {
        m_progressReporter->AdvancePhase();
        m_progressReporter->Report( _( "Performing polygon fills..." ) );
        m_progressReporter->SetMaxProgress( toFill.size() );
    }


    nextItem = 0;

    auto tri_lambda = [&] ( PROGRESS_REPORTER* aReporter ) -> size_t
    {
        size_t num = 0;

        for( size_t i = nextItem++; i < toFill.size(); i = nextItem++ )
        {
            toFill[i].m_zone->CacheTriangulation();
            num++;

            if( m_progressReporter )
                m_progressReporter->AdvanceProgress();
        }

        return num;
    };

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
                    m_progressReporter->KeepRefreshing();

                status = returns[ii].wait_for( std::chrono::milliseconds( 100 ) );
            } while( status != std::future_status::ready );
        }
    }

    if( m_progressReporter )
    {
        m_progressReporter->AdvancePhase();
        m_progressReporter->Report( _( "Committing changes..." ) );
        m_progressReporter->KeepRefreshing();
    }

    connectivity->SetProgressReporter( nullptr );

    if( m_commit )
    {
        m_commit->Push( _( "Fill Zone(s)" ), false );
    }
    else
    {
        for( auto& i : toFill )
            connectivity->Update( i.m_zone );

        connectivity->RecalculateRatsnest();
    }

    return true;
}


/**
 * Return true if the given pad has a thermal connection with the given zone.
 */
bool hasThermalConnection( D_PAD* pad, const ZONE_CONTAINER* aZone )
{
    // Rejects non-standard pads with tht-only thermal reliefs
    if( aZone->GetPadConnection( pad ) == PAD_ZONE_CONN_THT_THERMAL
        && pad->GetAttribute() != PAD_ATTRIB_STANDARD )
    {
        return false;
    }

    if( aZone->GetPadConnection( pad ) != PAD_ZONE_CONN_THERMAL
        && aZone->GetPadConnection( pad ) != PAD_ZONE_CONN_THT_THERMAL )
    {
        return false;
    }

    if( pad->GetNetCode() != aZone->GetNetCode() || pad->GetNetCode() <= 0 )
        return false;

    EDA_RECT item_boundingbox = pad->GetBoundingBox();
    int thermalGap = aZone->GetThermalReliefGap( pad );
    item_boundingbox.Inflate( thermalGap, thermalGap );

    return item_boundingbox.Intersects( aZone->GetBoundingBox() );
}


/**
 * Setup aDummyPad to have the same size and shape of aPad's hole.  This allows us to create
 * thermal reliefs and clearances for holes using the pad code.
 */
static void setupDummyPadForHole( const D_PAD* aPad, D_PAD& aDummyPad )
{
    aDummyPad.SetNetCode( aPad->GetNetCode() );
    aDummyPad.SetSize( aPad->GetDrillSize() );
    aDummyPad.SetOrientation( aPad->GetOrientation() );
    aDummyPad.SetShape( aPad->GetDrillShape() == PAD_DRILL_SHAPE_OBLONG ? PAD_SHAPE_OVAL
                                                                        : PAD_SHAPE_CIRCLE );
    aDummyPad.SetPosition( aPad->GetPosition() );
}


/**
 * Add a knockout for a pad.  The knockout is 'aGap' larger than the pad (which might be
 * either the thermal clearance or the electrical clearance).
 */
void ZONE_FILLER::addKnockout( D_PAD* aPad, int aGap, SHAPE_POLY_SET& aHoles )
{
    if( aPad->GetShape() == PAD_SHAPE_CUSTOM )
    {
        // the pad shape in zone can be its convex hull or the shape itself
        SHAPE_POLY_SET outline( aPad->GetCustomShapeAsPolygon() );
        int numSegs = std::max( GetArcToSegmentCount( aGap, m_high_def, 360.0 ), 6 );
        double correction = GetCircletoPolyCorrectionFactor( numSegs );
        outline.Inflate( KiROUND( aGap * correction ), numSegs );
        aPad->CustomShapeAsPolygonToBoardPosition( &outline, aPad->GetPosition(),
                                                   aPad->GetOrientation() );

        if( aPad->GetCustomShapeInZoneOpt() == CUST_PAD_SHAPE_IN_ZONE_CONVEXHULL )
        {
            std::vector<wxPoint> convex_hull;
            BuildConvexHull( convex_hull, outline );

            aHoles.NewOutline();

            for( const wxPoint& pt : convex_hull )
                aHoles.Append( pt );
        }
        else
            aHoles.Append( outline );
    }
    else
    {
        // Optimizing polygon vertex count: the high definition is used for round
        // and oval pads (pads with large arcs) but low def for other shapes (with
        // small arcs)
        if( aPad->GetShape() == PAD_SHAPE_CIRCLE || aPad->GetShape() == PAD_SHAPE_OVAL ||
          ( aPad->GetShape() == PAD_SHAPE_ROUNDRECT && aPad->GetRoundRectRadiusRatio() > 0.4 ) )
            aPad->TransformShapeWithClearanceToPolygon( aHoles, aGap, m_high_def );
        else
            aPad->TransformShapeWithClearanceToPolygon( aHoles, aGap, m_low_def );
    }
}


/**
 * Add a knockout for a graphic item.  The knockout is 'aGap' larger than the item (which
 * might be either the electrical clearance or the board edge clearance).
 */
void ZONE_FILLER::addKnockout( BOARD_ITEM* aItem, int aGap, bool aIgnoreLineWidth,
                               SHAPE_POLY_SET& aHoles )
{
    switch( aItem->Type() )
    {
    case PCB_LINE_T:
    {
        DRAWSEGMENT* seg = (DRAWSEGMENT*) aItem;
        seg->TransformShapeWithClearanceToPolygon( aHoles, aGap, m_high_def, aIgnoreLineWidth );
        break;
    }
    case PCB_TEXT_T:
    {
        TEXTE_PCB* text = (TEXTE_PCB*) aItem;
        text->TransformBoundingBoxWithClearanceToPolygon( &aHoles, aGap );
        break;
    }
    case PCB_MODULE_EDGE_T:
    {
        EDGE_MODULE* edge = (EDGE_MODULE*) aItem;
        edge->TransformShapeWithClearanceToPolygon( aHoles, aGap, m_high_def, aIgnoreLineWidth );
        break;
    }
    case PCB_MODULE_TEXT_T:
    {
        TEXTE_MODULE* text = (TEXTE_MODULE*) aItem;

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
void ZONE_FILLER::knockoutThermalReliefs( const ZONE_CONTAINER* aZone, SHAPE_POLY_SET& aFill )
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
            if( !pad->IsOnLayer( aZone->GetLayer() ) )
            {
                if( pad->GetDrillSize().x == 0 && pad->GetDrillSize().y == 0 )
                    continue;

                setupDummyPadForHole( pad, dummypad );
                pad = &dummypad;
            }

            addKnockout( pad, aZone->GetThermalReliefGap( pad ), holes );
        }
    }

    holes.Simplify( SHAPE_POLY_SET::PM_FAST );
    aFill.BooleanSubtract( holes, SHAPE_POLY_SET::PM_FAST );
}


/**
 * Removes clearance from the shape for copper items which share the zone's layer but are
 * not connected to it.
 */
void ZONE_FILLER::buildCopperItemClearances( const ZONE_CONTAINER* aZone, SHAPE_POLY_SET& aHoles )
{
    int zone_clearance = aZone->GetClearance();
    int edgeClearance = m_board->GetDesignSettings().m_CopperEdgeClearance;
    int zone_to_edgecut_clearance = std::max( aZone->GetZoneClearance(), edgeClearance );

    // items outside the zone bounding box are skipped
    // the bounding box is the zone bounding box + the biggest clearance found in Netclass list
    EDA_RECT zone_boundingbox = aZone->GetBoundingBox();
    int biggest_clearance = m_board->GetDesignSettings().GetBiggestClearanceValue();
    biggest_clearance = std::max( biggest_clearance, zone_clearance );
    zone_boundingbox.Inflate( biggest_clearance );

    // Use a dummy pad to calculate hole clearance when a pad has a hole but is not on the
    // zone's copper layer.  The dummy pad has the size and shape of the original pad's hole.
    // We have to give it a parent because some functions expect a non-null parent to find
    // clearance data, etc.
    MODULE  dummymodule( m_board );
    D_PAD   dummypad( &dummymodule );

    // Add non-connected pad clearances
    //
    for( auto module : m_board->Modules() )
    {
        for( auto pad : module->Pads() )
        {
            if( !pad->IsOnLayer( aZone->GetLayer() ) )
            {
                if( pad->GetDrillSize().x == 0 && pad->GetDrillSize().y == 0 )
                    continue;

                setupDummyPadForHole( pad, dummypad );
                pad = &dummypad;
            }

            if( pad->GetNetCode() != aZone->GetNetCode()
                  || pad->GetNetCode() <= 0
                  || aZone->GetPadConnection( pad ) == PAD_ZONE_CONN_NONE )
            {
                int gap = std::max( zone_clearance, pad->GetClearance() );
                EDA_RECT item_boundingbox = pad->GetBoundingBox();
                item_boundingbox.Inflate( pad->GetClearance() );

                if( item_boundingbox.Intersects( zone_boundingbox ) )
                    addKnockout( pad, gap, aHoles );
            }
        }
    }

    // Add non-connected track clearances
    //
    for( auto track : m_board->Tracks() )
    {
        if( !track->IsOnLayer( aZone->GetLayer() ) )
            continue;

        if( track->GetNetCode() == aZone->GetNetCode()  && ( aZone->GetNetCode() != 0) )
            continue;

        int gap = std::max( zone_clearance, track->GetClearance() );
        EDA_RECT item_boundingbox = track->GetBoundingBox();

        if( item_boundingbox.Intersects( zone_boundingbox ) )
            track->TransformShapeWithClearanceToPolygon( aHoles, gap, m_low_def );
    }

    // Add graphic item clearances.  They are by definition unconnected, and have no clearance
    // definitions of their own.
    //
    auto doGraphicItem = [&]( BOARD_ITEM* aItem )
    {
        // A item on the Edge_Cuts is always seen as on any layer:
        if( !aItem->IsOnLayer( aZone->GetLayer() ) && !aItem->IsOnLayer( Edge_Cuts ) )
            return;

        if( !aItem->GetBoundingBox().Intersects( zone_boundingbox ) )
            return;

        bool ignoreLineWidth = false;
        int gap = zone_clearance;

        if( aItem->IsOnLayer( Edge_Cuts ) )
        {
            gap = zone_to_edgecut_clearance;

            // edge cuts by definition don't have a width
            ignoreLineWidth = true;
        }

        addKnockout( aItem, gap, ignoreLineWidth, aHoles );
    };

    for( auto module : m_board->Modules() )
    {
        doGraphicItem( &module->Reference() );
        doGraphicItem( &module->Value() );

        for( auto item : module->GraphicalItems() )
            doGraphicItem( item );
    }

    for( auto item : m_board->Drawings() )
        doGraphicItem( item );

    // Add zones outlines having an higher priority and keepout
    //
    for( int ii = 0; ii < m_board->GetAreaCount(); ii++ )
    {
        ZONE_CONTAINER* zone = m_board->GetArea( ii );

        // If the zones share no common layers
        if( !aZone->CommonLayerExists( zone->GetLayerSet() ) )
            continue;

        if( !zone->GetIsKeepout() && zone->GetPriority() <= aZone->GetPriority() )
            continue;

        if( zone->GetIsKeepout() && !zone->GetDoNotAllowCopperPour() )
            continue;

        // A highter priority zone or keepout area is found: remove this area
        EDA_RECT item_boundingbox = zone->GetBoundingBox();

        if( !item_boundingbox.Intersects( zone_boundingbox ) )
            continue;

        // Add the zone outline area.  Don't use any clearance for keepouts, or for zones with
        // the same net (they will be connected but will honor their own clearance, thermal
        // connections, etc.).
        bool sameNet = aZone->GetNetCode() == zone->GetNetCode();
        bool useNetClearance = true;
        int  minClearance = zone_clearance;

        // The final clearance is obviously the max value of each zone clearance
        minClearance = std::max( minClearance, zone->GetClearance() );

        if( zone->GetIsKeepout() || sameNet )
        {
            minClearance = 0;
            useNetClearance = false;
        }

        zone->TransformOutlinesShapeWithClearanceToPolygon( aHoles, minClearance, useNetClearance );
    }

    aHoles.Simplify( SHAPE_POLY_SET::PM_FAST );
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
void ZONE_FILLER::computeRawFilledArea( const ZONE_CONTAINER* aZone,
                                        const SHAPE_POLY_SET& aSmoothedOutline,
                                        std::set<VECTOR2I>* aPreserveCorners,
                                        SHAPE_POLY_SET& aRawPolys,
                                        SHAPE_POLY_SET& aFinalPolys )
{
    m_high_def = m_board->GetDesignSettings().m_MaxError;
    m_low_def = std::min( ARC_LOW_DEF, int( m_high_def*1.5 ) );   // Reasonable value

    // Features which are min_width should survive pruning; features that are *less* than
    // min_width should not.  Therefore we subtract epsilon from the min_width when
    // deflating/inflating.
    int half_min_width = aZone->GetMinThickness() / 2;
    int epsilon = Millimeter2iu( 0.001 );
    int numSegs = std::max( GetArcToSegmentCount( half_min_width, m_high_def, 360.0 ), 6 );

    SHAPE_POLY_SET::CORNER_STRATEGY cornerStrategy = SHAPE_POLY_SET::CHOP_ACUTE_CORNERS;

    if( aZone->GetCornerSmoothingType() == ZONE_SETTINGS::SMOOTHING_FILLET )
        cornerStrategy = SHAPE_POLY_SET::ROUND_ACUTE_CORNERS;

    std::deque<SHAPE_LINE_CHAIN> thermalSpokes;
    SHAPE_POLY_SET clearanceHoles;

    std::unique_ptr<SHAPE_FILE_IO> dumper( new SHAPE_FILE_IO(
                    s_DumpZonesWhenFilling ? "zones_dump.txt" : "", SHAPE_FILE_IO::IOM_APPEND ) );

    aRawPolys = aSmoothedOutline;

    if( s_DumpZonesWhenFilling )
        dumper->BeginGroup( "clipper-zone" );

    knockoutThermalReliefs( aZone, aRawPolys );

    if( s_DumpZonesWhenFilling )
        dumper->Write( &aRawPolys, "solid-areas-minus-thermal-reliefs" );

    buildCopperItemClearances( aZone, clearanceHoles );

    if( s_DumpZonesWhenFilling )
        dumper->Write( &aRawPolys, "clearance holes" );

    buildThermalSpokes( aZone, thermalSpokes );

    // Create a temporary zone that we can hit-test spoke-ends against.  It's only temporary
    // because the "real" subtract-clearance-holes has to be done after the spokes are added.
    static const bool USE_BBOX_CACHES = true;
    SHAPE_POLY_SET testAreas = aRawPolys;
    testAreas.BooleanSubtract( clearanceHoles, SHAPE_POLY_SET::PM_FAST );

    // Prune features that don't meet minimum-width criteria
    if( half_min_width - epsilon > epsilon )
    {
        testAreas.Deflate( half_min_width - epsilon, numSegs, cornerStrategy );
        testAreas.Inflate( half_min_width - epsilon, numSegs, cornerStrategy );
    }

    // Spoke-end-testing is hugely expensive so we generate cached bounding-boxes to speed
    // things up a bit.
    testAreas.BuildBBoxCaches();

    for( const SHAPE_LINE_CHAIN& spoke : thermalSpokes )
    {
        const VECTOR2I& testPt = spoke.CPoint( 3 );

        // Hit-test against zone body
        if( testAreas.Contains( testPt, -1, 1, USE_BBOX_CACHES ) )
        {
            aRawPolys.AddOutline( spoke );
            continue;
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

    // Ensure previous changes (adding thermal stubs) do not add
    // filled areas outside the zone boundary
    aRawPolys.BooleanIntersection( aSmoothedOutline, SHAPE_POLY_SET::PM_FAST );
    aRawPolys.Simplify( SHAPE_POLY_SET::PM_FAST );

    if( s_DumpZonesWhenFilling )
        dumper->Write( &aRawPolys, "solid-areas-with-thermal-spokes" );

    aRawPolys.BooleanSubtract( clearanceHoles, SHAPE_POLY_SET::PM_FAST );
    // Prune features that don't meet minimum-width criteria
    if( half_min_width - epsilon > epsilon )
        aRawPolys.Deflate( half_min_width - epsilon, numSegs, cornerStrategy );

    if( s_DumpZonesWhenFilling )
        dumper->Write( &aRawPolys, "solid-areas-before-hatching" );

    // Now remove the non filled areas due to the hatch pattern
    if( aZone->GetFillMode() == ZFM_HATCH_PATTERN )
        addHatchFillTypeOnZone( aZone, aRawPolys );

    if( s_DumpZonesWhenFilling )
        dumper->Write( &aRawPolys, "solid-areas-after-hatching" );

    // Re-inflate after pruning of areas that don't meet minimum-width criteria
    if( aZone->GetFilledPolysUseThickness() )
    {
        // If we're stroking the zone with a min_width stroke then this will naturally
        // inflate the zone by half_min_width
    }
    else if( half_min_width - epsilon > epsilon )
    {
        aRawPolys.Simplify( SHAPE_POLY_SET::PM_FAST );
        aRawPolys.Inflate( half_min_width - epsilon, numSegs, cornerStrategy );

        // If we've deflated/inflated by something near our corner radius then we will have
        // ended up with too-sharp corners.  Apply outline smoothing again.
        if( aZone->GetMinThickness() > (int)aZone->GetCornerRadius() )
            aRawPolys.BooleanIntersection( aSmoothedOutline, SHAPE_POLY_SET::PM_FAST );
    }

    aRawPolys.Fracture( SHAPE_POLY_SET::PM_FAST );

    if( s_DumpZonesWhenFilling )
        dumper->Write( &aRawPolys, "areas_fractured" );

    aFinalPolys = aRawPolys;

    if( s_DumpZonesWhenFilling )
        dumper->EndGroup();
}


/*
 * Build the filled solid areas data from real outlines (stored in m_Poly)
 * The solid areas can be more than one on copper layers, and do not have holes
 * ( holes are linked by overlapping segments to the main outline)
 */
bool ZONE_FILLER::fillSingleZone( ZONE_CONTAINER* aZone, SHAPE_POLY_SET& aRawPolys,
                                  SHAPE_POLY_SET& aFinalPolys )
{
    SHAPE_POLY_SET smoothedPoly;
    std::set<VECTOR2I> colinearCorners;
    aZone->GetColinearCorners( m_board, colinearCorners );

    /*
     * convert outlines + holes to outlines without holes (adding extra segments if necessary)
     * m_Poly data is expected normalized, i.e. NormalizeAreaOutlines was used after building
     * this zone
     */
    if ( !aZone->BuildSmoothedPoly( smoothedPoly, &colinearCorners ) )
        return false;

    if( aZone->IsOnCopperLayer() )
    {
        computeRawFilledArea( aZone, smoothedPoly, &colinearCorners, aRawPolys, aFinalPolys );
    }
    else
    {
        // Features which are min_width should survive pruning; features that are *less* than
        // min_width should not.  Therefore we subtract epsilon from the min_width when
        // deflating/inflating.
        int half_min_width = aZone->GetMinThickness() / 2;
        int epsilon = Millimeter2iu( 0.001 );
        int numSegs = std::max( GetArcToSegmentCount( half_min_width, m_high_def, 360.0 ), 6 );

        if( m_brdOutlinesValid )
            smoothedPoly.BooleanIntersection( m_boardOutline, SHAPE_POLY_SET::PM_FAST );

        smoothedPoly.Deflate( half_min_width - epsilon, numSegs );

        // Remove the non filled areas due to the hatch pattern
        if( aZone->GetFillMode() == ZFM_HATCH_PATTERN )
            addHatchFillTypeOnZone( aZone, smoothedPoly );

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
void ZONE_FILLER::buildThermalSpokes( const ZONE_CONTAINER* aZone,
                                      std::deque<SHAPE_LINE_CHAIN>& aSpokesList )
{
    auto zoneBB = aZone->GetBoundingBox();
    int  zone_clearance = aZone->GetZoneClearance();
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
            if( !pad->IsOnLayer( aZone->GetLayer() ) )
                continue;

            int thermalReliefGap = aZone->GetThermalReliefGap( pad );

            // Calculate thermal bridge half width
            int spoke_w = aZone->GetThermalReliefCopperBridge( pad );
            // Avoid spoke_w bigger than the smaller pad size, because
            // it is not possible to create stubs bigger than the pad.
            // Possible refinement: have a separate size for vertical and horizontal stubs
            spoke_w = std::min( spoke_w, pad->GetSize().x );
            spoke_w = std::min( spoke_w, pad->GetSize().y );

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

            wxPoint shapePos = pad->ShapePos();
            wxPoint padPos = pad->GetPosition();
            double padAngle = pad->GetOrientation();
            pad->SetOrientation( 0.0 );
            pad->SetPosition( { 0, 0 } );
            BOX2I reliefBB = pad->GetBoundingBox();
            pad->SetPosition( padPos );
            pad->SetOrientation( padAngle );

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

                for( int j = 0; j < spoke.PointCount(); j++ )
                {
                    RotatePoint( spoke.Point( j ), padAngle );
                    spoke.Point( j ) += shapePos;
                }

                spoke.SetClosed( true );
                spoke.GenerateBBoxCache();
                aSpokesList.push_back( std::move( spoke ) );
            }
        }
    }
}


void ZONE_FILLER::addHatchFillTypeOnZone( const ZONE_CONTAINER* aZone, SHAPE_POLY_SET& aRawPolys )
{
    // Build grid:

    // obvously line thickness must be > zone min thickness. However, it should be
    // the case because the zone dialog setup ensure that. However, it can happens
    // if a board file was edited by hand by a python script
    int thickness = std::max( aZone->GetHatchFillTypeThickness(), aZone->GetMinThickness()+2 );
    int linethickness = thickness - aZone->GetMinThickness();
    int gridsize = thickness + aZone->GetHatchFillTypeGap();
    double orientation = aZone->GetHatchFillTypeOrientation();

    SHAPE_POLY_SET filledPolys = aRawPolys;
    // Use a area that contains the rotated bbox by orientation,
    // and after rotate the result by -orientation.
    if( orientation != 0.0 )
    {
        filledPolys.Rotate( M_PI/180.0 * orientation, VECTOR2I( 0,0 ) );
    }

    BOX2I bbox = filledPolys.BBox( 0 );

    // Build hole shape
    // the hole size is aZone->GetHatchFillTypeGap(), but because the outline thickness
    // is aZone->GetMinThickness(), the hole shape size must be larger
    SHAPE_LINE_CHAIN hole_base;
    int hole_size = aZone->GetHatchFillTypeGap() + aZone->GetMinThickness();
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
    double minimal_hole_area = hole_base.Area() / 2;

    // Now convert this hole to a smoothed shape:
    if( aZone->GetHatchFillTypeSmoothingLevel()  > 0 )
    {
        // the actual size of chamfer, or rounded corner radius is the half size
        // of the HatchFillTypeGap scaled by aZone->GetHatchFillTypeSmoothingValue()
        // aZone->GetHatchFillTypeSmoothingValue() = 1.0 is the max value for the chamfer or the
        // radius of corner (radius = half size of the hole)
        int smooth_value = KiROUND( aZone->GetHatchFillTypeGap()
                                    * aZone->GetHatchFillTypeSmoothingValue() / 2 );

        // Minimal optimization:
        // make smoothing only for reasonnable smooth values, to avoid a lot of useless segments
        // and if the smooth value is small, use chamfer even if fillet is requested
        #define SMOOTH_MIN_VAL_MM 0.02
        #define SMOOTH_SMALL_VAL_MM 0.04
        if( smooth_value > Millimeter2iu( SMOOTH_MIN_VAL_MM ) )
        {
            SHAPE_POLY_SET smooth_hole;
            smooth_hole.AddOutline( hole_base );
            int smooth_level = aZone->GetHatchFillTypeSmoothingLevel();

            if( smooth_value < Millimeter2iu( SMOOTH_SMALL_VAL_MM ) && smooth_level > 1 )
                smooth_level = 1;
            // Use a larger smooth_value to compensate the outline tickness
            // (chamfer is not visible is smooth value < outline thickess)
            smooth_value += aZone->GetMinThickness()/2;

            // smooth_value cannot be bigger than the half size oh the hole:
            smooth_value = std::min( smooth_value, aZone->GetHatchFillTypeGap()/2 );
            // the error to approximate a circle by segments when smoothing corners by a arc
            int error_max = std::max( Millimeter2iu( 0.01), smooth_value/20 );

            switch( smooth_level )
            {
            case 1:
                // Chamfer() uses the distance from a corner to create a end point
                // for the chamfer.
                hole_base = smooth_hole.Chamfer( smooth_value ).Outline( 0 );
                break;

            default:
                if( aZone->GetHatchFillTypeSmoothingLevel() > 2 )
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

    // Clamp holes to the area of filled zones with a outline thickness
    // > aZone->GetMinThickness() to be sure the thermal pads can be built
    int outline_margin = std::max( (aZone->GetMinThickness()*10)/9, linethickness/2 );
    filledPolys.Deflate( outline_margin, 16 );
    holes.BooleanIntersection( filledPolys, SHAPE_POLY_SET::PM_FAST );

    if( orientation != 0.0 )
        holes.Rotate( -M_PI/180.0 * orientation, VECTOR2I( 0,0 ) );

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
