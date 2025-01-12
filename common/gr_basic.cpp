/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <gr_basic.h>
#include <trigo.h>
#include <eda_item.h>
#include <wx/graphics.h>
#include <math/vector2wx.h>

#include <algorithm>

static const bool FILLED = true;
static const bool NOT_FILLED = false;

// For draw mode = XOR GR_XOR or GR_NXOR by background color
GR_DRAWMODE g_XorMode = GR_NXOR;


/* These functions are used by corresponding functions
 * ( GRSCircle is called by GRCircle for instance) after mapping coordinates
 * from user units to screen units(pixels coordinates)
 */
static void GRSRect( wxDC* aDC, int x1, int y1, int x2, int y2, int aWidth, const COLOR4D& aColor );

/**/

static int     GRLastMoveToX, GRLastMoveToY;
static bool    s_ForceBlackPen;   /* if true: draws in black instead of
                                        * color for printing. */
static COLOR4D s_DC_lastbrushcolor( 0, 0, 0, 0 );
static bool    s_DC_lastbrushfill  = false;
static wxDC*   s_DC_lastDC = nullptr;


static void vector2IwxDrawPolygon( wxDC* aDC, const VECTOR2I* Points, int n )
{
    wxPoint* points = new wxPoint[n];

    for( int i = 0; i < n; i++ )
        points[i] = wxPoint( Points[i].x, Points[i].y );

    aDC->DrawPolygon( n, points );
    delete[] points;
}


static void winDrawLine( wxDC* DC, int x1, int y1, int x2, int y2, int width )
{
    GRLastMoveToX = x2;
    GRLastMoveToY = y2;
    DC->DrawLine( x1, y1, x2, y2 );
}


void GRResetPenAndBrush( wxDC* DC )
{
    GRSetBrush( DC, BLACK );  // Force no fill
    s_DC_lastbrushcolor = COLOR4D::UNSPECIFIED;
    s_DC_lastDC    = nullptr;
}


void GRSetColorPen( wxDC* DC, const COLOR4D& Color, int width, wxPenStyle style )
{
    COLOR4D color = Color;

    wxDash dots[2] = { 1, 3 };

    // Under OSX and while printing when wxPen is set to 0, renderer follows the request drawing
    // nothing & in the bitmap world the minimum is enough to light a pixel, in vectorial one not
    if( width <= 1 && DC->GetBrush().GetStyle() != wxBRUSHSTYLE_SOLID )
        width = DC->DeviceToLogicalXRel( 1 );

    if( s_ForceBlackPen )
        color = COLOR4D::BLACK;

    // wxWidgets will enforce a minimum pen width when printing, so we have to make the pen
    // transparent when we don't want the object stroked.
    if( width == 0 )
    {
        color = COLOR4D::UNSPECIFIED;
        style = wxPENSTYLE_TRANSPARENT;
    }

    const wxPen& curr_pen = DC->GetPen();

    if( !curr_pen.IsOk() || curr_pen.GetColour() != color.ToColour()
       || curr_pen.GetWidth() != width || curr_pen.GetStyle() != style )
    {
        wxPen pen;
        pen.SetColour( color.ToColour() );

        if( style == wxPENSTYLE_DOT )
        {
            style = wxPENSTYLE_USER_DASH;
            pen.SetDashes( 2, dots );
        }

        pen.SetWidth( width );
        pen.SetStyle( style );
        DC->SetPen( pen );
    }
    else
    {
        // Should be not needed, but on Linux, in printing process
        // the curr pen settings needs to be sometimes re-initialized
        // Clearly, this is due to a bug, related to SetBrush(),
        // but we have to live with it, at least on wxWidgets 3.0
        DC->SetPen( curr_pen );
    }
}


void GRSetBrush( wxDC* DC, const COLOR4D& Color, bool fill )
{
    COLOR4D color = Color;

    if( s_ForceBlackPen )
        color = COLOR4D::BLACK;

    if( s_DC_lastbrushcolor != color || s_DC_lastbrushfill  != fill || s_DC_lastDC != DC )
    {
        wxBrush brush;

        brush.SetColour( color.ToColour() );

        if( fill )
            brush.SetStyle( wxBRUSHSTYLE_SOLID );
        else
            brush.SetStyle( wxBRUSHSTYLE_TRANSPARENT );

        DC->SetBrush( brush );

        s_DC_lastbrushcolor = color;
        s_DC_lastbrushfill  = fill;
        s_DC_lastDC = DC;
    }
}


void GRForceBlackPen( bool flagforce )
{
    s_ForceBlackPen = flagforce;
}


bool GetGRForceBlackPenState( void )
{
    return s_ForceBlackPen;
}


void GRLine( wxDC* DC, int x1, int y1, int x2, int y2, int width, const COLOR4D& Color,
             wxPenStyle aStyle)
{
    GRSetColorPen( DC, Color, width, aStyle );
    winDrawLine( DC, x1, y1, x2, y2, width );
    GRLastMoveToX = x2;
    GRLastMoveToY = y2;
}


void GRLine( wxDC* aDC, const VECTOR2I& aStart, const VECTOR2I& aEnd, int aWidth,
             const COLOR4D& aColor, wxPenStyle aStyle )
{
    GRLine( aDC, aStart.x, aStart.y, aEnd.x, aEnd.y, aWidth, aColor, aStyle );
}


void GRMoveTo( int x, int y )
{
    GRLastMoveToX = x;
    GRLastMoveToY = y;
}


void GRLineTo( wxDC* DC, int x, int y, int width, const COLOR4D& Color )
{
    GRLine( DC, GRLastMoveToX, GRLastMoveToY, x, y, width, Color );
}


void GRCSegm( wxDC* DC, const VECTOR2I& A, const VECTOR2I& B, int width, const COLOR4D& Color )
{
    GRLastMoveToX = B.x;
    GRLastMoveToY = B.y;

    if( width <= 2 )   /*  single line or 2 pixels */
    {
        GRSetColorPen( DC, Color, width );
        DC->DrawLine( A.x, A.y, B.x, B.y );
        return;
    }

    GRSetBrush( DC, Color, NOT_FILLED );
    GRSetColorPen( DC, Color, 0 );

    int radius = ( width + 1 ) >> 1;
    int dx = B.x - A.x;
    int dy = B.y - A.y;
    EDA_ANGLE angle( VECTOR2I( dx, dy ) );

    angle = -angle;

    VECTOR2I start;
    VECTOR2I end;
    VECTOR2I org( A.x, A.y );
    int len = (int) hypot( dx, dy );

    // We know if the DC is mirrored, to draw arcs
    int slx = DC->DeviceToLogicalX( 1 ) - DC->DeviceToLogicalX( 0 );
    int sly = DC->DeviceToLogicalY( 1 ) - DC->DeviceToLogicalY( 0 );
    bool mirrored = ( slx > 0 && sly < 0 ) || ( slx < 0 && sly > 0 );

    // first edge
    start.x = 0;
    start.y = radius;
    end.x = len;
    end.y = radius;
    RotatePoint( start, angle );
    RotatePoint( end, angle );

    start += org;
    end += org;

    DC->DrawLine( ToWxPoint( start ), ToWxPoint( end ) );

    // first rounded end
    end.x = 0;
    end.y = -radius;
    RotatePoint( end, angle );
    end += org;

    if( !mirrored )
        DC->DrawArc( ToWxPoint(end ), ToWxPoint(start ), ToWxPoint(org ) );
    else
        DC->DrawArc( ToWxPoint(start ), ToWxPoint(end ), ToWxPoint(org ) );

    // second edge
    start.x = len;
    start.y = -radius;
    RotatePoint( start, angle );
    start += org;

    DC->DrawLine( ToWxPoint( start ), ToWxPoint( end ) );

    // second rounded end
    end.x = len;
    end.y = radius;
    RotatePoint( end, angle);
    end += org;

    if( !mirrored )
        DC->DrawArc( end.x, end.y, start.x, start.y, B.x, B.y );
    else
        DC->DrawArc( start.x, start.y, end.x, end.y, B.x, B.y );
}


void GRFilledSegment( wxDC* aDC, const VECTOR2I& aStart, const VECTOR2I& aEnd, int aWidth,
                      const COLOR4D& aColor )
{
    GRSetColorPen( aDC, aColor, aWidth );
    winDrawLine( aDC, aStart.x, aStart.y, aEnd.x, aEnd.y, aWidth );
}


/**
 * Draw a new polyline and fill it if Fill, in screen space.
 */
static void GRSPoly( wxDC* DC, int n, const VECTOR2I* Points, bool Fill, int width,
                     const COLOR4D& Color, const COLOR4D& BgColor )
{
    if( Fill && ( n > 2 ) )
    {
        GRSetBrush( DC, BgColor, FILLED );
        GRSetColorPen( DC, Color, width );

        vector2IwxDrawPolygon( DC, Points, n );
    }
    else
    {
        GRMoveTo( Points[0].x, Points[0].y );

        for( int i = 1; i < n; ++i )
            GRLineTo( DC, Points[i].x, Points[i].y, width, Color );
    }
}


/**
 * Draw a new closed polyline and fill it if Fill, in screen space.
 */
static void GRSClosedPoly( wxDC* aDC, int aPointCount, const VECTOR2I* aPoints, bool aFill,
                           int aWidth, const COLOR4D& aColor, const COLOR4D& aBgColor )
{
    if( aFill && ( aPointCount > 2 ) )
    {
        GRLastMoveToX = aPoints[aPointCount - 1].x;
        GRLastMoveToY = aPoints[aPointCount - 1].y;
        GRSetBrush( aDC, aBgColor, FILLED );
        GRSetColorPen( aDC, aColor, aWidth );
        vector2IwxDrawPolygon( aDC, aPoints, aPointCount );
    }
    else
    {
        GRMoveTo( aPoints[0].x, aPoints[0].y );

        for( int i = 1; i < aPointCount; ++i )
            GRLineTo( aDC, aPoints[i].x, aPoints[i].y, aWidth, aColor );

        int lastpt = aPointCount - 1;

        // Close the polygon
        if( aPoints[lastpt] != aPoints[0] )
            GRLineTo( aDC, aPoints[0].x, aPoints[0].y, aWidth, aColor );
    }
}


/**
 * Draw a new polyline and fill it if Fill, in drawing space.
 */
void GRPoly( wxDC* DC, int n, const VECTOR2I* Points, bool Fill, int width, const COLOR4D& Color,
             const COLOR4D& BgColor )
{
    GRSPoly( DC, n, Points, Fill, width, Color, BgColor );
}


/**
 * Draw a closed polyline and fill it if Fill, in object space.
 */
void GRClosedPoly( wxDC* DC, int n, const VECTOR2I* Points, bool Fill, const COLOR4D& Color )
{
    GRSClosedPoly( DC, n, Points, Fill, 0, Color, Color );
}


void GRCircle( wxDC* aDC, const VECTOR2I& aPos, int aRadius, int aWidth, const COLOR4D& aColor )
{
    GRSetBrush( aDC, aColor, NOT_FILLED );
    GRSetColorPen( aDC, aColor, aWidth );

    // Draw two arcs here to make a circle.  Unfortunately, the printerDC doesn't handle
    // transparent brushes when used with circles.  It does work for for arcs, however
    aDC->DrawArc(aPos.x + aRadius, aPos.y, aPos.x - aRadius, aPos.y, aPos.x, aPos.y );
    aDC->DrawArc(aPos.x - aRadius, aPos.y, aPos.x + aRadius, aPos.y, aPos.x, aPos.y );
}


void GRFilledCircle( wxDC* aDC, const VECTOR2I& aPos, int aRadius, int aWidth,
                     const COLOR4D& aStrokeColor, const COLOR4D& aFillColor )
{
    GRSetBrush( aDC, aFillColor, FILLED );
    GRSetColorPen( aDC, aStrokeColor, aWidth );
    aDC->DrawEllipse( aPos.x - aRadius, aPos.y - aRadius, 2 * aRadius, 2 * aRadius );
}


void GRArc( wxDC* aDC, const VECTOR2I& aStart, const VECTOR2I& aEnd, const VECTOR2I& aCenter,
            int aWidth, const COLOR4D& aColor )
{
    GRSetBrush( aDC, aColor );
    GRSetColorPen( aDC, aColor, aWidth );
    aDC->DrawArc( aStart.x, aStart.y, aEnd.x, aEnd.y, aCenter.x, aCenter.y );
}


void GRFilledArc( wxDC* DC, const VECTOR2I& aStart, const VECTOR2I& aEnd, const VECTOR2I& aCenter,
                  int width, const COLOR4D& Color, const COLOR4D& BgColor )
{
    GRSetBrush( DC, BgColor, FILLED );
    GRSetColorPen( DC, Color, width );
    DC->DrawArc( aStart.x, aStart.y, aEnd.x, aEnd.y, aCenter.x, aCenter.y );
}


void GRRect( wxDC* DC, const VECTOR2I& aStart, const VECTOR2I& aEnd, int aWidth,
             const COLOR4D& aColor )
{
    GRSRect( DC, aStart.x, aStart.y, aEnd.x, aEnd.y, aWidth, aColor );
}


void GRFilledRect( wxDC* DC, const VECTOR2I& aStart, const VECTOR2I& aEnd, int aWidth,
                   const COLOR4D& aColor, const COLOR4D& aBgColor )
{
    GRSFilledRect( DC, aStart.x, aStart.y, aEnd.x, aEnd.y, aWidth, aColor, aBgColor );
}


void GRSRect( wxDC* aDC, int x1, int y1, int x2, int y2, int aWidth, const COLOR4D& aColor )
{
    VECTOR2I points[5];
    points[0] = VECTOR2I( x1, y1 );
    points[1] = VECTOR2I( x1, y2 );
    points[2] = VECTOR2I( x2, y2 );
    points[3] = VECTOR2I( x2, y1 );
    points[4] = points[0];
    GRSClosedPoly( aDC, 5, points, NOT_FILLED, aWidth, aColor, aColor );
}


void GRSFilledRect( wxDC* aDC, int x1, int y1, int x2, int y2, int aWidth, const COLOR4D& aColor,
                    const COLOR4D& aBgColor )
{
    VECTOR2I points[5];
    points[0] = VECTOR2I( x1, y1 );
    points[1] = VECTOR2I( x1, y2 );
    points[2] = VECTOR2I( x2, y2 );
    points[3] = VECTOR2I( x2, y1 );
    points[4] = points[0];

    GRSetBrush( aDC, aBgColor, FILLED );
    GRSetColorPen( aDC, aBgColor, aWidth );

    vector2IwxDrawPolygon( aDC, points, 5 );
}
