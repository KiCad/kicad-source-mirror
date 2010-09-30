/********************/
/**** rs274d.cpp ****/
/********************/


#include "fctsys.h"
#include "polygons_defs.h"
#include "common.h"
#include "confirm.h"
#include "macros.h"
#include "gerbview.h"
#include "pcbplot.h"
#include "trigo.h"
#include "protos.h"
#include "class_gerber_draw_item.h"

#include <math.h>

#define IsNumber( x ) ( ( ( (x) >= '0' ) && ( (x) <='9' ) )   \
                       || ( (x) == '-' ) || ( (x) == '+' )  || ( (x) == '.' ) )

/* Gerber: NOTES about some important commands found in RS274D and RS274X (G codes):
 * Gn =
 * G01 linear interpolation (right trace)
 * G02, G20, G21 Circular interpolation, meaning trig <0
 * G03, G30, G31 Circular interpolation, meaning trigo> 0
 * G04 = comment
 * G06 parabolic interpolation
 * G07 Cubic Interpolation
 * G10 linear interpolation (scale x10)
 * G11 linear interpolation (0.1x range)
 * G12 linear interpolation (0.01x scale)
 * G36 Start polygon mode
 * G37 Stop polygon mode (and close it)
 * G54 Selection Tool
 * G60 linear interpolation (scale x100)
 * G70 Select Units = Inches
 * G71 Select Units = Millimeters
 * G74 circular interpolation removes 360 degree (arc draw mode) finishing by G01
 * G75 circular interpolation on 360 degree
 * G90 mode absolute coordinates
 *
 * X, Y
 * X and Y are followed by + or - and m + n digits (not separated)
 * m = integer part
 * n = part after the comma
 * Classic formats: m = 2, n = 3 (size 2.3)
 * m = 3, n = 4 (size 3.4)
 * eg
 * GxxX00345Y-06123*
 *
 * Tools and D_CODES
 * Tool number (identification of shapes)
 * 10 to 999
 * D_CODES:
 * D01 ... D9 = command codes:
 *   D01 = activating light (pen down) when placement
 *   D02 = light extinction (pen up) when placement
 *   D03 = Flash
 *   D09 = VAPE Flash (I never see this command in gerber file)
 *   D51 = G54 preceded by -> Select VAPE
 *
 * D10 ... D999 = Identification Tool: tool selection
 */

// Photoplot actions:
#define GERB_ACTIVE_DRAW 1      // Activate light (lower pen)
#define GERB_STOP_DRAW   2      // Extinguish light (lift pen)
#define GERB_FLASH       3      // Flash

static wxPoint LastPosition;


/* Local Functions (are lower case since they are private to this source file)
**/


/**
 * Function fillCircularGBRITEM
 * initializes a given GBRITEM so that it can draw a circle which is not filled
 * and
 * has a given pen width (\a aPenWidth ).
 *
 * @param aGbrItem The GBRITEM to fill in.
 * @param Dcode_index The DCODE value, like D14
 * @param aLayer The layer index to set into the GBRITEM
 * @param aPos The center point of the flash
 * @param aDiameter The diameter of the round flash
 * @param aPenWidth The width of the pen used to draw the circle's
 * circumference.
 * @param isDark True if flash is positive and should use a drawing
 *   color other than the background color, else use the background color
 *   when drawing so that an erasure happens.
 */
static void fillCircularGBRITEM(  GERBER_DRAW_ITEM* aGbrItem,
                                  int               Dcode_index,
                                  int               aLayer,
                                  const wxPoint&    aPos,
                                  int               aDiameter,
                                  int               aPenWidth,
                                  bool              isDark )
{
    aGbrItem->m_Shape = GBR_CIRCLE;
    aGbrItem->m_Size.x = aGbrItem->m_Size.y = aPenWidth;

    aGbrItem->SetLayer( aLayer );
    aGbrItem->m_DCode = Dcode_index;

    // When drawing a GBRITEM with shape GBR_CIRCLE, the hypotenuse (i.e. distance)
    // between the Start and End points gives the radius of the circle.
    aGbrItem->m_Start  = aGbrItem->m_End = aPos;
    aGbrItem->m_End.x += max( 0, (aDiameter + 1) / 2 );

    NEGATE( aGbrItem->m_Start.y );
    NEGATE( aGbrItem->m_End.y );

    if( !isDark )
    {
        aGbrItem->m_Flags |= DRAW_ERASED;
    }
}


/**
 * Function fillRoundFlashGBRITEM
 * initializes a given GBRITEM so that it can draw a circle which is filled and
 * has no pen border.
 *
 * @param aGbrItem The GBRITEM to fill in.
 * @param Dcode_index The DCODE value, like D14
 * @param aLayer The layer index to set into the GBRITEM
 * @param aPos The center point of the flash
 * @param aDiameter The diameter of the round flash
 * @param isDark True if flash is positive and should use a drawing
 *   color other than the background color, else use the background color
 *   when drawing so that an erasure happens.
 */
static void fillRoundFlashGBRITEM(  GERBER_DRAW_ITEM* aGbrItem,
                                    int               Dcode_index,
                                    int               aLayer,
                                    const wxPoint&    aPos,
                                    int               aDiameter,
                                    bool              isDark )
{
    aGbrItem->SetLayer( aLayer );
    aGbrItem->m_Size.x = aGbrItem->m_Size.y = aDiameter;
    aGbrItem->m_Start = aPos;
    NEGATE( aGbrItem->m_Start.y );
    aGbrItem->m_End = aGbrItem->m_Start;
    aGbrItem->m_DCode = Dcode_index;
    aGbrItem->m_Shape = GBR_SPOT_CIRCLE;
    aGbrItem->m_Flashed = true;

    if( !isDark )
    {
        aGbrItem->m_Flags |= DRAW_ERASED;
    }
}


/**
 * Function fillOvalOrRectFlashGBRITEM
 * initializes a given GBRITEM so that it can draw an oval or rectangular
 * filled rectangle.
 *
 * @param aGbrItem The GERBER_DRAW_ITEM to fill in.
 * @param Dcode_index The DCODE value, like D14
 * @param aLayer The layer index to set into the GBRITEM
 * @param aPos The center point of the rectangle
 * @param aSize The size of the flash
 * @param aShape What type of flash, GBR_SPOT_OVALE or GBR_SPOT_RECT
 * @param isDark True if flash is positive and should use a drawing
 *   color other than the background color, else use the background color
 *   when drawing so that an erasure happens.
 */
static void fillOvalOrRectFlashGBRITEM(  GERBER_DRAW_ITEM* aGbrItem,
                                         int               Dcode_index,
                                         int               aLayer,
                                         const wxPoint&    aPos,
                                         const wxSize&     aSize,
                                         int               aShape,
                                         bool              isDark )
{
    aGbrItem->SetLayer( aLayer );
    aGbrItem->m_Flashed = true;

    aGbrItem->m_Size = aSize;

    aGbrItem->m_Start = aPos;
    NEGATE( aGbrItem->m_Start.y );
    aGbrItem->m_End = aGbrItem->m_Start;

    aGbrItem->m_DCode = Dcode_index;
    aGbrItem->m_Shape = aShape;

    if( !isDark )
    {
        aGbrItem->m_Flags |= DRAW_ERASED;
    }
}


/**
 * Function fillLineGBRITEM
 * initializes a given GBRITEM so that it can draw a linear D code.
 *
 * @param aGbrItem The GERBER_DRAW_ITEM to fill in.
 * @param Dcode_index The DCODE value, like D14
 * @param aLayer The layer index to set into the GBRITEM
 * @param aPos The center point of the flash
 * @param aDiameter The diameter of the round flash
 * @param isDark True if flash is positive and should use a drawing
 *   color other than the background color, else use the background color
 *   when drawing so that an erasure happens.
 */
static void fillLineGBRITEM(  GERBER_DRAW_ITEM* aGbrItem,
                              int               Dcode_index,
                              int               aLayer,
                              const wxPoint&    aStart,
                              const wxPoint&    aEnd,
                              int               aWidth,
                              bool              isDark )
{
    aGbrItem->SetLayer( aLayer );
    aGbrItem->m_Flashed = false;

    aGbrItem->m_Size.x = aGbrItem->m_Size.y = aWidth;

    aGbrItem->m_Start = aStart;
    NEGATE( aGbrItem->m_Start.y );

    aGbrItem->m_End = aEnd;
    NEGATE( aGbrItem->m_End.y );

    aGbrItem->m_DCode = Dcode_index;

    if( !isDark )
    {
        aGbrItem->m_Flags |= DRAW_ERASED;
    }
}


/**
 * Function fillArcGBRITEM
 * initializes a given GBRITEM so that it can draw an arc G code.
 * <p>
 * if multiquadrant == true : arc can be 0 to 360 degrees
 *   and \a rel_center is the center coordinate relative to start point.
 * <p>
 * if multiquadrant == false arc can be only 0 to 90 deg,
 *     and only in the same quadrant :
 * <ul>
 * <li> absolute angle 0 to 90 (quadrant 1) or
 * <li> absolute angle 90 to 180 (quadrant 2) or
 * <li> absolute angle 180 to 270 (quadrant 3) or
 * <li> absolute angle 270 to 0 (quadrant 4)
 * </ul><p>
 * @param GERBER_DRAW_ITEM is the GBRITEM to fill in.
 * @param Dcode_index is the DCODE value, like D14
 * @param aLayer is the layer index to set into the GBRITEM
 * @param aStart is the starting point
 * @param aEnd is the ending point
 * @param rel_center is the center coordinate relative to start point,
 *   given in ABSOLUTE VALUE and the sign of values x et y de rel_center
 *   must be calculated from the previously given constraint: arc only in the
 * same quadrant.
 * @param aDiameter The diameter of the round flash
 * @param aWidth is the pen width.
 * @param isDark True if flash is positive and should use a drawing
 *   color other than the background color, else use the background color
 *   when drawing so that an erasure happens.
 */
static void fillArcGBRITEM(  GERBER_DRAW_ITEM* aGbrItem, int Dcode_index, int aLayer,
                             const wxPoint& aStart, const wxPoint& aEnd,
                             const wxPoint& rel_center, int aWidth,
                             bool clockwise, bool multiquadrant, bool isDark )
{
    wxPoint center, delta;

    aGbrItem->m_Shape = GBR_ARC;
    aGbrItem->SetLayer( aLayer );
    aGbrItem->m_Size.x = aGbrItem->m_Size.y = aWidth;
    aGbrItem->m_Flashed = false;

    if( multiquadrant )
    {
        center.x = aStart.x + rel_center.x;
        center.y = aStart.y + rel_center.y;

        if( clockwise )
        {
            aGbrItem->m_Start = aStart;
            aGbrItem->m_End   = aEnd;
        }
        else
        {
            aGbrItem->m_Start = aEnd;
            aGbrItem->m_End   = aStart;
        }
    }
    else
    {
        center = rel_center;
        delta  = aEnd - aStart;

        if( (delta.x >= 0) && (delta.y >= 0) )
        {
            // Quadrant 2
        }
        else if( (delta.x >= 0) && (delta.y < 0) )
        {
            // Quadrant 1
            center.y = -center.y;
        }
        else if( (delta.x < 0) && (delta.y >= 0) )
        {
            // Quadrant 4
            center.x = -center.x;
        }
        else
        {
            // Quadrant 3
            center.x = -center.x;
            center.y = -center.y;
        }

        center.x += aStart.x;
        center.y += aStart.y;

        if( clockwise )
        {
            aGbrItem->m_Start = aStart;
            aGbrItem->m_End   = aEnd;
        }
        else
        {
            aGbrItem->m_Start = aEnd;
            aGbrItem->m_End   = aStart;
        }
    }

    aGbrItem->m_DCode     = Dcode_index;
    aGbrItem->m_ArcCentre = center;

    NEGATE( aGbrItem->m_Start.y );
    NEGATE( aGbrItem->m_End.y );
    NEGATE( aGbrItem->m_ArcCentre.y );

    if( !isDark )
    {
        aGbrItem->m_Flags |= DRAW_ERASED;
    }
}


/**
 * Function fillArcPOLY
 * creates an arc G code when found in poly outlines.
 * <p>
 * if multiquadrant == true : arc can be 0 to 360 degrees
 *   and \a rel_center is the center coordinate relative to start point.
 * <p>
 * if multiquadrant == false arc can be only 0 to 90 deg,
 *     and only in the same quadrant :
 * <ul>
 * <li> absolute angle 0 to 90 (quadrant 1) or
 * <li> absolute angle 90 to 180 (quadrant 2) or
 * <li> absolute angle 180 to 270 (quadrant 3) or
 * <li> absolute angle 270 to 0 (quadrant 4)
 * </ul><p>
 * @param aPcb is the board.
 * @param aLayer is the layer index to set into the GBRITEM
 * @param aStart is the starting point
 * @param aEnd is the ending point
 * @param rel_center is the center coordinate relative to start point,
 *   given in ABSOLUTE VALUE and the sign of values x et y de rel_center
 *   must be calculated from the previously given constraint: arc only in the
 * same quadrant.
 * @param aDiameter The diameter of the round flash
 * @param aWidth is the pen width.
 * @param isDark True if flash is positive and should use a drawing
 *   color other than the background color, else use the background color
 *   when drawing so that an erasure happens.
 */
static void fillArcPOLY(  BOARD* aPcb, GERBER_DRAW_ITEM* aGrbrItem,
                          const wxPoint& aStart, const wxPoint& aEnd,
                          const wxPoint& rel_center,
                          bool clockwise, bool multiquadrant, bool isDark )
{
    /* in order to calculate arc parameters, we use fillArcGBRITEM
     * so we muse create a dummy track and use its geometric parameters
     */
    static GERBER_DRAW_ITEM dummyGbrItem( NULL );

    fillArcGBRITEM(  &dummyGbrItem, 0, 0,
                     aStart, aEnd, rel_center, 0,
                     clockwise, multiquadrant, isDark );

    // dummyTrack has right geometric parameters, and has its Y coordinates negated (to match the pcbnew Y axis).
    // Approximate arc by 36 segments per 360 degree
    const int increment_angle = 360 / 36;

    wxPoint   center;
    center = dummyGbrItem.m_ArcCentre;

    // Calculate relative coordinates;
    wxPoint start = dummyGbrItem.m_Start - center;
    wxPoint end   = dummyGbrItem.m_End - center;

    /* Calculate angle arc
     * angle is here clockwise because Y axis is reversed
     */
    double start_angle = atan2( (double) start.y, (double) start.x );
    start_angle = 180 * start_angle / M_PI;
    double end_angle = atan2( (double) end.y, (double) end.x );
    end_angle = 180 * end_angle / M_PI;
    double angle = start_angle - end_angle;

//    D( printf( " >>>> st %d,%d angle %f, end %d,%d angle %f delta %f\n",
//        start.x, start.y, start_angle, end.x, end.y, end_angle, angle ) );
    if( !clockwise )
    {
        EXCHG( start, end );
        D( printf( " >>>> reverse arc\n" ) );
    }

    // Normalize angle
    while( angle > 360.0 )
        angle -= 360.0;

    while( angle < 0.0 )
        angle += 360.0;

    int count = (int) ( angle / increment_angle );
    if( count <= 0 )
        count = 1;

//    D( printf( " >>>> angle %f, cnt %d sens %d\n", angle, count, clockwise ) );

    // calculate segments
    wxPoint start_arc = start;
    for( int ii = 1; ii <= count; ii++ )
    {
        wxPoint end_arc = start;
        int     rot     = 10 * ii * increment_angle; // rot is in 0.1 deg
        if( ii < count )
        {
            if( clockwise )
                RotatePoint( &end_arc, rot );
            else
                RotatePoint( &end_arc, -rot );
        }
        else
            end_arc = end;

//        D( printf( " >> Add arc %d rot %d, edge poly item %d,%d to %d,%d\n",
//                ii, rot, start_arc.x, start_arc.y,end_arc.x, end_arc.y ); )

        if( aGrbrItem->m_PolyCorners.size() == 0 )
            aGrbrItem->m_PolyCorners.push_back( start_arc + center );
        aGrbrItem->m_PolyCorners.push_back( end_arc + center );

        if( !isDark )
        {
            aGrbrItem->m_Flags |= DRAW_ERASED;
        }
        start_arc = end_arc;
    }
}


/* These routines read the text string point from Text.
 * After use, advanced Text the beginning of the sequence unread
 */
wxPoint GERBER::ReadXYCoord( char*& Text )
{
    wxPoint pos = m_CurrentPos;
    int     type_coord = 0, current_coord, nbchar;
    bool    is_float   = false;
    char*   text;
    char    line[256];


    if( m_Relative )
        pos.x = pos.y = 0;
    else
        pos = m_CurrentPos;

    if( Text == NULL )
        return pos;

    text = line;
    while( *Text )
    {
        if( (*Text == 'X') || (*Text == 'Y') )
        {
            type_coord = *Text;
            Text++;
            text   = line;
            nbchar = 0;
            while( IsNumber( *Text ) )
            {
                if( *Text == '.' )
                    is_float = true;
                *(text++) = *(Text++);
                if( (*Text >= '0') && (*Text <='9') )
                    nbchar++;
            }

            *text = 0;
            if( is_float )
            {
                if( m_GerbMetric )
                    current_coord = wxRound( atof( line ) / 0.00254 );
                else
                    current_coord = wxRound( atof( line ) * PCB_INTERNAL_UNIT );
            }
            else
            {
                int fmt_scale =
                    (type_coord == 'X') ? m_FmtScale.x : m_FmtScale.y;
                if( m_NoTrailingZeros )
                {
                    int min_digit =
                        (type_coord == 'X') ? m_FmtLen.x : m_FmtLen.y;
                    while( nbchar < min_digit )
                    {
                        *(text++) = '0';
                        nbchar++;
                    }

                    *text = 0;
                }
                current_coord = atoi( line );
                double real_scale = 1.0;
                if( fmt_scale < 0 || fmt_scale > 9 )
                    fmt_scale = 4;
                double scale_list[10] =
                    { 10000.0, 1000.0, 100.0, 10.0,
                        1,
                        0.1, 0.01, 0.001, 0.0001, 0.00001
                    };
                real_scale = scale_list[fmt_scale];

                if( m_GerbMetric )
                    real_scale = real_scale / 25.4;

                current_coord = wxRound( current_coord * real_scale );
            }

            if( type_coord == 'X' )
                pos.x = current_coord;
            else if( type_coord == 'Y' )
                pos.y = current_coord;

            continue;
        }
        else
            break;
    }

    if( m_Relative )
    {
        pos.x += m_CurrentPos.x;
        pos.y += m_CurrentPos.y;
    }

    m_CurrentPos = pos;
    return pos;
}


/* Returns the current coordinate type pointed to by InnJnn Text (InnnnJmmmm)
 * These coordinates are relative, so if coordinate is absent, it's value
 * defaults to 0
 */
wxPoint GERBER::ReadIJCoord( char*& Text )
{
    wxPoint pos( 0, 0 );

    int     type_coord = 0, current_coord, nbchar;
    bool    is_float   = false;
    char*   text;
    char    line[256];

    if( Text == NULL )
        return pos;

    text = line;
    while( *Text )
    {
        if( (*Text == 'I') || (*Text == 'J') )
        {
            type_coord = *Text;
            Text++;
            text   = line;
            nbchar = 0;
            while( IsNumber( *Text ) )
            {
                if( *Text == '.' )
                    is_float = true;
                *(text++) = *(Text++);
                if( (*Text >= '0') && (*Text <='9') )
                    nbchar++;
            }

            *text = 0;
            if( is_float )
            {
                if( m_GerbMetric )
                    current_coord = wxRound( atof( line ) / 0.00254 );
                else
                    current_coord = wxRound( atof( line ) * PCB_INTERNAL_UNIT );
            }
            else
            {
                int fmt_scale =
                    (type_coord == 'I') ? m_FmtScale.x : m_FmtScale.y;
                if( m_NoTrailingZeros )
                {
                    int min_digit =
                        (type_coord == 'I') ? m_FmtLen.x : m_FmtLen.y;
                    while( nbchar < min_digit )
                    {
                        *(text++) = '0';
                        nbchar++;
                    }

                    *text = 0;
                }
                current_coord = atoi( line );
                double real_scale = 1.0;

                switch( fmt_scale )
                {
                case 0:
                    real_scale = 10000.0;
                    break;

                case 1:
                    real_scale = 1000.0;
                    break;

                case 2:
                    real_scale = 100.0;
                    break;

                case 3:
                    real_scale = 10.0;
                    break;

                case 4:
                    break;

                case 5:
                    real_scale = 0.1;
                    break;

                case 6:
                    real_scale = 0.01;
                    break;

                case 7:
                    real_scale = 0.001;
                    break;

                case 8:
                    real_scale = 0.0001;
                    break;

                case 9:
                    real_scale = 0.00001;
                    break;
                }

                if( m_GerbMetric )
                    real_scale = real_scale / 25.4;
                current_coord = wxRound( current_coord * real_scale );
            }
            if( type_coord == 'I' )
                pos.x = current_coord;
            else if( type_coord == 'J' )
                pos.y = current_coord;
            continue;
        }
        else
            break;
    }

    m_IJPos = pos;
    return pos;
}


/* Read the Gnn sequence and returns the value nn.
 */
int GERBER::ReturnGCodeNumber( char*& Text )
{
    int   ii = 0;
    char* text;
    char  line[1024];

    if( Text == NULL )
        return 0;
    Text++;
    text = line;
    while( IsNumber( *Text ) )
    {
        *(text++) = *(Text++);
    }

    *text = 0;
    ii    = atoi( line );
    return ii;
}


/* Get the sequence Dnn and returns the value nn
 */
int GERBER::ReturnDCodeNumber( char*& Text )
{
    int   ii = 0;
    char* text;
    char  line[1024];

    if( Text == NULL )
        return 0;

    Text++;
    text = line;
    while( IsNumber( *Text ) )
        *(text++) = *(Text++);

    *text = 0;
    ii    = atoi( line );
    return ii;
}


bool GERBER::Execute_G_Command( char*& text, int G_commande )
{
    D( printf( "%22s: G_CODE<%d>\n", __func__, G_commande ); )

    switch( G_commande )
    {
    case GC_PHOTO_MODE:     // can starts a D03 flash command: redundant, can
                            // be safely ignored
        break;

    case GC_LINEAR_INTERPOL_1X:
        m_Iterpolation = GERB_INTERPOL_LINEAR_1X;
        break;

    case GC_CIRCLE_NEG_INTERPOL:
        m_Iterpolation = GERB_INTERPOL_ARC_NEG;
        break;

    case GC_CIRCLE_POS_INTERPOL:
        m_Iterpolation = GERB_INTERPOL_ARC_POS;
        break;

    case GC_COMMENT:
        text = NULL;
        break;

    case GC_LINEAR_INTERPOL_10X:
        m_Iterpolation = GERB_INTERPOL_LINEAR_10X;
        break;

    case GC_LINEAR_INTERPOL_0P1X:
        m_Iterpolation = GERB_INTERPOL_LINEAR_01X;
        break;

    case GC_LINEAR_INTERPOL_0P01X:
        m_Iterpolation = GERB_INTERPOL_LINEAR_001X;
        break;

    case GC_SELECT_TOOL:
    {
        int D_commande = ReturnDCodeNumber( text );
        if( D_commande < FIRST_DCODE )
            return false;
        if( D_commande > (TOOLS_MAX_COUNT - 1) )
            D_commande = TOOLS_MAX_COUNT - 1;
        m_Current_Tool = D_commande;
        D_CODE* pt_Dcode = GetDCODE( D_commande, false );
        if( pt_Dcode )
            pt_Dcode->m_InUse = true;
        break;
    }

    case GC_SPECIFY_INCHES:
        m_GerbMetric = false;           // false = Inches, true = metric
        break;

    case GC_SPECIFY_MILLIMETERS:
        m_GerbMetric = true;            // false = Inches, true = metric
        break;

    case GC_TURN_OFF_360_INTERPOL:
        m_360Arc_enbl = false;
        break;

    case GC_TURN_ON_360_INTERPOL:
        m_360Arc_enbl = true;
        break;

    case GC_SPECIFY_ABSOLUES_COORD:
        m_Relative = false;         // false = absolute Coord, true = relative
                                    // Coord
        break;

    case GC_SPECIFY_RELATIVEES_COORD:
        m_Relative = true;          // false = absolute Coord, true = relative
                                    // Coord
        break;

    case GC_TURN_ON_POLY_FILL:
        m_PolygonFillMode = true;
        break;

    case GC_TURN_OFF_POLY_FILL:
        m_PolygonFillMode = false;
        m_PolygonFillModeState = 0;
        break;

    case GC_MOVE:       // Non existent
    default:
    {
        wxString msg; msg.Printf( wxT( "G%0.2d command not handled" ),
                                  G_commande );
        DisplayError( NULL, msg );
        return false;
    }
    }


    return true;
}


/**
 * Function scale
 * converts a distance given in floating point to our deci-mils
 */
static int scale( double aCoord, bool isMetric )
{
    int ret;

    if( isMetric )
        ret = wxRound( aCoord / 0.00254 );
    else
        ret = wxRound( aCoord * PCB_INTERNAL_UNIT );

    return ret;
}


/**
 * Function mapPt
 * translates a point from the aperture macro coordinate system to our
 * deci-mils coordinate system.
 * @return wxPoint - The gerbview coordinate system vector.
 */
static wxPoint mapPt( double x, double y, bool isMetric )
{
    wxPoint ret( scale( x, isMetric ),
                scale( y, isMetric ) );

    return ret;
}


/**
 * Function mapExposure
 * translates the first parameter from an aperture macro into a current
 * exposure
 * setting.
 * @param curExposure A dynamic setting which can change throughout the
 * reading of the gerber file, and it indicates whether the current tool
 * is lit or not.
 * @param isNegative A dynamic setting which can change throughout the reading
 * of
 *    the gerber file, and it indicates whether the current D codes are to
 *    be interpreted as erasures or not.
 */
static bool mapExposure( int param1, bool curExposure, bool isNegative )
{
    bool exposure;

    switch( param1 )
    {
    case 0:
        exposure = false;
        break;

    default:
    case 1:
        exposure = true;
        break;

    case 2:
        exposure = !curExposure;
    }

    return exposure ^ isNegative;
}


bool GERBER::Execute_DCODE_Command( WinEDA_GerberFrame* frame,
                                    char*& text, int D_commande )
{
    wxSize            size( 15, 15 );

    APERTURE_T        aperture = APT_CIRCLE;
    GERBER_DRAW_ITEM* gbritem;
    BOARD*            pcb = frame->GetBoard();

    int      activeLayer = frame->GetScreen()->m_Active_Layer;

    int      dcode = 0;
    D_CODE*  tool  = NULL;
    wxString msg;

    D( printf( "%22s: D_CODE<%d>\n", __func__, D_commande ); )

    if( D_commande >= FIRST_DCODE )  // This is a "Set tool" command
    {
        if( D_commande > (TOOLS_MAX_COUNT - 1) )
            D_commande = TOOLS_MAX_COUNT - 1;

        // remember which tool is selected, nothing is done with it in this
        // call
        m_Current_Tool = D_commande;

        D_CODE* pt_Dcode = GetDCODE( D_commande, false );
        if( pt_Dcode )
            pt_Dcode->m_InUse = true;

        return true;
    }
    else // D_commande = 0..9:  this is a pen command (usually D1, D2 or D3)
    {
        m_Last_Pen_Command = D_commande;
    }

    if( m_PolygonFillMode )    // Enter a polygon description:
    {
        switch( D_commande )
        {
        case 1:     // code D01 Draw line, exposure ON
            if( !m_Exposure )
            {
                m_Exposure = true;
                gbritem    = new GERBER_DRAW_ITEM( pcb );
                pcb->m_Drawings.Append( gbritem );
                gbritem->m_Shape = GBR_POLYGON;
                gbritem->SetLayer( activeLayer );
                gbritem->m_Flashed = false;
            }

            switch( m_Iterpolation )
            {
            case GERB_INTERPOL_ARC_NEG:
            case GERB_INTERPOL_ARC_POS:
                gbritem = (GERBER_DRAW_ITEM*)( pcb->m_Drawings.GetLast() );
                D( printf( "Add arc poly %d,%d to %d,%d fill %d interpol %d 360_enb %d\n",
                           m_PreviousPos.x, m_PreviousPos.y, m_CurrentPos.x,
                           m_CurrentPos.y, m_PolygonFillModeState, m_Iterpolation, m_360Arc_enbl ); )
                fillArcPOLY( pcb, gbritem, m_PreviousPos,
                            m_CurrentPos, m_IJPos,
                            ( m_Iterpolation == GERB_INTERPOL_ARC_NEG ) ?
                            false : true, m_360Arc_enbl,
                            !(m_LayerNegative ^ m_ImageNegative) );
                break;

            default:
                gbritem = (GERBER_DRAW_ITEM*)( pcb->m_Drawings.GetLast() );
                D( printf( "Add poly edge %d,%d to %d,%d fill %d\n",
                           m_PreviousPos.x, m_PreviousPos.y,
                           m_CurrentPos.x, m_CurrentPos.y, m_Iterpolation ); )

                gbritem->m_Start = m_PreviousPos;       // m_Start is used as temporary storage
                NEGATE( gbritem->m_Start.y );
                if( gbritem->m_PolyCorners.size() == 0 )
                    gbritem->m_PolyCorners.push_back( gbritem->m_Start );

                gbritem->m_End = m_CurrentPos;       // m_End is used as temporary storage
                NEGATE( gbritem->m_End.y );
                gbritem->m_PolyCorners.push_back( gbritem->m_End );

                // Set the erasure flag of gbritem if a negative polygon.
                if( !m_PolygonFillModeState )
                {
                    if( m_LayerNegative ^ m_ImageNegative )
                        gbritem->m_Flags |= DRAW_ERASED;
                    D( printf( "\nm_Flags=0x%08X\n", gbritem->m_Flags ); )
                }
                break;
            }

            m_PreviousPos = m_CurrentPos;
            m_PolygonFillModeState = 1;
            break;

        case 2:     // code D2: exposure OFF (i.e. "move to")
            m_Exposure    = false;
            m_PreviousPos = m_CurrentPos;
            m_PolygonFillModeState = 0;
            break;

        default:
            return false;
        }
    }
    else
    {
        switch( D_commande )
        {
        case 1:     // code D01 Draw line, exposure ON
            m_Exposure = true;

            tool = GetDCODE( m_Current_Tool, false );
            if( tool )
            {
                size     = tool->m_Size;
                dcode    = tool->m_Num_Dcode;
                aperture = tool->m_Shape;
            }

            switch( m_Iterpolation )
            {
            case GERB_INTERPOL_LINEAR_1X:
                gbritem = new GERBER_DRAW_ITEM( pcb );
                pcb->m_Drawings.Append( gbritem );
                D( printf( "R:%p\n", gbritem ); )
                fillLineGBRITEM( gbritem, dcode, activeLayer, m_PreviousPos,
                                m_CurrentPos, size.x,
                                !(m_LayerNegative ^ m_ImageNegative) );
                break;

            case GERB_INTERPOL_LINEAR_01X:
            case GERB_INTERPOL_LINEAR_001X:
            case GERB_INTERPOL_LINEAR_10X:
                wxBell();
                break;

            case GERB_INTERPOL_ARC_NEG:
            case GERB_INTERPOL_ARC_POS:
                gbritem = new GERBER_DRAW_ITEM( pcb );
                pcb->m_Drawings.Append( gbritem );
                D( printf( "R:%p\n", gbritem ); )
                fillArcGBRITEM( gbritem, dcode, activeLayer, m_PreviousPos,
                               m_CurrentPos, m_IJPos, size.x,
                               ( m_Iterpolation == GERB_INTERPOL_ARC_NEG ) ?
                               false : true, m_360Arc_enbl,
                               !(m_LayerNegative ^ m_ImageNegative) );
                break;

            default:
                msg.Printf( wxT( "Execute_DCODE_Command: interpol error (type %X)" ),
                            m_Iterpolation );
                DisplayError( frame, msg );
                break;
            }

            m_PreviousPos = m_CurrentPos;
            break;

        case 2:     // code D2: exposure OFF (i.e. "move to")
            m_Exposure    = false;
            m_PreviousPos = m_CurrentPos;
            break;

        case 3:     // code D3: flash aperture
            tool = GetDCODE( m_Current_Tool, false );
            if( tool )
            {
                size     = tool->m_Size;
                dcode    = tool->m_Num_Dcode;
                aperture = tool->m_Shape;
            }

            switch( aperture )
            {
            case APT_POLYGON:   // flashed regular polygon
            case APT_CIRCLE:
                gbritem = new GERBER_DRAW_ITEM( pcb );
                pcb->m_Drawings.Append( gbritem );
                D( printf( "R:%p\n", gbritem ); )
                fillRoundFlashGBRITEM( gbritem, dcode, activeLayer,
                                      m_CurrentPos, size.x,
                                      !(m_LayerNegative ^ m_ImageNegative) );
                if( aperture == APT_POLYGON )
                    gbritem->m_Shape = GBR_SPOT_POLY;
                break;

            case APT_OVAL:
            case APT_RECT:
                gbritem = new GERBER_DRAW_ITEM( pcb );
                pcb->m_Drawings.Append( gbritem );
                D( printf( "R:%p\n", gbritem ); )
                fillOvalOrRectFlashGBRITEM( gbritem, dcode, activeLayer,
                                           m_CurrentPos, size,
                                           ( aperture == APT_RECT ) ?
                                           GBR_SPOT_RECT : GBR_SPOT_OVAL,
                                           !(m_LayerNegative ^ m_ImageNegative) );
                break;

            case APT_MACRO:
            {
                APERTURE_MACRO* macro = tool->GetMacro();
                wxASSERT( macro );

                // split the macro primitives up into multiple normal GBRITEM
                // elements
                for( AM_PRIMITIVES::iterator p = macro->primitives.begin();
                     p!=macro->primitives.end();
                     ++p )
                {
                    bool    exposure;
                    wxPoint curPos = m_CurrentPos;

                    switch( p->primitive_id )
                    {
                    case AMP_CIRCLE:
                    {
                        exposure = mapExposure( p->GetExposure(), m_Exposure,
                                                m_ImageNegative );
                        curPos += mapPt( p->params[2].GetValue( tool ),
                                         p->params[3].GetValue( tool ),
                                         m_GerbMetric );
                        int diameter = scale( p->params[1].GetValue( tool ),
                                              m_GerbMetric );

                        gbritem = new GERBER_DRAW_ITEM( pcb );
                        pcb->m_Drawings.Append( gbritem );
                        D( printf( "R:%p\n", gbritem ); )
                        fillRoundFlashGBRITEM( gbritem, dcode, activeLayer,
                                               curPos, diameter, exposure );
                    }
                    break;

                    case AMP_LINE2:
                    case AMP_LINE20:
                    {
                        exposure = mapExposure(
                            p->GetExposure(), m_Exposure, m_ImageNegative );
                        int     width = scale( p->params[1].GetValue( tool ),
                                               m_GerbMetric );
                        wxPoint start = mapPt( p->params[2].GetValue( tool ),
                                               p->params[3].GetValue( tool ),
                                               m_GerbMetric );
                        wxPoint end = mapPt( p->params[4].GetValue( tool ),
                                             p->params[5].GetValue( tool ),
                                             m_GerbMetric );

                        if( start.x == end.x )
                        {
                            size.x = width;
                            size.y = ABS( end.y - start.y ) + 1;
                        }
                        else
                        {
                            size.x = ABS( end.x - start.x ) + 1;
                            size.y = width;
                        }

                        wxPoint midPoint( ( start.x + end.x ) / 2,
                                         ( start.y + end.y ) / 2 );
                        curPos += midPoint;
                        gbritem = new GERBER_DRAW_ITEM( pcb );
                        pcb->m_Drawings.Append( gbritem );
                        D( printf( "R:%p\n", gbritem ); )
                        fillOvalOrRectFlashGBRITEM( gbritem, dcode, activeLayer,
                                                    curPos, size, GBR_SPOT_RECT,
                                                    exposure );
                    }
                    break;

                    case AMP_LINE_CENTER:
                    {
                        exposure = mapExposure( p->GetExposure(), m_Exposure,
                                                m_ImageNegative );
                        wxPoint msize = mapPt( p->params[1].GetValue( tool ),
                                               p->params[2].GetValue( tool ),
                                               m_GerbMetric );
                        size.x  = msize.x;
                        size.y  = msize.y;
                        curPos += mapPt( p->params[3].GetValue( tool ),
                                         p->params[4].GetValue( tool ),
                                         m_GerbMetric );
                        gbritem = new GERBER_DRAW_ITEM( pcb );
                        pcb->m_Drawings.Append( gbritem );
                        D( printf( "R:%p\n", gbritem ); )
                        fillOvalOrRectFlashGBRITEM( gbritem, dcode, activeLayer,
                                                    curPos, size, GBR_SPOT_RECT,
                                                    exposure );
                    }
                    break;

                    case AMP_LINE_LOWER_LEFT:
                    {
                        exposure = mapExposure(
                            p->GetExposure(), m_Exposure, m_ImageNegative );
                        wxPoint msize = mapPt( p->params[1].GetValue( tool ),
                                               p->params[2].GetValue( tool ),
                                               m_GerbMetric );
                        size.x = msize.x;
                        size.y = msize.y;
                        wxPoint lowerLeft = mapPt( p->params[3].GetValue( tool ),
                                                   p->params[4].GetValue( tool ),
                                                   m_GerbMetric );
                        curPos += lowerLeft;

                        // need the middle, so adjust from the lower left
                        curPos.y += size.y / 2;
                        curPos.x += size.x / 2;
                        gbritem   = new GERBER_DRAW_ITEM( pcb );
                        pcb->m_Drawings.Append( gbritem );
                        D( printf( "R:%p\n", gbritem ); )
                        fillOvalOrRectFlashGBRITEM( gbritem, dcode, activeLayer,
                                                    curPos, size, GBR_SPOT_RECT,
                                                    exposure );
                    }
                    break;

                    case AMP_THERMAL:
                    {
                        int outerDiam = scale( p->params[2].GetValue( tool ),
                                               m_GerbMetric );
                        int innerDiam = scale( p->params[3].GetValue( tool ),
                                               m_GerbMetric );

                        curPos += mapPt( p->params[0].GetValue( tool ),
                                         p->params[1].GetValue( tool ),
                                         m_GerbMetric );

                        gbritem = new GERBER_DRAW_ITEM( pcb );
                        pcb->m_Drawings.Append( gbritem );
                        D( printf( "R:%p\n", gbritem ); )
                        fillRoundFlashGBRITEM( gbritem, dcode, activeLayer,
                                              curPos, outerDiam,
                                              !( m_LayerNegative ^ m_ImageNegative ) );

                        gbritem = new GERBER_DRAW_ITEM( pcb );
                        pcb->m_Drawings.Append( gbritem );
                        D( printf( "R:%p\n", gbritem ); )
                        fillRoundFlashGBRITEM( gbritem, dcode, activeLayer, curPos,
                                              innerDiam,
                                              ( m_LayerNegative ^ m_ImageNegative ) );

                        // @todo: draw the cross hairs, see page 23 of rs274
                        // spec. this might be done with two lines, thickness
                        // from params[4], and drawing
                        // darkness "(m_LayerNegative ^ m_ImageNegative)"
                    }
                    break;

                    case AMP_MOIRE:
                    {
                        curPos += mapPt( p->params[0].GetValue( tool ),
                                         p->params[1].GetValue( tool ),
                                         m_GerbMetric );

                        // e.g.: "6,0,0,0.125,.01,0.01,3,0.003,0.150,0"
                        int outerDiam = scale( p->params[2].GetValue( tool ),
                                               m_GerbMetric );
                        int penThickness = scale( p->params[3].GetValue( tool ),
                                                  m_GerbMetric );
                        int gap = scale( p->params[4].GetValue( tool ),
                                         m_GerbMetric );
                        int numCircles = (int) p->params[5].GetValue( tool );
                        int crossHairThickness =
                            scale( p->params[6].GetValue( tool ), m_GerbMetric );
                        int crossHairLength =
                            scale( p->params[7].GetValue( tool ), m_GerbMetric );

                        // ignore rotation, not supported
                        // adjust outerDiam by this on each nested circle
                        int diamAdjust = 2 * (gap + penThickness);
                        for( int i = 0; i < numCircles; ++i, outerDiam -= diamAdjust )
                        {
                            gbritem = new GERBER_DRAW_ITEM( pcb );
                            pcb->m_Drawings.Append( gbritem );
                            D( printf( "R:%p\n", gbritem ); )
                            fillCircularGBRITEM( gbritem, dcode, activeLayer,
                                                curPos, outerDiam,
                                                penThickness,
                                                !( m_LayerNegative ^ m_ImageNegative ) );
                        }

                        gbritem = new GERBER_DRAW_ITEM( pcb );
                        pcb->m_Drawings.Append( gbritem );
                        D( printf( "R:%p\n", gbritem ); )
                        fillOvalOrRectFlashGBRITEM( gbritem, dcode, activeLayer,
                                                   curPos,
                                                   wxSize( crossHairThickness,
                                                           crossHairLength ),
                                                   GBR_SPOT_RECT,
                                                   !( m_LayerNegative ^ m_ImageNegative ) );

                        gbritem = new GERBER_DRAW_ITEM( pcb );
                        pcb->m_Drawings.Append( gbritem );
                        D( printf( "R:%p\n", gbritem ); )

                        // swap x and y in wxSize() for this one
                        fillOvalOrRectFlashGBRITEM( gbritem, dcode, activeLayer,
                                                   curPos,
                                                   wxSize( crossHairLength,
                                                           crossHairThickness ),
                                                   GBR_SPOT_RECT,
                                                   !( m_LayerNegative ^ m_ImageNegative ) );
                    }
                    break;

                    case AMP_OUTLINE:
#if defined(DEBUG)
                        {
                            int numPoints = (int) p->params[1].GetValue( tool );

                            printf( "AMP_OUTLINE:\n" );
                            printf( " exposure: %g\n", p->params[0].GetValue( tool ) );
                            printf( " # points: %d\n", numPoints );

                            // numPoints does not include the starting point, so add 1.
                            for( int i = 0;  i<numPoints + 1;  ++i )
                            {
                                printf( " [%d]: X=%g  Y=%g\n", i,
                                        p->params[i * 2 + 2 + 0].GetValue( tool ),
                                        p->params[i * 2 + 2 + 1].GetValue( tool )
                                        );
                            }

                            printf( " rotation: %g\n", p->params[numPoints * 2 + 4].GetValue( tool ) );
                        }
#endif
                        break;

                    case AMP_POLYGON:
                    case AMP_EOF:
                    default:

                        // not yet supported, waiting for you.
                        break;
                    }
                }
            }
            break;

            default:
                break;
            }

            m_PreviousPos = m_CurrentPos;
            break;

        default:
            return false;
        }
    }

    return true;
}
