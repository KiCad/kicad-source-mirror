/*******************************************/
/* zones_convert_brd_items_to_polygons.cpp */
/*******************************************/

/* Functions to convert some board items to polygons
(pads, tracks ..)
This is used to calculate filled areas in copper zones.
Filled areas are the full zone area sub all polygons calculated from these items and the clearance area
*/

using namespace std;

#include <math.h>
#include <vector>

#include "fctsys.h"
#include "common.h"
#include "pcbnew.h"
#include "trigo.h"

#include "zones.h"


#include "PolyLine.h"

extern void Test_For_Copper_Island_And_Remove( BOARD* aPcb, ZONE_CONTAINER* aZone_container );


// Local Functions:
void        AddTrackWithClearancePolygon( Bool_Engine* aBooleng,
                                          TRACK& aTrack, int aClearanceValue );
void        AddPadWithClearancePolygon( Bool_Engine* aBooleng, D_PAD& aPad, int aClearanceValue );
void        AddThermalReliefPadPolygon( Bool_Engine* aBooleng,
                                        D_PAD&       aPad,
                                        int          aThermalGap,
                                        int          aCopperThickness );
void        AddRoundedEndsSegmentPolygon( Bool_Engine* aBooleng,
                                          wxPoint aStart, wxPoint aEnd,
                                          int aWidth );
void        AddTextBoxWithClearancePolygon( Bool_Engine* aBooleng,
                                            TEXTE_PCB* aText, int aClearanceValue );


// Local Variables:
/* how many segments are used to create a polygon from a circle: */
static int s_CircleToSegmentsCount = 16;   /* default value. the real value will be changed to 32
                                            * if g_Zone_Arc_Approximation == 1
                                            */
double     s_Correction; /* mult coeff used to enlarge rounded and oval pads (and vias)
                          * because the segment approximation for arcs and circles
                          * create a smaller gap than a true circle
                          */

/** function AddClearanceAreasPolygonsToPolysList
 * Add non copper areas polygons (pads and tracks with clearence)
 * to a filled copper area
 * used in BuildFilledPolysListData when calculating filled areas in a zone
 * Non copper areas are pads and track and their clearance area
 * The filled copper area must be computed just before.
 * BuildFilledPolysListData() call this function just after creating the
 *  filled copper area polygon (without clearence areas
 * to do that this function:
 * 1 - creates a Bool_Engine,with option: holes are linked to outer contours by double overlapping segments
 *       this means the created polygons have no holes (hole are linked to outer outline by double overlapped segments
 *       and are therefore compatible with draw functions (DC draw polygons and Gerber or PS outputs)
 * 2 - Add the main outline (zone outline) in group A
 * 3 - Add all non filled areas (pads, tracks) in group B
 * 4 - calculates the polygon A - B
 * 5 - put resulting list of polygons (filled areas) in m_FilledPolysList
 * 6 - Remove insulated copper islands
 */
void ZONE_CONTAINER::AddClearanceAreasPolygonsToPolysList( BOARD* aPcb )
{
    // Set the number of segments in arc approximations
    if( m_ArcToSegmentsCount == 32  )
        s_CircleToSegmentsCount = 32;
    else
        s_CircleToSegmentsCount = 16;

    /* calculates the coeff to compensate radius reduction of holes clearance
     * due to the segment approx.
     * For a circle the min radius is radius * cos( 2PI / s_CircleToSegmentsCount / 2)
     * s_Correction is 1 /cos( PI/s_CircleToSegmentsCount  )
     */
    s_Correction = 1.0 / cos( 3.14159265 / s_CircleToSegmentsCount );

    /* Uses a kbool engine to add holes in the m_FilledPolysList polygon.
     * Because this function is called just after creating the m_FilledPolysList,
     * only one polygon is in list.
     * (initial holes in zonesare linked into outer contours by double overlapping segments).
     * after adding holes, many polygons could be exist in this list.
     */

    Bool_Engine* booleng = new Bool_Engine();

    ArmBoolEng( booleng, true );

    /* Add the main polygon (i.e. the filled area using only one outline)
     * in GroupA in Bool_Engine
     */
    unsigned corners_count = m_FilledPolysList.size();
    unsigned ic = 0;
    if( booleng->StartPolygonAdd( GROUP_A ) )
    {
        for( ; ic < corners_count; ic++ )
        {
            CPolyPt* corner = &m_FilledPolysList[ic];
            booleng->AddPoint( corner->x, corner->y );
            if( corner->end_contour )
                break;
        }

        booleng->EndPolygonAdd();
    }

    // Calculates the clearance value that meet DRC requirements
    int      clearance = max( m_ZoneClearance, g_DesignSettings.m_TrackClearence );

    /* Add holes (i.e. tracks and pads areas as polygons outlines)
     * in GroupB in Bool_Engine
     */
    /* items ouside the zone bounding box are skipped */
    EDA_Rect item_boundingbox;
    EDA_Rect zone_boundingbox = GetBoundingBox();
    zone_boundingbox.Inflate( m_ZoneClearance, clearance );

    /*
     * First : Add pads
     */
    for( MODULE* module = aPcb->m_Modules;  module;  module = module->Next() )
    {
        for( D_PAD* pad = module->m_Pads; pad != NULL; pad = pad->Next() )
        {
            if( !pad->IsOnLayer( GetLayer() ) )
                continue;

            if( pad->GetNet() != GetNet() )
            {
                item_boundingbox = pad->GetBoundingBox();
                if( item_boundingbox.Intersects( zone_boundingbox ) )
                    AddPadWithClearancePolygon( booleng, *pad, clearance );
                continue;
            }

            switch( m_PadOption )
            {
            case PAD_NOT_IN_ZONE:
                item_boundingbox = pad->GetBoundingBox();
                if( item_boundingbox.Intersects( zone_boundingbox ) )
                    AddPadWithClearancePolygon( booleng, *pad, clearance );
                break;

            case THERMAL_PAD:
                item_boundingbox = pad->GetBoundingBox();
                item_boundingbox.Inflate( m_ThermalReliefGapValue, m_ThermalReliefGapValue );
                if( item_boundingbox.Intersects( zone_boundingbox ) )
                    AddThermalReliefPadPolygon( booleng, *pad,
                        m_ThermalReliefGapValue, m_ThermalReliefCopperBridgeValue );
                break;

            case PAD_IN_ZONE:
                break;
            }
        }
    }

    /* Add holes (i.e. tracks and pads areas as polygons outlines)
     * in GroupB in Bool_Engine
     * Next : Add tracks and vias
     */
    for( TRACK* track = aPcb->m_Track;  track;  track = track->Next() )
    {
        if( !track->IsOnLayer( GetLayer() ) )
            continue;
        if( track->GetNet() == GetNet() )
            continue;
        item_boundingbox = track->GetBoundingBox();
        if( item_boundingbox.Intersects( zone_boundingbox ) )
            AddTrackWithClearancePolygon( booleng, *track, clearance );
    }

    // Draw graphic items (copper texts) and board edges
    // zone clearance is used here regardless of the g_DesignSettings.m_TrackClearence value
    for( BOARD_ITEM* item = aPcb->m_Drawings;  item;  item = item->Next() )
    {
        if( item->GetLayer() != GetLayer() && item->GetLayer() != EDGE_N )
            continue;

        switch( item->Type() )
        {
        case TYPEDRAWSEGMENT:
            AddRoundedEndsSegmentPolygon( booleng,
                ( (DRAWSEGMENT*) item )->m_Start,
                ( (DRAWSEGMENT*) item )->m_End,
                ( (DRAWSEGMENT*) item )->m_Width + (2 * m_ZoneClearance) );
            break;

        case TYPETEXTE:
            if( ( (TEXTE_PCB*) item )->GetLength() == 0 )
                break;
            AddTextBoxWithClearancePolygon( booleng, (TEXTE_PCB*) item, m_ZoneClearance );
            break;

        default:
            break;
        }
    }

    /* compute copper areas */
    booleng->Do_Operation( BOOL_A_SUB_B );

    /* put these areas in m_FilledPolysList */
    m_FilledPolysList.clear();
    while( booleng->StartPolygonGet() )
    {
        CPolyPt corner( 0, 0, false );
        while( booleng->PolygonHasMorePoints() )
        {
            corner.x = (int) booleng->GetPolygonXPoint();
            corner.y = (int) booleng->GetPolygonYPoint();
            corner.end_contour = false;
            m_FilledPolysList.push_back( corner );
        }

        corner.end_contour = true;
        m_FilledPolysList.pop_back();
        m_FilledPolysList.push_back( corner );
        booleng->EndPolygonGet();
    }

    delete booleng;

    // Remove insulated islands:
    if( GetNet() > 0 )
        Test_For_Copper_Island_And_Remove_Insulated_Islands( aPcb );
}


/** Function AddPadPolygonWithPadClearance
 * Add a polygon cutout for a pad in a zone area
 * Convert arcs and circles to multiple straight lines
 */
void AddPadWithClearancePolygon( Bool_Engine* aBooleng,
                                 D_PAD& aPad, int aClearanceValue )
{
    if( aBooleng->StartPolygonAdd( GROUP_B ) == 0 )
        return;
    wxPoint corner_position;
    int     ii, angle;
    int     dx = (aPad.m_Size.x / 2) + aClearanceValue;
    int     dy = (aPad.m_Size.y / 2) + aClearanceValue;

    int     delta = 3600 / s_CircleToSegmentsCount; // rot angle in 0.1 degree
    wxPoint PadShapePos = aPad.ReturnShapePos();    /* Note: for pad having a shape offset,
                                                     * the pad position is NOT the shape position */

    switch( aPad.m_PadShape )
    {
    case PAD_CIRCLE:
        dx = (int) (dx * s_Correction);
        for( ii = 0; ii < s_CircleToSegmentsCount; ii++ )
        {
            corner_position = wxPoint( dx, 0 );
            angle = ii * delta;
            RotatePoint( &corner_position, angle );
            corner_position += PadShapePos;
            aBooleng->AddPoint( corner_position.x, corner_position.y );
        }

        break;

    case PAD_OVAL:
        angle = aPad.m_Orient;
        if( dy > dx )                                                   // Oval pad X/Y ratio for choosing translation axles
        {
            dy = (int) (dy * s_Correction);
            int     angle_pg;                                           // Polygon angle
            wxPoint shape_offset = wxPoint( 0, (dy - dx) );
            RotatePoint( &shape_offset, angle );                        // Rotating shape offset vector with component

            for( ii = 0; ii < s_CircleToSegmentsCount / 2 + 1; ii++ )   // Half circle end cap...
            {
                corner_position = wxPoint( dx, 0 );                     // Coordinate translation +dx
                RotatePoint( &corner_position, angle );
                angle_pg = ii * delta;
                RotatePoint( &corner_position, angle_pg );
                corner_position += PadShapePos - shape_offset;
                aBooleng->AddPoint( corner_position.x, corner_position.y );
            }

            for( ii = 0; ii < s_CircleToSegmentsCount / 2 + 1; ii++ )   // Second half circle end cap...
            {
                corner_position = wxPoint( -dx, 0 );                    // Coordinate translation -dx
                RotatePoint( &corner_position, angle );
                angle_pg = ii * delta;
                RotatePoint( &corner_position, angle_pg );
                corner_position += PadShapePos + shape_offset;
                aBooleng->AddPoint( corner_position.x, corner_position.y );
            }

            break;
        }
        else    //if( dy <= dx )
        {
            dx = (int) (dx * s_Correction);
            int     angle_pg; // Polygon angle
            wxPoint shape_offset = wxPoint( (dy - dx), 0 );
            RotatePoint( &shape_offset, angle );

            for( ii = 0; ii < s_CircleToSegmentsCount / 2 + 1; ii++ )
            {
                corner_position = wxPoint( 0, dy );
                RotatePoint( &corner_position, angle );
                angle_pg = ii * delta;
                RotatePoint( &corner_position, angle_pg );
                corner_position += PadShapePos - shape_offset;
                aBooleng->AddPoint( corner_position.x, corner_position.y );
            }

            for( ii = 0; ii < s_CircleToSegmentsCount / 2 + 1; ii++ )
            {
                corner_position = wxPoint( 0, -dy );
                RotatePoint( &corner_position, angle );
                angle_pg = ii * delta;
                RotatePoint( &corner_position, angle_pg );
                corner_position += PadShapePos + shape_offset;
                aBooleng->AddPoint( corner_position.x, corner_position.y );
            }

            break;
        }

    case PAD_RECT:
        angle = aPad.m_Orient;
        corner_position = wxPoint( -dx, -dy );
        RotatePoint( &corner_position, angle );
        corner_position += PadShapePos;
        aBooleng->AddPoint( corner_position.x, corner_position.y );

        corner_position = wxPoint( -dx, +dy );
        RotatePoint( &corner_position, angle );
        corner_position += aPad.ReturnShapePos();
        aBooleng->AddPoint( corner_position.x, corner_position.y );

        corner_position = wxPoint( +dx, +dy );
        RotatePoint( &corner_position, angle );
        corner_position += PadShapePos;
        aBooleng->AddPoint( corner_position.x, corner_position.y );

        corner_position = wxPoint( +dx, -dy );
        RotatePoint( &corner_position, angle );
        corner_position += PadShapePos;
        aBooleng->AddPoint( corner_position.x, corner_position.y );
        break;
    }

    aBooleng->EndPolygonAdd();
}


/** function AddThermalReliefPadPolygon
 * Add holes around a pad to create a thermal relief
 * copper tickness is min (dx/2, aCopperWitdh) or min (dy/2, aCopperWitdh)
 * gap is aThermalGap
 */

/* thermal reliefs are created as 4 polygons.
 * each corner of a polygon if calculated for a pad at position 0, 0, orient 0,
 * and then moved and rotated acroding to the pad position and orientation
 */

/* WARNING:
 * When Kbool calculates the filled areas :
 * i.e when substarcting holes (thermal shapes) to the full zone area
 * under certains circumstances kboll drop some holes.
 * These circumstances are:
 * some identical holes (same thermal shape and size) are *exactly* on the same vertical line
 * And
 * nothing else between holes
 * And
 * angles less than 90 deg between 2 consecutive lines in hole outline
 * And a hole above the identical holes
 *
 * In fact, it is easy to find these conditions in pad arrays.
 * So to avoid this, the workaround is do not use holes outlines that include
 * angles less than 90 deg between 2 consecutive lines
 * this is made in round and oblong thermal reliefs
 */
void    AddThermalReliefPadPolygon( Bool_Engine* aBooleng,
                                    D_PAD&       aPad,
                                    int          aThermalGap,
                                    int          aCopperThickness )
{
    wxPoint corner, corner_end;
    wxPoint PadShapePos = aPad.ReturnShapePos();    /* Note: for pad having a shape offset,
                                                     * the pad position is NOT the shape position */
    int     angle = 0;
    wxSize  copper_tickness;
    int     dx = aPad.m_Size.x / 2;
    int     dy = aPad.m_Size.y / 2;

    int     delta = 3600 / s_CircleToSegmentsCount; // rot angle in 0.1 degree

    copper_tickness.x = min( dx, aCopperThickness );
    copper_tickness.y = min( dy, aCopperThickness );

    switch( aPad.m_PadShape )
    {
    case PAD_CIRCLE:    // Add 4 similar holes
    {
        /* we create 4 copper holes and put them in position 1, 2, 3 and 4
         * here is the area of the rectangular pad + its thermal gap
         * the 4 copper holes remove the copper in order to create the thermal gap
         * 4 ------ 1
         * |        |
         * |        |
         * |        |
         * |        |
         * 3 ------ 2
         * holes 2, 3, 4 are the same as hole 1, rotated 90, 180, 270 deg
         */

        // Build the hole pattern, for the hole in the X >0, Y > 0 plane:
        // The pattern roughtly is a 90 deg arc pie
        std::vector <wxPoint> corners_buffer;

        // Radius of outer arcs of the shape:
        int outer_radius = dx + aThermalGap;     // The radius of the outer arc is pad radius + aThermalGap

        // Crosspoint of thermal spoke sides, the first point of polygon buffer
        corners_buffer.push_back( wxPoint( copper_tickness.x / 2, copper_tickness.y / 2 ) );

        // Add an intermediate point on spoke sides, to allow a > 90 deg angle between side and first seg of arc approx
        corner.x = copper_tickness.x / 2;
        int y = outer_radius - (aThermalGap / 4);
        corner.y = (int) sqrt( ( ( (double) y * y ) - (double) corner.x * corner.x ) );
        corners_buffer.push_back( corner );

        // calculate the starting point of the outter arc
        corner.x = copper_tickness.x / 2;
        double dtmp = sqrt( ( (double) outer_radius * outer_radius ) - ( (double) corner.x * corner.x ) );
        corner.y = (int) dtmp;
        RotatePoint( &corner, 90 );

        // calculate the ending point of the outter arc
        corner_end.x = corner.y;
        corner_end.y = corner.x;

        // calculate intermediate points (y coordinate from corner.y to corner_end.y
        while( (corner.y > corner_end.y)  && (corner.x < corner_end.x) )
        {
            corners_buffer.push_back( corner );
            RotatePoint( &corner, delta );
        }

        corners_buffer.push_back( corner_end );

        /* add an intermediate point, to avoid angles < 90 deg between last arc approx line and radius line
         */
        corner.x = corners_buffer[1].y;
        corner.y = corners_buffer[1].x;
        corners_buffer.push_back( corner );

        // Now, add the 4 holes ( each is the pattern, rotated by 0, 90, 180 and 270  deg
        // WARNING: problems with kbool if angle = 0 (in fact when angle < 200):
        // bad filled polygon on some cases, when pads are on a same vertical line
        // this seems a bug in kbool polygon (exists in 1.9 kbool version)
        // angle = 450 (45.0 degrees orientation) seems work fine.
        // angle = 0 with thermal shapes without angle < 90 deg seems works fine also
        angle = 0;
        int angle_pad = aPad.m_Orient;              // Pad orientation
        for( unsigned ihole = 0; ihole < 4; ihole++ )
        {
            if( aBooleng->StartPolygonAdd( GROUP_B ) )
            {
                for( unsigned ii = 0; ii < corners_buffer.size(); ii++ )
                {
                    corner = corners_buffer[ii];
                    RotatePoint( &corner, angle + angle_pad );      // Rotate by segment angle and pad orientation
                    corner += PadShapePos;
                    aBooleng->AddPoint( corner.x, corner.y );
                }

                aBooleng->EndPolygonAdd();

                angle += 900;   // Note: angle in in 0.1 deg.
            }
        }
    }
        break;

    case PAD_OVAL:
    {
        // Oval pad support along the lines of round and rectangular pads
        std::vector <wxPoint> corners_buffer;               // Polygon buffer as vector

        int     dx = (aPad.m_Size.x / 2) + aThermalGap;     // Cutout radius x
        int     dy = (aPad.m_Size.y / 2) + aThermalGap;     // Cutout radius y

        wxPoint shape_offset;

        // We want to calculate an oval shape with dx > dy.
        // if this is not the case, exchange dx and dy, and rotate the shape 90 deg.
        int supp_angle = 0;
        if( dx < dy )
        {
            EXCHG( dx, dy );
            supp_angle = 900;
            EXCHG( copper_tickness.x, copper_tickness.y);
        }
        int deltasize = dx - dy;        // = distance between shape position and the 2 demi-circle ends centre
        // here we have dx > dy
        // Radius of outer arcs of the shape:
        int outer_radius = dy;     // The radius of the outer arc is radius end + aThermalGap

        // Some coordinate fiddling, depending on the shape offset direction
        shape_offset = wxPoint( deltasize, 0 );

        // Crosspoint of thermal spoke sides, the first point of polygon buffer
        corners_buffer.push_back( wxPoint( copper_tickness.x / 2, copper_tickness.y / 2 ) );

        // Arc start point calculation, the intersecting point of cutout arc and thermal spoke edge
        if( copper_tickness.x > deltasize )          // If copper thickness is more than shape offset, we need to calculate arc intercept point.
        {
            corner.x = copper_tickness.x / 2;
            corner.y =
                (int) sqrt( ((double)outer_radius * outer_radius ) -
                    ( (double)( corner.x - delta ) * ( corner.x - deltasize ) ) );
            corner.x -= deltasize;

            /* creates an intermediate point, to have a > 90 deg angle
            * between the side and the first segment of arc approximation
            */
            wxPoint intpoint = corner;
            intpoint.y -= aThermalGap/4;
            corners_buffer.push_back( intpoint + shape_offset );
            RotatePoint( &corner, 90 );
        }
        else
        {
            corner.x = copper_tickness.x / 2;
            corner.y = outer_radius;
            corners_buffer.push_back( corner );
            corner.x = ( deltasize - copper_tickness.x ) / 2;
        }

        // Add an intermediate point on spoke sides, to allow a > 90 deg angle between side and first seg of arc approx
        wxPoint last_corner;
        last_corner.y = copper_tickness.y / 2;
        int px = outer_radius - (aThermalGap / 4);
        last_corner.x = (int) sqrt( ( ( (double) px * px ) - (double) last_corner.y * last_corner.y ) );

        // Arc stop point calculation, the intersecting point of cutout arc and thermal spoke edge
        corner_end.y = copper_tickness.y / 2;
        corner_end.x = (int) sqrt( ( (double) outer_radius * outer_radius ) - ( (double) corner_end.y * corner_end.y ) );
        RotatePoint( &corner_end, -90 );

        // calculate intermediate arc points till limit is reached
        while( (corner.y > corner_end.y)  && (corner.x < corner_end.x) )
        {
            corners_buffer.push_back( corner + shape_offset );
            RotatePoint( &corner, delta );
        }

        //corners_buffer.push_back(corner + shape_offset);		// TODO: about one mil geometry error forms somewhere.
        corners_buffer.push_back( corner_end + shape_offset );
        corners_buffer.push_back( last_corner + shape_offset );         // Enabling the line above shows intersection point.

        /* Create 2 holes, rotated by pad rotation.
         */
        angle = aPad.m_Orient + supp_angle;
        for( int irect = 0; irect < 2; irect++ )
        {
            if( aBooleng->StartPolygonAdd( GROUP_B ) )
            {
                for( unsigned ic = 0; ic < corners_buffer.size(); ic++ )
                {
                    wxPoint cpos = corners_buffer[ic];
                    RotatePoint( &cpos, angle );
                    cpos += PadShapePos;
                    aBooleng->AddPoint( cpos.x, cpos.y );
                }

                aBooleng->EndPolygonAdd();
                angle += 1800;   // this is calculate hole 3
                if( angle >= 3600 )
                    angle -= 3600;
            }
        }

        // Create holes, that are the mirrored from the previous holes
        for( unsigned ic = 0; ic < corners_buffer.size(); ic++ )
        {
            wxPoint swap = corners_buffer[ic];
            swap.x = -swap.x;
            corners_buffer[ic] = swap;
        }

        // Now add corner 4 and 2 (2 is the corner 4 rotated by 180 deg
        angle = aPad.m_Orient + supp_angle;
        for( int irect = 0; irect < 2; irect++ )
        {
            if( aBooleng->StartPolygonAdd( GROUP_B ) )
            {
                for( unsigned ic = 0; ic < corners_buffer.size(); ic++ )
                {
                    wxPoint cpos = corners_buffer[ic];
                    RotatePoint( &cpos, angle );
                    cpos += PadShapePos;
                    aBooleng->AddPoint( cpos.x, cpos.y );
                }

                aBooleng->EndPolygonAdd();
                angle += 1800;
                if( angle >= 3600 )
                    angle -= 3600;
            }
        }
    }
        break;

    case PAD_RECT:       // draw 4 Holes
    {
        /* we create 4 copper holes and put them in position 1, 2, 3 and 4
         * here is the area of the rectangular pad + its thermal gap
         * the 4 copper holes remove the copper in order to create the thermal gap
         * 4 ------ 1
         * |        |
         * |        |
         * |        |
         * |        |
         * 3 ------ 2
         * hole 3 is the same as hole 1, rotated 180 deg
         * hole 4 is the same as hole 2, rotated 180 deg and is the same as hole 1, mirrored
         */

        // First, create a rectangular hole for position 1 :
        // 2 ------- 3
        //  |        |
        //  |        |
        //  |        |
        // 1  -------4
        wxPoint corners_hole[4];            // buffer for 4 corners
        // Create 1 hole, for a pad centered at 0,0, orient 0
        // Calculate coordinates for corner 1 to corner 4:
        corners_hole[0] = wxPoint( copper_tickness.x / 2, -copper_tickness.y / 2 );
        corners_hole[1] = wxPoint( (copper_tickness.x / 2), -dy - aThermalGap );
        corners_hole[2] = wxPoint(   dx + aThermalGap, -dy - aThermalGap );
        corners_hole[3] = wxPoint( dx + aThermalGap, -(copper_tickness.y / 2) );

        /* Create 2 holes, rotated by pad rotation.
         */
        angle = aPad.m_Orient;
        for( int irect = 0; irect < 2; irect++ )
        {
            if( aBooleng->StartPolygonAdd( GROUP_B ) )
            {
                for( int ic = 0; ic < 4; ic++ )
                {
                    wxPoint cpos = corners_hole[ic];
                    RotatePoint( &cpos, angle );
                    cpos += PadShapePos;
                    aBooleng->AddPoint( cpos.x, cpos.y );
                }

                aBooleng->EndPolygonAdd();
                angle += 1800;  // this is calculate hole 3
                if( angle >= 3600 )
                    angle -= 3600;
            }
        }

        // Create a holes, that is the mirrored of the previous hole
        corners_hole[0].x = -corners_hole[0].x;
        corners_hole[1].x = -corners_hole[1].x;
        corners_hole[2].x = -corners_hole[2].x;
        corners_hole[3].x = -corners_hole[3].x;

        // Now add corner 4 and 2 (2 is the corner 4 rotated by 180 deg
        angle = aPad.m_Orient;
        for( int irect = 0; irect < 2; irect++ )
        {
            if( aBooleng->StartPolygonAdd( GROUP_B ) )
            {
                for( int ic = 0; ic < 4; ic++ )
                {
                    wxPoint cpos = corners_hole[ic];
                    RotatePoint( &cpos, angle );
                    cpos += PadShapePos;
                    aBooleng->AddPoint( cpos.x, cpos.y );
                }

                aBooleng->EndPolygonAdd();
                angle += 1800;
                if( angle >= 3600 )
                    angle -= 3600;
            }
        }
    }
        break;
    }
}


/** Function AddTrackWithClearancePolygon
 * Add a polygon cutout for a track in a zone area
 * Convert arcs and circles to multiple straight lines
 */
void AddTrackWithClearancePolygon( Bool_Engine* aBooleng,
                                   TRACK& aTrack, int aClearanceValue )
{
    wxPoint corner_position;
    int     ii, angle;
    int     dx = (aTrack.m_Width / 2) + aClearanceValue;

    int     delta = 3600 / s_CircleToSegmentsCount; // rot angle in 0.1 degree

    switch( aTrack.Type() )
    {
    case TYPEVIA:
        if( aBooleng->StartPolygonAdd( GROUP_B ) )
        {
            dx = (int) (dx * s_Correction);
            for( ii = 0; ii < s_CircleToSegmentsCount; ii++ )
            {
                corner_position = wxPoint( dx, 0 );
                angle = ii * delta;
                RotatePoint( &corner_position, angle );
                corner_position += aTrack.m_Start;
                aBooleng->AddPoint( corner_position.x, corner_position.y );
            }

            aBooleng->EndPolygonAdd();
        }
        break;

    default:
        AddRoundedEndsSegmentPolygon( aBooleng,
            aTrack.m_Start, aTrack.m_End,
            aTrack.m_Width + (2 * aClearanceValue) );
        break;
    }
}


/** Function AddRoundedEndsSegmentPolygon
 * Add a polygon cutout for a segment (with rounded ends) in a zone area
 * Convert arcs to multiple straight lines
 */
void AddRoundedEndsSegmentPolygon( Bool_Engine* aBooleng,
                                   wxPoint aStart, wxPoint aEnd,
                                   int aWidth )
{
    int     rayon  = aWidth / 2;
    wxPoint endp   = aEnd - aStart; // end point coordinate for the same segment starting at (0,0)
    wxPoint startp = aStart;
    wxPoint corner;
    int     seg_len;

    // normalize the position in order to have endp.x >= 0;
    if( endp.x < 0 )
    {
        endp   = aStart - aEnd;
        startp = aEnd;
    }
    int delta_angle = ArcTangente( endp.y, endp.x );    // delta_angle is in 0.1 degrees
    seg_len = (int) sqrt( ( (double) endp.y * endp.y ) + ( (double) endp.x * endp.x ) );

    if( !aBooleng->StartPolygonAdd( GROUP_B ) )
        return;                                 // error!

    int delta = 3600 / s_CircleToSegmentsCount; // rot angle in 0.1 degree

    // Compute the outlines of the segment, and creates a polygon
    corner = wxPoint( 0, rayon );
    RotatePoint( &corner, -delta_angle );
    corner += startp;
    aBooleng->AddPoint( corner.x, corner.y );

    corner = wxPoint( seg_len, rayon );
    RotatePoint( &corner, -delta_angle );
    corner += startp;
    aBooleng->AddPoint( corner.x, corner.y );

    // add right rounded end:
    for( int ii = delta; ii < 1800; ii += delta )
    {
        corner = wxPoint( 0, rayon );
        RotatePoint( &corner, ii );
        corner.x += seg_len;
        RotatePoint( &corner, -delta_angle );
        corner += startp;
        aBooleng->AddPoint( corner.x, corner.y );
    }

    corner = wxPoint( seg_len, -rayon );
    RotatePoint( &corner, -delta_angle );
    corner += startp;
    aBooleng->AddPoint( corner.x, corner.y );

    corner = wxPoint( 0, -rayon );
    RotatePoint( &corner, -delta_angle );
    corner += startp;
    aBooleng->AddPoint( corner.x, corner.y );

    // add left rounded end:
    for( int ii = delta; ii < 1800; ii += delta )
    {
        corner = wxPoint( 0, -rayon );
        RotatePoint( &corner, ii );
        RotatePoint( &corner, -delta_angle );
        corner += startp;
        aBooleng->AddPoint( corner.x, corner.y );
    }

    aBooleng->EndPolygonAdd();
}


/** function AddTextBoxWithClearancePolygon
 * creates a polygon containing the text and add it to bool engine
 */
void    AddTextBoxWithClearancePolygon( Bool_Engine* aBooleng,
                                        TEXTE_PCB* aText, int aClearanceValue )
{
    int corners[8];     // Buffer of coordinates
    int ii;

    int dx = aText->Pitch() * aText->GetLength();
    int dy = aText->m_Size.y + aText->m_Width;

    /* Creates bounding box (rectangle) for an horizontal text */
    dx /= 2; dy /= 2;    /* dx et dy = demi dimensionx X et Y */
    dx += aClearanceValue;
    dy += aClearanceValue;
    corners[0] = aText->m_Pos.x - dx;
    corners[1] = aText->m_Pos.y - dy;
    corners[2] = aText->m_Pos.x + dx;
    corners[3] = aText->m_Pos.y - dy;
    corners[4] = aText->m_Pos.x + dx;
    corners[5] = aText->m_Pos.y + dy;
    corners[6] = aText->m_Pos.x - dx;
    corners[7] = aText->m_Pos.y + dy;

    // Rotate rectangle
    RotatePoint( &corners[0], &corners[1], aText->m_Pos.x, aText->m_Pos.y, aText->m_Orient );
    RotatePoint( &corners[2], &corners[3], aText->m_Pos.x, aText->m_Pos.y, aText->m_Orient );
    RotatePoint( &corners[4], &corners[5], aText->m_Pos.x, aText->m_Pos.y, aText->m_Orient );
    RotatePoint( &corners[6], &corners[7], aText->m_Pos.x, aText->m_Pos.y, aText->m_Orient );

    if( aBooleng->StartPolygonAdd( GROUP_B ) )
    {
        for( ii = 0; ii < 8; ii += 2 )
        {
            aBooleng->AddPoint( corners[ii], corners[ii + 1] );
        }

        aBooleng->EndPolygonAdd();
    }
}
