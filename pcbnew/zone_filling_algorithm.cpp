/**
 * @file zone_filling_algorithm.cpp:
 * Algorithms used to fill a zone defined by a polygon and a filling starting point.
 */
/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2016 KiCad Developers, see AUTHORS.txt for contributors.
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


#include <algorithm> // sort

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
#include <geometry/convex_hull.h>

#include <connectivity_data.h>


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
// so we use fast mode when possible (intermediate calculations)
// (choice is SHAPE_POLY_SET::PM_STRICTLY_SIMPLE or SHAPE_POLY_SET::PM_FAST)
#define POLY_CALC_MODE SHAPE_POLY_SET::PM_FAST

/* DEBUG OPTION:
 * To emit zone data to a file when filling zones for the debugging purposes,
 * set this 'true' and build.
 */
static const bool s_DumpZonesWhenFilling = false;

extern void BuildUnconnectedThermalStubsPolygonList( SHAPE_POLY_SET& aCornerBuffer,
                                                     BOARD* aPcb, ZONE_CONTAINER* aZone,
                                                     double aArcCorrection,
                                                     double aRoundPadThermalRotation);


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

bool ZONE_CONTAINER::BuildFilledSolidAreasPolygons( BOARD* aPcb, SHAPE_POLY_SET* aOutlineBuffer )
{
    /* convert outlines + holes to outlines without holes (adding extra segments if necessary)
     * m_Poly data is expected normalized, i.e. NormalizeAreaOutlines was used after building
     * this zone
     */

    if( GetNumCorners() <= 2 )  // malformed zone. polygon calculations do not like it ...
        return false;

    // Make a smoothed polygon out of the user-drawn polygon if required
    if( m_smoothedPoly )
    {
        delete m_smoothedPoly;
        m_smoothedPoly = NULL;
    }

    switch( m_cornerSmoothingType )
    {
    case ZONE_SETTINGS::SMOOTHING_CHAMFER:
        m_smoothedPoly = new SHAPE_POLY_SET();
        *m_smoothedPoly = m_Poly->Chamfer( m_cornerRadius );
        break;

    case ZONE_SETTINGS::SMOOTHING_FILLET:
        m_smoothedPoly = new SHAPE_POLY_SET();
        *m_smoothedPoly = m_Poly->Fillet( m_cornerRadius, m_ArcToSegmentsCount );
        break;

    default:
        // Acute angles between adjacent edges can create issues in calculations,
        // in inflate/deflate outlines transforms, especially when the angle is very small.
        // We can avoid issues by creating a very small chamfer which remove acute angles,
        // or left it without chamfer and use only CPOLYGONS_LIST::InflateOutline to create
        // clearance areas
        m_smoothedPoly = new SHAPE_POLY_SET();
        *m_smoothedPoly = m_Poly->Chamfer( Millimeter2iu( 0.0 ) );
        break;
    }

    if( aOutlineBuffer )
        aOutlineBuffer->Append( *m_smoothedPoly );

    /* For copper layers, we now must add holes in the Polygon list.
     * holes are pads and tracks with their clearance area
     * For non copper layers, just recalculate the m_FilledPolysList
     * with m_ZoneMinThickness taken in account
     */
    else
    {
        m_FilledPolysList.RemoveAllContours();

        if( IsOnCopperLayer() )
        {
            ComputeRawFilledAreas( aPcb );

            if( m_FillMode )   // if fill mode uses segments, create them:
            {
                if( !FillZoneAreasWithSegments() )
                    return false;
            }
        }
        else
        {
            m_FillMode = 0;     // Fill by segments is no more used in non copper layers
                                // force use solid polygons (usefull only for old boards)
            m_FilledPolysList = *m_smoothedPoly;

            // The filled areas are deflated by -m_ZoneMinThickness / 2, because
            // the outlines are drawn with a line thickness = m_ZoneMinThickness to
            // give a good shape with the minimal thickness
            m_FilledPolysList.Inflate( -m_ZoneMinThickness / 2, 16 );
            m_FilledPolysList.Fracture( SHAPE_POLY_SET::PM_FAST );
        }

        m_IsFilled = true;
    }

    return true;
}


/** Helper function fillPolygonWithHorizontalSegments
 * fills a polygon with horizontal segments.
 * It can be used for any angle, if the zone outline to fill is rotated by this angle
 * and the result is rotated by -angle
 * @param aPolygon = a SHAPE_LINE_CHAIN polygon to fill
 * @param aFillSegmList = a std::vector\<SEGMENT\> which will be populated by filling segments
 * @param aStep = the horizontal grid size
 */
bool fillPolygonWithHorizontalSegments( const SHAPE_LINE_CHAIN& aPolygon,
                                 std::vector <SEGMENT>& aFillSegmList, int aStep );

bool ZONE_CONTAINER::FillZoneAreasWithSegments()
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
    int grid_size = std::max( mingrid_size, m_ZoneMinThickness );
    // Make segments slightly overlapping to ensure a good full filling
    grid_size -= grid_size/20;

    // All filled areas are in m_FilledPolysList
    // m_FillSegmList will contain the horizontal and vertical segments
    // the segment width is m_ZoneMinThickness.
    m_FillSegmList.clear();

    // Creates the horizontal segments
    for ( int index = 0; index < m_FilledPolysList.OutlineCount(); index++ )
    {
        const SHAPE_LINE_CHAIN& outline0 = m_FilledPolysList.COutline( index );
        success = fillPolygonWithHorizontalSegments( outline0, m_FillSegmList, grid_size );

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

        int first_point = m_FillSegmList.size();
        success = fillPolygonWithHorizontalSegments( outline90, m_FillSegmList, grid_size );

        if( !success )
            break;

        // Rotate -90 degrees the segments:
        for( unsigned ii = first_point; ii < m_FillSegmList.size(); ii++ )
        {
            SEGMENT& segm = m_FillSegmList[ii];
            std::swap( segm.m_Start.x, segm.m_Start.y );
            std::swap( segm.m_End.x, segm.m_End.y );
            segm.m_Start.x = - segm.m_Start.x;
            segm.m_End.x = - segm.m_End.x;
        }
    }

    if( success )
        m_IsFilled = true;
    else
        m_FillSegmList.clear();

    return success;
}


bool fillPolygonWithHorizontalSegments( const SHAPE_LINE_CHAIN& aPolygon,
                                        std::vector <SEGMENT>& aFillSegmList, int aStep )
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
            wxPoint  seg_start, seg_end;
            seg_start.x = x_coordinates[ii];
            seg_start.y = refy;
            seg_end.x = x_coordinates[ii + 1];
            seg_end.y = refy;
            SEGMENT segment( seg_start, seg_end );
            aFillSegmList.push_back( segment );
        }
    }   // End examine segments in one area

    return success;
}

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

    // When removing holes, the holes must be expanded by outline_half_thickness
    // to take in account the thickness of the zone outlines
    int zone_clearance = GetClearance() + outline_half_thickness;

    // When holes are created by non copper items (edge cut items), use only
    // the m_ZoneClearance parameter (zone clearance with no netclass clearance)
    int zone_to_edgecut_clearance = GetZoneClearance() + outline_half_thickness;

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

    /* Use a dummy pad to calculate hole clearance when a pad is not on all copper layers
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

        for( D_PAD* pad = module->PadsList(); pad != NULL; pad = nextpad )
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
                int item_clearance = pad->GetClearance() + outline_half_thickness;
                item_boundingbox = pad->GetBoundingBox();
                item_boundingbox.Inflate( item_clearance );

                if( item_boundingbox.Intersects( zone_boundingbox ) )
                {
                    int clearance = std::max( zone_clearance, item_clearance );

                    // PAD_SHAPE_CUSTOM can have a specific keepout, to avoid to break the shape
                    if( pad->GetShape() == PAD_SHAPE_CUSTOM &&
                        pad->GetCustomShapeInZoneOpt() == CUST_PAD_SHAPE_IN_ZONE_CONVEXHULL )
                    {
                        // the pad shape in zone can be its convex hull or
                        // the shape itself
                        SHAPE_POLY_SET outline( pad->GetCustomShapeAsPolygon() );
                        outline.Inflate( KiROUND( clearance*correctionFactor) , segsPerCircle );
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
            if( GetPadConnection( pad ) == PAD_ZONE_CONN_NONE ||
                pad->GetShape() == PAD_SHAPE_CUSTOM )
            {
                int gap = zone_clearance;
                int thermalGap = GetThermalReliefGap( pad );
                gap = std::max( gap, thermalGap );
                item_boundingbox = pad->GetBoundingBox();
                item_boundingbox.Inflate( gap );

                if( item_boundingbox.Intersects( zone_boundingbox ) )
                {
                    // PAD_SHAPE_CUSTOM has a specific keepout, to avoid to break the shape
                    // the pad shape in zone can be its convex hull or the shape itself
                    if( pad->GetShape() == PAD_SHAPE_CUSTOM &&
                        pad->GetCustomShapeInZoneOpt() == CUST_PAD_SHAPE_IN_ZONE_CONVEXHULL )
                    {
                        // the pad shape in zone can be its convex hull or
                        // the shape itself
                        SHAPE_POLY_SET outline( pad->GetCustomShapeAsPolygon() );
                        outline.Inflate( KiROUND( gap*correctionFactor) , segsPerCircle );
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
    for( TRACK* track = aPcb->m_Track;  track;  track = track->Next() )
    {
        if( !track->IsOnLayer( GetLayer() ) )
            continue;

        if( track->GetNetCode() == GetNetCode()  && (GetNetCode() != 0) )
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
    for( MODULE* module = aPcb->m_Modules;  module;  module = module->Next() )
    {
        for( BOARD_ITEM* item = module->GraphicalItemsList();  item;  item = item->Next() )
        {
            if( !item->IsOnLayer( GetLayer() ) && !item->IsOnLayer( Edge_Cuts ) )
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
    for( auto item : aPcb->Drawings() )
    {
        if( item->GetLayer() != GetLayer() && item->GetLayer() != Edge_Cuts )
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
    for( int ii = 0; ii < GetBoard()->GetAreaCount(); ii++ )
    {
        ZONE_CONTAINER* zone = GetBoard()->GetArea( ii );

        // If the zones share no common layers
        if( !CommonLayerExists( zone->GetLayerSet() ) )
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
                    aFeatures, min_clearance, use_net_clearance );
    }

   // Remove thermal symbols
    for( MODULE* module = aPcb->m_Modules;  module;  module = module->Next() )
    {
        for( D_PAD* pad = module->PadsList(); pad != NULL; pad = pad->Next() )
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

void ZONE_CONTAINER::ComputeRawFilledAreas( BOARD* aPcb )
{
    int segsPerCircle;
    double correctionFactor;
    int outline_half_thickness = m_ZoneMinThickness / 2;


    std::unique_ptr<SHAPE_FILE_IO> dumper( new SHAPE_FILE_IO(
            s_DumpZonesWhenFilling ? "zones_dump.txt" : "", SHAPE_FILE_IO::IOM_APPEND ) );

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

    if(s_DumpZonesWhenFilling)
        dumper->BeginGroup("clipper-zone");

    SHAPE_POLY_SET solidAreas = *m_smoothedPoly;

    solidAreas.Inflate( -outline_half_thickness, segsPerCircle );
    solidAreas.Simplify( POLY_CALC_MODE );

    SHAPE_POLY_SET holes;

    if(s_DumpZonesWhenFilling)
        dumper->Write( &solidAreas, "solid-areas" );

    tmp.RemoveAllContours();
    buildFeatureHoleList( aPcb, holes );

    if(s_DumpZonesWhenFilling)
        dumper->Write( &holes, "feature-holes" );

    holes.Simplify( POLY_CALC_MODE );

    if (s_DumpZonesWhenFilling)
        dumper->Write( &holes, "feature-holes-postsimplify" );

    // Generate the filled areas (currently, without thermal shapes, which will
    // be created later).
    // Use SHAPE_POLY_SET::PM_STRICTLY_SIMPLE to generate strictly simple polygons
    // needed by Gerber files and Fracture()
    solidAreas.BooleanSubtract( holes, SHAPE_POLY_SET::PM_STRICTLY_SIMPLE );

    if (s_DumpZonesWhenFilling)
        dumper->Write( &solidAreas, "solid-areas-minus-holes" );

    SHAPE_POLY_SET areas_fractured = solidAreas;
    areas_fractured.Fracture( POLY_CALC_MODE );

    if (s_DumpZonesWhenFilling)
        dumper->Write( &areas_fractured, "areas_fractured" );

    m_FilledPolysList = areas_fractured;

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
        // Remove unconnected stubs. Use SHAPE_POLY_SET::PM_STRICTLY_SIMPLE to
        // generate strictly simple polygons
        // needed by Gerber files and Fracture()
        solidAreas.BooleanSubtract( thermalHoles, SHAPE_POLY_SET::PM_STRICTLY_SIMPLE );

        if( s_DumpZonesWhenFilling )
            dumper->Write( &thermalHoles, "thermal-holes" );

        // put these areas in m_FilledPolysList
        SHAPE_POLY_SET th_fractured = solidAreas;
        th_fractured.Fracture( POLY_CALC_MODE );

        if( s_DumpZonesWhenFilling )
            dumper->Write ( &th_fractured, "th_fractured" );

        m_FilledPolysList = th_fractured;

    }

    m_RawPolysList = m_FilledPolysList;

    if(s_DumpZonesWhenFilling)
        dumper->EndGroup();
}

void ZONE_CONTAINER::RemoveInsulatedCopperIslands( BOARD* aPcb )
{
    std::vector<int> islands;

    auto connectivity = aPcb->GetConnectivity();

    connectivity->FindIsolatedCopperIslands( this, islands );

    std::sort( islands.begin(), islands.end(), std::greater<int>() );

    for( auto idx : islands )
    {
        m_FilledPolysList.DeletePolygon( idx );
    }

    connectivity->Update( this );
}
