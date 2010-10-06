/********************************/
/* Low level graphics routines  */
/********************************/


#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "trigo.h"
#include "macros.h"
#include "base_struct.h"
#include "class_base_screen.h"
#include "bezier_curves.h"
#include "math_for_graphics.h"


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
 * (mainly at low zoom values (2, 1 or 0.5), in pcbnew)
 * some of these problems could be now fixed in recent distributions.
 *
 * Currently (feb 2009) there are overflow problems when drawing solid (filled)
 * polygons under linux without clipping
 *
 * So before removing clipping functions, be aware these bug (they are not in
 * kicad or wxWidgets) are fixed by testing how are drawn complex lines arcs
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
void ClipAndDrawFilledPoly( EDA_Rect* ClipBox, wxDC * DC, wxPoint Points[], int n );
#endif

/* These functions are used by corresponding functions
 * ( GRSCircle is called by GRCircle for instance) after mapping coordinates
 * from user units to screen units(pixels coordinates)
 */
static void GRSCircle( EDA_Rect* ClipBox, wxDC* DC, int x, int y, int r, int width, int Color );
static void GRSFilledCircle( EDA_Rect* ClipBox, wxDC* DC, int x, int y, int r,
                             int width, int Color, int BgColor );
static void GRSMixedLine( EDA_Rect* ClipBox, wxDC* DC, int x1, int y1, int x2, int y2,
                          int width, int Color );
static void GRSDashedLineTo( EDA_Rect* ClipBox, wxDC* DC, int x2, int y2, int width, int Color );
static void GRSDashedLine( EDA_Rect* ClipBox, wxDC* DC, int x1, int y1, int x2,
                           int y2, int width, int Color );
static void GRSLine( EDA_Rect* ClipBox, wxDC* DC, int x1, int y1, int x2, int y2,
                     int width, int Color );
static void GRSMoveTo( int x, int y );
static void GRSArc1( EDA_Rect* ClipBox, wxDC* DC, int x1, int y1, int x2, int y2,
                     int xc, int yc, int width, int Color );
static void GRSArc( EDA_Rect* ClipBox, wxDC* DC, int x, int y, int StAngle,
                    int EndAngle, int r, int width, int Color );
static void GRSFilledArc( EDA_Rect* ClipBox, wxDC* DC, int x, int y, int StAngle,
                          int EndAngle, int r, int width, int Color, int BgColor );
static void GRSCSegm( EDA_Rect* ClipBox, wxDC* DC, int x1, int y1, int x2, int y2,
                      int width, int aPenSize, int Color );
static void GRSFillCSegm( EDA_Rect* ClipBox, wxDC* DC, int x1, int y1, int x2, int y2,
                          int width, int Color );
/**/

extern BASE_SCREEN* ActiveScreen;


static int          GRLastMoveToX, GRLastMoveToY;
static bool         s_ForceBlackPen;   /* if true: draws in black instead of
                                      * color for printing. */
static int          xcliplo = 0,
                    ycliplo = 0,
                    xcliphi = 2000,
                    ycliphi = 2000;

static int   s_DC_lastcolor      = -1;
static int   s_DC_lastwidth      = -1;
static int   s_DC_lastpenstyle   = -1;
static int   s_DC_lastbrushcolor = -1;
static int   s_DC_lastbrushfill  = -1;
static wxDC* s_DC_lastDC         = NULL;


/* Local functions: */
static void GRSRect( EDA_Rect* aClipBox, wxDC* aDC, int x1, int y1,
                     int x2, int y2, int aWidth, int aColor,
                     wxPenStyle aStyle = wxPENSTYLE_SOLID );

/*
 * Macro clipping the trace of a line:
 * Line (x1, y1 x2, y2) is clipped to remain within
 * (Xcliplo, ycliplo xcliphi, ycliphi) (global variables, local to this file)
 * This is necessary because under WIN95 coord trace
 * (Although an int has 32 bits) are truncated to 16 bits (stupid)
 */
static inline int USCALE( unsigned int arg, unsigned int num, unsigned int den )
{
#ifndef USE_WX_ZOOM
    int ii;
    ii = (int) ( ( (float) arg * num ) / den );
    return ii;
#else
    return arg;
#endif
}


static int inline ZoomValue( int val )
{
    return ActiveScreen->Scale( val );
}


/****************************************/
/* External reference for the mappings. */
/****************************************/
int GRMapX( int x )
{
#ifndef USE_WX_ZOOM
    int coord = x - ActiveScreen->m_DrawOrg.x;
    coord  = ZoomValue( coord );
    coord -= ActiveScreen->m_StartVisu.x;
    return coord;
#else
    return x;
#endif
}


int GRMapY( int y )
{
#ifndef USE_WX_ZOOM
    int coord = y - ActiveScreen->m_DrawOrg.y;
    coord  = ZoomValue( coord );
    coord -= ActiveScreen->m_StartVisu.y;
    return coord;
#else
    return y;
#endif
}


#define WHEN_OUTSIDE return true;
#define WHEN_INSIDE


#if defined( USE_WX_ZOOM )
// currently only used if USE_WX_ZOOM is defined.
/**
 * Test if any part of a line falls within the bounds of a rectangle.
 *
 * Please note that this is only accurate for lines that are one pixel wide.
 *
 * @param aRect - The rectangle to test.
 * @param x1 - X coordinate of one end of a line.
 * @param y1 - Y coordinate of one end of a line.
 * @param x2 - X coordinate of the other end of a line.
 * @param y2 - Y coordinate of the other  end of a line.
 *
 * @return - False if any part of the line lies within the rectangle.
 */
static bool clipLine( EDA_Rect* aClipBox, int& x1, int& y1, int& x2, int& y2 )
{
    if( aClipBox->Inside( x1, y1 ) && aClipBox->Inside( x2, y2 ) )
        return false;

    wxRect rect = *aClipBox;
    int minX = rect.GetLeft();
    int maxX = rect.GetRight();
    int minY = rect.GetTop();
    int maxY = rect.GetBottom();
    int clippedX, clippedY;

#if DEBUG_DUMP_CLIP_COORDS
    int tmpX1, tmpY1, tmpX2, tmpY2;
    tmpX1 = x1;
    tmpY1 = y1;
    tmpX2 = x2;
    tmpY2 = y2;
#endif

    if( aClipBox->Inside( x1, y1 ) )
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

        if(    TestForIntersectionOfStraightLineSegments( x1, y1, x2, y2, minX, minY, minX, maxY,
                                                          &clippedX, &clippedY ) /* Left */
            || TestForIntersectionOfStraightLineSegments( x1, y1, x2, y2, minX, minY, maxX, minY,
                                                          &clippedX, &clippedY ) /* Top */
            || TestForIntersectionOfStraightLineSegments( x1, y1, x2, y2, maxX, minY, maxX, maxY,
                                                          &clippedX, &clippedY ) /* Right */
            || TestForIntersectionOfStraightLineSegments( x1, y1, x2, y2, minX, maxY, maxX, maxY,
                                                          &clippedX, &clippedY ) )  /* Bottom */
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
    else if( aClipBox->Inside( x2, y2 ) )
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

        if(    TestForIntersectionOfStraightLineSegments( x1, y1, x2, y2, minX, minY, minX, maxY,
                                                          &clippedX, &clippedY ) /* Left */
            || TestForIntersectionOfStraightLineSegments( x1, y1, x2, y2, minX, minY, maxX, minY,
                                                          &clippedX, &clippedY ) /* Top */
            || TestForIntersectionOfStraightLineSegments( x1, y1, x2, y2, maxX, minY, maxX, maxY,
                                                          &clippedX, &clippedY ) /* Right */
            || TestForIntersectionOfStraightLineSegments( x1, y1, x2, y2, minX, maxY, maxX, maxY,
                                                          &clippedX, &clippedY ) )  /* Bottom */
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
        int intersectX1, intersectY1, intersectX2, intersectY2;
        bool haveFirstPoint = false;

        intersectX = &intersectX1;
        intersectY = &intersectY1;

        /* Left clip rectangle line. */
        if( TestForIntersectionOfStraightLineSegments( x1, y1, x2, y2, minX, minY, minX, maxY,
                                                       intersectX, intersectY ) )
        {
            intersectX = &intersectX2;
            intersectY = &intersectY2;
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
#endif      // if defined( USE_WX_ZOOM )


/**
 * Function clip_line
 * @return bool - true when WHEN_OUTSIDE fires, else false.
 */
static inline bool clip_line( int& x1, int& y1, int& x2, int& y2 )
{
    int temp;

    if( x1 > x2 )
    {
        EXCHG( x1, x2 );
        EXCHG( y1, y2 );
    }
    if( (x2 < xcliplo) || (x1 > xcliphi) )
    {
        WHEN_OUTSIDE;
    }
    if( y1 < y2 )
    {
        if( (y2 < ycliplo) || (y1 > ycliphi) )
        {
            WHEN_OUTSIDE;
        }
        if( y1 < ycliplo )
        {
            temp = USCALE( (x2 - x1), (ycliplo - y1), (y2 - y1) );
            if( (x1 += temp) > xcliphi )
            {
                WHEN_OUTSIDE;
            }
            y1 = ycliplo;
            WHEN_INSIDE;
        }
        if( y2 > ycliphi )
        {
            temp = USCALE( (x2 - x1), (y2 - ycliphi), (y2 - y1) );
            if( (x2 -= temp) < xcliplo )
            {
                WHEN_OUTSIDE;
            }
            y2 = ycliphi;
            WHEN_INSIDE;
        }
        if( x1 < xcliplo )
        {
            temp = USCALE( (y2 - y1), (xcliplo - x1), (x2 - x1) );
            y1  += temp;
            x1   = xcliplo;
            WHEN_INSIDE;
        }
        if( x2 > xcliphi )
        {
            temp = USCALE( (y2 - y1), (x2 - xcliphi), (x2 - x1) );
            y2  -= temp;
            x2   = xcliphi;
            WHEN_INSIDE;
        }
    }
    else
    {
        if( (y1 < ycliplo) || (y2 > ycliphi) )
        {
            WHEN_OUTSIDE;
        }
        if( y1 > ycliphi )
        {
            temp = USCALE( (x2 - x1), (y1 - ycliphi), (y1 - y2) );
            if( (x1 += temp) > xcliphi )
            {
                WHEN_OUTSIDE;
            }
            y1 = ycliphi;
            WHEN_INSIDE;
        }
        if( y2 < ycliplo )
        {
            temp = USCALE( (x2 - x1), (ycliplo - y2), (y1 - y2) );
            if( (x2 -= temp) < xcliplo )
            {
                WHEN_OUTSIDE;
            }
            y2 = ycliplo;
            WHEN_INSIDE;
        }
        if( x1 < xcliplo )
        {
            temp = USCALE( (y1 - y2), (xcliplo - x1), (x2 - x1) );
            y1  -= temp;
            x1   = xcliplo;
            WHEN_INSIDE;
        }
        if( x2 > xcliphi )
        {
            temp = USCALE( (y1 - y2), (x2 - xcliphi), (x2 - x1) );
            y2  += temp;
            x2   = xcliphi;
            WHEN_INSIDE;
        }
    }

    return false;
}


static void WinClipAndDrawLine( EDA_Rect* ClipBox, wxDC* DC,
                                int x1, int y1, int x2, int y2,
                                int Color, int width = 1 )
{
    GRLastMoveToX = x2;
    GRLastMoveToY = y2;

    if( ClipBox )
    {
        xcliplo = ClipBox->GetX();
        ycliplo = ClipBox->GetY();
        xcliphi = ClipBox->GetRight();
        ycliphi = ClipBox->GetBottom();

        xcliplo -= width;
        ycliplo -= width;

        xcliphi += width;
        ycliphi += width;

#if defined( USE_WX_ZOOM )
        if ( clipLine( ClipBox, x1, y1, x2, y2 ) )
#else
        if( clip_line( x1, y1, x2, y2 ) )
#endif
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
    s_DC_lastDC = NULL;
}


/**
 * Function GRSetColorPen
 * sets a pen style, width, color, and alpha into the given device context.
 */
void GRSetColorPen( wxDC* DC, int Color, int width, wxPenStyle style )
{
    if( width < 0 )
        width = 0;

    if( s_ForceBlackPen )
    {
        Color = BLACK;
    }

    if(   s_DC_lastcolor != Color ||
          s_DC_lastwidth != width ||
          s_DC_lastpenstyle != style ||
          s_DC_lastDC != DC  )
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
        s_DC_lastDC       = DC;

    }
}


void GRSetBrush( wxDC* DC, int Color, int fill )
{
    if( s_ForceBlackPen )
        Color = BLACK;

    if(   s_DC_lastbrushcolor != Color ||
          s_DC_lastbrushfill  != fill ||
          s_DC_lastDC != DC  )
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



/** function GRForceBlackPen
 * @param flagforce True to force a black pen whenever the asked color
 */
void GRForceBlackPen( bool flagforce )
{
    s_ForceBlackPen = flagforce;
}


/** function GetGRForceBlackPenState
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
}


void GRPutPixel( EDA_Rect* ClipBox, wxDC* DC, int x, int y, int Color )
{
    GRSPutPixel( ClipBox, DC, GRMapX( x ), GRMapY( y ), Color );
}


void GRSPutPixel( EDA_Rect* ClipBox, wxDC* DC, int x, int y, int Color )
{
    if( ClipBox && !ClipBox->Inside( x, y ) )
        return;

    GRSetColorPen( DC, Color );
    DC->DrawPoint( x, y );
}


/*
 * Draw a line, in object space.
 */
void GRLine( EDA_Rect* ClipBox,
             wxDC*     DC,
             int       x1,
             int       y1,
             int       x2,
             int       y2,
             int       width,
             int       Color )
{
    GRSLine( ClipBox, DC, GRMapX( x1 ), GRMapY( y1 ), GRMapX( x2 ),
             GRMapY( y2 ), ZoomValue( width ), Color );
}


void GRLine( EDA_Rect* aClipBox,
             wxDC*     aDC,
             wxPoint   aStart,
             wxPoint   aEnd,
             int       aWidth,
             int       aColor )
{
    GRSLine( aClipBox, aDC, GRMapX( aStart.x ), GRMapY( aStart.y ),
             GRMapX( aEnd.x ), GRMapY( aEnd.y ),
             ZoomValue( aWidth ), aColor );
}


/*
 * Draw a dashed line, in screen space.
 */
void GRSDashedLine( EDA_Rect* ClipBox,
                    wxDC*     DC,
                    int       x1,
                    int       y1,
                    int       x2,
                    int       y2,
                    int       width,
                    int       Color )
{
    GRLastMoveToX = x2;
    GRLastMoveToY = y2;
    s_DC_lastcolor     = -1;
    GRSetColorPen( DC, Color, width, wxSHORT_DASH );
    GRSLine( ClipBox, DC, x1, y1, x2, y2, width, Color );
    s_DC_lastcolor = -1;
    GRSetColorPen( DC, Color, width );
}


void GRSDashedLineTo( EDA_Rect* ClipBox,
                      wxDC*     DC,
                      int       x2,
                      int       y2,
                      int       width,
                      int       Color )
{
    s_DC_lastcolor = -1;
    GRSetColorPen( DC, Color, width, wxSHORT_DASH );
    GRSLine( ClipBox, DC, GRLastMoveToX, GRLastMoveToY, x2, y2, width, Color );
    s_DC_lastcolor = -1;
    GRSetColorPen( DC, Color, width );
    GRLastMoveToX = x2;
    GRLastMoveToY = y2;
}


/*
 * Draw a dashed line, in object space.
 */
void GRDashedLineTo( EDA_Rect* ClipBox,
                     wxDC*     DC,
                     int       x2,
                     int       y2,
                     int       width,
                     int       Color )
{
    GRSDashedLineTo( ClipBox, DC, GRMapX( x2 ), GRMapY( y2 ),
                     ZoomValue( width ), Color );
}


void GRDashedLine( EDA_Rect* ClipBox,
                   wxDC*     DC,
                   int       x1,
                   int       y1,
                   int       x2,
                   int       y2,
                   int       width,
                   int       Color )
{
    GRSDashedLine( ClipBox, DC, GRMapX( x1 ), GRMapY( y1 ), GRMapX( x2 ),
                   GRMapY( y2 ), ZoomValue( width ), Color );
}


/*
 * Move to a new position, in object space.
 */
void GRMoveTo( int x, int y )
{
    GRLastMoveToX = GRMapX( x );
    GRLastMoveToY = GRMapY( y );
}


/*
 * Draw line to a new position, in object space.
 */
void GRLineTo( EDA_Rect* ClipBox, wxDC* DC, int x, int y, int width, int Color )
{
    int GRLineToX, GRLineToY;

    GRLineToX = GRMapX( x ); GRLineToY = GRMapY( y );
    GRSLine( ClipBox,
             DC,
             GRLastMoveToX,
             GRLastMoveToY,
             GRLineToX,
             GRLineToY,
             ZoomValue( width ),
             Color );
}


/*
 * Draw a mixed line, in object space.
 */
void GRMixedLine( EDA_Rect* ClipBox,
                  wxDC*     DC,
                  int       x1,
                  int       y1,
                  int       x2,
                  int       y2,
                  int       width,
                  int       Color )
{
    GRSMixedLine( ClipBox, DC, GRMapX( x1 ), GRMapY( y1 ), GRMapX( x2 ),
                  GRMapY( y2 ), ZoomValue( width ), Color );
}


/*
 * Draw a mixed line, in screen (Pixels) space.
 * Currently, draw a line (not a mixed line)
 * Perhaps this function is not very useful.
 */
void GRSMixedLine( EDA_Rect* ClipBox,
                   wxDC*     DC,
                   int       x1,
                   int       y1,
                   int       x2,
                   int       y2,
                   int       width,
                   int       Color )
{
    GRSetColorPen( DC, Color, width, wxDOT_DASH );
    GRSLine( ClipBox, DC, x1, y1, x2, y2, width, Color );
    GRSetColorPen( DC, Color, width );
}


/*
 * Move to a new position, in screen (pixels) space.
 */
void GRSMoveTo( int x, int y )
{
    GRLastMoveToX = x;
    GRLastMoveToY = y;
}



/*
 * Draw line to a new position, in screen (pixels) space.
 */
void GRSLine( EDA_Rect* ClipBox,
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


/*
 * Move to a new position relative to current one, in object space.
 */
void GRMoveRel( int x, int y )
{
    GRLastMoveToX += ZoomValue( x );
    GRLastMoveToY += ZoomValue( y );
}


/*
 * Draw a line to a new position relative to current one, in object space.
 */
void GRLineRel( EDA_Rect* ClipBox,
                wxDC*     DC,
                int       x,
                int       y,
                int       width,
                int       Color )
{
    int GRLineToX = GRLastMoveToX,
        GRLineToY = GRLastMoveToY;

    GRLineToX += ZoomValue( x );
    GRLineToY += ZoomValue( y );

    GRSLine( ClipBox,
             DC,
             GRLastMoveToX,
             GRLastMoveToY,
             GRLineToX,
             GRLineToY,
             ZoomValue( width ),
             Color );
}


/*
 * Draw segment with rounded ends in object space.
 */
void GRCSegm( EDA_Rect* ClipBox, wxDC* DC, int x1, int y1, int x2, int y2,
              int width, int aPenSize, int Color )
{
    GRSCSegm( ClipBox, DC, GRMapX( x1 ), GRMapY( y1 ), GRMapX( x2 ),
              GRMapY( y2 ), ZoomValue( width ), ZoomValue( aPenSize ), Color );
}

void GRCSegm( EDA_Rect* ClipBox, wxDC* DC, int x1, int y1, int x2, int y2,
              int width, int Color )
{
    GRSCSegm( ClipBox, DC, GRMapX( x1 ), GRMapY( y1 ), GRMapX( x2 ),
              GRMapY( y2 ), ZoomValue( width ), 0, Color );
}


/*
 * Draw segment (full) with rounded ends in object space (real coords.).
 */
void GRFillCSegm( EDA_Rect* ClipBox, wxDC* DC, int x1, int y1, int x2, int y2,
                  int width, int Color )
{
    GRSFillCSegm( ClipBox, DC, GRMapX( x1 ), GRMapY( y1 ), GRMapX( x2 ),
                  GRMapY( y2 ), ZoomValue( width ), Color );
}


/*
 * Draw segment with rounded ends in screen space.
 */
void GRSFillCSegm( EDA_Rect* ClipBox,
                   wxDC*     DC,
                   int       x1,
                   int       y1,
                   int       x2,
                   int       y2,
                   int       width,
                   int       Color )
{
    WinClipAndDrawLine( ClipBox, DC, x1, y1, x2, y2, Color, width );
}


/*
 * Draw segment with rounded ends (SKETCH mode) in screen space
 */
void GRSCSegm( EDA_Rect* ClipBox,
               wxDC*     DC,
               int       x1,
               int       y1,
               int       x2,
               int       y2,
               int       width,
               int       aPenSize,
               int       Color )
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
        xcliplo = ClipBox->GetX();
        ycliplo = ClipBox->GetY();
        xcliphi = ClipBox->GetRight();
        ycliphi = ClipBox->GetHeight();

        xcliplo -= width;
        ycliplo -= width;

        xcliphi += width;
        ycliphi += width;

#if defined( USE_WX_ZOOM )
        if( clipLine( ClipBox, x1, y1, x2, y2 ) )
#else
        if( clip_line( x1, y1, x2, y2 ) )
#endif
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


static bool IsGRSPolyDrawable( EDA_Rect* ClipBox, int n, wxPoint Points[] )
{
    if( ! ClipBox )
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
static void GRSPoly( EDA_Rect* ClipBox,
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
static void GRSClosedPoly( EDA_Rect* ClipBox,
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
        GRSMoveTo( aPoints[aPointCount - 1].x, aPoints[aPointCount - 1].y );
        GRSetBrush( DC, BgColor, FILLED );
        DC->DrawPolygon( aPointCount, aPoints, 0, 0, wxODDEVEN_RULE );
    }
    else
    {
        GRSetBrush( DC, BgColor );
        DC->DrawLines( aPointCount, aPoints );

        /* Close the polygon. */
        if( aPoints[aPointCount - 1] != aPoints[0] )
        {
            GRSLine( ClipBox,
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
void GRPoly( EDA_Rect* ClipBox, wxDC* DC, int n, wxPoint Points[],
             bool Fill, int width, int Color, int BgColor )
{
    for( int i = 0; i<n; ++i )
    {
        Points[i].x = GRMapX( Points[i].x );
        Points[i].y = GRMapY( Points[i].y );
    }

    width = ZoomValue( width );
    GRSPoly( ClipBox, DC, n, Points, Fill, width, Color, BgColor );
}


/*
 * Draw a closed polyline and fill it if Fill, in object space.
 */
void GRClosedPoly( EDA_Rect* ClipBox, wxDC* DC, int n, wxPoint Points[],
                   bool Fill, int Color, int BgColor )
{
    GRClosedPoly( ClipBox, DC, n, Points, Fill, 0, Color, BgColor );
}


void GRClosedPoly( EDA_Rect* ClipBox, wxDC* DC, int n, wxPoint Points[],
                   bool Fill, int width, int Color, int BgColor )
{
    for( int i = 0; i<n; ++i )
    {
        Points[i].x = GRMapX( Points[i].x );
        Points[i].y = GRMapY( Points[i].y );
    }

    width = ZoomValue( width );
    GRSClosedPoly( ClipBox, DC, n, Points, Fill, width, Color, BgColor );
}


/*
 * Draw a circle, in object space.
 */
void GRCircle( EDA_Rect* ClipBox, wxDC* DC, int x, int y, int r, int Color )
{
    int cx     = GRMapX( x );
    int cy     = GRMapY( y );
    int radius = ZoomValue( r );

    GRSCircle( ClipBox, DC, cx, cy, radius, 0, Color );
}

/*
 * Draw a circle in object space.
 */
void GRCircle( EDA_Rect* ClipBox, wxDC* DC, int x, int y, int r, int width, int Color )
{
    r     = ZoomValue( r );
    width = ZoomValue( width );
    GRSCircle( ClipBox, DC, GRMapX( x ), GRMapY( y ), r, width, Color );
}

/*
 * Draw a circle in object space.
 */
void GRCircle( EDA_Rect* aClipBox, wxDC* aDC, wxPoint aPos, int aRadius, int aWidth, int aColor )
{
    aRadius     = ZoomValue( aRadius );
    aWidth = ZoomValue( aWidth );
    GRSCircle( aClipBox, aDC, GRMapX( aPos.x ), GRMapY( aPos.y ), aRadius, aWidth, aColor );
}


/*
 * Draw a filled circle, in object space.
 */
void GRFilledCircle( EDA_Rect* ClipBox, wxDC* DC, int x, int y, int r,
                     int width, int Color, int BgColor )
{
    r     = ZoomValue( r );
    width = ZoomValue( width );
    GRSFilledCircle( ClipBox, DC, GRMapX( x ), GRMapY( y ), r, width,
                     Color, BgColor );
}

/*
 * Draw a filled circle, in object space.
 */
void GRFilledCircle( EDA_Rect* aClipBox, wxDC* aDC, wxPoint aPos, int aRadius, int aColor )
{
    aRadius     = ZoomValue( aRadius );
    GRSFilledCircle( aClipBox, aDC, GRMapX( aPos.x ), GRMapY( aPos.y ), aRadius, 0,
                     aColor, aColor );
}

/*
 * Draw a filled circle, in drawing space.
 */
void GRSFilledCircle( EDA_Rect* ClipBox, wxDC* DC, int x, int y, int r,
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



/*
 * Draw a circle in drawing space.
 */
void GRSCircle( EDA_Rect* ClipBox,
                wxDC*     DC,
                int       xc,
                int       yc,
                int       r,
                int       width,
                int       Color )
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


/*
 * Draw an arc in user space.
 */
void GRArc1( EDA_Rect* ClipBox, wxDC* DC, int x1, int y1, int x2, int y2,
             int xc, int yc, int Color )
{
    GRSArc1( ClipBox, DC, GRMapX( x1 ), GRMapY( y1 ), GRMapX( x2 ),
             GRMapY( y2 ), GRMapX( xc ), GRMapY( yc ), 0, Color );
}


/*
 * Draw an arc, width = width in user space.
 */
void GRArc1( EDA_Rect* ClipBox, wxDC* DC, int x1, int y1, int x2, int y2,
             int xc, int yc, int width, int Color )
{
    GRSArc1( ClipBox, DC, GRMapX( x1 ), GRMapY( y1 ), GRMapX( x2 ),
             GRMapY( y2 ), GRMapX( xc ), GRMapY( yc ), ZoomValue( width ),
             Color );
}


/*
 * Draw an arc, width = width, in screen space.
 */
void GRSArc1( EDA_Rect* ClipBox, wxDC* DC, int x1, int y1, int x2, int y2,
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


/*
 * Draw an arc in screen space.
 */
void GRSArc( EDA_Rect* ClipBox,
             wxDC*     DC,
             int       xc,
             int       yc,
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
        if( xc < ( x0 - r - width ) )
            return;
        if( yc < ( y0 - r - width ) )
            return;
        if( xc > ( r + xm + width ) )
            return;
        if( yc > ( r + ym + width ) )
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
    DC->DrawArc( xc + x1, yc - y1, xc + x2, yc - y2, xc, yc );
}


/*
 * Draw an filled arc in screen space.
 */
void GRSFilledArc( EDA_Rect* ClipBox,
                   wxDC*     DC,
                   int       xc,
                   int       yc,
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
        if( xc < ( x0 - r - 1 ) )
            return;
        if( yc < ( y0 - r - 1 ) )
            return;
        if( xc > ( r + xm + 1 ) )
            return;
        if( yc > ( r + ym + 1 ) )
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
    DC->DrawArc( xc + x1, yc - y1, xc + x2, yc - y2, xc, yc );
}


/*
 * Draw a filled arc in drawing space.
 */
void GRFilledArc( EDA_Rect* ClipBox,
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
    width = ZoomValue( width );
    GRSFilledArc( ClipBox, DC, GRMapX( x ), GRMapY( y ), StAngle, EndAngle,
                  ZoomValue( r ), width, Color, BgColor );
}


void GRFilledArc( EDA_Rect* ClipBox, wxDC* DC, int x, int y,
                  int StAngle, int EndAngle, int r, int Color, int BgColor )
{
    GRSFilledArc( ClipBox, DC, GRMapX( x ), GRMapY( y ), StAngle, EndAngle,
                  ZoomValue( r ), 0, Color, BgColor );
}


/*
 * Draw an arc in drawing space.
 */
void GRArc( EDA_Rect* ClipBox, wxDC* DC, int xc, int yc, int StAngle,
            int EndAngle, int r, int Color )
{
    int x1, y1, x2, y2;

    /* Clip arcs off screen */
    if( ClipBox )
    {
        int radius = ZoomValue( r ) + 1;
        int x0, y0, xm, ym, x, y;
        x0 = ClipBox->GetX();
        y0 = ClipBox->GetY();
        xm = ClipBox->GetRight();
        ym = ClipBox->GetBottom();
        x  = GRMapX( xc ); y = GRMapY( yc );
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
    DC->DrawArc( GRMapX( xc + x1 ), GRMapY( yc - y1 ), GRMapX( xc + x2 ),
                GRMapY( yc - y2 ), GRMapX( xc ), GRMapY( yc ) );
}


/*
 * Draw an arc with width = width in drawing space.
 */
void GRArc( EDA_Rect* ClipBox,
            wxDC*     DC,
            int       x,
            int       y,
            int       StAngle,
            int       EndAngle,
            int       r,
            int       width,
            int       Color )
{
    GRSArc( ClipBox, DC, GRMapX( x ), GRMapY( y ), StAngle, EndAngle,
            ZoomValue( r ), ZoomValue( width ), Color );
}


/*
 * Draw a rectangle in drawing space.
 */
void GRRect( EDA_Rect* aClipBox, wxDC* aDC, int x1, int y1, int x2, int y2, int aColor )
{
    x1 = GRMapX( x1 );
    y1 = GRMapY( y1 );
    x2 = GRMapX( x2 );
    y2 = GRMapY( y2 );

    GRSRect( aClipBox, aDC, x1, y1, x2, y2, 0, aColor );
}

void GRRectPs( EDA_Rect* aClipBox, wxDC* aDC, const EDA_Rect& aRect, int aColor, wxPenStyle aStyle )
{
    int x1 = GRMapX( aRect.GetX() );
    int y1 = GRMapY( aRect.GetY() );
    int x2 = GRMapX( aRect.GetRight() );
    int y2 = GRMapY( aRect.GetBottom() );

    GRSRect( aClipBox, aDC, x1, y1, x2, y2, 0, aColor, aStyle );
}


/*
 * Draw a rectangle (thick lines) in drawing space.
 */
void GRRect( EDA_Rect* ClipBox, wxDC* DC, int x1, int y1, int x2, int y2, int width, int Color )
{
    x1    = GRMapX( x1 );
    y1    = GRMapY( y1 );
    x2    = GRMapX( x2 );
    y2    = GRMapY( y2 );
    width = ZoomValue( width );

    GRSRect( ClipBox, DC, x1, y1, x2, y2, width, Color );
}

void GRRect( EDA_Rect* aClipBox, wxDC* aDC, const EDA_Rect& aRect, int aWidth, int aColor )
{
    int x1 = GRMapX( aRect.GetX() );
    int y1 = GRMapY( aRect.GetY() );
    int x2 = GRMapX( aRect.GetRight() );
    int y2 = GRMapY( aRect.GetBottom() );

    int width = ZoomValue( aWidth );

    GRSRect( aClipBox, aDC, x1, y1, x2, y2, width, aColor );
}


/*
 * Draw a rectangle (filled with AreaColor) in drawing space.
 */
void GRFilledRect( EDA_Rect* ClipBox, wxDC* DC, int x1, int y1, int x2, int y2,
                   int Color, int BgColor )
{
    x1 = GRMapX( x1 );
    y1 = GRMapY( y1 );
    x2 = GRMapX( x2 );
    y2 = GRMapY( y2 );

    GRSFilledRect( ClipBox, DC, x1, y1, x2, y2, 0, Color, BgColor );
}


/*
 * Draw a rectangle (filled with AreaColor) in drawing space.
 */
void GRFilledRect( EDA_Rect* ClipBox, wxDC* DC, int x1, int y1, int x2, int y2,
                   int width, int Color, int BgColor )
{
    x1    = GRMapX( x1 );
    y1    = GRMapY( y1 );
    x2    = GRMapX( x2 );
    y2    = GRMapY( y2 );
    width = ZoomValue( width );

    GRSFilledRect( ClipBox, DC, x1, y1, x2, y2, width, Color, BgColor );
}


/*
 * Draw a rectangle in screen space.
 */

void GRSRect( EDA_Rect* aClipBox, wxDC* aDC, int x1, int y1, int x2, int y2,
              int aWidth, int aColor, wxPenStyle aStyle )
{
    if( x1 > x2 )
        EXCHG( x1, x2 );
    if( y1 > y2 )
        EXCHG( y1, y2 );

    if( aClipBox )
    {
        int xmin = aClipBox->GetX();
        int ymin = aClipBox->GetY();
        int xmax = aClipBox->GetRight();
        int ymax = aClipBox->GetBottom();

        if( x1 > xmax )
            return;
        if( x2 < xmin )
            return;
        if( y1 > ymax )
            return;
        if( y2 < ymin )
            return;
    }

    GRSetColorPen( aDC, aColor, aWidth, aStyle );
    if( ( x1 == x2 ) || ( y1 == y2 ) )
        aDC->DrawLine( x1, y1, x2, y2 );
    else
    {
        GRSetBrush( aDC, BLACK );
        aDC->DrawRectangle( x1, y1, x2 - x1, y2 - y1 );
    }
}



void GRSFilledRect( EDA_Rect* ClipBox, wxDC*     DC,
                    int       x1, int       y1, int       x2, int       y2,
                    int       width, int       Color, int       BgColor )
{
    if( x1 > x2 )
        EXCHG( x1, x2 );
    if( y1 > y2 )
        EXCHG( y1, y2 );
    if( ClipBox )
    {
        int xmin = ClipBox->GetX();
        int ymin = ClipBox->GetY();
        int xmax = ClipBox->GetRight();
        int ymax = ClipBox->GetBottom();

        if( x1 > xmax )
            return;
        if( x2 < xmin )
            return;
        if( y1 > ymax )
            return;
        if( y2 < ymin )
            return;

        // Clipping coordinates
        if( x1 < xmin )
            x1 = xmin - 1;
        if( y1 < ymin )
            y1 = ymin - 1;
        if( x2 > xmax )
            x2 = xmax + 1;
        if( y2 > ymax )
            y2 = ymax + 1;
    }

    GRSetColorPen( DC, Color, width );
    if( ( x1 == x2 ) || ( y1 == y2 ) )
        DC->DrawLine( x1, y1, x2, y2 );
    else
    {
        GRSetBrush( DC, BgColor, FILLED );
        DC->DrawRectangle( x1, y1, x2 - x1, y2 - y1 );
    }
}


#ifdef USE_CLIP_FILLED_POLYGONS

/** Function ClipAndDrawFilledPoly
 *  Used to clip a polygon and draw it as Filled Polygon
 *  uses the Sutherland and Hodgman algo to clip the given poly against a
 *  rectangle.  This rectangle is the drawing area this is useful under
 *  Linux (2009) because filled polygons are incorrectly drawn if they have
 *  too large coordinates (seems due to integer overflows in calculations)
 *  Could be removed in some years, if become unnecessary.
 */
#include "SutherlandHodgmanClipPoly.h"
void ClipAndDrawFilledPoly( EDA_Rect* aClipBox,
                            wxDC*     aDC,
                            wxPoint   aPoints[],
                            int       n )
{
    static vector<wxPoint> clippedPolygon;
    static pointVector     inputPolygon, outputPolygon;

    inputPolygon.clear();
    outputPolygon.clear();
    clippedPolygon.clear();
    for( int ii = 0; ii < n; ii++ )
        inputPolygon.push_back( PointF( (REAL) aPoints[ii].x,
                                        (REAL) aPoints[ii].y ) );

    RectF window( (REAL) aClipBox->GetX(), (REAL) aClipBox->GetY(),
                  (REAL) aClipBox->GetWidth(),
                  (REAL) aClipBox->GetHeight() );

    SutherlandHodgman sh( window );
    sh.Clip( inputPolygon, outputPolygon );

    for( cpointIterator cit = outputPolygon.begin();
         cit != outputPolygon.end();
         ++cit )
    {
        clippedPolygon.push_back( wxPoint( wxRound( cit->X ),
                                           wxRound( cit->Y ) ) );
    }

    if( clippedPolygon.size() )
        aDC->DrawPolygon( clippedPolygon.size(), &clippedPolygon[0] );
}


#endif


void GRBezier( EDA_Rect* ClipBox,
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


void GRBezier( EDA_Rect* ClipBox,
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
