using namespace std;

#include <math.h>
#include <vector>

#include "fctsys.h"
#include "common.h"
#include "pcbnew.h"
#include "trigo.h"

#include "zones.h"


#include "PolyLine.h"

void    AddTrackWithClearancePolygon( Bool_Engine* aBooleng,
                                      TRACK& aTrack, int aClearanceValue );
void    AddPadWithClearancePolygon( Bool_Engine* aBooleng,
                                    D_PAD& aPad, int aClearanceValue,
                                    bool bThermal, int spoke_w );
void    AddRoundedEndsSegmentPolygon( Bool_Engine* aBooleng,
                                      wxPoint aStart, wxPoint aEnd,
                                      int aWidth );

/* how many segments are used to create a polygon from a circle: */
static int s_CircleToSegmentsCount = 16;

/** function AddClearanceAreasPolygonsToPolysList
 * Add non copper areas polygons (pads and tracks with clearence)
 * to a filled copper area
 * used in BuildFilledPolysListData when calculating filled areas in a zone
 * Non copper areas are pads and track and their clearance area
 * The filled copper area must be computed just before.
 * BuildFilledPolysListData() call this function just after creating the
 *  filled copper area polygon (without clearence areas
 * to do that this function:
 * 1 - creates aBool_Engine,with option: holes are linked into outer contours by double overlapping segments
 * 2 - Add the main outline (zone outline) in group A
 * 3 - Add all non filled areas (pads, tracks) in group B
 * 4 - calculates the polygon A - B
 * 5 - put resulting list of polygons (filled areas) in m_FilledPolysList
 */
void ZONE_CONTAINER::AddClearanceAreasPolygonsToPolysList( BOARD* aPcb )
{
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

    /* Add holes (i.e. tracks and pads areas as polygons outlines)
     * in GroupB in Bool_Engine
     * First : Add pads
     */
    for( MODULE* module = aPcb->m_Modules;  module;  module = module->Next() )
    {
        for( D_PAD* pad = module->m_Pads; pad != NULL; pad = pad->Next() )
        {
            if( !pad->IsOnLayer( GetLayer() ) )
                continue;
            if ( pad->GetNet() == GetNet() )
                continue;
            AddPadWithClearancePolygon( booleng, *pad, m_ZoneClearance,
                                        false, 0 );
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
        if ( track->GetNet() == GetNet() )
            continue;
        AddTrackWithClearancePolygon( booleng, *track, m_ZoneClearance );
    }

    // Draw graphic items (copper texts) and board edges
    for( BOARD_ITEM* item = aPcb->m_Drawings;  item;  item = item->Next() )
    {
        switch( item->Type() )
        {
        case TYPEDRAWSEGMENT:

            // TODO: add segment
            break;

        case TYPETEXTE:

            // TODO: add rectangular area
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
}


/** Function AddPadPolygonWithPadClearance
 * Add a polygon cutout for a pad in a zone area
 * Convert arcs and circles to multiple straight lines
 */
void AddPadWithClearancePolygon( Bool_Engine* aBooleng,
                                 D_PAD& aPad, int aClearanceValue,
                                 bool bThermal, int spoke_w )
{
    wxPoint corner_position;
    int     ii, angle;
    int     dx = (aPad.m_Size.x / 2) + aClearanceValue;
    int     dy = (aPad.m_Size.y / 2) + aClearanceValue;

    int     delta = 3600 / s_CircleToSegmentsCount; // rot angle in 0.1 degree

    if( !bThermal )
    {
        switch( aPad.m_PadShape )
        {
        case PAD_CIRCLE:
            if( aBooleng->StartPolygonAdd( GROUP_B ) )
            {
                for( ii = 0; ii < s_CircleToSegmentsCount; ii++ )
                {
                    corner_position = wxPoint( dx, 0 );
                    angle = ii * delta;
                    RotatePoint( &corner_position, angle );
                    corner_position += aPad.ReturnShapePos();
                    aBooleng->AddPoint( corner_position.x, corner_position.y );
                }

                aBooleng->EndPolygonAdd();
            }
            break;

        case PAD_OVAL:
        case PAD_RECT:
            if( aBooleng->StartPolygonAdd( GROUP_B ) )
            {
                angle = aPad.m_Orient;
                corner_position = wxPoint( -dx, -dy );
                RotatePoint( &corner_position, angle );
                corner_position += aPad.ReturnShapePos();
                aBooleng->AddPoint( corner_position.x, corner_position.y );

                corner_position = wxPoint( -dx, +dy );
                RotatePoint( &corner_position, angle );
                corner_position += aPad.ReturnShapePos();
                aBooleng->AddPoint( corner_position.x, corner_position.y );

                corner_position = wxPoint( +dx, +dy );
                RotatePoint( &corner_position, angle );
                corner_position += aPad.ReturnShapePos();
                aBooleng->AddPoint( corner_position.x, corner_position.y );

                corner_position = wxPoint( +dx, -dy );
                RotatePoint( &corner_position, angle );
                corner_position += aPad.ReturnShapePos();
                aBooleng->AddPoint( corner_position.x, corner_position.y );

                aBooleng->EndPolygonAdd();
            }
            break;
        }
    }
    else
    {
        // thermal relief (from FreePCB: must be converted to pcbnew data)
#if 0
        if( type == PAD_ROUND || (type == PAD_NONE && hole_w > 0) )
        {
            // draw 4 "wedges"
            double r = max( w / 2 + fill_clearance, hole_w / 2 + hole_clearance );
            double start_angle = asin( spoke_w / (2.0 * r) );
            double th1, th2, corner_x, corner_y;
            th1 = th2 = corner_x = corner_y = 0; // gcc warning fix
            for( int i = 0; i<4; i++ )
            {
                if( i == 0 )
                {
                    corner_x = spoke_w / 2;
                    corner_y = spoke_w / 2;
                    th1 = start_angle;
                    th2 = pi / 2.0 - start_angle;
                }
                else if( i == 1 )
                {
                    corner_x = -spoke_w / 2;
                    corner_y = spoke_w / 2;
                    th1 = pi / 2.0 + start_angle;
                    th2 = pi - start_angle;
                }
                else if( i == 2 )
                {
                    corner_x = -spoke_w / 2;
                    corner_y = -spoke_w / 2;
                    th1 = -pi + start_angle;
                    th2 = -pi / 2.0 - start_angle;
                }
                else if( i == 3 )
                {
                    corner_x = spoke_w / 2;
                    corner_y = -spoke_w / 2;
                    th1 = -pi / 2.0 + start_angle;
                    th2 = -start_angle;
                }
                AppendCorner( to_int( x + corner_x ), to_int( y + corner_y ), STRAIGHT, 0 );
                AppendCorner( to_int( x + r * cos( th1 ) ), to_int( y + r * sin(
                                                                       th1 ) ), STRAIGHT, 0 );
                AppendCorner( to_int( x + r * cos( th2 ) ), to_int( y + r * sin(
                                                                       th2 ) ), ARC_CCW, 0 );
                Close( STRAIGHT );
            }
        }
        else if( type == PAD_SQUARE || type == PAD_RECT
                 || type == PAD_RRECT || type == PAD_OVAL )
        {
            // draw 4 rectangles
            int xL = x - dx;
            int xR = x - spoke_w / 2;
            int yB = y - dy;
            int yT = y - spoke_w / 2;
            AppendCorner( xL, yB, STRAIGHT, 0 );
            AppendCorner( xR, yB, STRAIGHT, 0 );
            AppendCorner( xR, yT, STRAIGHT, 0 );
            AppendCorner( xL, yT, STRAIGHT, 0 );
            Close( STRAIGHT );
            xL = x + spoke_w / 2;
            xR = x + dx;
            AppendCorner( xL, yB, STRAIGHT, 0 );
            AppendCorner( xR, yB, STRAIGHT, 0 );
            AppendCorner( xR, yT, STRAIGHT, 0 );
            AppendCorner( xL, yT, STRAIGHT, 0 );
            Close( STRAIGHT );
            xL = x - dx;
            xR = x - spoke_w / 2;
            yB = y + spoke_w / 2;
            yT = y + dy;
            AppendCorner( xL, yB, STRAIGHT, 0 );
            AppendCorner( xR, yB, STRAIGHT, 0 );
            AppendCorner( xR, yT, STRAIGHT, 0 );
            AppendCorner( xL, yT, STRAIGHT, 0 );
            Close( STRAIGHT );
            xL = x + spoke_w / 2;
            xR = x + dx;
            AppendCorner( xL, yB, STRAIGHT, 0 );
            AppendCorner( xR, yB, STRAIGHT, 0 );
            AppendCorner( xR, yT, STRAIGHT, 0 );
            AppendCorner( xL, yT, STRAIGHT, 0 );
            Close( STRAIGHT );
        }
#endif
    }
    return;
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
    seg_len = (int) sqrt( (endp.y * endp.y) + (endp.x * endp.x) );

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
        corner   += startp;
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
        corner   += startp;
        aBooleng->AddPoint( corner.x, corner.y );
    }
    aBooleng->EndPolygonAdd();
}
