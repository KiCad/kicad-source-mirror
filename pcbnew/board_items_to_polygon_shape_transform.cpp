/**********************************************/
/* board_items_to_polygon_shape_transform.cpp */
/**********************************************/

/* Function to convert pads and tranck shapes to polygons
 * Used to fill zones areas
 */
#include <vector>

#include "fctsys.h"
#include "common.h"
#include "pcbnew.h"
#include "wxPcbStruct.h"
#include "trigo.h"

/* Exported functions */

/** Function TransformRoundedEndsSegmentToPolygon
 * convert a segment with rounded ends to a polygon
 * Convert arcs to multiple straight lines
 * @param aCornerBuffer = a buffer to store the polygon
 * @param aStart = the segment start point coordinate
 * @param aEnd = the segment end point coordinate
 * @param aWidth = the segment width
 * @param aCircleToSegmentsCount = the number of segments to approximate a circle
 */
void TransformRoundedEndsSegmentToPolygon( std::vector <wxPoint>& aCornerBuffer,
                                           wxPoint aStart, wxPoint aEnd,
                                           int aCircleToSegmentsCount,
                                           int aWidth );


/** Function TransformTrackWithClearanceToPolygon
 * Convert the track shape to a closed polygon
 * Used in filling zones calculations
 * Circles (vias) and arcs (ends of tracks) are approximated by segments
 * @param aCornerBuffer = a buffer to store the polygon
 * @param aClearanceValue = the clearance around the pad
 * @param aCircleToSegmentsCount = the number of segments to approximate a circle
 * @param aCorrectionFactor = the correction to apply to circles radius to keep
 * clearance when the circle is approxiamted by segment bigger or equal
 * to the real clearance value (usually near from 1.0)
 */
void TRACK::TransformTrackWithClearanceToPolygon( std::vector <wxPoint>& aCornerBuffer,
                                                  int                    aClearanceValue,
                                                  int                    aCircleToSegmentsCount,
                                                  double                 aCorrectionFactor )
{
    wxPoint corner_position;
    int     ii, angle;
    int     dx = (m_Width / 2) + aClearanceValue;

    int     delta = 3600 / aCircleToSegmentsCount; // rot angle in 0.1 degree

    switch( Type() )
    {
    case TYPE_VIA:
        dx = (int) ( dx * aCorrectionFactor );
        for( ii = 0; ii < aCircleToSegmentsCount; ii++ )
        {
            corner_position = wxPoint( dx, 0 );
            RotatePoint( &corner_position, (1800 / aCircleToSegmentsCount) );
            angle = ii * delta;
            RotatePoint( &corner_position, angle );
            corner_position += m_Start;
            aCornerBuffer.push_back( corner_position );
        }

        break;

    default:
        TransformRoundedEndsSegmentToPolygon( aCornerBuffer,
                                             m_Start, m_End, aCircleToSegmentsCount,
                                             m_Width + (2 * aClearanceValue) );
        break;
    }
}


/* Function TransformRoundedEndsSegmentToPolygon
 */
void TransformRoundedEndsSegmentToPolygon( std::vector <wxPoint>& aCornerBuffer,
                                           wxPoint aStart, wxPoint aEnd,
                                           int aCircleToSegmentsCount,
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

    int delta = 3600 / aCircleToSegmentsCount; // rot angle in 0.1 degree

    // Compute the outlines of the segment, and creates a polygon
    corner = wxPoint( 0, rayon );
    RotatePoint( &corner, -delta_angle );
    corner += startp;
    aCornerBuffer.push_back( corner );

    corner = wxPoint( seg_len, rayon );
    RotatePoint( &corner, -delta_angle );
    corner += startp;
    aCornerBuffer.push_back( corner );

    // add right rounded end:
    for( int ii = delta; ii < 1800; ii += delta )
    {
        corner = wxPoint( 0, rayon );
        RotatePoint( &corner, ii );
        corner.x += seg_len;
        RotatePoint( &corner, -delta_angle );
        corner += startp;
        aCornerBuffer.push_back( corner );
    }

    corner = wxPoint( seg_len, -rayon );
    RotatePoint( &corner, -delta_angle );
    corner += startp;
    aCornerBuffer.push_back( corner );

    corner = wxPoint( 0, -rayon );
    RotatePoint( &corner, -delta_angle );
    corner += startp;
    aCornerBuffer.push_back( corner );

    // add left rounded end:
    for( int ii = delta; ii < 1800; ii += delta )
    {
        corner = wxPoint( 0, -rayon );
        RotatePoint( &corner, ii );
        RotatePoint( &corner, -delta_angle );
        corner += startp;
        aCornerBuffer.push_back( corner );
    }
}


/** function TransformPadWithClearanceToPolygon
 * Convert the pad shape to a closed polygon
 * Used in filling zones calculations
 * Circles and arcs are approximated by segments
 * @param aCornerBuffer = a buffer to store the polygon
 * @param aClearanceValue = the clearance around the pad
 * @param aCircleToSegmentsCount = the number of segments to approximate a circle
 * @param aCorrectionFactor = the correction to apply to circles radius to keep
 * clearance when the circle is approxiamted by segment bigger or equal
 * to the real clearance value (usually near from 1.0)
 */
void D_PAD::TransformPadWithClearanceToPolygon( std::vector <wxPoint>& aCornerBuffer,
                                                int                    aClearanceValue,
                                                int                    aCircleToSegmentsCount,
                                                double                 aCorrectionFactor )
{
    wxPoint corner_position;
    int     ii, angle;
    int     dx = (m_Size.x / 2) + aClearanceValue;
    int     dy = (m_Size.y / 2) + aClearanceValue;

    int     delta = 3600 / aCircleToSegmentsCount; // rot angle in 0.1 degree
    wxPoint PadShapePos = ReturnShapePos();    /* Note: for pad having a shape offset,
                                                * the pad position is NOT the shape position */
    wxSize  psize = m_Size;                    /* pad size unsed in RECT and TRAPEZOIDAL pads
                                                *  trapezoidal pads are considered as rect pad shape having they boudary box size
                                                */

    switch( m_PadShape )
    {
    case PAD_CIRCLE:
        dx = (int) ( dx * aCorrectionFactor );
        for( ii = 0; ii < aCircleToSegmentsCount; ii++ )
        {
            corner_position = wxPoint( dx, 0 );
            RotatePoint( &corner_position, (1800 / aCircleToSegmentsCount) );    // Half increment offset to get more space between
            angle = ii * delta;
            RotatePoint( &corner_position, angle );
            corner_position += PadShapePos;
            aCornerBuffer.push_back( corner_position );
        }

        break;

    case PAD_OVAL:
        angle = m_Orient;
        if( dy > dx )     // Oval pad X/Y ratio for choosing translation axles
        {
            dy = (int) ( dy * aCorrectionFactor );
            int     angle_pg;                                           // Polygon angle
            wxPoint shape_offset = wxPoint( 0, (dy - dx) );
            RotatePoint( &shape_offset, angle );                        // Rotating shape offset vector with component

            for( ii = 0; ii < aCircleToSegmentsCount / 2 + 1; ii++ )    // Half circle end cap...
            {
                corner_position = wxPoint( dx, 0 );                     // Coordinate translation +dx
                RotatePoint( &corner_position, (1800 / aCircleToSegmentsCount) );
                RotatePoint( &corner_position, angle );
                angle_pg = ii * delta;
                RotatePoint( &corner_position, angle_pg );
                corner_position += PadShapePos - shape_offset;
                aCornerBuffer.push_back( corner_position );
            }

            for( ii = 0; ii < aCircleToSegmentsCount / 2 + 1; ii++ )    // Second half circle end cap...
            {
                corner_position = wxPoint( -dx, 0 );                    // Coordinate translation -dx
                RotatePoint( &corner_position, (1800 / aCircleToSegmentsCount) );
                RotatePoint( &corner_position, angle );
                angle_pg = ii * delta;
                RotatePoint( &corner_position, angle_pg );
                corner_position += PadShapePos + shape_offset;
                aCornerBuffer.push_back( corner_position );
            }

            break;
        }
        else    //if( dy <= dx )
        {
            dx = (int) ( dx * aCorrectionFactor );
            int     angle_pg; // Polygon angle
            wxPoint shape_offset = wxPoint( (dy - dx), 0 );
            RotatePoint( &shape_offset, angle );

            for( ii = 0; ii < aCircleToSegmentsCount / 2 + 1; ii++ )
            {
                corner_position = wxPoint( 0, dy );
                RotatePoint( &corner_position, (1800 / aCircleToSegmentsCount) );
                RotatePoint( &corner_position, angle );
                angle_pg = ii * delta;
                RotatePoint( &corner_position, angle_pg );
                corner_position += PadShapePos - shape_offset;
                aCornerBuffer.push_back( corner_position );
            }

            for( ii = 0; ii < aCircleToSegmentsCount / 2 + 1; ii++ )
            {
                corner_position = wxPoint( 0, -dy );
                RotatePoint( &corner_position, (1800 / aCircleToSegmentsCount) );
                RotatePoint( &corner_position, angle );
                angle_pg = ii * delta;
                RotatePoint( &corner_position, angle_pg );
                corner_position += PadShapePos + shape_offset;
                aCornerBuffer.push_back( corner_position );
            }

            break;
        }

    default:
    case PAD_TRAPEZOID:
        psize.x += ABS( m_DeltaSize.y );
        psize.y += ABS( m_DeltaSize.x );

    // fall through
    case PAD_RECT:                                                                          // Easy implementation for rectangular cutouts with rounded corners                                                                  // Easy implementation for rectangular cutouts with rounded corners
        angle = m_Orient;
        int rounding_radius = (int) ( aClearanceValue * aCorrectionFactor );                // Corner rounding radius
        int angle_pg;                                                                       // Polygon increment angle

        for( int i = 0; i < aCircleToSegmentsCount / 4 + 1; i++ )
        {
            corner_position = wxPoint( 0, -rounding_radius );
            RotatePoint( &corner_position, (1800 / aCircleToSegmentsCount) );       // Start at half increment offset
            angle_pg = i * delta;
            RotatePoint( &corner_position, angle_pg );                              // Rounding vector rotation
            corner_position -= psize / 2;                                           // Rounding vector + Pad corner offset
            RotatePoint( &corner_position, angle );                                 // Rotate according to module orientation
            corner_position += PadShapePos;                                         // Shift origin to position
            aCornerBuffer.push_back( corner_position );
        }

        for( int i = 0; i < aCircleToSegmentsCount / 4 + 1; i++ )
        {
            corner_position = wxPoint( -rounding_radius, 0 );
            RotatePoint( &corner_position, (1800 / aCircleToSegmentsCount) );
            angle_pg = i * delta;
            RotatePoint( &corner_position, angle_pg );
            corner_position -= wxPoint( psize.x / 2, -psize.y / 2 );
            RotatePoint( &corner_position, angle );
            corner_position += PadShapePos;
            aCornerBuffer.push_back( corner_position );
        }

        for( int i = 0; i < aCircleToSegmentsCount / 4 + 1; i++ )
        {
            corner_position = wxPoint( 0, rounding_radius );
            RotatePoint( &corner_position, (1800 / aCircleToSegmentsCount) );
            angle_pg = i * delta;
            RotatePoint( &corner_position, angle_pg );
            corner_position += psize / 2;
            RotatePoint( &corner_position, angle );
            corner_position += PadShapePos;
            aCornerBuffer.push_back( corner_position );
        }

        for( int i = 0; i < aCircleToSegmentsCount / 4 + 1; i++ )
        {
            corner_position = wxPoint( rounding_radius, 0 );
            RotatePoint( &corner_position, (1800 / aCircleToSegmentsCount) );
            angle_pg = i * delta;
            RotatePoint( &corner_position, angle_pg );
            corner_position -= wxPoint( -psize.x / 2, psize.y / 2 );
            RotatePoint( &corner_position, angle );
            corner_position += PadShapePos;
            aCornerBuffer.push_back( corner_position );
        }

        break;
    }
}
