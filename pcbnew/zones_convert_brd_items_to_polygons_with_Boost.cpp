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
#include <sstream>

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
#include <project.h>

#include <pcbnew.h>
#include <zones.h>
#include <convert_basic_shapes_to_polygon.h>

#include <geometry/shape_poly_set.h>
#include <geometry/shape_file_io.h>

#include <boost/foreach.hpp>

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

void ZONE_CONTAINER::buildFeatureHoleList( BOARD* aPcb, CPOLYGONS_LIST& aFeatures )
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
                dummypad.SetShape( pad->GetDrillShape() == PAD_DRILL_OBLONG ?
                                   PAD_OVAL : PAD_CIRCLE );
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

            if( GetPadConnection( pad ) == PAD_NOT_IN_ZONE )
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

static const SHAPE_POLY_SET convertPolyListToPolySet(const CPOLYGONS_LIST& aList)
{
    SHAPE_POLY_SET rv;

    unsigned    corners_count = aList.GetCornersCount();

    // Enter main outline: this is the first contour
    unsigned    ic = 0;

    if(!corners_count)
        return rv;

    while( ic < corners_count )
    {
        rv.NewOutline( );

        while( ic < corners_count )
        {
            rv.AppendVertex( aList.GetX(ic), aList.GetY(ic) );
            if( aList.IsEndContour( ic ) )
                break;

            ic++;
        }
        ic++;
    }

    return rv;
}

static const CPOLYGONS_LIST convertPolySetToPolyList(const SHAPE_POLY_SET& aPolyset)
{
    CPOLYGONS_LIST list;

    CPolyPt corner, firstCorner;

    for( int ii = 0; ii < aPolyset.OutlineCount(); ii++ )
    {

        for( int jj = 0; jj < aPolyset.VertexCount(ii); jj++ )
        {
            VECTOR2I v = aPolyset.GetVertex( jj, ii );


            corner.x    = v.x;
            corner.y    = v.y;
            corner.end_contour = false;

            if(!jj)
                firstCorner = corner;

            list.AddCorner( corner );
        }

        firstCorner.end_contour = true;
        list.AddCorner( firstCorner );
    }

    return list;

}

static const SHAPE_POLY_SET convertBoostToPolySet ( const KI_POLYGON_SET& aSet )
{
    SHAPE_POLY_SET rv;

    BOOST_FOREACH ( const KI_POLYGON &poly, aSet )
    {
        rv.NewOutline();
        for ( KI_POLYGON::iterator_type corner = poly.begin(); corner != poly.end(); ++ corner )
        {
            rv.AppendVertex ( corner->x(), corner->y() );
        }
    }

    return rv;
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

    std::auto_ptr<SHAPE_FILE_IO> dumper( new SHAPE_FILE_IO( "zones_dump.txt", true ) );

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

    m_smoothedPoly->m_CornersList.InflateOutline( tmp, -outline_half_thickness, true );
    SHAPE_POLY_SET solidAreas = convertPolyListToPolySet( tmp );

    if(g_DumpZonesWhenFilling)
        dumper->Write( &solidAreas, "solid-areas" );


    tmp.RemoveAllContours();
    buildFeatureHoleList( aPcb, tmp );
    SHAPE_POLY_SET holes = convertPolyListToPolySet( tmp );

    if(g_DumpZonesWhenFilling)
        dumper->Write( &holes, "feature-holes" );

    holes.Simplify();

    if (g_DumpZonesWhenFilling)
        dumper->Write( &holes, "feature-holes-postsimplify" );

    solidAreas.Subtract( holes );

    if (g_DumpZonesWhenFilling)
        dumper->Write( &solidAreas, "solid-areas-minus-holes" );

    m_FilledPolysList.RemoveAllContours();

    SHAPE_POLY_SET fractured = solidAreas;
    fractured.Fracture();

    if (g_DumpZonesWhenFilling)
        dumper->Write( &fractured, "fractured" );

    m_FilledPolysList =  convertPolySetToPolyList( fractured );

    if (g_DumpZonesWhenFilling)
    {
        SHAPE_POLY_SET dupa = convertPolyListToPolySet ( m_FilledPolysList );
        dumper->Write( &dupa, "verify-conv" );
    }


    // Remove insulated islands:
    if( GetNetCode() > 0 )
        TestForCopperIslandAndRemoveInsulatedIslands( aPcb );

    tmp.RemoveAllContours();
    // Test thermal stubs connections and add polygons to remove unconnected stubs.
    // (this is a refinement for thermal relief shapes)
    if( GetNetCode() > 0 )
        BuildUnconnectedThermalStubsPolygonList( tmp, aPcb, this,
                                                 correctionFactor, s_thermalRot );

    // remove copper areas corresponding to not connected stubs
    if( tmp.GetCornersCount() )
    {
        SHAPE_POLY_SET thermalHoles = convertPolyListToPolySet ( tmp );
        thermalHoles.Simplify();
        // Remove unconnected stubs
        solidAreas.Subtract ( thermalHoles );

        if (g_DumpZonesWhenFilling)
            dumper->Write ( &thermalHoles, "thermal-holes" );


        // put these areas in m_FilledPolysList
        m_FilledPolysList.RemoveAllContours();

        SHAPE_POLY_SET fractured = solidAreas;
        fractured.Fracture();

        if (g_DumpZonesWhenFilling)
            dumper->Write ( &fractured, "fractured" );

        m_FilledPolysList = convertPolySetToPolyList( fractured );

        if( GetNetCode() > 0 )
            TestForCopperIslandAndRemoveInsulatedIslands( aPcb );
    }

    if(g_DumpZonesWhenFilling)
        dumper->EndGroup();
}

void ZONE_CONTAINER::AddClearanceAreasPolygonsToPolysList( BOARD* aPcb )
{
    int segsPerCircle;
    double correctionFactor;

    std::auto_ptr<SHAPE_FILE_IO> dumper( new SHAPE_FILE_IO( "zones_dump.txt", true ) );


    if(g_DumpZonesWhenFilling)
        dumper->BeginGroup("boost-zone");

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

    // this is a place to store holes (i.e. tracks, pads ... areas as polygons outlines)
    // static to avoid unnecessary memory allocation when filling many zones.
    CPOLYGONS_LIST cornerBufferPolysToSubstract;

    // This KI_POLYGON_SET is the area(s) to fill, with m_ZoneMinThickness/2
    KI_POLYGON_SET polyset_zone_solid_areas;



    int outline_half_thickness = m_ZoneMinThickness / 2;

    /* First, creates the main polygon (i.e. the filled area using only one outline)
     * to reserve a m_ZoneMinThickness/2 margin around the outlines and holes
     * this margin is the room to redraw outlines with segments having a width set to
     * m_ZoneMinThickness
     * so m_ZoneMinThickness is the min thickness of the filled zones areas
     * the main polygon is stored in polyset_zone_solid_areas
     */
    CPOLYGONS_LIST tmp;
    m_smoothedPoly->m_CornersList.InflateOutline( tmp, -outline_half_thickness, true );
    tmp.ExportTo( polyset_zone_solid_areas );

    if( polyset_zone_solid_areas.size() == 0 )
        return;

    if (g_DumpZonesWhenFilling)
        dumper->Write ( convertBoostToPolySet( polyset_zone_solid_areas ), "solid-areas" );

    buildFeatureHoleList( aPcb, cornerBufferPolysToSubstract );

    // cornerBufferPolysToSubstract contains polygons to substract.
    // polyset_zone_solid_areas contains the main filled area
    // Calculate now actual solid areas
    if( cornerBufferPolysToSubstract.GetCornersCount() > 0 )
    {
        KI_POLYGON_SET polyset_holes;

        cornerBufferPolysToSubstract.ExportTo( polyset_holes );

        if (g_DumpZonesWhenFilling)
            dumper->Write ( convertBoostToPolySet( polyset_holes ), "feature-holes" );

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
                                                 correctionFactor, s_thermalRot );

    // remove copper areas corresponding to not connected stubs
    if( cornerBufferPolysToSubstract.GetCornersCount() )
    {
        KI_POLYGON_SET polyset_holes;
        cornerBufferPolysToSubstract.ExportTo( polyset_holes );

        if (g_DumpZonesWhenFilling)
            dumper->Write ( convertBoostToPolySet( polyset_holes ), "thermal-holes" );

        // Remove unconnected stubs
        polyset_zone_solid_areas -= polyset_holes;

        // put these areas in m_FilledPolysList
        m_FilledPolysList.RemoveAllContours();
        CopyPolygonsFromKiPolygonListToFilledPolysList( polyset_zone_solid_areas );

        if( GetNetCode() > 0 )
            TestForCopperIslandAndRemoveInsulatedIslands( aPcb );
    }


    if (g_DumpZonesWhenFilling)
        dumper->Write ( convertBoostToPolySet( polyset_zone_solid_areas ), "complete" );

    cornerBufferPolysToSubstract.RemoveAllContours();
}


void ZONE_CONTAINER::CopyPolygonsFromKiPolygonListToFilledPolysList( KI_POLYGON_SET& aKiPolyList )
{
    m_FilledPolysList.RemoveAllContours();
    m_FilledPolysList.ImportFrom( aKiPolyList );
}
