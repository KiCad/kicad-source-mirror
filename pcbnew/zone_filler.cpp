/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014-2017 CERN
 * Copyright (C) 2014-2017 KiCad Developers, see AUTHORS.txt for contributors.
 * @author Tomasz Włostowski <tomasz.wlostowski@cern.ch>
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

#include <cstdint>
#include <thread>
#include <mutex>

#include <class_board.h>
#include <class_zone.h>
#include <class_module.h>
#include <class_edge_mod.h>
#include <class_drawsegment.h>
#include <class_track.h>
#include <class_pcb_text.h>
#include <class_pcb_target.h>

#include <connectivity_data.h>
#include <board_commit.h>

#include <widgets/progress_reporter.h>

#include <geometry/shape_poly_set.h>
#include <geometry/shape_file_io.h>
#include <geometry/convex_hull.h>

#include "zone_filler.h"

#ifdef USE_OPENMP
#include <omp.h>
#endif /* USE_OPENMP */

extern void BuildUnconnectedThermalStubsPolygonList( SHAPE_POLY_SET& aCornerBuffer,
        const BOARD* aPcb,
        const ZONE_CONTAINER* aZone,
        double aArcCorrection,
        double aRoundPadThermalRotation );


extern void CreateThermalReliefPadPolygon( SHAPE_POLY_SET& aCornerBuffer,
        const D_PAD& aPad,
        int aThermalGap,
        int aCopperThickness,
        int aMinThicknessValue,
        int aCircleToSegmentsCount,
        double aCorrectionFactor,
        double aThermalRot );

static double s_thermalRot = 450;    // angle of stubs in thermal reliefs for round pads
static const bool s_DumpZonesWhenFilling = false;

ZONE_FILLER::ZONE_FILLER(  BOARD* aBoard, COMMIT* aCommit ) :
    m_board( aBoard ),
    m_commit( aCommit ),
    m_progressReporter( nullptr )
{
}


ZONE_FILLER::~ZONE_FILLER()
{
}


void ZONE_FILLER::SetProgressReporter( PROGRESS_REPORTER* aReporter )
{
    m_progressReporter = aReporter;
}


void ZONE_FILLER::Fill( std::vector<ZONE_CONTAINER*> aZones )
{
    std::vector<CN_ZONE_ISOLATED_ISLAND_LIST> toFill;
    auto connectivity = m_board->GetConnectivity();

    // Remove segment zones
    m_board->m_Zone.DeleteAll();

    for( auto zone : aZones )
    {
        // Keepout zones are not filled
        if( zone->GetIsKeepout() )
            continue;

        CN_ZONE_ISOLATED_ISLAND_LIST l;
        l.m_zone = zone;
        toFill.push_back( l );
    }

    for( unsigned i = 0; i < toFill.size(); i++ )
    {
        if( m_commit )
        {
            m_commit->Modify( toFill[i].m_zone );
        }
    }

    if( m_progressReporter )
    {
        m_progressReporter->Report( _( "Calculating zone fills..." ) );
        m_progressReporter->SetMaxProgress( toFill.size() );
    }


    #ifdef USE_OPENMP
        #pragma omp parallel
    #endif
    {
        #ifdef USE_OPENMP
            #pragma omp master
        #endif
        if( m_progressReporter )
        {
            m_progressReporter->KeepRefreshing();
        }

        #ifdef USE_OPENMP
            #pragma omp for schedule(dynamic)
        #endif
        for( unsigned i = 0; i < toFill.size(); i++ )
        {
            SHAPE_POLY_SET rawPolys, finalPolys;
            ZONE_SEGMENT_FILL segFill;
            fillSingleZone ( toFill[i].m_zone, rawPolys, finalPolys, segFill );

            toFill[i].m_zone->SetRawPolysList( rawPolys );
            toFill[i].m_zone->SetFilledPolysList( finalPolys );
            toFill[i].m_zone->SetFillSegments( segFill );
            toFill[i].m_zone->SetIsFilled( true );

            if( m_progressReporter )
            {
                m_progressReporter->AdvanceProgress();
            }
        }

    }

    if( m_progressReporter )
    {
        m_progressReporter->AdvancePhase();
        m_progressReporter->Report( _( "Removing insulated copper islands..." ) );
    }

    connectivity->SetProgressReporter( m_progressReporter );
    connectivity->FindIsolatedCopperIslands( toFill );

    for( auto& zone : toFill )
    {
        std::sort( zone.m_islands.begin(), zone.m_islands.end(), std::greater<int>() );
        SHAPE_POLY_SET poly = zone.m_zone->GetFilledPolysList();

        for( auto idx : zone.m_islands )
        {
            poly.DeletePolygon( idx );
        }

        zone.m_zone->SetFilledPolysList( poly );
    }

    if( m_progressReporter )
    {
        m_progressReporter->AdvancePhase();
        m_progressReporter->Report( _( "Caching polygon triangulations..." ) );
        m_progressReporter->SetMaxProgress( toFill.size() );
    }
    #ifdef USE_OPENMP
        #pragma omp parallel
    #endif
    {
        #ifdef USE_OPENMP
            #pragma omp master
        #endif
        if( m_progressReporter )
        {
            m_progressReporter->KeepRefreshing();
        }

        #ifdef USE_OPENMP
            #pragma omp for schedule(dynamic)
        #endif
        for( unsigned i = 0; i < toFill.size(); i++ )
        {
            if( m_progressReporter )
            {
                m_progressReporter->AdvanceProgress();
            }

            toFill[i].m_zone->CacheTriangulation();
        }
    }
    if( m_progressReporter )
    {
        m_progressReporter->AdvancePhase();
        m_progressReporter->Report( _( "Committing changes..." ) );
    }

    connectivity->SetProgressReporter( nullptr );

    if( m_commit )
    {
        m_commit->Push( _( "Fill Zone(s)" ), false );
    }
    else
    {
        for( unsigned i = 0; i < toFill.size(); i++ )
        {
            connectivity->Update( toFill[i].m_zone );
        }
    }
}


void ZONE_FILLER::buildZoneFeatureHoleList( const ZONE_CONTAINER* aZone,
        SHAPE_POLY_SET& aFeatures ) const
{
    int segsPerCircle;
    double correctionFactor;

    // Set the number of segments in arc approximations
    if( aZone->GetArcSegmentCount() == ARC_APPROX_SEGMENTS_COUNT_HIGHT_DEF  )
        segsPerCircle = ARC_APPROX_SEGMENTS_COUNT_HIGHT_DEF;
    else
        segsPerCircle = ARC_APPROX_SEGMENTS_COUNT_LOW_DEF;

    /* calculates the coeff to compensate radius reduction of holes clearance
     * due to the segment approx.
     * For a circle the min radius is radius * cos( 2PI / s_CircleToSegmentsCount / 2)
     * s_Correction is 1 /cos( PI/s_CircleToSegmentsCount  )
     */
    correctionFactor = 1.0 / cos( M_PI / (double) segsPerCircle );

    aFeatures.RemoveAllContours();

    int outline_half_thickness = aZone->GetMinThickness() / 2;

    // When removing holes, the holes must be expanded by outline_half_thickness
    // to take in account the thickness of the zone outlines
    int zone_clearance = aZone->GetClearance() + outline_half_thickness;

    // When holes are created by non copper items (edge cut items), use only
    // the m_ZoneClearance parameter (zone clearance with no netclass clearance)
    int zone_to_edgecut_clearance = aZone->GetZoneClearance() + outline_half_thickness;

    /* store holes (i.e. tracks and pads areas as polygons outlines)
     * in a polygon list
     */

    /* items ouside the zone bounding box are skipped
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

    for( MODULE* module = m_board->m_Modules; module; module = module->Next() )
    {
        D_PAD* nextpad;

        for( D_PAD* pad = module->PadsList(); pad != NULL; pad = nextpad )
        {
            nextpad = pad->Next();      // pad pointer can be modified by next code, so
                                        // calculate the next pad here

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
                        outline.Inflate( KiROUND( clearance * correctionFactor ), segsPerCircle );
                        pad->CustomShapeAsPolygonToBoardPosition( &outline,
                                pad->GetPosition(), pad->GetOrientation() );

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
                        pad->TransformShapeWithClearanceToPolygon( aFeatures,
                                clearance,
                                segsPerCircle,
                                correctionFactor );
                }

                continue;
            }

            // Pads are removed from zone if the setup is PAD_ZONE_CONN_NONE
            // or if they have a custom shape, because a thermal relief will break
            // the shape
            if( aZone->GetPadConnection( pad ) == PAD_ZONE_CONN_NONE
                || pad->GetShape() == PAD_SHAPE_CUSTOM )
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
                        SHAPE_POLY_SET outline( pad->GetCustomShapeAsPolygon() );
                        outline.Inflate( KiROUND( gap * correctionFactor ), segsPerCircle );
                        pad->CustomShapeAsPolygonToBoardPosition( &outline,
                                pad->GetPosition(), pad->GetOrientation() );

                        std::vector<wxPoint> convex_hull;
                        BuildConvexHull( convex_hull, outline );

                        aFeatures.NewOutline();

                        for( unsigned ii = 0; ii < convex_hull.size(); ++ii )
                            aFeatures.Append( convex_hull[ii] );
                    }
                    else
                        pad->TransformShapeWithClearanceToPolygon( aFeatures,
                                gap, segsPerCircle, correctionFactor );
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
            track->TransformShapeWithClearanceToPolygon( aFeatures,
                    clearance,
                    segsPerCircle,
                    correctionFactor );
        }
    }

    /* Add module edge items that are on copper layers
     * Pcbnew allows these items to be on copper layers in microwave applictions
     * This is a bad thing, but must be handled here, until a better way is found
     */
    for( auto module : m_board->Modules() )
    {
        for( auto item : module->GraphicalItems() )
        {
            if( !item->IsOnLayer( aZone->GetLayer() ) && !item->IsOnLayer( Edge_Cuts ) )
                continue;

            if( item->Type() != PCB_MODULE_EDGE_T )
                continue;

            item_boundingbox = item->GetBoundingBox();

            if( item_boundingbox.Intersects( zone_boundingbox ) )
            {
                int zclearance = zone_clearance;

                if( item->IsOnLayer( Edge_Cuts ) )
                    // use only the m_ZoneClearance, not the clearance using
                    // the netclass value, because we do not have a copper item
                    zclearance = zone_to_edgecut_clearance;

                ( (EDGE_MODULE*) item )->TransformShapeWithClearanceToPolygon(
                        aFeatures, zclearance, segsPerCircle, correctionFactor );
            }
        }
    }

    // Add graphic items (copper texts) and board edges
    // Currently copper texts have no net, so only the zone_clearance
    // is used.
    for( auto item : m_board->Drawings() )
    {
        if( item->GetLayer() != aZone->GetLayer() && item->GetLayer() != Edge_Cuts )
            continue;

        int zclearance = zone_clearance;

        if( item->GetLayer() == Edge_Cuts )
            // use only the m_ZoneClearance, not the clearance using
            // the netclass value, because we do not have a copper item
            zclearance = zone_to_edgecut_clearance;

        switch( item->Type() )
        {
        case PCB_LINE_T:
            ( (DRAWSEGMENT*) item )->TransformShapeWithClearanceToPolygon(
                    aFeatures,
                    zclearance, segsPerCircle, correctionFactor );
            break;

        case PCB_TEXT_T:
            ( (TEXTE_PCB*) item )->TransformBoundingBoxWithClearanceToPolygon(
                    aFeatures, zclearance );
            break;

        default:
            break;
        }
    }

    // Add zones outlines having an higher priority and keepout
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

    // Remove thermal symbols
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

            item_boundingbox = pad->GetBoundingBox();
            int thermalGap = aZone->GetThermalReliefGap( pad );
            item_boundingbox.Inflate( thermalGap, thermalGap );

            if( item_boundingbox.Intersects( zone_boundingbox ) )
            {
                CreateThermalReliefPadPolygon( aFeatures,
                        *pad, thermalGap,
                        aZone->GetThermalReliefCopperBridge( pad ),
                        aZone->GetMinThickness(),
                        segsPerCircle,
                        correctionFactor, s_thermalRot );
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
    int segsPerCircle;
    double correctionFactor;
    int outline_half_thickness = aZone->GetMinThickness() / 2;

    std::unique_ptr<SHAPE_FILE_IO> dumper( new SHAPE_FILE_IO(
                    s_DumpZonesWhenFilling ? "zones_dump.txt" : "", SHAPE_FILE_IO::IOM_APPEND ) );

    // Set the number of segments in arc approximations
    if( aZone->GetArcSegmentCount() == ARC_APPROX_SEGMENTS_COUNT_HIGHT_DEF  )
        segsPerCircle = ARC_APPROX_SEGMENTS_COUNT_HIGHT_DEF;
    else
        segsPerCircle = ARC_APPROX_SEGMENTS_COUNT_LOW_DEF;

    /* calculates the coeff to compensate radius reduction of holes clearance
     * due to the segment approx.
     * For a circle the min radius is radius * cos( 2PI / s_CircleToSegmentsCount / 2)
     * s_Correction is 1 /cos( PI/s_CircleToSegmentsCount  )
     */
    correctionFactor = 1.0 / cos( M_PI / (double) segsPerCircle );

    if( s_DumpZonesWhenFilling )
        dumper->BeginGroup( "clipper-zone" );

    SHAPE_POLY_SET solidAreas = aSmoothedOutline;

    solidAreas.Inflate( -outline_half_thickness, segsPerCircle );
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

    if( s_DumpZonesWhenFilling )
        dumper->Write( &solidAreas, "solid-areas-minus-holes" );

    SHAPE_POLY_SET areas_fractured = solidAreas;
    areas_fractured.Fracture( SHAPE_POLY_SET::PM_FAST );

    if( s_DumpZonesWhenFilling )
        dumper->Write( &areas_fractured, "areas_fractured" );

    aFinalPolys = areas_fractured;

    SHAPE_POLY_SET thermalHoles;

    // Test thermal stubs connections and add polygons to remove unconnected stubs.
    // (this is a refinement for thermal relief shapes)
    if( aZone->GetNetCode() > 0 )
        BuildUnconnectedThermalStubsPolygonList( thermalHoles, m_board, aZone,
                correctionFactor, s_thermalRot );

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
        th_fractured.Fracture( SHAPE_POLY_SET::PM_FAST );

        if( s_DumpZonesWhenFilling )
            dumper->Write( &th_fractured, "th_fractured" );

        aFinalPolys = th_fractured;
    }

    aRawPolys = aFinalPolys;

    if( s_DumpZonesWhenFilling )
        dumper->EndGroup();
}

/* Build the filled solid areas data from real outlines (stored in m_Poly)
 * The solid areas can be more than one on copper layers, and do not have holes
  ( holes are linked by overlapping segments to the main outline)
 * aPcb: the current board (can be NULL for non copper zones)
 * aCornerBuffer: A reference to a buffer to store polygon corners, or NULL
 * if aCornerBuffer == NULL:
 * - m_FilledPolysList is used to store solid areas polygons.
 * - on copper layers, tracks and other items shapes of other nets are
 * removed from solid areas
 * if not null:
 * Only the zone outline (with holes, if any) are stored in aCornerBuffer
 * with holes linked. Therefore only one polygon is created
 * This function calls ComputeRawFilledAreas()
 * to add holes for pads and tracks and other items not in net.
 */
bool ZONE_FILLER::fillSingleZone( const ZONE_CONTAINER* aZone, SHAPE_POLY_SET& aRawPolys, SHAPE_POLY_SET& aFinalPolys, ZONE_SEGMENT_FILL& aSegmentFill ) const
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

        if( aZone->GetFillMode() == ZFM_SEGMENTS )   // if fill mode uses segments, create them:
        {
            if( !fillZoneWithSegments( aZone, aFinalPolys, aSegmentFill) )
                return false;
        }
    }
    else
    {
        aRawPolys = smoothedPoly;
        aFinalPolys = smoothedPoly;
        aFinalPolys.Inflate( -aZone->GetMinThickness() / 2, 16 );
        aFinalPolys.Fracture( SHAPE_POLY_SET::PM_FAST );
    }

    return true;
}

bool ZONE_FILLER::fillZoneWithSegments( const ZONE_CONTAINER* aZone, const SHAPE_POLY_SET& aFilledPolys, ZONE_SEGMENT_FILL& aFillSegs ) const
{
    bool success = true;
    // segments are on something like a grid. Give it a minimal size
    // to avoid too many segments, and use the m_ZoneMinThickness when (this is usually the case)
    // the size is > mingrid_size.
    // This is not perfect, but the actual purpose of this code
    // is to allow filling zones on a grid, with grid size > m_ZoneMinThickness,
    // in order to have really a grid.
    //
    // Using a user selectable grid size is for future Kicad versions.
    // For now the area is fully filled.
    int mingrid_size = Millimeter2iu( 0.05 );
    int grid_size = std::max( mingrid_size, aZone->GetMinThickness() );
    // Make segments slightly overlapping to ensure a good full filling
    grid_size -= grid_size/20;

    // Creates the horizontal segments
    for ( int index = 0; index < aFilledPolys.OutlineCount(); index++ )
    {
        const SHAPE_LINE_CHAIN& outline0 = aFilledPolys.COutline( index );
        success = fillPolygonWithHorizontalSegments( outline0, aFillSegs, grid_size );

        if( !success )
            break;

        // Creates the vertical segments. Because the filling algo creates horizontal segments,
        // to reuse the fillPolygonWithHorizontalSegments function, we rotate the polygons to fill
        // then fill them, then inverse rotate the result
        SHAPE_LINE_CHAIN outline90;
        outline90.Append( outline0 );

        // Rotate 90 degrees the outline:
        for( int ii = 0; ii < outline90.PointCount(); ii++ )
        {
            VECTOR2I& point = outline90.Point( ii );
            std::swap( point.x, point.y );
            point.y = -point.y;
        }

        int first_point = aFillSegs.size();
        success = fillPolygonWithHorizontalSegments( outline90, aFillSegs, grid_size );

        if( !success )
            break;

        // Rotate -90 degrees the segments:
        for( unsigned ii = first_point; ii < aFillSegs.size(); ii++ )
        {
            SEG& segm = aFillSegs[ii];
            std::swap( segm.A.x, segm.A.y );
            std::swap( segm.B.x, segm.B.y );
            segm.A.x = - segm.A.x;
            segm.B.x = - segm.B.x;
        }
    }

    return success;
}

/** Helper function fillPolygonWithHorizontalSegments
 * fills a polygon with horizontal segments.
 * It can be used for any angle, if the zone outline to fill is rotated by this angle
 * and the result is rotated by -angle
 * @param aPolygon = a SHAPE_LINE_CHAIN polygon to fill
 * @param aFillSegmList = a std::vector\<SEGMENT\> which will be populated by filling segments
 * @param aStep = the horizontal grid size
 */
bool ZONE_FILLER::fillPolygonWithHorizontalSegments( const SHAPE_LINE_CHAIN& aPolygon,
                                            ZONE_SEGMENT_FILL& aFillSegmList, int aStep ) const
{
    std::vector <int> x_coordinates;
    bool success = true;

    // Creates the horizontal segments
    const SHAPE_LINE_CHAIN& outline = aPolygon;
    const BOX2I& rect = outline.BBox();

    // Calculate the y limits of the zone
    for( int refy = rect.GetY(), endy = rect.GetBottom(); refy < endy; refy += aStep )
    {
        // find all intersection points of an infinite line with polyline sides
        x_coordinates.clear();

        for( int v = 0; v < outline.PointCount(); v++ )
        {

            int seg_startX = outline.CPoint( v ).x;
            int seg_startY = outline.CPoint( v ).y;
            int seg_endX   = outline.CPoint( v + 1 ).x;
            int seg_endY   = outline.CPoint( v + 1 ).y;

            /* Trivial cases: skip if ref above or below the segment to test */
            if( ( seg_startY > refy ) && ( seg_endY > refy ) )
                continue;

            // segment below ref point, or its Y end pos on Y coordinate ref point: skip
            if( ( seg_startY <= refy ) && (seg_endY <= refy ) )
                continue;

            /* at this point refy is between seg_startY and seg_endY
             * see if an horizontal line at Y = refy is intersecting this segment
             */
            // calculate the x position of the intersection of this segment and the
            // infinite line this is more easier if we move the X,Y axis origin to
            // the segment start point:

            seg_endX -= seg_startX;
            seg_endY -= seg_startY;
            double newrefy = (double) ( refy - seg_startY );
            double intersec_x;

            if ( seg_endY == 0 )    // horizontal segment on the same line: skip
                continue;

            // Now calculate the x intersection coordinate of the horizontal line at
            // y = newrefy and the segment from (0,0) to (seg_endX,seg_endY) with the
            // horizontal line at the new refy position the line slope is:
            // slope = seg_endY/seg_endX; and inv_slope = seg_endX/seg_endY
            // and the x pos relative to the new origin is:
            // intersec_x = refy/slope = refy * inv_slope
            // Note: because horizontal segments are already tested and skipped, slope
            // exists (seg_end_y not O)
            double inv_slope = (double) seg_endX / seg_endY;
            intersec_x = newrefy * inv_slope;
            x_coordinates.push_back( (int) intersec_x + seg_startX );
        }

        // A line scan is finished: build list of segments

        // Sort intersection points by increasing x value:
        // So 2 consecutive points are the ends of a segment
        std::sort( x_coordinates.begin(), x_coordinates.end() );

        // An even number of coordinates is expected, because a segment has 2 ends.
        // An if this algorithm always works, it must always find an even count.
        if( ( x_coordinates.size() & 1 ) != 0 )
        {
            success = false;
            break;
        }

        // Create segments having the same Y coordinate
        int iimax = x_coordinates.size() - 1;

        for( int ii = 0; ii < iimax; ii += 2 )
        {
            VECTOR2I  seg_start, seg_end;
            seg_start.x = x_coordinates[ii];
            seg_start.y = refy;
            seg_end.x = x_coordinates[ii + 1];
            seg_end.y = refy;
            SEG segment( seg_start, seg_end );
            aFillSegmList.push_back( segment );
        }
    }   // End examine segments in one area

    return success;
}
