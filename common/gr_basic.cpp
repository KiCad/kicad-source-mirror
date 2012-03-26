/********************************/
/* Low level graphics routines  */
/********************************/


#include <fctsys.h>
#include <gr_basic.h>
#include <common.h>
#include <trigo.h>
#include <macros.h>
#include <base_struct.h>
#include <class_base_screen.h>
#include <bezier_curves.h>
#include <math_for_graphics.h>
#include <wx/graphics.h>


#ifndef FILLED
#define FILLED 1
#endif

/* Important Note:
 * These drawing functions  clip draw item before send these items to wxDC draw
 * functions.  For guy who asks why i did it, see a sample of problems encountered
 * when pixels
 * coordinates overflow 16 bits values:
 * http://trac.wxwidgets.org/ticket/10446
 * Problems can be found under Windows **and** Linux (mainly when drawing arcs)
 * (mainly at low zoom values (2, 1 or 0.5), in Pcbnew)
 * some of these problems could be now fixed in recent distributions.
 *
 * Currently (feb 2009) there are overflow problems when drawing solid (filled)
 * polygons under linux without clipping
 *
 * So before removing clipping functions, be aware these bug (they are not in
 * KiCad or wxWidgets) are fixed by testing how are drawn complex lines arcs
 * and solid polygons under Windows and Linux and remember users can have old
 * versions with bugs
 */


/* Definitions for enabling and disabling debugging features in gr_basic.cpp.
 * Please remember to set these back to 0 before making LAUNCHPAD commits.
 */
#define DEBUG_DUMP_CLIP_ERROR_COORDS 0  // Set to 1 to dump clip algorithm errors.
#define DEBUG_DUMP_CLIP_COORDS       0  // Set to 1 to dump clipped coordinates.


// For draw mode = XOR GR_XOR or GR_NXOR by background color
int g_XorMode = GR_NXOR;

// Background color of the design frame
int g_DrawBgColor = WHITE;


#define USE_CLIP_FILLED_POLYGONS

#ifdef USE_CLIP_FILLED_POLYGONS
static void ClipAndDrawFilledPoly( EDA_RECT * ClipBox, wxDC * DC, wxPoint Points[], int n );
#endif

/* These functions are used by corresponding functions
 * ( GRSCircle is called by GRCircle for instance) after mapping coordinates
 * from user units to screen units(pixels coordinates)
 */
static void GRSRect( EDA_RECT* aClipBox, wxDC* aDC, int x1, int y1,
                     int x2, int y2, int aWidth, int aColor,
                     wxPenStyle aStyle = wxPENSTYLE_SOLID );

/**/

static int          GRLastMoveToX, GRLastMoveToY;
static bool         s_ForceBlackPen;   /* if true: draws in black instead of
                                        * color for printing. */
static int          xcliplo = 0,
                    ycliplo = 0,
                    xcliphi = 2000,
                    ycliphi = 2000;

static int   s_DC_lastcolor = -1;
static int   s_DC_lastwidth = -1;
static int   s_DC_lastpenstyle   = -1;
static int   s_DC_lastbrushcolor = -1;
static int   s_DC_lastbrushfill  = -1;
static wxDC* s_DC_lastDC = NULL;


/**
 * Test if any part of a line falls within the bounds of a rectangle.
 *
 * Please note that this is only accurate for lines that are one pixel wide.
 *
 * @param aClipBox - The rectangle to test.
 * @param x1 - X coordinate of one end of a line.
 * @param y1 - Y coordinate of one end of a line.
 * @param x2 - X coordinate of the other end of a line.
 * @param y2 - Y coordinate of the other  end of a line.
 *
 * @return - False if any part of the line lies within the rectangle.
 */
static bool clipLine( EDA_RECT* aClipBox, int& x1, int& y1, int& x2, int& y2 )
{
    if( aClipBox->Contains( x1, y1 ) && aClipBox->Contains( x2, y2 ) )
        return false;

    wxRect rect = *aClipBox;
    int    minX = rect.GetLeft();
    int    maxX = rect.GetRight();
    int    minY = rect.GetTop();
    int    maxY = rect.GetBottom();
    int    clippedX, clippedY;

#if DEBUG_DUMP_CLIP_COORDS
    int    tmpX1, tmpY1, tmpX2, tmpY2;
    tmpX1 = x1;
    tmpY1 = y1;
    tmpX2 = x2;
    tmpY2 = y2;
#endif

    if( aClipBox->Contains( x1, y1 ) )
    {
        if( x1 == x2 )         /* Vertical line, clip Y. */
        {
            if( y2 < minY )
            {
                y2 = minY;
                return false;
            }

            if( y2 > maxY )
            {
                y2 = maxY;
                return false;
            }
        }
        else if( y1 == y2 )    /* Horizontal line, clip X. */
        {
            if( x2 < minX )
            {
                x2 = minX;
                return false;
            }

            if( x2 > maxX )
            {
                x2 = maxX;
                return false;
            }
        }

        /* If we're here, it's a diagonal line. */

        if( TestForIntersectionOfStraightLineSegments( x1, y1, x2, y2, minX, minY, minX, maxY,
                                                       &clippedX, &clippedY )       /* Left */
           || TestForIntersectionOfStraightLineSegments( x1, y1, x2, y2, minX, minY, maxX, minY,
                                                         &clippedX, &clippedY )     /* Top */
           || TestForIntersectionOfStraightLineSegments( x1, y1, x2, y2, maxX, minY, maxX, maxY,
                                                         &clippedX, &clippedY )     /* Right */
           || TestForIntersectionOfStraightLineSegments( x1, y1, x2, y2, minX, maxY, maxX, maxY,
                                                         &clippedX, &clippedY ) )   /* Bottom */
        {
            if( x2 != clippedX )
                x2 = clippedX;
            if( y2 != clippedY )
                y2 = clippedY;
            return false;
        }

        /* If we're here, something has gone terribly wrong. */
#if DEBUG_DUMP_CLIP_ERROR_COORDS
        wxLogDebug( wxT( "Line (%d,%d):(%d,%d) in rectangle (%d,%d,%d,%d) clipped to (%d,%d,%d,%d)" ),
                    tmpX1, tmpY1, tmpX2, tmpY2, minX, minY, maxX, maxY, x1, y1, x2, y2 );
#endif
        return false;
    }
    else if( aClipBox->Contains( x2, y2 ) )
    {
        if( x1 == x2 )         /* Vertical line, clip Y. */
        {
            if( y2 < minY )
            {
                y2 = minY;
                return false;
            }

            if( y2 > maxY )
            {
                y2 = maxY;
                return false;
            }
        }
        else if( y1 == y2 )    /* Horizontal line, clip X. */
        {
            if( x2 < minX )
            {
                x2 = minX;
                return false;
            }

            if( x2 > maxX )
            {
                x2 = maxX;
                return false;
            }
        }

        if( TestForIntersectionOfStraightLineSegments( x1, y1, x2, y2, minX, minY, minX, maxY,
                                                       &clippedX, &clippedY )       /* Left */
           || TestForIntersectionOfStraightLineSegments( x1, y1, x2, y2, minX, minY, maxX, minY,
                                                         &clippedX, &clippedY )     /* Top */
           || TestForIntersectionOfStraightLineSegments( x1, y1, x2, y2, maxX, minY, maxX, maxY,
                                                         &clippedX, &clippedY )     /* Right */
           || TestForIntersectionOfStraightLineSegments( x1, y1, x2, y2, minX, maxY, maxX, maxY,
                                                         &clippedX, &clippedY ) )   /* Bottom */
        {
            if( x1 != clippedX )
                x1 = clippedX;
            if( y1 != clippedY )
                y1 = clippedY;
            return false;
        }

        /* If we're here, something has gone terribly wrong. */
#if DEBUG_DUMP_CLIP_ERROR_COORDS
        wxLogDebug( wxT( "Line (%d,%d):(%d,%d) in rectangle (%d,%d,%d,%d) clipped to (%d,%d,%d,%d)" ),
                    tmpX1, tmpY1, tmpX2, tmpY2, minX, minY, maxX, maxY, x1, y1, x2, y2 );
#endif
        return false;
    }
    else
    {
        int* intersectX;
        int* intersectY;
        int  intersectX1, intersectY1, intersectX2, intersectY2;
        bool haveFirstPoint = false;

        intersectX = &intersectX1;
        intersectY = &intersectY1;

        /* Left clip rectangle line. */
        if( TestForIntersectionOfStraightLineSegments( x1, y1, x2, y2, minX, minY, minX, maxY,
                                                       intersectX, intersectY ) )
        {
            intersectX     = &intersectX2;
            intersectY     = &intersectY2;
            haveFirstPoint = true;
        }

        /* Top clip rectangle line. */
        if( TestForIntersectionOfStraightLineSegments( x1, y1, x2, y2, minX, minY, maxX, minY,
                                                       intersectX, intersectY ) )
        {
            intersectX = &intersectX2;
            intersectY = &intersectY2;
            if( haveFirstPoint )
            {
                x1 = intersectX1;
                y1 = intersectY1;
                x2 = intersectX2;
                y2 = intersectY2;
                return false;
            }
            haveFirstPoint = true;
        }

        /* Right clip rectangle line. */
        if( TestForIntersectionOfStraightLineSegments( x1, y1, x2, y2, maxX, minY, maxX, maxY,
                                                       intersectX, intersectY ) )
        {
            intersectX = &intersectX2;
            intersectY = &intersectY2;
            if( haveFirstPoint )
            {
                x1 = intersectX1;
                y1 = intersectY1;
                x2 = intersectX2;
                y2 = intersectY2;
                return false;
            }
            haveFirstPoint = true;
        }

        /* Bottom clip rectangle line. */
        if( TestForIntersectionOfStraightLineSegments( x1, y1, x2, y2, minX, maxY, maxX, maxY,
                                                       intersectX, intersectY ) )
        {
            intersectX = &intersectX2;
            intersectY = &intersectY2;
            if( haveFirstPoint )
            {
                x1 = intersectX1;
                y1 = intersectY1;
                x2 = intersectX2;
                y2 = intersectY2;
                return false;
            }
        }

        /* If we're here and only one line of the clip box has been intersected,
         * something has gone terribly wrong. */
#if DEBUG_DUMP_CLIP_ERROR_COORDS
        if( haveFirstPoint )
            wxLogDebug( wxT( "Line (%d,%d):(%d,%d) in rectangle (%d,%d,%d,%d) clipped to (%d,%d,%d,%d)" ),
                        tmpX1, tmpY1, tmpX2, tmpY2, minX, minY, maxX, maxY, x1, y1, x2, y2 );
#endif
    }

    /* Set this to one to verify that diagonal lines get clipped properly. */
#if DEBUG_DUMP_CLIP_COORDS
    if( !( x1 == x2 || y1 == y2 ) )
        wxLogDebug( wxT( "Clipped line (%d,%d):(%d,%d) from rectangle (%d,%d,%d,%d)" ),
                    tmpX1, tmpY1, tmpX2, tmpY2, minX, minY, maxX, maxY );
#endif

    return true;
}


static void WinClipAndDrawLine( EDA_RECT* ClipBox, wxDC* DC, int x1, int y1, int x2, int y2,
                                int Color, int width = 1 )
{
    GRLastMoveToX = x2;
    GRLastMoveToY = y2;

    if( ClipBox )
    {
        EDA_RECT clipbox(*ClipBox);
        clipbox.Inflate(width/2);
        if( clipLine( &clipbox, x1, y1, x2, y2 ) )
            return;
    }

    GRSetColorPen( DC, Color, width );
    DC->DrawLine( x1, y1, x2, y2 );
}


/* Forcing a reset of the current pen.
 * Must be called after changing the graphical device before any trace.
 */
void GRResetPenAndBrush( wxDC* DC )
{
    GRSetBrush( DC, BLACK );  // Force no fill
    s_DC_lastbrushcolor = -1;
    s_DC_lastcolor = -1;
    s_DC_lastDC    = NULL;
}


/**
 * Function GRSetColorPen
 * sets a pen style, width, color, and alpha into the given device context.
 */
void GRSetColorPen( wxDC* DC, int Color, int width, wxPenStyle style )
{
    if( width < 0 )
        width = 0;

#ifdef __WXMAC__
    // Under OSX when wxPen is set to 0, cocoa follows the request drawing nothing
    if( width == 0)
        width = DC->DeviceToLogicalXRel( 1 );
#endif

    if( s_ForceBlackPen )
    {
        Color = BLACK;
    }

    if(   s_DC_lastcolor != Color
       || s_DC_lastwidth != width
       || s_DC_lastpenstyle != style
       || s_DC_lastDC != DC  )
    {
        wxPen    pen;

        wxColour wx_color = MakeColour( Color );

        pen.SetColour( wx_color );
        pen.SetWidth( width );
        pen.SetStyle( style );

        DC->SetPen( pen );

        s_DC_lastcolor    = Color;
        s_DC_lastwidth    = width;
        s_DC_lastpenstyle = style;
        s_DC_lastDC = DC;
    }
}


void GRSetBrush( wxDC* DC, int Color, int fill )
{
    if( s_ForceBlackPen )
        Color = BLACK;

    if(   s_DC_lastbrushcolor != Color
       || s_DC_lastbrushfill  != fill
       || s_DC_lastDC != DC  )
    {
        wxBrush DrawBrush;
        DrawBrush.SetColour( MakeColour( Color ) );

        if( fill )
            DrawBrush.SetStyle( wxSOLID );
        else
            DrawBrush.SetStyle( wxTRANSPARENT );

        DC->SetBrush( DrawBrush );

        s_DC_lastbrushcolor = Color;
        s_DC_lastbrushfill  = fill;
        s_DC_lastDC = DC;
    }
}


/**
 * Function GRForceBlackPen
 * @param flagforce True to force a black pen whenever the asked color
 */
void GRForceBlackPen( bool flagforce )
{
    s_ForceBlackPen = flagforce;
}


/**
 * Function GetGRForceBlackPenState
 * @return s_ForceBlackPen (True if  a black pen was forced)
 */
bool GetGRForceBlackPenState( void )
{
    return s_ForceBlackPen;
}


/*************************************/
/* Set the device context draw mode. */
/*************************************/
void GRSetDrawMode( wxDC* DC, int draw_mode )
{
    if( draw_mode & GR_OR )
#if defined(__WXMAC__) && (wxMAC_USE_CORE_GRAPHICS || wxCHECK_VERSION( 2, 9, 0 ) )

        DC->SetLogicalFunction( wxCOPY );
#elif defined( USE_WX_GRAPHICS_CONTEXT )

        DC->SetLogicalFunction( wxCOPY );
#else

        DC->SetLogicalFunction( wxOR );
#endif
    else if( draw_mode & GR_XOR )
#if defined( USE_WX_GRAPHICS_CONTEXT )

        DC->SetLogicalFunction( wxCOPY );
#else

        DC->SetLogicalFunction( wxXOR );
#endif
    else if( draw_mode & GR_NXOR )
#if defined(__WXMAC__) && (wxMAC_USE_CORE_GRAPHICS || wxCHECK_VERSION( 2, 9, 0 ) )

        DC->SetLogicalFunction( wxXOR );
#elif defined( USE_WX_GRAPHICS_CONTEXT )

        DC->SetLogicalFunction( wxCOPY );
#else

        DC->SetLogicalFunction( wxEQUIV );
#endif
    else if( draw_mode & GR_INVERT )
#if defined( USE_WX_GRAPHICS_CONTEXT )

        DC->SetLogicalFunction( wxCOPY );
#else

        DC->SetLogicalFunction( wxINVERT );
#endif
    else
        DC->SetLogicalFunction( wxCOPY );

#ifdef USE_WX_OVERLAY
    DC->SetLogicalFunction( wxCOPY );
#endif
}


void GRPutPixel( EDA_RECT* ClipBox, wxDC* DC, int x, int y, int Color )
{
    if( ClipBox && !ClipBox->Contains( x, y ) )
        return;

    GRSetColorPen( DC, Color );
    DC->DrawPoint( x, y );
}


/*
 * Draw a line, in object space.
 */
void GRLine( EDA_RECT* ClipBox,
             wxDC*     DC,
             int       x1,
             int       y1,
             int       x2,
             int       y2,
             int       width,
             int       Color )
{
    WinClipAndDrawLine( ClipBox, DC, x1, y1, x2, y2, Color, width );
    GRLastMoveToX = x2;
    GRLastMoveToY = y2;
}


void GRLine( EDA_RECT* aClipBox, wxDC* aDC, wxPoint aStart, wxPoint aEnd, int aWidth, int aColor )
{
    GRLine( aClipBox, aDC, aStart.x, aStart.y, aEnd.x, aEnd.y, aWidth, aColor );
}


void GRDashedLineTo( EDA_RECT* ClipBox, wxDC* DC, int x2, int y2, int width, int Color )
{
    s_DC_lastcolor = -1;
    GRSetColorPen( DC, Color, width, wxPENSTYLE_SHORT_DASH );
    GRLine( ClipBox, DC, GRLastMoveToX, GRLastMoveToY, x2, y2, width, Color );
    s_DC_lastcolor = -1;
    GRSetColorPen( DC, Color, width );
    GRLastMoveToX = x2;
    GRLastMoveToY = y2;
}


void GRDashedLine( EDA_RECT* ClipBox,
                   wxDC*     DC,
                   int       x1,
                   int       y1,
                   int       x2,
                   int       y2,
                   int       width,
                   int       Color )
{
    GRLastMoveToX  = x2;
    GRLastMoveToY  = y2;
    s_DC_lastcolor = -1;
    GRSetColorPen( DC, Color, width, wxPENSTYLE_SHORT_DASH );
    GRLine( ClipBox, DC, x1, y1, x2, y2, width, Color );
    s_DC_lastcolor = -1;
    GRSetColorPen( DC, Color, width );
}


/*
 * Move to a new position, in object space.
 */
void GRMoveTo( int x, int y )
{
    GRLastMoveToX = x;
    GRLastMoveToY = y;
}


/*
 * Draw line to a new position, in object space.
 */
void GRLineTo( EDA_RECT* ClipBox, wxDC* DC, int x, int y, int width, int Color )
{
    int GRLineToX, GRLineToY;

    GRLineToX = x;
    GRLineToY = y;

    GRLine( ClipBox, DC, GRLastMoveToX, GRLastMoveToY, GRLineToX, GRLineToY, width, Color );
}


void GRMixedLine( EDA_RECT* ClipBox, wxDC* DC, int x1, int y1, int x2, int y2,
                  int width, int Color )
{
    GRSetColorPen( DC, Color, width, wxPENSTYLE_DOT_DASH );
    GRLine( ClipBox, DC, x1, y1, x2, y2, width, Color );
    GRSetColorPen( DC, Color, width );
}



/**
 * Function GRLineArray
 * draws an array of lines (not a polygon).
 * @param aClipBox = the clip box
 * @param aDC = the device context into which drawing should occur.
 * @param aLines = a list of pair of coordinate in user space: a pair for each line.
 * @param aWidth = the width of each line.
 * @param aColor = an index into our color table of RGB colors.
 * @see EDA_COLOR_T and colors.h
 */
void GRLineArray( EDA_RECT* aClipBox, wxDC* aDC, std::vector<wxPoint>& aLines,
                  int aWidth, int aColor )
{
    GRSetColorPen( aDC, aColor, aWidth );

#if defined( USE_WX_GRAPHICS_CONTEXT ) || defined(__WXMAC__)
    wxGraphicsContext* gc = wxGraphicsContext::Create( aDC );
    wxASSERT( gc );
    gc->Clip( aClipBox->GetX(), aClipBox->GetY(), aClipBox->GetRight(), aClipBox->GetHeight() );
    wxGraphicsPath     path = gc->CreatePath();

    for( unsigned i = 0; i < aLines.size(); )
    {
        path.MoveToPoint( aLines[i].x, aLines[i].y );
        i++;
        path.AddLineToPoint( aLines[i].x, aLines[i].y );
        i++;
    }

    gc->StrokePath( path );
    gc->ResetClip();
    delete gc;
#else

    if( aClipBox )
        aClipBox->Inflate(aWidth/2);
    for( unsigned i = 0; i < aLines.size(); i += 2)
    {
        int x1 = aLines[i].x;
        int y1 = aLines[i].y;
        int x2 = aLines[i+1].x;
        int y2 = aLines[i+1].y;
        GRLastMoveToX = x2;
        GRLastMoveToY = y2;
        if( ( aClipBox == NULL ) || !clipLine( aClipBox, x1, y1, x2, y2 ) )
            aDC->DrawLine( x1, y1, x2, y2 );
    }
    if( aClipBox )
        aClipBox->Inflate(-aWidth/2);
#endif
}


void GRCSegm( EDA_RECT* ClipBox, wxDC* DC, int x1, int y1, int x2, int y2,
              int width, int aPenSize, int Color )
{
    long radius;
    int  dwx, dwy;
    long dx, dy, dwx2, dwy2;
    long sx1, sy1, ex1, ey1;
    long sx2, sy2, ex2, ey2;
    bool swap_ends = false;


    GRLastMoveToX = x2;
    GRLastMoveToY = y2;

    if( ClipBox )
    {
        EDA_RECT clipbox(*ClipBox);
        clipbox.Inflate(width/2);

        if( clipLine( &clipbox, x1, y1, x2, y2 ) )
            return;
    }


    if( width <= 2 )   /*  single line or 2 pixels */
    {
        GRSetColorPen( DC, Color, width );
        DC->DrawLine( x1, y1, x2, y2 );
        return;
    }

    GRSetColorPen( DC, Color, aPenSize );
    GRSetBrush( DC, Color, false );

    radius = (width + 1) >> 1;

    dx = x2 - x1;
    dy = y2 - y1;

    if( dx == 0 )  /* segment vertical */
    {
        dwx = radius;
        if( dy >= 0 )
            dwx = -dwx;

        sx1 = x1 - dwx;
        sy1 = y1;

        ex1 = x2 - dwx;
        ey1 = y2;

        DC->DrawLine( sx1, sy1, ex1, ey1 );

        sx2 = x1 + dwx;
        sy2 = y1;

        ex2 = x2 + dwx;
        ey2 = y2;

        DC->DrawLine( sx2, sy2, ex2, ey2 );
    }
    else if( dy == 0 ) /* segment horizontal */
    {
        dwy = radius;
        if( dx < 0 )
            dwy = -dwy;

        sx1 = x1;
        sy1 = y1 - dwy;

        ex1 = x2;
        ey1 = y2 - dwy;

        DC->DrawLine( sx1, sy1, ex1, ey1 );

        sx2 = x1;
        sy2 = y1 + dwy;

        ex2 = x2;
        ey2 = y2 + dwy;

        DC->DrawLine( sx2, sy2, ex2, ey2 );
    }
    else
    {
        if( ABS( dx ) == ABS( dy ) )                /* segment 45 degrees */
        {
            dwx = dwy = ( (width * 5) + 4 ) / 7;    // = width / 2 * 0.707
            if( dy < 0 )
            {
                if( dx <= 0 )
                {
                    dwx = -dwx; swap_ends = true;
                }
            }
            else    // dy >= 0
            {
                if( dx > 0 )
                {
                    dwy = -dwy; swap_ends = true;
                }
                else
                    swap_ends = true;
            }
        }
        else
        {
            int delta_angle = ArcTangente( dy, dx );
            dwx = 0;
            dwy = width;
            RotatePoint( &dwx, &dwy, -delta_angle );
        }
        dwx2 = dwx >> 1;
        dwy2 = dwy >> 1;

        sx1 = x1 - dwx2;
        sy1 = y1 - dwy2;

        ex1 = x2 - dwx2;
        ey1 = y2 - dwy2;

        DC->DrawLine( sx1, sy1, ex1, ey1 );

        sx2 = x1 + dwx2;
        sy2 = y1 + dwy2;

        ex2 = x2 + dwx2;
        ey2 = y2 + dwy2;

        DC->DrawLine( sx2, sy2, ex2, ey2 );
    }

    if( swap_ends )
    {
        DC->DrawArc( sx2, sy2, sx1, sy1, x1, y1 );
        DC->DrawArc( ex1, ey1, ex2, ey2, x2, y2 );
    }
    else
    {
        DC->DrawArc( sx1, sy1, sx2, sy2, x1, y1 );
        DC->DrawArc( ex2, ey2, ex1, ey1, x2, y2 );
    }
}


void GRCSegm( EDA_RECT* ClipBox, wxDC* DC, int x1, int y1, int x2, int y2,
              int width, int Color )
{
    GRCSegm( ClipBox, DC, x1, y1, x2, y2, width, 0, Color );
}


void GRCSegm( EDA_RECT* aClipBox, wxDC* aDC, wxPoint aStart, wxPoint aEnd,
              int aWidth, int aColor )
{
    GRCSegm( aClipBox, aDC, aStart.x, aStart.y, aEnd.x, aEnd.y, aWidth, 0, aColor );
}


/*
 * Draw segment (full) with rounded ends in object space (real coords.).
 */
void GRFillCSegm( EDA_RECT* ClipBox, wxDC* DC, int x1, int y1, int x2, int y2,
                  int width, int Color )
{
    WinClipAndDrawLine( ClipBox, DC, x1, y1, x2, y2, Color, width );
}


void GRFilledSegment( EDA_RECT* aClipBox, wxDC* aDC, wxPoint aStart, wxPoint aEnd,
                      int aWidth, int aColor )
{
    WinClipAndDrawLine( aClipBox, aDC, aStart.x, aStart.y, aEnd.x, aEnd.y, aColor, aWidth );
}


static bool IsGRSPolyDrawable( EDA_RECT* ClipBox, int n, wxPoint Points[] )
{
    if( !ClipBox )
        return true;

    if( n <= 0 )
        return false;

    int Xmin, Xmax, Ymin, Ymax;

    Xmin = Xmax = Points[0].x;
    Ymin = Ymax = Points[0].y;

    for( int ii = 1; ii < n; ii++ )     // calculate rectangle
    {
        Xmin = MIN( Xmin, Points[ii].x );
        Xmax = MAX( Xmax, Points[ii].x );
        Ymin = MIN( Ymin, Points[ii].y );
        Ymax = MAX( Ymax, Points[ii].y );
    }

    xcliplo = ClipBox->GetX();
    ycliplo = ClipBox->GetY();
    xcliphi = ClipBox->GetRight();
    ycliphi = ClipBox->GetBottom();

    if( Xmax < xcliplo )
        return false;
    if( Xmin > xcliphi )
        return false;
    if( Ymax < ycliplo )
        return false;
    if( Ymin > ycliphi )
        return false;

    return true;
}


/*
 * Draw a new polyline and fill it if Fill, in screen space.
 */
static void GRSPoly( EDA_RECT* ClipBox,
                     wxDC*     DC,
                     int       n,
                     wxPoint   Points[],
                     bool      Fill,
                     int       width,
                     int       Color,
                     int       BgColor )
{
    if( !IsGRSPolyDrawable( ClipBox, n, Points ) )
        return;

    GRSetColorPen( DC, Color, width );

    if( Fill && ( n > 2 ) )
    {
        GRSetBrush( DC, BgColor, FILLED );


        /* clip before send the filled polygon to wxDC, because under linux
         * (GTK?) polygons having large coordinates are incorrectly drawn
         */
#ifdef USE_CLIP_FILLED_POLYGONS
        ClipAndDrawFilledPoly( ClipBox, DC, Points, n );
#else
        DC->DrawPolygon( n, Points );  // does not work very well under linux
#endif
    }
    else
    {
        wxPoint endPt = Points[n - 1];

        GRSetBrush( DC, Color );
        DC->DrawLines( n, Points );

        // The last point is not drawn by DrawLine and DrawLines
        // Add it if the polygon is not closed
        if( endPt != Points[0] )
            DC->DrawPoint( endPt.x, endPt.y );
    }
}


/*
 * Draw a new closed polyline and fill it if Fill, in screen space.
 */
static void GRSClosedPoly( EDA_RECT* ClipBox,
                           wxDC*     DC,
                           int       aPointCount,
                           wxPoint   aPoints[],
                           bool      Fill,
                           int       width,
                           int       Color,
                           int       BgColor )
{
    if( !IsGRSPolyDrawable( ClipBox, aPointCount, aPoints ) )
        return;

    GRSetColorPen( DC, Color, width );

    if( Fill && ( aPointCount > 2 ) )
    {
        GRLastMoveToX = aPoints[aPointCount - 1].x;
        GRLastMoveToY = aPoints[aPointCount - 1].y;
        GRSetBrush( DC, BgColor, FILLED );
#ifdef USE_CLIP_FILLED_POLYGONS
        ClipAndDrawFilledPoly( ClipBox, DC, aPoints, aPointCount );
#else
        DC->DrawPolygon( aPointCount, aPoints );  // does not work very well under linux
#endif
    }
    else
    {
        GRSetBrush( DC, BgColor );
        DC->DrawLines( aPointCount, aPoints );

        /* Close the polygon. */
        if( aPoints[aPointCount - 1] != aPoints[0] )
        {
            GRLine( ClipBox,
                    DC,
                    aPoints[0].x,
                    aPoints[0].y,
                    aPoints[aPointCount - 1].x,
                    aPoints[aPointCount - 1].y,
                    width,
                    Color );
        }
    }
}


/*
 * Draw a new polyline and fill it if Fill, in drawing space.
 */
void GRPoly( EDA_RECT* ClipBox, wxDC* DC, int n, wxPoint Points[],
             bool Fill, int width, int Color, int BgColor )
{
    GRSPoly( ClipBox, DC, n, Points, Fill, width, Color, BgColor );
}


/*
 * Draw a closed polyline and fill it if Fill, in object space.
 */
void GRClosedPoly( EDA_RECT* ClipBox, wxDC* DC, int n, wxPoint Points[],
                   bool Fill, int Color, int BgColor )
{
    GRClosedPoly( ClipBox, DC, n, Points, Fill, 0, Color, BgColor );
}


void GRClosedPoly( EDA_RECT* ClipBox, wxDC* DC, int n, wxPoint Points[],
                   bool Fill, int width, int Color, int BgColor )
{
    GRSClosedPoly( ClipBox, DC, n, Points, Fill, width, Color, BgColor );
}


void GRCircle( EDA_RECT* ClipBox, wxDC* DC, int xc, int yc, int r, int width, int Color )
{
    /* Clip circles off screen. */
    if( ClipBox )
    {
        int x0, y0, xm, ym;
        x0 = ClipBox->GetX();
        y0 = ClipBox->GetY();
        xm = ClipBox->GetRight();
        ym = ClipBox->GetBottom();

        if( xc < ( x0 - r - width ) )
            return;

        if( yc < ( y0 - r - width ) )
            return;

        if( xc > ( r + xm + width ) )
            return;

        if( yc > ( r + ym + width ) )
            return;
    }

    GRSetColorPen( DC, Color, width );
    GRSetBrush( DC, Color, false );
    DC->DrawEllipse( xc - r, yc - r, r + r, r + r );
}


void GRCircle( EDA_RECT* ClipBox, wxDC* DC, int x, int y, int r, int Color )
{
    GRCircle( ClipBox, DC, x, y, r, 0, Color );
}


void GRCircle( EDA_RECT* aClipBox, wxDC* aDC, wxPoint aPos, int aRadius, int aWidth, int aColor )
{
    GRCircle( aClipBox, aDC, aPos.x, aPos.y, aRadius, aWidth, aColor );
}


void GRFilledCircle( EDA_RECT* ClipBox, wxDC* DC, int x, int y, int r,
                     int width, int Color, int BgColor )
{
    /* Clip circles off screen. */
    if( ClipBox )
    {
        int x0, y0, xm, ym;
        x0 = ClipBox->GetX();
        y0 = ClipBox->GetY();
        xm = ClipBox->GetRight();
        ym = ClipBox->GetBottom();
        if( x < (x0 - r) )
            return;
        if( y < (y0 - r) )
            return;
        if( x > (r + xm) )
            return;
        if( y > (r + ym) )
            return;
    }

    GRSetColorPen( DC, Color, width );
    GRSetBrush( DC, BgColor, FILLED );
    DC->DrawEllipse( x - r, y - r, r + r, r + r );
}


void GRFilledCircle( EDA_RECT* aClipBox, wxDC* aDC, wxPoint aPos, int aRadius, int aColor )
{
    GRFilledCircle( aClipBox, aDC, aPos.x, aPos.y, aRadius, 0, aColor, aColor );
}


/*
 * Draw an arc in user space.
 */
void GRArc1( EDA_RECT* ClipBox, wxDC* DC, int x1, int y1, int x2, int y2,
             int xc, int yc, int Color )
{
    GRArc1( ClipBox, DC, x1, y1, x2, y2, xc, yc, 0, Color );
}


/*
 * Draw an arc, width = width in user space.
 */
void GRArc1( EDA_RECT* ClipBox, wxDC* DC, int x1, int y1, int x2, int y2,
             int xc, int yc, int width, int Color )
{
    /* Clip arcs off screen. */
    if( ClipBox )
    {
        int x0, y0, xm, ym, r;
        x0 = ClipBox->GetX();
        y0 = ClipBox->GetY();
        xm = ClipBox->GetRight();
        ym = ClipBox->GetBottom();
        r  = (int) hypot( x1 - xc, y1 - yc );
        if( xc < ( x0 - r ) )
            return;
        if( yc < ( y0 - r ) )
            return;
        if( xc > ( r + xm ) )
            return;
        if( yc > ( r + ym ) )
            return;
    }

    GRSetColorPen( DC, Color, width );
    GRSetBrush( DC, Color );
    DC->DrawArc( x1, y1, x2, y2, xc, yc );
}


void GRArc1( EDA_RECT* aClipBox, wxDC* aDC, wxPoint aStart, wxPoint aEnd,
             wxPoint aCenter, int aWidth, int aColor )
{
    GRArc1( aClipBox, aDC, aStart.x, aStart.y, aEnd.x, aEnd.y, aCenter.x, aCenter.y,
            aWidth, aColor );
}


/*
 * Draw a filled arc in drawing space.
 */
void GRFilledArc( EDA_RECT* ClipBox,
                  wxDC*     DC,
                  int       x,
                  int       y,
                  int       StAngle,
                  int       EndAngle,
                  int       r,
                  int       width,
                  int       Color,
                  int       BgColor )
{
    int x1, y1, x2, y2;

    /* Clip arcs off screen */
    if( ClipBox )
    {
        int x0, y0, xm, ym;
        x0 = ClipBox->GetX();
        y0 = ClipBox->GetY();
        xm = ClipBox->GetRight();
        ym = ClipBox->GetBottom();

        if( x < ( x0 - r - 1 ) )
            return;

        if( y < ( y0 - r - 1 ) )
            return;

        if( x > ( r + xm + 1 ) )
            return;

        if( y > ( r + ym + 1 ) )
            return;
    }

    x1 = r;
    y1 = 0;
    RotatePoint( &x1, &y1, EndAngle );

    x2 = r;
    y2 = 0;
    RotatePoint( &x2, &y2, StAngle );

    GRSetBrush( DC, BgColor, FILLED );
    GRSetColorPen( DC, Color, width );
    DC->DrawArc( x + x1, y - y1, x + x2, y - y2, x, y );
}


void GRFilledArc( EDA_RECT* ClipBox, wxDC* DC, int x, int y,
                  int StAngle, int EndAngle, int r, int Color, int BgColor )
{
    GRFilledArc( ClipBox, DC, x, y, StAngle, EndAngle, r, 0, Color, BgColor );
}


/*
 * Draw an arc in drawing space.
 */
void GRArc( EDA_RECT* ClipBox, wxDC* DC, int xc, int yc, int StAngle,
            int EndAngle, int r, int Color )
{
    int x1, y1, x2, y2;

    /* Clip arcs off screen */
    if( ClipBox )
    {
        int radius = r + 1;
        int x0, y0, xm, ym, x, y;
        x0 = ClipBox->GetX();
        y0 = ClipBox->GetY();
        xm = ClipBox->GetRight();
        ym = ClipBox->GetBottom();
        x = xc;
        y = yc;

        if( x < ( x0 - radius ) )
            return;
        if( y < ( y0 - radius ) )
            return;
        if( x > ( xm + radius ) )
            return;
        if( y > ( ym + radius ) )
            return;
    }

    x1 = r;
    y1 = 0;
    RotatePoint( &x1, &y1, EndAngle );

    x2 = r;
    y2 = 0;
    RotatePoint( &x2, &y2, StAngle );

    GRSetColorPen( DC, Color );
    GRSetBrush( DC, Color, false );
    DC->DrawArc( xc + x1, yc - y1, xc + x2, yc - y2, xc, yc );
}


/*
 * Draw an arc with width = width in drawing space.
 */
void GRArc( EDA_RECT* ClipBox,
            wxDC*     DC,
            int       x,
            int       y,
            int       StAngle,
            int       EndAngle,
            int       r,
            int       width,
            int       Color )
{
    int x1, y1, x2, y2;

    /* Clip arcs off screen. */
    if( ClipBox )
    {
        int x0, y0, xm, ym;
        x0 = ClipBox->GetX();
        y0 = ClipBox->GetY();
        xm = ClipBox->GetRight();
        ym = ClipBox->GetBottom();

        if( x < ( x0 - r - width ) )
            return;

        if( y < ( y0 - r - width ) )
            return;

        if( x > ( r + xm + width ) )
            return;

        if( y > ( r + ym + width ) )
            return;
    }

    x1 = r;
    y1 = 0;
    RotatePoint( &x1, &y1, EndAngle );

    x2 = r;
    y2 = 0;
    RotatePoint( &x2, &y2, StAngle );

    GRSetColorPen( DC, Color, width );
    GRSetBrush( DC, Color );
    DC->DrawArc( x + x1, y - y1, x + x2, y - y2, x, y );
}


/*
 * Draw a rectangle in drawing space.
 */
void GRRect( EDA_RECT* aClipBox, wxDC* aDC, int x1, int y1, int x2, int y2, int aColor )
{
    GRSRect( aClipBox, aDC, x1, y1, x2, y2, 0, aColor );
}


void GRRectPs( EDA_RECT* aClipBox, wxDC* aDC, const EDA_RECT& aRect, int aColor, wxPenStyle aStyle )
{
    int x1 = aRect.GetX();
    int y1 = aRect.GetY();
    int x2 = aRect.GetRight();
    int y2 = aRect.GetBottom();

    GRSRect( aClipBox, aDC, x1, y1, x2, y2, 0, aColor, aStyle );
}


/*
 * Draw a rectangle (thick lines) in drawing space.
 */
void GRRect( EDA_RECT* ClipBox, wxDC* DC, int x1, int y1, int x2, int y2, int width, int Color )
{
    GRSRect( ClipBox, DC, x1, y1, x2, y2, width, Color );
}


void GRRect( EDA_RECT* aClipBox, wxDC* aDC, const EDA_RECT& aRect, int aWidth, int aColor )
{
    int x1 = aRect.GetX();
    int y1 = aRect.GetY();
    int x2 = aRect.GetRight();
    int y2 = aRect.GetBottom();

    GRSRect( aClipBox, aDC, x1, y1, x2, y2, aWidth, aColor );
}


/*
 * Draw a rectangle (filled with AreaColor) in drawing space.
 */
void GRFilledRect( EDA_RECT* ClipBox, wxDC* DC, int x1, int y1, int x2, int y2,
                   int Color, int BgColor )
{
    GRSFilledRect( ClipBox, DC, x1, y1, x2, y2, 0, Color, BgColor );
}


/*
 * Draw a rectangle (filled with AreaColor) in drawing space.
 */
void GRFilledRect( EDA_RECT* ClipBox, wxDC* DC, int x1, int y1, int x2, int y2,
                   int width, int Color, int BgColor )
{
    GRSFilledRect( ClipBox, DC, x1, y1, x2, y2, width, Color, BgColor );
}


/*
 * Draw a rectangle in screen space.
 */

void GRSRect( EDA_RECT* aClipBox, wxDC* aDC, int x1, int y1, int x2, int y2,
              int aWidth, int aColor, wxPenStyle aStyle )
{

    wxPoint points[5];
    points[0] = wxPoint(x1, y1);
    points[1] = wxPoint(x1, y2);
    points[2] = wxPoint(x2, y2);
    points[3] = wxPoint(x2, y1);
    points[4] = points[0];
    GRSetColorPen( aDC, aColor, aWidth, aStyle );
    GRSetBrush( aDC, BLACK );
    if( aClipBox )
    {
        EDA_RECT clipbox(*aClipBox);
        clipbox.Inflate(aWidth);
        ClipAndDrawFilledPoly(&clipbox, aDC, points, 5); // polygon approach is more accurate
    }
    else
        ClipAndDrawFilledPoly(aClipBox, aDC, points, 5);
}


void GRSFilledRect( EDA_RECT* aClipBox, wxDC* aDC, int x1, int y1, int x2, int y2,
                    int aWidth, int aColor, int aBgColor )
{

    wxPoint points[5];
    points[0] = wxPoint(x1, y1);
    points[1] = wxPoint(x1, y2);
    points[2] = wxPoint(x2, y2);
    points[3] = wxPoint(x2, y1);
    points[4] = points[0];
    GRSetBrush( aDC, aBgColor, FILLED );
    GRSetColorPen( aDC, aBgColor, aWidth );
    if( aClipBox && (aWidth > 0) )
    {
        EDA_RECT clipbox(*aClipBox);
        clipbox.Inflate(aWidth);
        ClipAndDrawFilledPoly(&clipbox, aDC, points, 5); // polygon approach is more accurate
    }
    else
        ClipAndDrawFilledPoly(aClipBox, aDC, points, 5);
}


#ifdef USE_CLIP_FILLED_POLYGONS

/**
 * Function ClipAndDrawFilledPoly
 *  Used to clip a polygon and draw it as Filled Polygon
 *  uses the Sutherland and Hodgman algo to clip the given poly against a
 *  rectangle.  This rectangle is the drawing area this is useful under
 *  Linux (2009) because filled polygons are incorrectly drawn if they have
 *  too large coordinates (seems due to integer overflows in calculations)
 *  Could be removed in some years, if become unnecessary.
 */

/* Note: aClipBox == NULL is legal, so if aClipBox == NULL,
 * the polygon is drawn, but not clipped
 */
#include <SutherlandHodgmanClipPoly.h>

void ClipAndDrawFilledPoly( EDA_RECT* aClipBox, wxDC* aDC, wxPoint aPoints[], int n )
{
    if( aClipBox == NULL )
    {
        aDC->DrawPolygon( n, aPoints );
        return;
    }

    // A clip box exists: clip and draw the polygon.
    static vector<wxPoint> clippedPolygon;
    static pointVector     inputPolygon, outputPolygon;

    inputPolygon.clear();
    outputPolygon.clear();
    clippedPolygon.clear();

    for( int ii = 0; ii < n; ii++ )
        inputPolygon.push_back( PointF( (REAL) aPoints[ii].x, (REAL) aPoints[ii].y ) );

    RectF window( (REAL) aClipBox->GetX(), (REAL) aClipBox->GetY(),
                  (REAL) aClipBox->GetWidth(), (REAL) aClipBox->GetHeight() );

    SutherlandHodgman sh( window );
    sh.Clip( inputPolygon, outputPolygon );

    for( cpointIterator cit = outputPolygon.begin(); cit != outputPolygon.end(); ++cit )
    {
        clippedPolygon.push_back( wxPoint( wxRound( cit->X ), wxRound( cit->Y ) ) );
    }

    if( clippedPolygon.size() )
        aDC->DrawPolygon( clippedPolygon.size(), &clippedPolygon[0] );
}


#endif


void GRBezier( EDA_RECT* ClipBox,
               wxDC*     DC,
               int       x1,
               int       y1,
               int       x2,
               int       y2,
               int       x3,
               int       y3,
               int       width,
               int       Color )
{
    std::vector<wxPoint> Points = Bezier2Poly( x1, y1, x2, y2, x3, y3 );
    GRPoly( ClipBox, DC, Points.size(), &Points[0], false, width, Color, 0 );
}


void GRBezier( EDA_RECT* ClipBox,
               wxDC*     DC,
               int       x1,
               int       y1,
               int       x2,
               int       y2,
               int       x3,
               int       y3,
               int       x4,
               int       y4,
               int       width,
               int       Color )
{
    std::vector<wxPoint> Points = Bezier2Poly( x1, y1, x2, y2, x3, y3, x4, y4 );
    GRPoly( ClipBox, DC, Points.size(), &Points[0], false, width, Color, 0 );
}
