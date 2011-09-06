/**********************************************/
/* board_items_to_polygon_shape_transform.cpp */
/**********************************************/

/* Function to convert pads and tranck shapes to polygons
 * Used to fill zones areas
 */
#include <vector>

#include "fctsys.h"
#include "pcbnew.h"
#include "wxPcbStruct.h"
#include "trigo.h"

/* Exported functions */

/**
 * Function TransformRoundedEndsSegmentToPolygon
 * convert a segment with rounded ends to a polygon
 * Convert arcs to multiple straight lines
 * @param aCornerBuffer = a buffer to store the polygon
 * @param aStart = the segment start point coordinate
 * @param aEnd = the segment end point coordinate
 * @param aCircleToSegmentsCount = the number of segments to approximate a circle
 * @param aWidth = the segment width
 */
void TransformRoundedEndsSegmentToPolygon( std::vector <CPolyPt>& aCornerBuffer,
                                           wxPoint aStart, wxPoint aEnd,
                                           int aCircleToSegmentsCount,
                                           int aWidth );


/**
 * Function TransformArcToPolygon
 * Creates a polygon from an Arc
 * Convert arcs to multiple straight segments
 * @param aCornerBuffer = a buffer to store the polygon
 * @param aCentre = centre of the arc or circle
 * @param aStart = start point of the arc, or a point on the circle
 * @param aArcAngle = arc angle in 0.1 degrees. For a circle, aArcAngle = 3600
 * @param aCircleToSegmentsCount = the number of segments to approximate a circle
 * @param aWidth = width (thickness) of the line
 */
void TransformArcToPolygon( std::vector <CPolyPt>& aCornerBuffer,
                            wxPoint aCentre, wxPoint aStart, int aArcAngle,
                            int aCircleToSegmentsCount, int aWidth )
{
    wxPoint arc_start, arc_end;
    int     delta = 3600 / aCircleToSegmentsCount;                                                                 // rot angle in 0.1 degree

    arc_end = arc_start = aStart;
    if( aArcAngle != 3600 )
    {
        RotatePoint( &arc_end, aCentre, -aArcAngle );
    }

    if( aArcAngle < 0 )
    {
        EXCHG( arc_start, arc_end );
        NEGATE( aArcAngle );
    }

    // Compute the ends of segments and creates poly
    wxPoint curr_end   = arc_start;
    wxPoint curr_start = arc_start;
    for( int ii = delta; ii < aArcAngle; ii += delta )
    {
        curr_end = arc_start;
        RotatePoint( &curr_end, aCentre, -ii );
        TransformRoundedEndsSegmentToPolygon( aCornerBuffer,
                                              curr_start, curr_end, aCircleToSegmentsCount, aWidth );
        curr_start = curr_end;
    }

    if( curr_end != arc_end )
        TransformRoundedEndsSegmentToPolygon( aCornerBuffer,
                                              curr_end, arc_end, aCircleToSegmentsCount, aWidth );
}


/**
 * Function TransformShapeWithClearanceToPolygon
 * Convert the track shape to a closed polygon
 * Used in filling zones calculations
 * Circles and arcs are approximated by segments
 * @param aCornerBuffer = a buffer to store the polygon
 * @param aClearanceValue = the clearance around the pad
 * @param aCircleToSegmentsCount = the number of segments to approximate a circle
 * @param aCorrectionFactor = the correction to apply to circles radius to keep
 * clearance when the circle is approximated by segment bigger or equal
 * to the real clearance value (usually near from 1.0)
 */
void TEXTE_PCB::TransformShapeWithClearanceToPolygon(
    std::vector <CPolyPt>& aCornerBuffer,
    int                    aClearanceValue,
    int                    aCircleToSegmentsCount,
    double                 aCorrectionFactor )
{
    if( GetLength() == 0 )
        return;

    CPolyPt  corners[4];    // Buffer of polygon corners

    EDA_RECT rect = GetTextBox( -1 );
    rect.Inflate( aClearanceValue );
    corners[0].x = rect.GetOrigin().x;
    corners[0].y = rect.GetOrigin().y;
    corners[1].y = corners[0].y;
    corners[1].x = rect.GetRight();
    corners[2].x = corners[1].x;
    corners[2].y = rect.GetBottom();
    corners[3].y = corners[2].y;
    corners[3].x = corners[0].x;

    for( int ii = 0; ii < 4; ii++ )
    {
        // Rotate polygon
        RotatePoint( &corners[ii].x, &corners[ii].y,
                     m_Pos.x, m_Pos.y,
                     m_Orient );
        aCornerBuffer.push_back( corners[ii] );
    }

    aCornerBuffer.back().end_contour = true;
}


/**
 * Function TransformShapeWithClearanceToPolygon
 * Convert the track shape to a closed polygon
 * Used in filling zones calculations
 * Circles and arcs are approximated by segments
 * @param aCornerBuffer = a buffer to store the polygon
 * @param aClearanceValue = the clearance around the pad
 * @param aCircleToSegmentsCount = the number of segments to approximate a circle
 * @param aCorrectionFactor = the correction to apply to circles radius to keep
 * clearance when the circle is approxiamted by segment bigger or equal
 * to the real clearance value (usually near from 1.0)
 */
void DRAWSEGMENT::TransformShapeWithClearanceToPolygon(
    std::vector <CPolyPt>& aCornerBuffer,
    int                    aClearanceValue,
    int                    aCircleToSegmentsCount,
    double                 aCorrectionFactor )
{
    switch( m_Shape )
    {
    case S_CIRCLE:
        TransformArcToPolygon( aCornerBuffer, m_Start,         // Circle centre
                              m_End, 3600,
                              aCircleToSegmentsCount,
                              m_Width + (2 * aClearanceValue) );
        break;

    case S_ARC:
        TransformArcToPolygon( aCornerBuffer, m_Start,
                              m_End, m_Angle,
                              aCircleToSegmentsCount,
                              m_Width + (2 * aClearanceValue) );
        break;

    default:
        TransformRoundedEndsSegmentToPolygon(
            aCornerBuffer, m_Start, m_End,
            aCircleToSegmentsCount, m_Width + (2 * aClearanceValue) );
        break;
    }
}


/**
 * Function TransformShapeWithClearanceToPolygon
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
void TRACK:: TransformShapeWithClearanceToPolygon( std:: vector < CPolyPt>& aCornerBuffer,
                                                   int                      aClearanceValue,
                                                   int                      aCircleToSegmentsCount,
                                                   double                   aCorrectionFactor )
{
    wxPoint corner_position;
    int     ii, angle;
    int     dx    = (m_Width / 2) + aClearanceValue;
    int     delta = 3600 / aCircleToSegmentsCount;                                                // rot angle in 0.1 degree

    switch( Type() )
    {
    case TYPE_VIA:
        dx = (int) ( dx * aCorrectionFactor );
        for( ii = 0; ii < aCircleToSegmentsCount; ii++ )
        {
            corner_position = wxPoint( dx, 0 );
            RotatePoint( &corner_position.x, &corner_position.y,
                        (1800 / aCircleToSegmentsCount) );
            angle = ii * delta;
            RotatePoint( &corner_position.x, &corner_position.y, angle );
            corner_position.x += m_Start.x;
            corner_position.y += m_Start.y;
            CPolyPt polypoint( corner_position.x, corner_position.y );
            aCornerBuffer.push_back( polypoint );
        }

        aCornerBuffer.back().end_contour = true;
        break;

    default:
        TransformRoundedEndsSegmentToPolygon(
            aCornerBuffer,
            m_Start, m_End,
            aCircleToSegmentsCount,
            m_Width + ( 2 * aClearanceValue) );
        break;
    }
}


/* Function TransformRoundedEndsSegmentToPolygon
 */
void TransformRoundedEndsSegmentToPolygon( std::vector <CPolyPt>& aCornerBuffer,
                                           wxPoint aStart, wxPoint aEnd,
                                           int aCircleToSegmentsCount,
                                           int aWidth )
{
    int     rayon  = aWidth / 2;
    wxPoint endp   = aEnd - aStart; // end point coordinate for the same segment starting at (0,0)
    wxPoint startp = aStart;
    wxPoint corner;
    int     seg_len;
    CPolyPt polypoint;

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
    polypoint.x = corner.x;
    polypoint.y = corner.y;
    aCornerBuffer.push_back( polypoint );

    corner = wxPoint( seg_len, rayon );
    RotatePoint( &corner, -delta_angle );
    corner += startp;
    polypoint.x = corner.x;
    polypoint.y = corner.y;
    aCornerBuffer.push_back( polypoint );

    // add right rounded end:
    for( int ii = delta; ii < 1800; ii += delta )
    {
        corner = wxPoint( 0, rayon );
        RotatePoint( &corner, ii );
        corner.x += seg_len;
        RotatePoint( &corner, -delta_angle );
        corner += startp;
        polypoint.x = corner.x;
        polypoint.y = corner.y;
        aCornerBuffer.push_back( polypoint );
    }

    corner = wxPoint( seg_len, -rayon );
    RotatePoint( &corner, -delta_angle );
    corner += startp;
    polypoint.x = corner.x;
    polypoint.y = corner.y;
    aCornerBuffer.push_back( polypoint );

    corner = wxPoint( 0, -rayon );
    RotatePoint( &corner, -delta_angle );
    corner += startp;
    polypoint.x = corner.x;
    polypoint.y = corner.y;
    aCornerBuffer.push_back( polypoint );

    // add left rounded end:
    for( int ii = delta; ii < 1800; ii += delta )
    {
        corner = wxPoint( 0, -rayon );
        RotatePoint( &corner, ii );
        RotatePoint( &corner, -delta_angle );
        corner += startp;
        polypoint.x = corner.x;
        polypoint.y = corner.y;
        aCornerBuffer.push_back( polypoint );
    }

    aCornerBuffer.back().end_contour = true;
}


/**
 * Function TransformShapeWithClearanceToPolygon
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
void D_PAD:: TransformShapeWithClearanceToPolygon( std:: vector < CPolyPt>& aCornerBuffer,
                                                   int                      aClearanceValue,
                                                   int                      aCircleToSegmentsCount,
                                                   double                   aCorrectionFactor )
{
    wxPoint corner_position;
    int     ii, angle;
    int     dx = (m_Size.x / 2) + aClearanceValue;
    int     dy = (m_Size.y / 2) + aClearanceValue;

    int     delta = 3600 / aCircleToSegmentsCount;                   // rot angle in 0.1 degree
    wxPoint PadShapePos = ReturnShapePos();                                                    /* Note: for pad having a shape offset,
                                                                                                * the pad position is NOT the shape position */
    wxSize  psize = m_Size;                                                   /* pad size unsed in RECT and TRAPEZOIDAL pads
                                                                               *  trapezoidal pads are considered as rect pad shape having they boudary box size
                                                                               */

    switch( m_PadShape )
    {
    case PAD_CIRCLE:
        dx = (int) ( dx * aCorrectionFactor );
        for( ii = 0; ii < aCircleToSegmentsCount; ii++ )
        {
            corner_position = wxPoint( dx, 0 );
            RotatePoint( &corner_position, (1800 / aCircleToSegmentsCount) );

            // Half increment offset to get more space between
            angle = ii * delta;
            RotatePoint( &corner_position, angle );
            corner_position += PadShapePos;
            CPolyPt polypoint( corner_position.x, corner_position.y );
            aCornerBuffer.push_back( polypoint );
        }

        aCornerBuffer.back().end_contour = true;
        break;

    case PAD_OVAL:
        angle = m_Orient;
        if( dy > dx )   // Oval pad X/Y ratio for choosing translation axles
        {
            dy = (int) ( dy * aCorrectionFactor );
            int     angle_pg;                                           // Polygon angle
            wxPoint shape_offset = wxPoint( 0, dy - dx );
            RotatePoint( &shape_offset, angle );                        // Rotating shape offset vector with component

            for( ii = 0; ii < aCircleToSegmentsCount / 2 + 1; ii++ )    // Half circle end cap...
            {
                corner_position = wxPoint( dx, 0 );

                // Coordinate translation +dx
                RotatePoint( &corner_position, (1800 / aCircleToSegmentsCount) );
                RotatePoint( &corner_position, angle );
                angle_pg = ii * delta;
                RotatePoint( &corner_position, angle_pg );
                corner_position += PadShapePos - shape_offset;
                CPolyPt polypoint( corner_position.x, corner_position.y );
                aCornerBuffer.push_back( polypoint );
            }

            for( ii = 0; ii < aCircleToSegmentsCount / 2 + 1; ii++ )  // Second half circle end cap...
            {
                corner_position = wxPoint( -dx, 0 );

                // Coordinate translation -dx
                RotatePoint( &corner_position, (1800 / aCircleToSegmentsCount) );
                RotatePoint( &corner_position, angle );
                angle_pg = ii * delta;
                RotatePoint( &corner_position, angle_pg );
                corner_position += PadShapePos + shape_offset;
                CPolyPt polypoint( corner_position.x, corner_position.y );
                aCornerBuffer.push_back( polypoint );
            }

            aCornerBuffer.back().end_contour = true;
            break;
        }
        else    //if( dy <= dx )
        {
            dx = (int) ( dx * aCorrectionFactor );
            int     angle_pg;   // Polygon angle
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
                CPolyPt polypoint( corner_position.x, corner_position.y );
                aCornerBuffer.push_back( polypoint );
            }

            for( ii = 0; ii < aCircleToSegmentsCount / 2 + 1; ii++ )
            {
                corner_position = wxPoint( 0, -dy );
                RotatePoint( &corner_position, (1800 / aCircleToSegmentsCount) );
                RotatePoint( &corner_position, angle );
                angle_pg = ii * delta;
                RotatePoint( &corner_position, angle_pg );
                corner_position += PadShapePos + shape_offset;
                CPolyPt polypoint( corner_position.x, corner_position.y );
                aCornerBuffer.push_back( polypoint );
            }

            aCornerBuffer.back().end_contour = true;
            break;
        }

    default:
    case PAD_TRAPEZOID:
        psize.x += ABS( m_DeltaSize.y );
        psize.y += ABS( m_DeltaSize.x );

    // fall through
    case PAD_RECT:                                                                                                              // Easy implementation for rectangular cutouts with rounded corners                                                                  // Easy implementation for rectangular cutouts with rounded corners
        angle = m_Orient;
        int rounding_radius = (int) ( aClearanceValue * aCorrectionFactor );                                                    // Corner rounding radius
        int angle_pg;                                                                                                           // Polygon increment angle

        for( int i = 0; i < aCircleToSegmentsCount / 4 + 1; i++ )
        {
            corner_position = wxPoint( 0, -rounding_radius );
            RotatePoint( &corner_position, (1800 / aCircleToSegmentsCount) );

            // Start at half increment offset
            angle_pg = i * delta;
            RotatePoint( &corner_position, angle_pg );

            // Rounding vector rotation
            corner_position -= psize / 2;                                                       // Rounding vector + Pad corner offset
            RotatePoint( &corner_position, angle );

            // Rotate according to module orientation
            corner_position += PadShapePos;                                                    // Shift origin to position
            CPolyPt polypoint( corner_position.x, corner_position.y );
            aCornerBuffer.push_back( polypoint );
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
            CPolyPt polypoint( corner_position.x, corner_position.y );
            aCornerBuffer.push_back( polypoint );
        }

        for( int i = 0;
             i < aCircleToSegmentsCount / 4 + 1;
             i++ )
        {
            corner_position = wxPoint( 0, rounding_radius );
            RotatePoint( &corner_position, (1800 / aCircleToSegmentsCount) );
            angle_pg = i * delta;
            RotatePoint( &corner_position, angle_pg );
            corner_position += psize / 2;
            RotatePoint( &corner_position, angle );
            corner_position += PadShapePos;
            CPolyPt polypoint( corner_position.x, corner_position.y );
            aCornerBuffer.push_back( polypoint );
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
            CPolyPt polypoint( corner_position.x, corner_position.y );
            aCornerBuffer.push_back( polypoint );
        }

        aCornerBuffer.back().end_contour = true;
        break;
    }
}


/**
 * Function CreateThermalReliefPadPolygon
 * Add holes around a pad to create a thermal relief
 * copper thickness is min (dx/2, aCopperWitdh) or min (dy/2, aCopperWitdh)
 * @param aCornerBuffer = a buffer to store the polygon
 * @param aPad     = the current pad used to create the thermal shape
 * @param aThermalGap = gap in thermal shape
 * @param aCopperThickness = stubs thickness in thermal shape
 * @param aMinThicknessValue = min copper thickness allowed
 * @param aCircleToSegmentsCount = the number of segments to approximate a circle
 * @param aCorrectionFactor = the correction to apply to circles radius to keep
 * @param aThermalRot = for rond pads the rotation of thermal stubs (450 usually for 45 deg.)
 */

/* thermal reliefs are created as 4 polygons.
 * each corner of a polygon if calculated for a pad at position 0, 0, orient 0,
 * and then moved and rotated acroding to the pad position and orientation
 */

/* WARNING:
 * When Kbool calculates the filled areas :
 * i.e when substracting holes (thermal shapes) to the full zone area
 * under certains circumstances kboll drop some holes.
 * These circumstances are:
 * some identical holes (same thermal shape and size) are *exactly* on the same vertical line
 * And
 * nothing else between holes
 * And
 * angles less than 90 deg between 2 consecutive lines in hole outline (sometime occurs without this condition)
 * And
 * a hole above the identical holes
 *
 * In fact, it is easy to find these conditions in pad arrays.
 * So to avoid this, the workaround is do not use holes outlines that include
 * angles less than 90 deg between 2 consecutive lines
 * this is made in round and oblong thermal reliefs
 *
 * Note 1: polygons are drawm using outlines witk a thickness = aMinThicknessValue
 * so shapes must keep in account this outline thickness
 *
 * Note 2:
 *      Trapezoidal pads are not considered here because they are very special case
 *      and are used in microwave applications and they *DO NOT* have a thermal relief that change the shape
 *      by creating stubs and destroy their properties.
 */
void    CreateThermalReliefPadPolygon( std::vector<CPolyPt>& aCornerBuffer,
                                       D_PAD&                aPad,
                                       int                   aThermalGap,
                                       int                   aCopperThickness,
                                       int                   aMinThicknessValue,
                                       int                   aCircleToSegmentsCount,
                                       double                aCorrectionFactor,
                                       int                   aThermalRot )
{
    wxPoint corner, corner_end;
    wxPoint PadShapePos = aPad.ReturnShapePos();    /* Note: for pad having a shape offset,
                                                     * the pad position is NOT the shape position */
    wxSize  copper_thickness;
    int     dx = aPad.m_Size.x / 2;
    int     dy = aPad.m_Size.y / 2;

    int     delta = 3600 / aCircleToSegmentsCount; // rot angle in 0.1 degree

    /* Keep in account the polygon outline thickness
     * aThermalGap must be increased by aMinThicknessValue/2 because drawing external outline
     * with a thickness of aMinThicknessValue will reduce gap by aMinThicknessValue/2
     */
    aThermalGap += aMinThicknessValue / 2;

    /* Keep in account the polygon outline thickness
     * copper_thickness must be decreased by aMinThicknessValue because drawing outlines
     * with a thickness of aMinThicknessValue will increase real thickness by aMinThicknessValue
     */
    aCopperThickness -= aMinThicknessValue;
    if( aCopperThickness < 0 )
        aCopperThickness = 0;

    copper_thickness.x = min( dx, aCopperThickness );
    copper_thickness.y = min( dy, aCopperThickness );

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
        corners_buffer.push_back( wxPoint( copper_thickness.x / 2, copper_thickness.y / 2 ) );

        // Add an intermediate point on spoke sides, to allow a > 90 deg angle between side and first seg of arc approx
        corner.x = copper_thickness.x / 2;
        int y = outer_radius - (aThermalGap / 4);
        corner.y = (int) sqrt( ( ( (double) y * y ) - (double) corner.x * corner.x ) );
        if( aThermalRot != 0 )
            corners_buffer.push_back( corner );

        // calculate the starting point of the outter arc
        corner.x = copper_thickness.x / 2;
        double dtmp =
            sqrt( ( (double) outer_radius * outer_radius ) - ( (double) corner.x * corner.x ) );
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
        // this seems a bug in kbool polygon (exists in 2.0 kbool version)
        // aThermalRot = 450 (45.0 degrees orientation) seems work fine.
        // aThermalRot = 0 with thermal shapes without angle < 90 deg has problems in rare circumstances
        // Note: with the 2 step build ( thermal shapes added after areas are built), 0 seems work
        int angle_pad = aPad.m_Orient;              // Pad orientation
        int th_angle  = aThermalRot;
        for( unsigned ihole = 0; ihole < 4; ihole++ )
        {
            for( unsigned ii = 0; ii < corners_buffer.size(); ii++ )
            {
                corner = corners_buffer[ii];
                RotatePoint( &corner, th_angle + angle_pad );          // Rotate by segment angle and pad orientation
                corner += PadShapePos;
                aCornerBuffer.push_back( CPolyPt( corner.x, corner.y ) );
            }

            aCornerBuffer.back().end_contour = true;
            th_angle += 900;       // Note: th_angle in in 0.1 deg.
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
            EXCHG( copper_thickness.x, copper_thickness.y );
        }
        int deltasize = dx - dy;        // = distance between shape position and the 2 demi-circle ends centre
        // here we have dx > dy
        // Radius of outer arcs of the shape:
        int outer_radius = dy;     // The radius of the outer arc is radius end + aThermalGap

        // Some coordinate fiddling, depending on the shape offset direction
        shape_offset = wxPoint( deltasize, 0 );

        // Crosspoint of thermal spoke sides, the first point of polygon buffer
        corners_buffer.push_back( wxPoint( copper_thickness.x / 2, copper_thickness.y / 2 ) );

        // Arc start point calculation, the intersecting point of cutout arc and thermal spoke edge
        if( copper_thickness.x > deltasize )          // If copper thickness is more than shape offset, we need to calculate arc intercept point.
        {
            corner.x = copper_thickness.x / 2;
            corner.y =
                (int) sqrt( ( (double) outer_radius * outer_radius ) -
                           ( (double) ( corner.x - delta ) * ( corner.x - deltasize ) ) );
            corner.x -= deltasize;

            /* creates an intermediate point, to have a > 90 deg angle
             * between the side and the first segment of arc approximation
             */
            wxPoint intpoint = corner;
            intpoint.y -= aThermalGap / 4;
            corners_buffer.push_back( intpoint + shape_offset );
            RotatePoint( &corner, 90 );
        }
        else
        {
            corner.x = copper_thickness.x / 2;
            corner.y = outer_radius;
            corners_buffer.push_back( corner );
            corner.x = ( deltasize - copper_thickness.x ) / 2;
        }

        // Add an intermediate point on spoke sides, to allow a > 90 deg angle between side and first seg of arc approx
        wxPoint last_corner;
        last_corner.y = copper_thickness.y / 2;
        int     px = outer_radius - (aThermalGap / 4);
        last_corner.x =
            (int) sqrt( ( ( (double) px * px ) - (double) last_corner.y * last_corner.y ) );

        // Arc stop point calculation, the intersecting point of cutout arc and thermal spoke edge
        corner_end.y = copper_thickness.y / 2;
        corner_end.x =
            (int) sqrt( ( (double) outer_radius *
                         outer_radius ) - ( (double) corner_end.y * corner_end.y ) );
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
        int angle = aPad.m_Orient + supp_angle;
        for( int irect = 0; irect < 2; irect++ )
        {
            for( unsigned ic = 0; ic < corners_buffer.size(); ic++ )
            {
                wxPoint cpos = corners_buffer[ic];
                RotatePoint( &cpos, angle );
                cpos += PadShapePos;
                aCornerBuffer.push_back( CPolyPt( cpos.x, cpos.y ) );
            }

            aCornerBuffer.back().end_contour = true;
            angle += 1800;       // this is calculate hole 3
            if( angle >= 3600 )
                angle -= 3600;
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
            for( unsigned ic = 0; ic < corners_buffer.size(); ic++ )
            {
                wxPoint cpos = corners_buffer[ic];
                RotatePoint( &cpos, angle );
                cpos += PadShapePos;
                aCornerBuffer.push_back( CPolyPt( cpos.x, cpos.y ) );
            }

            aCornerBuffer.back().end_contour = true;
            angle += 1800;
            if( angle >= 3600 )
                angle -= 3600;
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

        // Modified rectangles with one corner rounded. TODO: merging with oval thermals and possibly round too.

        std::vector <wxPoint> corners_buffer;               // Polygon buffer as vector

        int dx = (aPad.m_Size.x / 2) + aThermalGap;         // Cutout radius x
        int dy = (aPad.m_Size.y / 2) + aThermalGap;         // Cutout radius y

        // The first point of polygon buffer is left lower corner, second the crosspoint of thermal spoke sides,
        // the third is upper right corner and the rest are rounding vertices going anticlockwise. Note the inveted Y-axis in CG.
        corners_buffer.push_back( wxPoint( -dx, -(aThermalGap / 4 + copper_thickness.y / 2) ) );    // Adds small miters to zone
        corners_buffer.push_back( wxPoint( -(dx - aThermalGap / 4), -copper_thickness.y / 2 ) );    // fill and spoke corner
        corners_buffer.push_back( wxPoint( -copper_thickness.x / 2, -copper_thickness.y / 2 ) );
        corners_buffer.push_back( wxPoint( -copper_thickness.x / 2, -(dy - aThermalGap / 4) ) );
        corners_buffer.push_back( wxPoint( -(aThermalGap / 4 + copper_thickness.x / 2), -dy ) );

        int angle = aPad.m_Orient;
        int rounding_radius = (int) ( aThermalGap * aCorrectionFactor );            // Corner rounding radius
        int angle_pg;                                                               // Polygon increment angle

        for( int i = 0; i < aCircleToSegmentsCount / 4 + 1; i++ )
        {
            wxPoint corner_position = wxPoint( 0, -rounding_radius );
            RotatePoint( &corner_position, 1800 / aCircleToSegmentsCount );         // Start at half increment offset
            angle_pg = i * delta;
            RotatePoint( &corner_position, angle_pg );                              // Rounding vector rotation
            corner_position -= aPad.m_Size / 2;                                     // Rounding vector + Pad corner offset
            corners_buffer.push_back( wxPoint( corner_position.x, corner_position.y ) );
        }

        for( int irect = 0; irect < 2; irect++ )
        {
            for( unsigned ic = 0; ic < corners_buffer.size(); ic++ )
            {
                wxPoint cpos = corners_buffer[ic];
                RotatePoint( &cpos, angle );                                    // Rotate according to module orientation
                cpos += PadShapePos;                                            // Shift origin to position
                aCornerBuffer.push_back( CPolyPt( cpos.x, cpos.y ) );
            }

            aCornerBuffer.back().end_contour = true;
            angle += 1800;       // this is calculate hole 3
            if( angle >= 3600 )
                angle -= 3600;
        }

        // Create holes, that are the mirrored from the previous holes
        for( unsigned ic = 0; ic < corners_buffer.size(); ic++ )
        {
            wxPoint swap = corners_buffer[ic];
            swap.x = -swap.x;
            corners_buffer[ic] = swap;
        }

        // Now add corner 4 and 2 (2 is the corner 4 rotated by 180 deg
        for( int irect = 0; irect < 2; irect++ )
        {
            for( unsigned ic = 0; ic < corners_buffer.size(); ic++ )
            {
                wxPoint cpos = corners_buffer[ic];
                RotatePoint( &cpos, angle );
                cpos += PadShapePos;
                aCornerBuffer.push_back( CPolyPt( cpos.x, cpos.y ) );
            }

            aCornerBuffer.back().end_contour = true;
            angle += 1800;
            if( angle >= 3600 )
                angle -= 3600;
        }

        break;
    }
    }
}
