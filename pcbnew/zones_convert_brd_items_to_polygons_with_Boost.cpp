/**
 * @file zones_convert_brd_items_to_polygons_with_Boost.cpp
 */
/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 1992-2012 KiCad Developers, see AUTHORS.txt for contributors.
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

/* Functions to convert some board items to polygons
 * (pads, tracks ..)
 * This is used to calculate filled areas in copper zones.
 * Filled areas are areas remainder of the full zone area after removed all polygons
 * calculated from these items shapes and the clearance area
 *
 * Important note:
 * Because filled areas must have a minimum thickness to match with Design rule, they are
 * draw in 2 step:
 * 1 - filled polygons are drawn
 * 2 - polygon outlines are drawn with a "minimum thickness width" ( or with a minimum
 *     thickness pen )
 * So outlines of filled polygons are calculated with the constraint they match with clearance,
 * taking in account outlines have thickness
 * This ensures:
 *      - areas meet the minimum thickness requirement.
 *      - shapes are smoothed.
 */

// Polygon calculations can use fast mode or force strickly simple polygons after calculations
// Forcing strickly simple polygons is time consuming, and we have not see issues in fast mode
// so we use fast mode
#define POLY_CALC_MODE SHAPE_POLY_SET::PM_FAST

#include <cmath>
#include <sstream>

#include <fctsys.h>
#include <wxPcbStruct.h>
#include <trigo.h>

#include <class_board.h>
#include <class_module.h>
#include <class_track.h>
#include <class_edge_mod.h>
#include <class_drawsegment.h>
#include <class_pcb_text.h>
#include <class_zone.h>
#include <project.h>

#include <pcbnew.h>
#include <zones.h>
#include <convert_basic_shapes_to_polygon.h>

#include <geometry/shape_poly_set.h>
#include <geometry/shape_file_io.h>

#include <boost/foreach.hpp>

extern void BuildUnconnectedThermalStubsPolygonList( SHAPE_POLY_SET& aCornerBuffer,
                                                     BOARD* aPcb, ZONE_CONTAINER* aZone,
                                                     double aArcCorrection,
                                                     double aRoundPadThermalRotation);

extern void Test_For_Copper_Island_And_Remove( BOARD*          aPcb,
                                               ZONE_CONTAINER* aZone_container );

extern void CreateThermalReliefPadPolygon( SHAPE_POLY_SET&       aCornerBuffer,
                                           D_PAD&                aPad,
                                           int                   aThermalGap,
                                           int                   aCopperThickness,
                                           int                   aMinThicknessValue,
                                           int                   aCircleToSegmentsCount,
                                           double                aCorrectionFactor,
                                           double                aThermalRot );

// Local Variables:
static double s_thermalRot = 450;  // angle of stubs in thermal reliefs for round pads

void ZONE_CONTAINER::buildFeatureHoleList( BOARD* aPcb, SHAPE_POLY_SET& aFeatures )
{
    int segsPerCircle;
    double correctionFactor;

    // Set the number of segments in arc approximations
    if( m_ArcToSegmentsCount == ARC_APPROX_SEGMENTS_COUNT_HIGHT_DEF  )
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

    int outline_half_thickness = m_ZoneMinThickness / 2;

    int zone_clearance = std::max( m_ZoneClearance, GetClearance() );
    zone_clearance += outline_half_thickness;

    /* store holes (i.e. tracks and pads areas as polygons outlines)
     * in a polygon list
     */

    /* items ouside the zone bounding box are skipped
     * the bounding box is the zone bounding box + the biggest clearance found in Netclass list
     */
    EDA_RECT item_boundingbox;
    EDA_RECT zone_boundingbox  = GetBoundingBox();
    int      biggest_clearance = aPcb->GetDesignSettings().GetBiggestClearanceValue();
    biggest_clearance = std::max( biggest_clearance, zone_clearance );
    zone_boundingbox.Inflate( biggest_clearance );

    /*
     * First : Add pads. Note: pads having the same net as zone are left in zone.
     * Thermal shapes will be created later if necessary
     */
    int item_clearance;

    /* Use a dummy pad to calculate hole clerance when a pad is not on all copper layers
     * and this pad has a hole
     * This dummy pad has the size and shape of the hole
    * Therefore, this dummy pad is a circle or an oval.
     * A pad must have a parent because some functions expect a non null parent
     * to find the parent board, and some other data
     */
    MODULE dummymodule( aPcb );    // Creates a dummy parent
    D_PAD dummypad( &dummymodule );

    for( MODULE* module = aPcb->m_Modules;  module;  module = module->Next() )
    {
        D_PAD* nextpad;

        for( D_PAD* pad = module->Pads(); pad != NULL; pad = nextpad )
        {
            nextpad = pad->Next();  // pad pointer can be modified by next code, so
                                    // calculate the next pad here

            if( !pad->IsOnLayer( GetLayer() ) )
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
            if( ( pad->GetNetCode() != GetNetCode() ) || ( pad->GetNetCode() <= 0 ) )
            {
                item_clearance   = pad->GetClearance() + outline_half_thickness;
                item_boundingbox = pad->GetBoundingBox();
                item_boundingbox.Inflate( item_clearance );

                if( item_boundingbox.Intersects( zone_boundingbox ) )
                {
                    int clearance = std::max( zone_clearance, item_clearance );
                    pad->TransformShapeWithClearanceToPolygon( aFeatures,
                                                               clearance,
                                                               segsPerCircle,
                                                               correctionFactor );
                }

                continue;
            }

            if( GetPadConnection( pad ) == PAD_ZONE_CONN_NONE )
            {
                int gap = zone_clearance;
                int thermalGap = GetThermalReliefGap( pad );
                gap = std::max( gap, thermalGap );
                item_boundingbox = pad->GetBoundingBox();

                if( item_boundingbox.Intersects( zone_boundingbox ) )
                {
                    pad->TransformShapeWithClearanceToPolygon( aFeatures,
                                                               gap,
                                                               segsPerCircle,
                                                               correctionFactor );
                }
            }
        }
    }

    /* Add holes (i.e. tracks and vias areas as polygons outlines)
     * in cornerBufferPolysToSubstract
     */
    for( TRACK* track = aPcb->m_Track;  track;  track = track->Next() )
    {
        if( !track->IsOnLayer( GetLayer() ) )
            continue;

        if( track->GetNetCode() == GetNetCode()  && (GetNetCode() != 0) )
            continue;

        item_clearance   = track->GetClearance() + outline_half_thickness;
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
    for( MODULE* module = aPcb->m_Modules;  module;  module = module->Next() )
    {
        for( BOARD_ITEM* item = module->GraphicalItems();  item;  item = item->Next() )
        {
            if( !item->IsOnLayer( GetLayer() ) && !item->IsOnLayer( Edge_Cuts ) )
                continue;

            if( item->Type() != PCB_MODULE_EDGE_T )
                continue;

            item_boundingbox = item->GetBoundingBox();

            if( item_boundingbox.Intersects( zone_boundingbox ) )
            {
                ( (EDGE_MODULE*) item )->TransformShapeWithClearanceToPolygon(
                    aFeatures, zone_clearance,
                    segsPerCircle, correctionFactor );
            }
        }
    }

    // Add graphic items (copper texts) and board edges
    for( BOARD_ITEM* item = aPcb->m_Drawings; item; item = item->Next() )
    {
        if( item->GetLayer() != GetLayer() && item->GetLayer() != Edge_Cuts )
            continue;

        switch( item->Type() )
        {
        case PCB_LINE_T:
            ( (DRAWSEGMENT*) item )->TransformShapeWithClearanceToPolygon(
                aFeatures,
                zone_clearance, segsPerCircle, correctionFactor );
            break;

        case PCB_TEXT_T:
            ( (TEXTE_PCB*) item )->TransformBoundingBoxWithClearanceToPolygon(
                aFeatures, zone_clearance );
            break;

        default:
            break;
        }
    }

    // Add zones outlines having an higher priority and keepout
    for( int ii = 0; ii < GetBoard()->GetAreaCount(); ii++ )
    {
        ZONE_CONTAINER* zone = GetBoard()->GetArea( ii );
        if( zone->GetLayer() != GetLayer() )
            continue;

        if( !zone->GetIsKeepout() && zone->GetPriority() <= GetPriority() )
            continue;

        if( zone->GetIsKeepout() && ! zone->GetDoNotAllowCopperPour() )
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
        bool same_net = GetNetCode() == zone->GetNetCode();
        bool use_net_clearance = true;
        int min_clearance = zone_clearance;

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
                    aFeatures,
                    min_clearance, use_net_clearance );
    }

   // Remove thermal symbols
    for( MODULE* module = aPcb->m_Modules;  module;  module = module->Next() )
    {
        for( D_PAD* pad = module->Pads(); pad != NULL; pad = pad->Next() )
        {
            // Rejects non-standard pads with tht-only thermal reliefs
            if( GetPadConnection( pad ) == PAD_ZONE_CONN_THT_THERMAL
             && pad->GetAttribute() != PAD_ATTRIB_STANDARD )
                continue;

            if( GetPadConnection( pad ) != PAD_ZONE_CONN_THERMAL
             && GetPadConnection( pad ) != PAD_ZONE_CONN_THT_THERMAL )
                continue;

            if( !pad->IsOnLayer( GetLayer() ) )
                continue;

            if( pad->GetNetCode() != GetNetCode() )
                continue;
            item_boundingbox = pad->GetBoundingBox();
            int thermalGap = GetThermalReliefGap( pad );
            item_boundingbox.Inflate( thermalGap, thermalGap );

            if( item_boundingbox.Intersects( zone_boundingbox ) )
            {
                CreateThermalReliefPadPolygon( aFeatures,
                                               *pad, thermalGap,
                                               GetThermalReliefCopperBridge( pad ),
                                               m_ZoneMinThickness,
                                               segsPerCircle,
                                               correctionFactor, s_thermalRot );
            }
        }
    }

}


/**
 * Function AddClearanceAreasPolygonsToPolysList
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

void ZONE_CONTAINER::AddClearanceAreasPolygonsToPolysList_NG( BOARD* aPcb )
{
    int segsPerCircle;
    double correctionFactor;
    int outline_half_thickness = m_ZoneMinThickness / 2;


    std::auto_ptr<SHAPE_FILE_IO> dumper( new SHAPE_FILE_IO(
            g_DumpZonesWhenFilling ? "zones_dump.txt" : "", SHAPE_FILE_IO::IOM_APPEND ) );

    // Set the number of segments in arc approximations
    if( m_ArcToSegmentsCount == ARC_APPROX_SEGMENTS_COUNT_HIGHT_DEF  )
        segsPerCircle = ARC_APPROX_SEGMENTS_COUNT_HIGHT_DEF;
    else
        segsPerCircle = ARC_APPROX_SEGMENTS_COUNT_LOW_DEF;

    /* calculates the coeff to compensate radius reduction of holes clearance
     * due to the segment approx.
     * For a circle the min radius is radius * cos( 2PI / s_CircleToSegmentsCount / 2)
     * s_Correction is 1 /cos( PI/s_CircleToSegmentsCount  )
     */
    correctionFactor = 1.0 / cos( M_PI / (double) segsPerCircle );

    CPOLYGONS_LIST tmp;

    if(g_DumpZonesWhenFilling)
        dumper->BeginGroup("clipper-zone");

    SHAPE_POLY_SET solidAreas = ConvertPolyListToPolySet( m_smoothedPoly->m_CornersList );

    solidAreas.Inflate( -outline_half_thickness, segsPerCircle );
    solidAreas.Simplify( POLY_CALC_MODE );

    SHAPE_POLY_SET holes;

    if(g_DumpZonesWhenFilling)
        dumper->Write( &solidAreas, "solid-areas" );

    tmp.RemoveAllContours();
    buildFeatureHoleList( aPcb, holes );

    if(g_DumpZonesWhenFilling)
        dumper->Write( &holes, "feature-holes" );

    holes.Simplify( POLY_CALC_MODE );

    if (g_DumpZonesWhenFilling)
        dumper->Write( &holes, "feature-holes-postsimplify" );

    solidAreas.BooleanSubtract( holes, POLY_CALC_MODE );

    if (g_DumpZonesWhenFilling)
        dumper->Write( &solidAreas, "solid-areas-minus-holes" );

    SHAPE_POLY_SET fractured = solidAreas;
    fractured.Fracture( POLY_CALC_MODE );

    if (g_DumpZonesWhenFilling)
        dumper->Write( &fractured, "fractured" );

    m_FilledPolysList = fractured;

    // Remove insulated islands:
    if( GetNetCode() > 0 )
        TestForCopperIslandAndRemoveInsulatedIslands( aPcb );

    SHAPE_POLY_SET thermalHoles;

    // Test thermal stubs connections and add polygons to remove unconnected stubs.
    // (this is a refinement for thermal relief shapes)
    if( GetNetCode() > 0 )
        BuildUnconnectedThermalStubsPolygonList( thermalHoles, aPcb, this,
                                                 correctionFactor, s_thermalRot );

    // remove copper areas corresponding to not connected stubs
    if( !thermalHoles.IsEmpty() )
    {
        thermalHoles.Simplify( POLY_CALC_MODE );
        // Remove unconnected stubs
        solidAreas.BooleanSubtract( thermalHoles, POLY_CALC_MODE );

        if( g_DumpZonesWhenFilling )
            dumper->Write( &thermalHoles, "thermal-holes" );

        // put these areas in m_FilledPolysList
        SHAPE_POLY_SET fractured = solidAreas;
        fractured.Fracture( POLY_CALC_MODE );

        if( g_DumpZonesWhenFilling )
            dumper->Write ( &fractured, "fractured" );

        m_FilledPolysList = fractured;

        if( GetNetCode() > 0 )
            TestForCopperIslandAndRemoveInsulatedIslands( aPcb );
    }

    if(g_DumpZonesWhenFilling)
        dumper->EndGroup();
}

void ZONE_CONTAINER::AddClearanceAreasPolygonsToPolysList( BOARD* aPcb )
{
}
