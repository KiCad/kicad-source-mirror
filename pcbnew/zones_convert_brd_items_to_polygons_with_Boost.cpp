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

#include <cmath>

#include <fctsys.h>
#include <polygons_defs.h>
#include <wxPcbStruct.h>
#include <trigo.h>

#include <class_board.h>
#include <class_module.h>
#include <class_track.h>
#include <class_edge_mod.h>
#include <class_drawsegment.h>
#include <class_pcb_text.h>
#include <class_zone.h>

#include <pcbnew.h>
#include <zones.h>
#include <convert_basic_shapes_to_polygon.h>


extern void BuildUnconnectedThermalStubsPolygonList( CPOLYGONS_LIST& aCornerBuffer,
                                                     BOARD* aPcb, ZONE_CONTAINER* aZone,
                                                     double aArcCorrection,
                                                     double aRoundPadThermalRotation);

extern void Test_For_Copper_Island_And_Remove( BOARD*          aPcb,
                                               ZONE_CONTAINER* aZone_container );

extern void CreateThermalReliefPadPolygon( CPOLYGONS_LIST& aCornerBuffer,
                                           D_PAD&                aPad,
                                           int                   aThermalGap,
                                           int                   aCopperThickness,
                                           int                   aMinThicknessValue,
                                           int                   aCircleToSegmentsCount,
                                           double                aCorrectionFactor,
                                           double                aThermalRot );

// Local Variables:
static double s_thermalRot = 450;  // angle of stubs in thermal reliefs for round pads

// how many segments are used to create a polygon from a circle:
static int s_CircleToSegmentsCount = ARC_APPROX_SEGMENTS_COUNT_LOW_DEF;   /* default value. the real value will be changed to
                                                                           * ARC_APPROX_SEGMENTS_COUNT_HIGHT_DEF
                                                                           * if m_ArcToSegmentsCount == ARC_APPROX_SEGMENTS_COUNT_HIGHT_DEF
                                                                           */
double s_Correction;     /* mult coeff used to enlarge rounded and oval pads (and vias)
                          * because the segment approximation for arcs and circles
                          * create a smaller gap than a true circle
                          */

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
void ZONE_CONTAINER::AddClearanceAreasPolygonsToPolysList( BOARD* aPcb )
{
    // Set the number of segments in arc approximations
    if( m_ArcToSegmentsCount == ARC_APPROX_SEGMENTS_COUNT_HIGHT_DEF  )
        s_CircleToSegmentsCount = ARC_APPROX_SEGMENTS_COUNT_HIGHT_DEF;
    else
        s_CircleToSegmentsCount = ARC_APPROX_SEGMENTS_COUNT_LOW_DEF;

    /* calculates the coeff to compensate radius reduction of holes clearance
     * due to the segment approx.
     * For a circle the min radius is radius * cos( 2PI / s_CircleToSegmentsCount / 2)
     * s_Correction is 1 /cos( PI/s_CircleToSegmentsCount  )
     */
    s_Correction = 1.0 / cos( M_PI / s_CircleToSegmentsCount );

    // This KI_POLYGON_SET is the area(s) to fill, with m_ZoneMinThickness/2
    KI_POLYGON_SET polyset_zone_solid_areas;
    int         margin = m_ZoneMinThickness / 2;

    /* First, creates the main polygon (i.e. the filled area using only one outline)
     * to reserve a m_ZoneMinThickness/2 margin around the outlines and holes
     * this margin is the room to redraw outlines with segments having a width set to
     * m_ZoneMinThickness
     * so m_ZoneMinThickness is the min thickness of the filled zones areas
     * the main polygon is stored in polyset_zone_solid_areas
     */

    CopyPolygonsFromFilledPolysListToKiPolygonList( polyset_zone_solid_areas );
    polyset_zone_solid_areas -= margin;

    if( polyset_zone_solid_areas.size() == 0 )
        return;

    /* Calculates the clearance value that meet DRC requirements
     * from m_ZoneClearance and clearance from the corresponding netclass
     * We have a "local" clearance in zones because most of time
     * clearance between a zone and others items is bigger than the netclass clearance
     * this is more true for small clearance values
     * Note also the "local" clearance is used for clearance between non copper items
     *    or items like texts on copper layers
     */
    int zone_clearance = std::max( m_ZoneClearance, GetClearance() );
    zone_clearance += margin;

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

    // static to avoid unnecessary memory allocation when filling many zones.
    static CPOLYGONS_LIST cornerBufferPolysToSubstract;
    cornerBufferPolysToSubstract.RemoveAllContours();

    /* Use a dummy pad to calculate hole clerance when a pad is not on all copper layers
     * and this pad has a hole
     * This dummy pad has the size and shape of the hole
    * Therefore, this dummy pad is a circle or an oval.
     * A pad must have a parent because some functions expect a non null parent
     * to find the parent board, and some other data
     */
    MODULE dummymodule( aPcb );    // Creates a dummy parent
    D_PAD dummypad( &dummymodule );
    D_PAD* nextpad;
    for( MODULE* module = aPcb->m_Modules;  module;  module = module->Next() )
    {
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
                dummypad.SetShape( pad->GetDrillShape() == PAD_DRILL_OBLONG ?
                                   PAD_OVAL : PAD_CIRCLE );
                dummypad.SetPosition( pad->GetPosition() );

                pad = &dummypad;
            }

            if( pad->GetNetCode() != GetNetCode() )
            {
                item_clearance   = pad->GetClearance() + margin;
                item_boundingbox = pad->GetBoundingBox();
                item_boundingbox.Inflate( item_clearance );

                if( item_boundingbox.Intersects( zone_boundingbox ) )
                {
                    int clearance = std::max( zone_clearance, item_clearance );
                    pad->TransformShapeWithClearanceToPolygon( cornerBufferPolysToSubstract,
                                                               clearance,
                                                               s_CircleToSegmentsCount,
                                                               s_Correction );
                }

                continue;
            }

            int gap = zone_clearance;

            if( ( GetPadConnection( pad ) == PAD_NOT_IN_ZONE )
                || ( GetNetCode() == 0 ) || ( pad->GetShape() == PAD_TRAPEZOID ) )

            // PAD_TRAPEZOID shapes are not in zones because they are used in microwave apps
            // and i think it is good that shapes are not changed by thermal pads or others
            {
                item_boundingbox = pad->GetBoundingBox();

                if( item_boundingbox.Intersects( zone_boundingbox ) )
                {
                    pad->TransformShapeWithClearanceToPolygon( cornerBufferPolysToSubstract,
                                                               gap,
                                                               s_CircleToSegmentsCount,
                                                               s_Correction );
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

        item_clearance   = track->GetClearance() + margin;
        item_boundingbox = track->GetBoundingBox();

        if( item_boundingbox.Intersects( zone_boundingbox ) )
        {
            int clearance = std::max( zone_clearance, item_clearance );
            track->TransformShapeWithClearanceToPolygon( cornerBufferPolysToSubstract,
                                                         clearance,
                                                         s_CircleToSegmentsCount,
                                                         s_Correction );
        }
    }

    /* Add module edge items that are on copper layers
     * Pcbnew allows these items to be on copper layers in microwave applictions
     * This is a bad thing, but must be handle here, until a better way is found
     */
    for( MODULE* module = aPcb->m_Modules;  module;  module = module->Next() )
    {
        for( BOARD_ITEM* item = module->GraphicalItems();  item;  item = item->Next() )
        {
            if( !item->IsOnLayer( GetLayer() ) )
                continue;

            if( item->Type() != PCB_MODULE_EDGE_T )
                continue;

            item_boundingbox = item->GetBoundingBox();

            if( item_boundingbox.Intersects( zone_boundingbox ) )
            {
                ( (EDGE_MODULE*) item )->TransformShapeWithClearanceToPolygon(
                    cornerBufferPolysToSubstract, zone_clearance,
                    s_CircleToSegmentsCount, s_Correction );
            }
        }
    }

    // Add graphic items (copper texts) and board edges
    for( BOARD_ITEM* item = aPcb->m_Drawings; item; item = item->Next() )
    {
        if( item->GetLayer() != GetLayer() && item->GetLayer() != EDGE_N )
            continue;

        switch( item->Type() )
        {
        case PCB_LINE_T:
            ( (DRAWSEGMENT*) item )->TransformShapeWithClearanceToPolygon(
                cornerBufferPolysToSubstract,
                zone_clearance, s_CircleToSegmentsCount, s_Correction );
            break;

        case PCB_TEXT_T:
            ( (TEXTE_PCB*) item )->TransformBoundingBoxWithClearanceToPolygon(
                cornerBufferPolysToSubstract, zone_clearance );
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

        // A highter priority zone or keepout area is found: remove its area
        item_boundingbox = zone->GetBoundingBox();
        if( !item_boundingbox.Intersects( zone_boundingbox ) )
            continue;

        // Add the zone outline area.
        // However if the zone has the same net as the current zone,
        // do not add clearance.
        // the zone will be connected to the current zone, but filled areas
        // will use different parameters (clearance, thermal shapes )
        bool addclearance = GetNetCode() != zone->GetNetCode();
        int clearance = zone_clearance;

        if( zone->GetIsKeepout() )
        {
            addclearance = true;
            clearance = m_ZoneMinThickness / 2;
        }

        zone->TransformOutlinesShapeWithClearanceToPolygon(
                    cornerBufferPolysToSubstract,
                    clearance, addclearance );
    }

   // Remove thermal symbols
    for( MODULE* module = aPcb->m_Modules;  module;  module = module->Next() )
    {
        for( D_PAD* pad = module->Pads(); pad != NULL; pad = pad->Next() )
        {
            // Rejects non-standard pads with tht-only thermal reliefs
            if( GetPadConnection( pad ) == THT_THERMAL
             && pad->GetAttribute() != PAD_STANDARD )
                continue;

            if( GetPadConnection( pad ) != THERMAL_PAD
             && GetPadConnection( pad ) != THT_THERMAL )
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
                CreateThermalReliefPadPolygon( cornerBufferPolysToSubstract,
                                               *pad, thermalGap,
                                               GetThermalReliefCopperBridge( pad ),
                                               m_ZoneMinThickness,
                                               s_CircleToSegmentsCount,
                                               s_Correction, s_thermalRot );
            }
        }
    }

    // cornerBufferPolysToSubstract contains polygons to substract.
    // polyset_zone_solid_areas contains the main filled area
    // Calculate now actual solid areas
    if( cornerBufferPolysToSubstract.GetCornersCount() > 0 )
    {
        KI_POLYGON_SET polyset_holes;
        cornerBufferPolysToSubstract.ExportTo( polyset_holes );
        // Remove holes from initial area.:
        polyset_zone_solid_areas -= polyset_holes;
    }

    // put solid areas in m_FilledPolysList:
    m_FilledPolysList.RemoveAllContours();
    CopyPolygonsFromKiPolygonListToFilledPolysList( polyset_zone_solid_areas );

    // Remove insulated islands:
    if( GetNetCode() > 0 )
        TestForCopperIslandAndRemoveInsulatedIslands( aPcb );

    // Now we remove all unused thermal stubs.
    cornerBufferPolysToSubstract.RemoveAllContours();

    // Test thermal stubs connections and add polygons to remove unconnected stubs.
    // (this is a refinement for thermal relief shapes)
    if( GetNetCode() > 0 )
        BuildUnconnectedThermalStubsPolygonList( cornerBufferPolysToSubstract, aPcb, this,
                                                 s_Correction, s_thermalRot );

    // remove copper areas corresponding to not connected stubs
    if( cornerBufferPolysToSubstract.GetCornersCount() )
    {
        KI_POLYGON_SET polyset_holes;
        cornerBufferPolysToSubstract.ExportTo( polyset_holes );

        // Remove unconnected stubs
        polyset_zone_solid_areas -= polyset_holes;

        // put these areas in m_FilledPolysList
        m_FilledPolysList.RemoveAllContours();
        CopyPolygonsFromKiPolygonListToFilledPolysList( polyset_zone_solid_areas );

        if( GetNetCode() > 0 )
            TestForCopperIslandAndRemoveInsulatedIslands( aPcb );
    }

    cornerBufferPolysToSubstract.RemoveAllContours();
}


void ZONE_CONTAINER::CopyPolygonsFromKiPolygonListToFilledPolysList( KI_POLYGON_SET& aKiPolyList )
{
    m_FilledPolysList.RemoveAllContours();
    m_FilledPolysList.ImportFrom( aKiPolyList );
}


void ZONE_CONTAINER::CopyPolygonsFromFilledPolysListToKiPolygonList( KI_POLYGON_SET& aKiPolyList )
{
    m_FilledPolysList.ExportTo( aKiPolyList );
}
