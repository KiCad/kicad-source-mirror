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


extern void CreateThermalReliefPadPolygon( SHAPE_POLY_SET& aCornerBuffer, const D_PAD& aPad,
        int aThermalGap, int aCopperThickness, int aMinThicknessValue, int aError,
        double aThermalRot );

static double s_thermalRot = 450;   // angle of stubs in thermal reliefs for round pads
static const bool s_DumpZonesWhenFilling = false;


ZONE_FILLER::ZONE_FILLER(  BOARD* aBoard, COMMIT* aCommit ) :
    m_board( aBoard ), m_commit( aCommit ), m_progressReporter( nullptr )
{
}


ZONE_FILLER::~ZONE_FILLER()
{
}


void ZONE_FILLER::SetProgressReporter( WX_PROGRESS_REPORTER* aReporter )
{
    m_progressReporter = aReporter;
}


bool ZONE_FILLER::Fill( const std::vector<ZONE_CONTAINER*>& aZones, bool aCheck )
{
    std::vector<CN_ZONE_ISOLATED_ISLAND_LIST> toFill;
    auto connectivity = m_board->GetConnectivity();
    bool filledPolyWithOutline = not m_board->GetDesignSettings().m_ZoneUseNoOutlineInFill;

    if( ADVANCED_CFG::GetCfg().m_forceThickOutlinesInZones )
    {
        filledPolyWithOutline = true;
    }

    std::unique_lock<std::mutex> lock( connectivity->GetLock(), std::try_to_lock );

    if( !lock )
        return false;

    if( m_progressReporter )
    {
        m_progressReporter->Report( aCheck ? _( "Checking zone fills..." ) : _( "Building zone fills..." ) );
        m_progressReporter->SetMaxProgress( toFill.size() );
    }

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
    SHAPE_POLY_SET boardOutline;
    bool clip_to_brd_outlines = m_board->GetBoardPolygonOutlines( boardOutline );

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
        else if( clip_to_brd_outlines )
        {
             for( int idx = 0; idx < poly.OutlineCount(); )
            {
                if( poly.Polygon( idx ).empty() ||
                    !boardOutline.Contains( poly.Polygon( idx ).front().CPoint( 0 ) ) )
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


void ZONE_FILLER::buildZoneFeatureHoleList( const ZONE_CONTAINER* aZone,
        SHAPE_POLY_SET& aFeatures ) const
{
    aFeatures.RemoveAllContours();

    int zone_clearance = aZone->GetClearance();
    int edgeClearance = m_board->GetDesignSettings().m_CopperEdgeClearance;
    int zone_to_edgecut_clearance = std::max( aZone->GetZoneClearance(), edgeClearance );

    // dist_high_def can be used to define a high definition arc to polygon approximation:
    // (shorter than m_board->GetDesignSettings().m_MaxError and is a better wording here)
    int dist_high_def = m_board->GetDesignSettings().m_MaxError;

    // dist_low_def can be used to define a low definition arc to polygon approximation:
    // when converting some pad shapes that can accept lower resolution),
    // vias and track ends to polygons.
    // rect pads use dist_low_def to reduce the number of segments. For these shapes a low def
    // gives a good shape, because the arc is small (90 degrees) and a small part of the shape.
    int dist_low_def = std::min( ARC_LOW_DEF, int( dist_high_def*1.5 ) );   // Reasonable value

    // When removing holes, the holes must be expanded by outline_half_thickness
    // to take in account the thickness of the zone outlines
    int outline_half_thickness = aZone->GetMinThickness() / 2;
    zone_clearance += outline_half_thickness;
    zone_to_edgecut_clearance += outline_half_thickness;

    /* store holes (i.e. tracks and pads areas as polygons outlines)
     * in a polygon list
     */

    /* items outside the zone bounding box are skipped
     * the bounding box is the zone bounding box + the biggest clearance found in Netclass list
     */
    EDA_RECT    item_boundingbox;
    EDA_RECT    zone_boundingbox = aZone->GetBoundingBox();
    int biggest_clearance = m_board->GetDesignSettings().GetBiggestClearanceValue();
    biggest_clearance = std::max( biggest_clearance, zone_clearance );
    zone_boundingbox.Inflate( biggest_clearance );

    /*
     * First : Add pads. Note: pads having the same net as zone are left in zone.
     * Thermal shapes will be created later if necessary
     */

    /* Use a dummy pad to calculate hole clearance when a pad is not on all copper layers
     * and this pad has a hole
     * This dummy pad has the size and shape of the hole
     * Therefore, this dummy pad is a circle or an oval.
     * A pad must have a parent because some functions expect a non null parent
     * to find the parent board, and some other data
     */
    MODULE  dummymodule( m_board );   // Creates a dummy parent
    D_PAD   dummypad( &dummymodule );

    for( auto module : m_board->Modules() )
    {
        for( auto pad : module->Pads() )
        {
            if( !pad->IsOnLayer( aZone->GetLayer() ) )
            {
                /* Test for pads that are on top or bottom only and have a hole.
                 * There are curious pads but they can be used for some components that are
                 * inside the board (in fact inside the hole. Some photo diodes and Leds are
                 * like this)
                 */
                if( pad->GetDrillSize().x == 0 && pad->GetDrillSize().y == 0 )
                    continue;

                // Use a dummy pad to calculate a hole shape that have the same dimension as
                // the pad hole
                dummypad.SetSize( pad->GetDrillSize() );
                dummypad.SetOrientation( pad->GetOrientation() );
                dummypad.SetShape( pad->GetDrillShape() == PAD_DRILL_SHAPE_OBLONG ?
                        PAD_SHAPE_OVAL : PAD_SHAPE_CIRCLE );
                dummypad.SetPosition( pad->GetPosition() );

                pad = &dummypad;
            }

            // Note: netcode <=0 means not connected item
            if( ( pad->GetNetCode() != aZone->GetNetCode() ) || ( pad->GetNetCode() <= 0 ) )
            {
                int item_clearance = pad->GetClearance() + outline_half_thickness;
                item_boundingbox = pad->GetBoundingBox();
                item_boundingbox.Inflate( item_clearance );

                if( item_boundingbox.Intersects( zone_boundingbox ) )
                {
                    int clearance = std::max( zone_clearance, item_clearance );

                    // PAD_SHAPE_CUSTOM can have a specific keepout, to avoid to break the shape
                    if( pad->GetShape() == PAD_SHAPE_CUSTOM
                        && pad->GetCustomShapeInZoneOpt() == CUST_PAD_SHAPE_IN_ZONE_CONVEXHULL )
                    {
                        // the pad shape in zone can be its convex hull or
                        // the shape itself
                        SHAPE_POLY_SET outline( pad->GetCustomShapeAsPolygon() );
                        int numSegs = std::max(
                                GetArcToSegmentCount( clearance, dist_high_def, 360.0 ), 6 );
                        double correction = GetCircletoPolyCorrectionFactor( numSegs );
                        outline.Inflate( KiROUND( clearance * correction ), numSegs );
                        pad->CustomShapeAsPolygonToBoardPosition(
                                &outline, pad->GetPosition(), pad->GetOrientation() );

                        if( pad->GetCustomShapeInZoneOpt() == CUST_PAD_SHAPE_IN_ZONE_CONVEXHULL )
                        {
                            std::vector<wxPoint> convex_hull;
                            BuildConvexHull( convex_hull, outline );

                            aFeatures.NewOutline();

                            for( unsigned ii = 0; ii < convex_hull.size(); ++ii )
                                aFeatures.Append( convex_hull[ii] );
                        }
                        else
                            aFeatures.Append( outline );
                    }
                    else
                    {
                        // Optimizing polygon vertex count: the high definition is used for round and
                        // oval pads (pads with large arcs) but low def for other shapes (with smal arcs)
                        if( pad->GetShape() == PAD_SHAPE_CIRCLE ||
                            pad->GetShape() == PAD_SHAPE_OVAL )
                            pad->TransformShapeWithClearanceToPolygon( aFeatures, clearance,
                                                                       dist_high_def );
                        else
                            pad->TransformShapeWithClearanceToPolygon( aFeatures, clearance,
                                                                       dist_low_def );
                    }
                }

                continue;
            }

            // Pads are removed from zone if the setup is PAD_ZONE_CONN_NONE
            // or if they have a custom shape and not PAD_ZONE_CONN_FULL,
            // because a thermal relief will break
            // the shape
            if( aZone->GetPadConnection( pad ) == PAD_ZONE_CONN_NONE
                || ( pad->GetShape() == PAD_SHAPE_CUSTOM && aZone->GetPadConnection( pad ) != PAD_ZONE_CONN_FULL ) )
            {
                int gap = zone_clearance;
                int thermalGap = aZone->GetThermalReliefGap( pad );
                gap = std::max( gap, thermalGap );
                item_boundingbox = pad->GetBoundingBox();
                item_boundingbox.Inflate( gap );

                if( item_boundingbox.Intersects( zone_boundingbox ) )
                {
                    // PAD_SHAPE_CUSTOM has a specific keepout, to avoid to break the shape
                    // the pad shape in zone can be its convex hull or the shape itself
                    if( pad->GetShape() == PAD_SHAPE_CUSTOM
                        && pad->GetCustomShapeInZoneOpt() == CUST_PAD_SHAPE_IN_ZONE_CONVEXHULL )
                    {
                        // the pad shape in zone can be its convex hull or
                        // the shape itself
                        int numSegs = std::max(
                                GetArcToSegmentCount( gap, dist_high_def, 360.0 ), 6 );
                        double correction = GetCircletoPolyCorrectionFactor( numSegs );
                        SHAPE_POLY_SET outline( pad->GetCustomShapeAsPolygon() );
                        outline.Inflate( KiROUND( gap * correction ), numSegs );
                        pad->CustomShapeAsPolygonToBoardPosition(
                                &outline, pad->GetPosition(), pad->GetOrientation() );

                        std::vector<wxPoint> convex_hull;
                        BuildConvexHull( convex_hull, outline );

                        aFeatures.NewOutline();

                        for( unsigned ii = 0; ii < convex_hull.size(); ++ii )
                            aFeatures.Append( convex_hull[ii] );
                    }
                    else
                    {
                        if( pad->GetShape() == PAD_SHAPE_CIRCLE ||
                            pad->GetShape() == PAD_SHAPE_OVAL )
                            pad->TransformShapeWithClearanceToPolygon( aFeatures, gap,
                                                                       dist_high_def );
                        else
                            pad->TransformShapeWithClearanceToPolygon( aFeatures, gap,
                                                                       dist_low_def );
                    }
                }
            }
        }
    }

    /* Add holes (i.e. tracks and vias areas as polygons outlines)
     * in cornerBufferPolysToSubstract
     */
    for( auto track : m_board->Tracks() )
    {
        if( !track->IsOnLayer( aZone->GetLayer() ) )
            continue;

        if( track->GetNetCode() == aZone->GetNetCode()  && ( aZone->GetNetCode() != 0) )
            continue;

        int item_clearance = track->GetClearance() + outline_half_thickness;
        item_boundingbox = track->GetBoundingBox();

        if( item_boundingbox.Intersects( zone_boundingbox ) )
        {
            int clearance = std::max( zone_clearance, item_clearance );
            track->TransformShapeWithClearanceToPolygon( aFeatures, clearance,
                                                         dist_low_def );
        }
    }

    /* Add graphic items that are on copper layers.  These have no net, so we just
     * use the zone clearance (or edge clearance).
     */
    auto doGraphicItem = [&]( BOARD_ITEM* aItem )
    {
        // A item on the Edge_Cuts is always seen as on any layer:
        if( !aItem->IsOnLayer( aZone->GetLayer() ) && !aItem->IsOnLayer( Edge_Cuts ) )
            return;

        if( !aItem->GetBoundingBox().Intersects( zone_boundingbox ) )
            return;

        bool ignoreLineWidth = false;
        int zclearance = zone_clearance;

        if( aItem->IsOnLayer( Edge_Cuts ) )
        {
            // use only the m_ZoneClearance, not the clearance using
            // the netclass value, because we do not have a copper item
            zclearance = zone_to_edgecut_clearance;

            // edge cuts by definition don't have a width
            ignoreLineWidth = true;
        }

        switch( aItem->Type() )
        {
        case PCB_LINE_T:
            static_cast<DRAWSEGMENT*>( aItem )->TransformShapeWithClearanceToPolygon(
                    aFeatures, zclearance, m_board->GetDesignSettings().m_MaxError,
                    ignoreLineWidth );
            break;

        case PCB_TEXT_T:
            static_cast<TEXTE_PCB*>( aItem )->TransformBoundingBoxWithClearanceToPolygon(
                    &aFeatures, zclearance );
            break;

        case PCB_MODULE_EDGE_T:
            static_cast<EDGE_MODULE*>( aItem )->TransformShapeWithClearanceToPolygon(
                    aFeatures, zclearance, m_board->GetDesignSettings().m_MaxError,
                    ignoreLineWidth );
            break;

        case PCB_MODULE_TEXT_T:
            if( static_cast<TEXTE_MODULE*>( aItem )->IsVisible() )
            {
                static_cast<TEXTE_MODULE*>( aItem )->TransformBoundingBoxWithClearanceToPolygon(
                        &aFeatures, zclearance );
            }
            break;

        default:
            break;
        }
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

    /* Add zones outlines having an higher priority and keepout
     */
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
        item_boundingbox = zone->GetBoundingBox();

        if( !item_boundingbox.Intersects( zone_boundingbox ) )
            continue;

        // Add the zone outline area.
        // However if the zone has the same net as the current zone,
        // do not add any clearance.
        // the zone will be connected to the current zone, but filled areas
        // will use different parameters (clearance, thermal shapes )
        bool    same_net = aZone->GetNetCode() == zone->GetNetCode();
        bool    use_net_clearance = true;
        int     min_clearance = zone_clearance;

        // Do not forget to make room to draw the thick outlines
        // of the hole created by the area of the zone to remove
        int holeclearance = zone->GetClearance() + outline_half_thickness;

        // The final clearance is obviously the max value of each zone clearance
        min_clearance = std::max( min_clearance, holeclearance );

        if( zone->GetIsKeepout() || same_net )
        {
            // Just take in account the fact the outline has a thickness, so
            // the actual area to substract is inflated to take in account this fact
            min_clearance = outline_half_thickness;
            use_net_clearance = false;
        }

        zone->TransformOutlinesShapeWithClearanceToPolygon(
                aFeatures, min_clearance, use_net_clearance );
    }

    /* Remove thermal symbols
     */
    for( auto module : m_board->Modules() )
    {
        for( auto pad : module->Pads() )
        {
            // Rejects non-standard pads with tht-only thermal reliefs
            if( aZone->GetPadConnection( pad ) == PAD_ZONE_CONN_THT_THERMAL
                && pad->GetAttribute() != PAD_ATTRIB_STANDARD )
                continue;

            if( aZone->GetPadConnection( pad ) != PAD_ZONE_CONN_THERMAL
                && aZone->GetPadConnection( pad ) != PAD_ZONE_CONN_THT_THERMAL )
                continue;

            if( !pad->IsOnLayer( aZone->GetLayer() ) )
                continue;

            if( pad->GetNetCode() != aZone->GetNetCode() )
                continue;

            if( pad->GetNetCode() <= 0 )
                continue;

            item_boundingbox = pad->GetBoundingBox();
            int thermalGap = aZone->GetThermalReliefGap( pad );
            item_boundingbox.Inflate( thermalGap, thermalGap );

            if( item_boundingbox.Intersects( zone_boundingbox ) )
            {
                CreateThermalReliefPadPolygon( aFeatures, *pad, thermalGap,
                        aZone->GetThermalReliefCopperBridge( pad ), aZone->GetMinThickness(),
                        m_board->GetDesignSettings().m_MaxError, s_thermalRot );
            }
        }
    }
}

/**
 * Function ComputeRawFilledAreas
 * Supports a min thickness area constraint.
 * Add non copper areas polygons (pads and tracks with clearance)
 * to the filled copper area found
 * in BuildFilledPolysListData after calculating filled areas in a zone
 * Non filled copper areas are pads and track and their clearance areas
 * The filled copper area must be computed just before.
 * BuildFilledPolysListData() call this function just after creating the
 *  filled copper area polygon (without clearance areas)
 * to do that this function:
 * 1 - Creates the main outline (zone outline) using a correction to shrink the resulting area
 *     with m_ZoneMinThickness/2 value.
 *     The result is areas with a margin of m_ZoneMinThickness/2
 *     When drawing outline with segments having a thickness of m_ZoneMinThickness, the
 *      outlines will match exactly the initial outlines
 * 3 - Add all non filled areas (pads, tracks) in group B with a clearance of m_Clearance +
 *     m_ZoneMinThickness/2
 *     in a buffer
 *   - If Thermal shapes are wanted, add non filled area, in order to create these thermal shapes
 * 4 - calculates the polygon A - B
 * 5 - put resulting list of polygons (filled areas) in m_FilledPolysList
 *     This zone contains pads with the same net.
 * 6 - Remove insulated copper islands
 * 7 - If Thermal shapes are wanted, remove unconnected stubs in thermal shapes:
 *     creates a buffer of polygons corresponding to stubs to remove
 *     sub them to the filled areas.
 *     Remove new insulated copper islands
 */
void ZONE_FILLER::computeRawFilledAreas( const ZONE_CONTAINER* aZone,
        const SHAPE_POLY_SET& aSmoothedOutline,
        SHAPE_POLY_SET& aRawPolys,
        SHAPE_POLY_SET& aFinalPolys ) const
{
    int outline_half_thickness = aZone->GetMinThickness() / 2;

    std::unique_ptr<SHAPE_FILE_IO> dumper( new SHAPE_FILE_IO(
                    s_DumpZonesWhenFilling ? "zones_dump.txt" : "", SHAPE_FILE_IO::IOM_APPEND ) );

    if( s_DumpZonesWhenFilling )
        dumper->BeginGroup( "clipper-zone" );

    SHAPE_POLY_SET solidAreas = aSmoothedOutline;

    int numSegs = std::max(
            GetArcToSegmentCount( outline_half_thickness, m_board->GetDesignSettings().m_MaxError,
                    360.0 ), 6 );
    double correction = GetCircletoPolyCorrectionFactor( numSegs );

    solidAreas.Inflate( -outline_half_thickness, numSegs );
    solidAreas.Simplify( SHAPE_POLY_SET::PM_FAST );

    SHAPE_POLY_SET holes;

    if( s_DumpZonesWhenFilling )
        dumper->Write( &solidAreas, "solid-areas" );

    buildZoneFeatureHoleList( aZone, holes );

    if( s_DumpZonesWhenFilling )
        dumper->Write( &holes, "feature-holes" );

    holes.Simplify( SHAPE_POLY_SET::PM_FAST );

    if( s_DumpZonesWhenFilling )
        dumper->Write( &holes, "feature-holes-postsimplify" );

    // Generate the filled areas (currently, without thermal shapes, which will
    // be created later).
    // Use SHAPE_POLY_SET::PM_STRICTLY_SIMPLE to generate strictly simple polygons
    // needed by Gerber files and Fracture()
    solidAreas.BooleanSubtract( holes, SHAPE_POLY_SET::PM_STRICTLY_SIMPLE );

    // Now remove the non filled areas due to the hatch pattern
    if( aZone->GetFillMode() == ZFM_HATCH_PATTERN )
        addHatchFillTypeOnZone( aZone, solidAreas );

    if( s_DumpZonesWhenFilling )
        dumper->Write( &solidAreas, "solid-areas-minus-holes" );

    if( !aZone->IsOnCopperLayer() )
    {
        SHAPE_POLY_SET areas_fractured = solidAreas;
        areas_fractured.Fracture( SHAPE_POLY_SET::PM_FAST );

        if( s_DumpZonesWhenFilling )
            dumper->Write( &areas_fractured, "areas_fractured" );

        aFinalPolys = areas_fractured;
        aRawPolys = aFinalPolys;

        if( s_DumpZonesWhenFilling )
            dumper->EndGroup();

        return;
    }

    // Test thermal stubs connections and add polygons to remove unconnected stubs.
    // (this is a refinement for thermal relief shapes)
    // Note: we are using not fractured solid area polygons, to avoid a side effect of extra segments
    // created by Fracture(): if a tested point used in buildUnconnectedThermalStubsPolygonList
    // is on a extra segment, the tested point is seen outside the solid area, but it is inside.
    // This is not a bug, just the fact when a point is on a polygon outline, it is hard to say
    // if it is inside or outside the polygon.
    SHAPE_POLY_SET thermalHoles;

    if( aZone->GetNetCode() > 0 )
    {
        buildUnconnectedThermalStubsPolygonList(
                thermalHoles, aZone, solidAreas, correction, s_thermalRot );
    }

    // remove copper areas corresponding to not connected stubs
    if( !thermalHoles.IsEmpty() )
    {
        thermalHoles.Simplify( SHAPE_POLY_SET::PM_FAST );
        // Remove unconnected stubs. Use SHAPE_POLY_SET::PM_STRICTLY_SIMPLE to
        // generate strictly simple polygons
        // needed by Gerber files and Fracture()
        solidAreas.BooleanSubtract( thermalHoles, SHAPE_POLY_SET::PM_STRICTLY_SIMPLE );

        if( s_DumpZonesWhenFilling )
            dumper->Write( &thermalHoles, "thermal-holes" );

        // put these areas in m_FilledPolysList
        SHAPE_POLY_SET th_fractured = solidAreas;

        // Inflate polygon to recreate the polygon (without the too narrow areas)
        // if the filled polygons have a outline thickness = 0
        int inflate_value = aZone->GetFilledPolysUseThickness() ? 0 :outline_half_thickness;

        if( inflate_value <= Millimeter2iu( 0.001 ) )    // avoid very small outline thickness
            inflate_value = 0;

        if( inflate_value )
        {
            th_fractured.Simplify( SHAPE_POLY_SET::PM_FAST );
            th_fractured.Inflate( outline_half_thickness, 16 );
        }

        th_fractured.Fracture( SHAPE_POLY_SET::PM_FAST );

        if( s_DumpZonesWhenFilling )
            dumper->Write( &th_fractured, "th_fractured" );

        aFinalPolys = th_fractured;
    }
    else
    {
        SHAPE_POLY_SET areas_fractured = solidAreas;

        // Inflate polygon to recreate the polygon (without the too narrow areas)
        // if the filled polygons have a outline thickness = 0
        int inflate_value = aZone->GetFilledPolysUseThickness() ? 0 : outline_half_thickness;

        if( inflate_value <= Millimeter2iu( 0.001 ) )    // avoid very small outline thickness
            inflate_value = 0;

        if( inflate_value )
        {
            areas_fractured.Simplify( SHAPE_POLY_SET::PM_FAST );
            areas_fractured.Inflate( outline_half_thickness, 16 );
        }

        areas_fractured.Fracture( SHAPE_POLY_SET::PM_FAST );

        if( s_DumpZonesWhenFilling )
            dumper->Write( &areas_fractured, "areas_fractured" );

        aFinalPolys = areas_fractured;
    }

    aRawPolys = aFinalPolys;

    if( s_DumpZonesWhenFilling )
        dumper->EndGroup();
}

/* Build the filled solid areas data from real outlines (stored in m_Poly)
 * The solid areas can be more than one on copper layers, and do not have holes
 * ( holes are linked by overlapping segments to the main outline)
 */
bool ZONE_FILLER::fillSingleZone( ZONE_CONTAINER* aZone, SHAPE_POLY_SET& aRawPolys,
                                  SHAPE_POLY_SET& aFinalPolys ) const
{
    SHAPE_POLY_SET smoothedPoly;

    /* convert outlines + holes to outlines without holes (adding extra segments if necessary)
     * m_Poly data is expected normalized, i.e. NormalizeAreaOutlines was used after building
     * this zone
     */
    if ( !aZone->BuildSmoothedPoly( smoothedPoly ) )
        return false;

    if( aZone->IsOnCopperLayer() )
    {
        computeRawFilledAreas( aZone, smoothedPoly, aRawPolys, aFinalPolys );
    }
    else
    {
        int numSegs = std::max(
                GetArcToSegmentCount( aZone->GetMinThickness() / 2,
                        m_board->GetDesignSettings().m_MaxError, 360.0 ), 6 );
        aFinalPolys.Inflate( -aZone->GetMinThickness() / 2, numSegs );

        // Remove the non filled areas due to the hatch pattern
        if( aZone->GetFillMode() == ZFM_HATCH_PATTERN )
            addHatchFillTypeOnZone( aZone, smoothedPoly );

        aRawPolys = smoothedPoly;
        aFinalPolys = smoothedPoly;
        aFinalPolys.Fracture( SHAPE_POLY_SET::PM_STRICTLY_SIMPLE );
    }

    aZone->SetNeedRefill( false );
    return true;
}


/**
 * Function buildUnconnectedThermalStubsPolygonList
 * Creates a set of polygons corresponding to stubs created by thermal shapes on pads
 * which are not connected to a zone (dangling bridges)
 * @param aCornerBuffer = a SHAPE_POLY_SET where to store polygons
 * @param aZone = a pointer to the ZONE_CONTAINER  to examine.
 * @param aArcCorrection = arc correction factor.
 * @param aRoundPadThermalRotation = the rotation in 1.0 degree for thermal stubs in round pads
 */

void ZONE_FILLER::buildUnconnectedThermalStubsPolygonList( SHAPE_POLY_SET& aCornerBuffer,
                                              const ZONE_CONTAINER*       aZone,
                                              const SHAPE_POLY_SET&       aRawFilledArea,
                                              double                aArcCorrection,
                                              double                aRoundPadThermalRotation ) const
{
    SHAPE_LINE_CHAIN spokes;
    BOX2I itemBB;
    VECTOR2I ptTest[4];
    auto zoneBB = aRawFilledArea.BBox();


    int      zone_clearance = aZone->GetZoneClearance();

    int      biggest_clearance = m_board->GetDesignSettings().GetBiggestClearanceValue();
    biggest_clearance = std::max( biggest_clearance, zone_clearance );
    zoneBB.Inflate( biggest_clearance );

    // half size of the pen used to draw/plot zones outlines
    int pen_radius = aZone->GetMinThickness() / 2;

    for( auto module : m_board->Modules() )
    {
        for( auto pad : module->Pads() )
        {
            // Rejects non-standard pads with tht-only thermal reliefs
            if( aZone->GetPadConnection( pad ) == PAD_ZONE_CONN_THT_THERMAL
             && pad->GetAttribute() != PAD_ATTRIB_STANDARD )
                continue;

            if( aZone->GetPadConnection( pad ) != PAD_ZONE_CONN_THERMAL
             && aZone->GetPadConnection( pad ) != PAD_ZONE_CONN_THT_THERMAL )
                continue;

            if( !pad->IsOnLayer( aZone->GetLayer() ) )
                continue;

            if( pad->GetNetCode() != aZone->GetNetCode() )
                continue;

            // Calculate thermal bridge half width
            int thermalBridgeWidth = aZone->GetThermalReliefCopperBridge( pad )
                                     - aZone->GetMinThickness();
            if( thermalBridgeWidth <= 0 )
                continue;

            // we need the thermal bridge half width
            // with a small extra size to be sure we create a stub
            // slightly larger than the actual stub
            thermalBridgeWidth = ( thermalBridgeWidth + 4 ) / 2;

            int thermalReliefGap = aZone->GetThermalReliefGap( pad );

            itemBB = pad->GetBoundingBox();
            itemBB.Inflate( thermalReliefGap );
            if( !( itemBB.Intersects( zoneBB ) ) )
                continue;

            // Thermal bridges are like a segment from a starting point inside the pad
            // to an ending point outside the pad

            // calculate the ending point of the thermal pad, outside the pad
            VECTOR2I endpoint;
            endpoint.x = ( pad->GetSize().x / 2 ) + thermalReliefGap;
            endpoint.y = ( pad->GetSize().y / 2 ) + thermalReliefGap;

            // Calculate the starting point of the thermal stub
            // inside the pad
            VECTOR2I startpoint;
            int copperThickness = aZone->GetThermalReliefCopperBridge( pad )
                                  - aZone->GetMinThickness();

            if( copperThickness < 0 )
                copperThickness = 0;

            // Leave a small extra size to the copper area inside to pad
            copperThickness += KiROUND( IU_PER_MM * 0.04 );

            startpoint.x = std::min( pad->GetSize().x, copperThickness );
            startpoint.y = std::min( pad->GetSize().y, copperThickness );

            startpoint.x /= 2;
            startpoint.y /= 2;

            // This is a CIRCLE pad tweak
            // for circle pads, the thermal stubs orientation is 45 deg
            double fAngle = pad->GetOrientation();
            if( pad->GetShape() == PAD_SHAPE_CIRCLE )
            {
                endpoint.x     = KiROUND( endpoint.x * aArcCorrection );
                endpoint.y     = endpoint.x;
                fAngle = aRoundPadThermalRotation;
            }

            // contour line width has to be taken into calculation to avoid "thermal stub bleed"
            endpoint.x += pen_radius;
            endpoint.y += pen_radius;
            // compute north, south, west and east points for zone connection.
            ptTest[0] = VECTOR2I( 0, endpoint.y );       // lower point
            ptTest[1] = VECTOR2I( 0, -endpoint.y );      // upper point
            ptTest[2] = VECTOR2I( endpoint.x, 0 );       // right point
            ptTest[3] = VECTOR2I( -endpoint.x, 0 );      // left point

            // Test all sides
            for( int i = 0; i < 4; i++ )
            {
                // rotate point
                RotatePoint( ptTest[i], fAngle );

                // translate point
                ptTest[i] += pad->ShapePos();

                if( aRawFilledArea.Contains( ptTest[i] ) )
                    continue;

                spokes.Clear();

                // polygons are rectangles with width of copper bridge value
                switch( i )
                {
                case 0:       // lower stub
                    spokes.Append( -thermalBridgeWidth, endpoint.y );
                    spokes.Append( +thermalBridgeWidth, endpoint.y );
                    spokes.Append( +thermalBridgeWidth, startpoint.y );
                    spokes.Append( -thermalBridgeWidth, startpoint.y );
                    break;

                case 1:       // upper stub
                    spokes.Append( -thermalBridgeWidth, -endpoint.y );
                    spokes.Append( +thermalBridgeWidth, -endpoint.y );
                    spokes.Append( +thermalBridgeWidth, -startpoint.y );
                    spokes.Append( -thermalBridgeWidth, -startpoint.y );
                    break;

                case 2:       // right stub
                    spokes.Append( endpoint.x, -thermalBridgeWidth );
                    spokes.Append( endpoint.x, thermalBridgeWidth );
                    spokes.Append( +startpoint.x, thermalBridgeWidth );
                    spokes.Append( +startpoint.x, -thermalBridgeWidth );
                    break;

                case 3:       // left stub
                    spokes.Append( -endpoint.x, -thermalBridgeWidth );
                    spokes.Append( -endpoint.x, thermalBridgeWidth );
                    spokes.Append( -startpoint.x, thermalBridgeWidth );
                    spokes.Append( -startpoint.x, -thermalBridgeWidth );
                    break;
                }

                aCornerBuffer.NewOutline();

                // add computed polygon to list
                for( int ic = 0; ic < spokes.PointCount(); ic++ )
                {
                    auto cpos = spokes.CPoint( ic );
                    RotatePoint( cpos, fAngle );                               // Rotate according to module orientation
                    cpos += pad->ShapePos();                              // Shift origin to position
                    aCornerBuffer.Append( cpos );
                }
            }
        }
    }
}


void ZONE_FILLER::addHatchFillTypeOnZone( const ZONE_CONTAINER* aZone, SHAPE_POLY_SET& aRawPolys ) const
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
    filledPolys.Inflate( -outline_margin, 16 );
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
