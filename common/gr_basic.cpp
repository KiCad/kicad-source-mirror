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

static const bool FILLED = true;
static const bool NOT_FILLED = false;

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
GR_DRAWMODE g_XorMode = GR_NXOR;


static void ClipAndDrawPoly( EDA_RECT * ClipBox, wxDC * DC, wxPoint Points[],
                             int n );

/* These functions are used by corresponding functions
 * ( GRSCircle is called by GRCircle for instance) after mapping coordinates
 * from user units to screen units(pixels coordinates)
 */
static void GRSRect( EDA_RECT* aClipBox, wxDC* aDC, int x1, int y1,
                     int x2, int y2, int aWidth, EDA_COLOR_T aColor,
                     wxPenStyle aStyle = wxPENSTYLE_SOLID );

/**/

static int          GRLastMoveToX, GRLastMoveToY;
static bool         s_ForceBlackPen;   /* if true: draws in black instead of
                                        * color for printing. */
static int          xcliplo = 0,
                    ycliplo = 0,
                    xcliphi = 2000,
                    ycliphi = 2000;

static EDA_COLOR_T   s_DC_lastcolor = UNSPECIFIED_COLOR;
static EDA_COLOR_T   s_DC_lastbrushcolor = UNSPECIFIED_COLOR;
static bool  s_DC_lastbrushfill  = false;
static wxDC* s_DC_lastDC = NULL;

/***
 * Utility for the line clipping code, returns the boundary code of
 * a point. Bit allocation is arbitrary
 */
static inline int clipOutCode( const EDA_RECT *aClipBox, int x, int y )
{
    int code;
    if( y < aClipBox->GetY() )
        code = 2;
    else if( y > aClipBox->GetBottom() )
        code = 1;
    else
        code = 0;
    if( x < aClipBox->GetX() )
        code |= 4;
    else if( x > aClipBox->GetRight() )
        code |= 8;
    return code;
}


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
static bool clipLine( const EDA_RECT *aClipBox, int &x1, int &y1, int &x2, int &y2 )
{
    // Stock Cohen-Sutherland algorithm; check *any* CG book for details
    int outcode1 = clipOutCode( aClipBox, x1, y1 );
    int outcode2 = clipOutCode( aClipBox, x2, y2 );

    while( outcode1 || outcode2 )
    {
        // Fast reject
        if( outcode1 & outcode2 )
            return true;

        // Choose a side to clip
        int thisoutcode, x, y;
        if( outcode1 )
            thisoutcode = outcode1;
        else
            thisoutcode = outcode2;

        /* One clip round
         * Since we use the full range of 32 bit ints, the proportion
         * computation has to be done in 64 bits to avoid horrible
         * results */
        if( thisoutcode & 1 ) // Clip the bottom
        {
            y = aClipBox->GetBottom();
            x = x1 + (x2 - x1) * int64_t(y - y1) / (y2 - y1);
        }
        else if( thisoutcode & 2 ) // Clip the top
        {
            y = aClipBox->GetY();
            x = x1 + (x2 - x1) * int64_t(y - y1) / (y2 - y1);
        }
        else if( thisoutcode & 8 ) // Clip the right
        {
            x = aClipBox->GetRight();
            y = y1 + (y2 - y1) * int64_t(x - x1) / (x2 - x1);
        }
        else // if( thisoutcode & 4), obviously, clip the left
        {
            x = aClipBox->GetX();
            y = y1 + (y2 - y1) * int64_t(x - x1) / (x2 - x1);
        }

        // Put the result back and update the boundary code
        // No ambiguity, otherwise it would have been a fast reject
        if( thisoutcode == outcode1 )
        {
            x1 = x;
            y1 = y;
            outcode1 = clipOutCode( aClipBox, x1, y1 );
        }
        else
        {
            x2 = x;
            y2 = y;
            outcode2 = clipOutCode( aClipBox, x2, y2 );
        }
    }
    return false;
}

static void WinClipAndDrawLine( EDA_RECT* ClipBox, wxDC* DC, int x1, int y1, int x2, int y2, int width )
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

    DC->DrawLine( x1, y1, x2, y2 );
}


/* Forcing a reset of the current pen.
 * Must be called after changing the graphical device before any trace.
 */
void GRResetPenAndBrush( wxDC* DC )
{
    GRSetBrush( DC, BLACK );  // Force no fill
    s_DC_lastbrushcolor = UNSPECIFIED_COLOR;
    s_DC_lastcolor =  UNSPECIFIED_COLOR;
    s_DC_lastDC    = NULL;
}


/**
 * Function GRSetColorPen
 * sets a pen style, width, color, and alpha into the given device context.
 */
void GRSetColorPen( wxDC* DC, EDA_COLOR_T Color, int width, wxPenStyle style )
{
    // Under OSX and while printing when wxPen is set to 0, renderer follows the request drawing
    // nothing & in the bitmap world the minimum is enough to light a pixel, in vectorial one not
    if( width <= 1 )
        width = DC->DeviceToLogicalXRel( 1 );

    if( s_ForceBlackPen )
        Color = BLACK;

    wxColour wx_color = MakeColour( Color );
    const wxPen& curr_pen = DC->GetPen();

    if( !curr_pen.IsOk() || curr_pen.GetColour() != wx_color
       || curr_pen.GetWidth() != width
       || curr_pen.GetStyle() != style )
    {
        wxPen pen;
        pen.SetColour( wx_color );
        pen.SetWidth( width );
        pen.SetStyle( style );
        DC->SetPen( pen );
    }
    else
        // Should be not needed, but on Linux, in printing process
        // the curr pen settings needs to be sometimes re-initialized
        // Clearly, this is due to a bug, related to SetBrush(),
        // but we have to live with it, at least on wxWidgets 3.0
        DC->SetPen( curr_pen );
}


void GRSetBrush( wxDC* DC, EDA_COLOR_T Color, bool fill )
{
    if( s_ForceBlackPen )
        Color = BLACK;

    if(   s_DC_lastbrushcolor != Color
       || s_DC_lastbrushfill  != fill
       || s_DC_lastDC != DC )
    {
        wxBrush brush;

        brush.SetColour( MakeColour( Color ) );

        if( fill )
            brush.SetStyle( wxBRUSHSTYLE_SOLID );
        else
            brush.SetStyle( wxBRUSHSTYLE_TRANSPARENT );

        DC->SetBrush( brush );

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
void GRSetDrawMode( wxDC* DC, GR_DRAWMODE draw_mode )
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
    else if( draw_mode & GR_COPY )
        DC->SetLogicalFunction( wxCOPY );

#ifdef USE_WX_OVERLAY
    DC->SetLogicalFunction( wxCOPY );
#endif
}


void GRPutPixel( EDA_RECT* ClipBox, wxDC* DC, int x, int y, EDA_COLOR_T Color )
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
             EDA_COLOR_T       Color )
{
    GRSetColorPen( DC, Color, width );
    WinClipAndDrawLine( ClipBox, DC, x1, y1, x2, y2, width );
    GRLastMoveToX = x2;
    GRLastMoveToY = y2;
}


void GRLine( EDA_RECT* aClipBox, wxDC* aDC, wxPoint aStart, wxPoint aEnd, int aWidth, EDA_COLOR_T aColor )
{
    GRLine( aClipBox, aDC, aStart.x, aStart.y, aEnd.x, aEnd.y, aWidth, aColor );
}


void GRDashedLine( EDA_RECT* ClipBox, wxDC*     DC,
                   int x1, int y1, int x2, int  y2,
                   int       width, EDA_COLOR_T Color )
{
    GRLastMoveToX  = x2;
    GRLastMoveToY  = y2;
    s_DC_lastcolor = UNSPECIFIED_COLOR;
    GRSetColorPen( DC, Color, width, wxPENSTYLE_SHORT_DASH );
    WinClipAndDrawLine( ClipBox, DC, x1, y1, x2, y2, width );
    s_DC_lastcolor = UNSPECIFIED_COLOR;
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
void GRLineTo( EDA_RECT* ClipBox, wxDC* DC, int x, int y, int width, EDA_COLOR_T Color )
{
    GRLine( ClipBox, DC, GRLastMoveToX, GRLastMoveToY, x, y, width, Color );
}


void GRMixedLine( EDA_RECT* ClipBox, wxDC* DC, int x1, int y1, int x2, int y2,
                  int width, EDA_COLOR_T Color )
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
                  int aWidth, EDA_COLOR_T aColor )
{
    GRSetColorPen( aDC, aColor, aWidth );

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
}

// Draw  the outline of a thick segment wih rounded ends
void GRCSegm( EDA_RECT* ClipBox, wxDC* DC, int x1, int y1, int x2, int y2,
              int width, int aPenSize, EDA_COLOR_T Color )
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


    if( width <= 2 )   /*  single line or 2 pixels */
    {
        GRSetColorPen( DC, Color, width );
        DC->DrawLine( x1, y1, x2, y2 );
        return;
    }

    GRSetBrush( DC, Color, NOT_FILLED );
    GRSetColorPen( DC, Color, aPenSize );

    int radius = (width + 1) >> 1;
    int dx = x2 - x1;
    int dy = y2 - y1;
    double angle = -ArcTangente( dy, dx );
    wxPoint start;
    wxPoint end;
    wxPoint org( x1, y1);
    int len = (int) hypot( dx, dy );

    // We know if the DC is mirrored, to draw arcs
    int slx = DC->DeviceToLogicalX( 1 ) - DC->DeviceToLogicalX( 0 );
    int sly = DC->DeviceToLogicalY( 1 ) - DC->DeviceToLogicalY( 0 );
    bool mirrored = (slx > 0 && sly < 0) || (slx < 0 && sly > 0);

    // first edge
    start.x = 0;
    start.y = radius;
    end.x = len;
    end.y = radius;
    RotatePoint( &start, angle);
    RotatePoint( &end, angle);

    start += org;
    end += org;

    DC->DrawLine( start, end );

    // first rounded end
    end.x = 0;
    end.y = -radius;
    RotatePoint( &end, angle);
    end += org;

    if( !mirrored )
        DC->DrawArc( end, start, org );
    else
        DC->DrawArc( start, end, org );


    // second edge
    start.x = len;
    start.y = -radius;
    RotatePoint( &start, angle);
    start += org;

    DC->DrawLine( start, end );

    // second rounded end
    end.x = len;
    end.y = radius;
    RotatePoint( &end, angle);
    end += org;

    if( !mirrored )
        DC->DrawArc( end.x, end.y, start.x, start.y, x2, y2 );
    else
        DC->DrawArc( start.x, start.y, end.x, end.y, x2, y2 );
}


void GRCSegm( EDA_RECT* ClipBox, wxDC* DC, int x1, int y1, int x2, int y2,
              int width, EDA_COLOR_T Color )
{
    GRCSegm( ClipBox, DC, x1, y1, x2, y2, width, 0, Color );
}


void GRCSegm( EDA_RECT* aClipBox, wxDC* aDC, wxPoint aStart, wxPoint aEnd,
              int aWidth, EDA_COLOR_T aColor )
{
    GRCSegm( aClipBox, aDC, aStart.x, aStart.y, aEnd.x, aEnd.y, aWidth, 0, aColor );
}


/*
 * Draw segment (full) with rounded ends in object space (real coords.).
 */
void GRFillCSegm( EDA_RECT* ClipBox, wxDC* DC, int x1, int y1, int x2, int y2,
                  int width, EDA_COLOR_T Color )
{
    GRSetColorPen( DC, Color, width );
    WinClipAndDrawLine( ClipBox, DC, x1, y1, x2, y2, width );
}


void GRFilledSegment( EDA_RECT* aClipBox, wxDC* aDC, wxPoint aStart, wxPoint aEnd,
                      int aWidth, EDA_COLOR_T aColor )
{
    GRSetColorPen( aDC, aColor, aWidth );
    WinClipAndDrawLine( aClipBox, aDC, aStart.x, aStart.y, aEnd.x, aEnd.y, aWidth );
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
        Xmin = std::min( Xmin, Points[ii].x );
        Xmax = std::max( Xmax, Points[ii].x );
        Ymin = std::min( Ymin, Points[ii].y );
        Ymax = std::max( Ymax, Points[ii].y );
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
static void GRSPoly( EDA_RECT* ClipBox, wxDC* DC, int n, wxPoint Points[],
                     bool      Fill, int width,
                     EDA_COLOR_T Color, EDA_COLOR_T BgColor )
{
    if( !IsGRSPolyDrawable( ClipBox, n, Points ) )
        return;

    if( Fill && ( n > 2 ) )
    {
        GRSetBrush( DC, BgColor, FILLED );
        GRSetColorPen( DC, Color, width );

        /* clip before send the filled polygon to wxDC, because under linux
         * (GTK?) polygons having large coordinates are incorrectly drawn
         * (integer overflow in coordinates, I am guessing)
         */
        ClipAndDrawPoly( ClipBox, DC, Points, n );
    }
    else
    {
        GRMoveTo( Points[0].x, Points[0].y );
        for( int i = 1; i < n; ++i )
        {
            GRLineTo( ClipBox, DC, Points[i].x, Points[i].y, width, Color );
        }
    }
}


/*
 * Draw a new closed polyline and fill it if Fill, in screen space.
 */
static void GRSClosedPoly( EDA_RECT* aClipBox, wxDC* aDC,
                           int       aPointCount, wxPoint aPoints[],
                           bool      aFill, int aWidth,
                           EDA_COLOR_T       aColor,
                           EDA_COLOR_T       aBgColor )
{
    if( !IsGRSPolyDrawable( aClipBox, aPointCount, aPoints ) )
        return;

    if( aFill && ( aPointCount > 2 ) )
    {
        GRLastMoveToX = aPoints[aPointCount - 1].x;
        GRLastMoveToY = aPoints[aPointCount - 1].y;
        GRSetBrush( aDC, aBgColor, FILLED );
        GRSetColorPen( aDC, aColor, aWidth );
        ClipAndDrawPoly( aClipBox, aDC, aPoints, aPointCount );
    }
    else
    {
        GRMoveTo( aPoints[0].x, aPoints[0].y );
        for( int i = 1; i < aPointCount; ++i )
        {
            GRLineTo( aClipBox, aDC, aPoints[i].x, aPoints[i].y, aWidth, aColor );
        }

        int lastpt = aPointCount - 1;

        // Close the polygon
        if( aPoints[lastpt] != aPoints[0] )
        {
            GRLineTo( aClipBox, aDC, aPoints[0].x, aPoints[0].y, aWidth, aColor );
        }
    }
}


/*
 * Draw a new polyline and fill it if Fill, in drawing space.
 */
void GRPoly( EDA_RECT* ClipBox, wxDC* DC, int n, wxPoint Points[],
             bool Fill, int width, EDA_COLOR_T Color, EDA_COLOR_T BgColor )
{
    GRSPoly( ClipBox, DC, n, Points, Fill, width, Color, BgColor );
}


/*
 * Draw a closed polyline and fill it if Fill, in object space.
 */
void GRClosedPoly( EDA_RECT* ClipBox, wxDC* DC, int n, wxPoint Points[],
                   bool Fill, EDA_COLOR_T Color, EDA_COLOR_T BgColor )
{
    GRClosedPoly( ClipBox, DC, n, Points, Fill, 0, Color, BgColor );
}


void GRClosedPoly( EDA_RECT* ClipBox, wxDC* DC, int n, wxPoint Points[],
                   bool Fill, int width, EDA_COLOR_T Color, EDA_COLOR_T BgColor )
{
    GRSClosedPoly( ClipBox, DC, n, Points, Fill, width, Color, BgColor );
}


void GRCircle( EDA_RECT* ClipBox, wxDC* DC, int xc, int yc, int r, int width, EDA_COLOR_T Color )
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

    GRSetBrush( DC, Color, NOT_FILLED );
    GRSetColorPen( DC, Color, width );
    DC->DrawEllipse( xc - r, yc - r, r + r, r + r );
}


void GRCircle( EDA_RECT* ClipBox, wxDC* DC, int x, int y, int r, EDA_COLOR_T Color )
{
    GRCircle( ClipBox, DC, x, y, r, 0, Color );
}


void GRCircle( EDA_RECT* aClipBox, wxDC* aDC, wxPoint aPos, int aRadius, int aWidth, EDA_COLOR_T aColor )
{
    GRCircle( aClipBox, aDC, aPos.x, aPos.y, aRadius, aWidth, aColor );
}


void GRFilledCircle( EDA_RECT* ClipBox, wxDC* DC, int x, int y, int r,
                     int width, EDA_COLOR_T Color, EDA_COLOR_T BgColor )
{
    /* Clip circles off screen. */
    if( ClipBox )
    {
        int x0, y0, xm, ym;
        x0 = ClipBox->GetX();
        y0 = ClipBox->GetY();
        xm = ClipBox->GetRight();
        ym = ClipBox->GetBottom();

        if( x < ( x0 - r ) )
            return;

        if( y < ( y0 - r ) )
            return;

        if( x > ( r + xm ) )
            return;

        if( y > ( r + ym ) )
            return;
    }

    GRSetBrush( DC, BgColor, FILLED );
    GRSetColorPen( DC, Color, width );
    DC->DrawEllipse( x - r, y - r, r + r, r + r );
}


void GRFilledCircle( EDA_RECT* aClipBox, wxDC* aDC, wxPoint aPos, int aRadius, EDA_COLOR_T aColor )
{
    GRFilledCircle( aClipBox, aDC, aPos.x, aPos.y, aRadius, 0, aColor, aColor );
}


/*
 * Draw an arc in user space.
 */
void GRArc1( EDA_RECT* ClipBox, wxDC* DC, int x1, int y1, int x2, int y2,
             int xc, int yc, EDA_COLOR_T Color )
{
    GRArc1( ClipBox, DC, x1, y1, x2, y2, xc, yc, 0, Color );
}


/*
 * Draw an arc, width = width in user space.
 */
void GRArc1( EDA_RECT* ClipBox, wxDC* DC, int x1, int y1, int x2, int y2,
             int xc, int yc, int width, EDA_COLOR_T Color )
{
    /* Clip arcs off screen. */
    if( ClipBox )
    {
        int x0, y0, xm, ym, r;
        x0 = ClipBox->GetX();
        y0 = ClipBox->GetY();
        xm = ClipBox->GetRight();
        ym = ClipBox->GetBottom();
        r  = KiROUND( Distance( x1, y1, xc, yc ) );
        if( xc < ( x0 - r ) )
            return;
        if( yc < ( y0 - r ) )
            return;
        if( xc > ( r + xm ) )
            return;
        if( yc > ( r + ym ) )
            return;
    }

    GRSetBrush( DC, Color );
    GRSetColorPen( DC, Color, width );
    DC->DrawArc( x1, y1, x2, y2, xc, yc );
}


void GRArc1( EDA_RECT* aClipBox, wxDC* aDC, wxPoint aStart, wxPoint aEnd,
             wxPoint aCenter, int aWidth, EDA_COLOR_T aColor )
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
                  double    StAngle,
                  double    EndAngle,
                  int       r,
                  int       width,
                  EDA_COLOR_T       Color,
                  EDA_COLOR_T       BgColor )
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
                  double StAngle, double EndAngle, int r,
                  EDA_COLOR_T Color, EDA_COLOR_T BgColor )
{
    GRFilledArc( ClipBox, DC, x, y, StAngle, EndAngle, r, 0, Color, BgColor );
}


/*
 * Draw an arc in drawing space.
 */
void GRArc( EDA_RECT* ClipBox, wxDC* DC, int xc, int yc, double StAngle,
            double EndAngle, int r, EDA_COLOR_T Color )
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

    GRSetBrush( DC, Color, NOT_FILLED );
    GRSetColorPen( DC, Color );
    DC->DrawArc( xc + x1, yc - y1, xc + x2, yc - y2, xc, yc );
}


/*
 * Draw an arc with width = width in drawing space.
 */
void GRArc( EDA_RECT* ClipBox,
            wxDC*     DC,
            int       x,
            int       y,
            double    StAngle,
            double    EndAngle,
            int       r,
            int       width,
            EDA_COLOR_T       Color )
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

    GRSetBrush( DC, Color );
    GRSetColorPen( DC, Color, width );
    DC->DrawArc( x + x1, y - y1, x + x2, y - y2, x, y );
}


/*
 * Draw a rectangle in drawing space.
 */
void GRRect( EDA_RECT* aClipBox, wxDC* aDC, int x1, int y1, int x2, int y2, EDA_COLOR_T aColor )
{
    GRSRect( aClipBox, aDC, x1, y1, x2, y2, 0, aColor );
}


void GRRectPs( EDA_RECT* aClipBox, wxDC* aDC, const EDA_RECT& aRect, EDA_COLOR_T aColor, wxPenStyle aStyle )
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
void GRRect( EDA_RECT* ClipBox, wxDC* DC, int x1, int y1, int x2, int y2, int width, EDA_COLOR_T Color )
{
    GRSRect( ClipBox, DC, x1, y1, x2, y2, width, Color );
}


void GRRect( EDA_RECT* aClipBox, wxDC* aDC, const EDA_RECT& aRect, int aWidth, EDA_COLOR_T aColor )
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
                   EDA_COLOR_T Color, EDA_COLOR_T BgColor )
{
    GRSFilledRect( ClipBox, DC, x1, y1, x2, y2, 0, Color, BgColor );
}


/*
 * Draw a rectangle (filled with AreaColor) in drawing space.
 */
void GRFilledRect( EDA_RECT* ClipBox, wxDC* DC, int x1, int y1, int x2, int y2,
                   int width, EDA_COLOR_T Color, EDA_COLOR_T BgColor )
{
    GRSFilledRect( ClipBox, DC, x1, y1, x2, y2, width, Color, BgColor );
}


/*
 * Draw a rectangle in screen space.
 */

void GRSRect( EDA_RECT* aClipBox, wxDC* aDC, int x1, int y1, int x2, int y2,
              int aWidth, EDA_COLOR_T aColor, wxPenStyle aStyle )
{
    wxPoint points[5];
    points[0] = wxPoint(x1, y1);
    points[1] = wxPoint(x1, y2);
    points[2] = wxPoint(x2, y2);
    points[3] = wxPoint(x2, y1);
    points[4] = points[0];
    GRSClosedPoly( aClipBox, aDC, 5, points, NOT_FILLED, aWidth,
                           aColor, aColor );
}


void GRSFilledRect( EDA_RECT* aClipBox, wxDC* aDC, int x1, int y1, int x2, int y2,
                    int aWidth, EDA_COLOR_T aColor, EDA_COLOR_T aBgColor )
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
        ClipAndDrawPoly(&clipbox, aDC, points, 5); // polygon approach is more accurate
    }
    else
        ClipAndDrawPoly(aClipBox, aDC, points, 5 );
}

/**
 * Function ClipAndDrawPoly
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

void ClipAndDrawPoly( EDA_RECT* aClipBox, wxDC* aDC, wxPoint aPoints[], int n )
{
    if( aClipBox == NULL )
    {
        aDC->DrawPolygon( n, aPoints );
        return;
    }

    // A clip box exists: clip and draw the polygon.
    static std::vector<wxPoint> clippedPolygon;
    static pointVector inputPolygon, outputPolygon;

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
        clippedPolygon.push_back( wxPoint( KiROUND( cit->X ), KiROUND( cit->Y ) ) );
    }

    if( clippedPolygon.size() )
        aDC->DrawPolygon( clippedPolygon.size(), &clippedPolygon[0] );
}


void GRBezier( EDA_RECT* ClipBox,
               wxDC*     DC,
               int       x1,
               int       y1,
               int       x2,
               int       y2,
               int       x3,
               int       y3,
               int       width,
               EDA_COLOR_T       Color )
{
    std::vector<wxPoint> Points = Bezier2Poly( x1, y1, x2, y2, x3, y3 );
    GRPoly( ClipBox, DC, Points.size(), &Points[0], false, width, Color, Color );
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
               EDA_COLOR_T       Color )
{
    std::vector<wxPoint> Points = Bezier2Poly( x1, y1, x2, y2, x3, y3, x4, y4 );
    GRPoly( ClipBox, DC, Points.size(), &Points[0], false, width, Color, Color );
}


void GRDrawAnchor( EDA_RECT *aClipBox, wxDC *aDC, int x, int y,
                   int aSize, EDA_COLOR_T aColor )
{
        int anchor_size = aDC->DeviceToLogicalXRel( aSize );

        GRLine( aClipBox, aDC,
                x - anchor_size, y,
                x + anchor_size, y, 0, aColor );
        GRLine( aClipBox, aDC,
                x, y - anchor_size,
                x, y + anchor_size, 0, aColor );
}
