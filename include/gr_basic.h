/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2015 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file gr_basic.h
 */

#ifndef GR_BASIC
#define GR_BASIC

#include <gal/color4d.h>
#include <vector>
class EDA_RECT;

using KIGFX::COLOR4D;


/// Drawmode. Compositing mode plus a flag or two
enum GR_DRAWMODE {
    GR_OR                 = 0x01000000,
    GR_XOR                = 0x02000000,
    GR_AND                = 0x04000000,
    GR_NXOR               = 0x08000000,
    GR_INVERT             = 0x10000000,
    GR_ALLOW_HIGHCONTRAST = 0x20000000,
    GR_COPY               = 0x40000000,
    GR_HIGHLIGHT          = 0x80000000,
    UNSPECIFIED_DRAWMODE  = -1
};

inline void DrawModeAddHighlight(GR_DRAWMODE *mode)
{
    *mode = static_cast<GR_DRAWMODE>( int( *mode ) | GR_HIGHLIGHT );
}

inline void DrawModeAllowHighContrast(GR_DRAWMODE *mode)
{
    *mode = static_cast<GR_DRAWMODE>( int( *mode ) | GR_ALLOW_HIGHCONTRAST );
}

inline GR_DRAWMODE operator ~(const GR_DRAWMODE& a)
{
    return static_cast<GR_DRAWMODE>( ~int( a ) );
}

inline GR_DRAWMODE operator |(const GR_DRAWMODE& a, const GR_DRAWMODE& b)
{
    return static_cast<GR_DRAWMODE>( int( a ) | int( b ) );
}

inline GR_DRAWMODE operator &(const GR_DRAWMODE& a, const GR_DRAWMODE& b)
{
    return static_cast<GR_DRAWMODE>( int( a ) & int( b ) );
}

#define GR_M_LEFT_DOWN   0x10000000
#define GR_M_RIGHT_DOWN  0x20000000
#define GR_M_MIDDLE_DOWN 0x40000000
#define GR_M_DCLICK      0x80000000


extern GR_DRAWMODE g_XorMode;

typedef enum {
    /* Line styles for Get/SetLineStyle. */
    GR_SOLID_LINE  = 0,
    GR_DOTTED_LINE = 1,
    GR_DASHED_LINE = 3
} GRLineStypeType;


class EDA_DRAW_PANEL;

void GRSetDrawMode( wxDC* DC, GR_DRAWMODE mode );
GR_DRAWMODE  GRGetDrawMode( wxDC* DC );
void GRResetPenAndBrush( wxDC* DC );
void GRSetColorPen( wxDC* DC, COLOR4D Color, int width = 1, wxPenStyle stype = wxPENSTYLE_SOLID );
void GRSetBrush( wxDC* DC, COLOR4D Color, bool fill = false );

/**
 * Function GRForceBlackPen
 * @param flagforce True to force a black pen whenever the asked color
 */
void GRForceBlackPen( bool flagforce );

/**
 * Function GetGRForceBlackPenState
 * @return ForceBlackPen (True if a black pen was forced)
 */
bool GetGRForceBlackPenState( void );

void GRLine( EDA_RECT* aClipBox, wxDC* aDC,
             wxPoint aStart, wxPoint aEnd, int aWidth, COLOR4D aColor );
void GRLine( EDA_RECT* ClipBox, wxDC* DC,
             int x1, int y1, int x2, int y2, int width, COLOR4D Color );
void GRMixedLine( EDA_RECT* ClipBox, wxDC* DC, int x1, int y1, int x2, int y2,
                  int width, COLOR4D Color );
void GRDashedLine( EDA_RECT* ClipBox, wxDC* DC, int x1, int y1, int x2, int  y2,
                   int width, COLOR4D Color );
void GRMoveTo( int x, int y );
void GRLineTo( EDA_RECT* ClipBox, wxDC* DC,
               int x, int y, int width, COLOR4D Color );

void GRPoly( EDA_RECT* ClipBox, wxDC* DC, int n, wxPoint Points[], bool Fill,
             int width, COLOR4D Color, COLOR4D BgColor );

void GRBezier( EDA_RECT* ClipBox, wxDC* DC, int x1, int y1, int x2, int y2,
               int x3, int y3, int width, COLOR4D Color );
void GRBezier( EDA_RECT* ClipBox, wxDC* DC, int x1, int y1, int x2, int y2,
               int x3, int y3, int x4, int y4, int width, COLOR4D Color );

/**
 * Function GRClosedPoly
 * draws a closed polygon onto the drawing context \a aDC and optionally fills
 * and/or draws a border around it.
 * @param ClipBox defines a rectangular boundary outside of which no drawing will occur.
 * @param aDC the device context into which drawing should occur.
 * @param aPointCount the number of points in the array \a aPoints.
 * @param aPoints The points to draw.
 * @param doFill true if polygon is to be filled, else false and only the boundary is drawn.
 * @param aPenColor the color of the border.
 * @param aFillColor the fill color of the polygon's interior.
 */
void GRClosedPoly( EDA_RECT* ClipBox,
                   wxDC  *   aDC,
                   int       aPointCount,
                   wxPoint   aPoints[],
                   bool      doFill,
                   COLOR4D   aPenColor,
                   COLOR4D   aFillColor );

// @todo could make these 2 closed polygons calls a single function and default
// the aPenWidth argument

/**
 * Function GRClosedPoly
 * draws a closed polygon onto the drawing context \a aDC and optionally fills
 * and/or draws a border around it.
 * @param ClipBox defines a rectangular boundary outside of which no drawing will occur.
 * @param aDC the device context into which drawing should occur.
 * @param aPointCount the number of points in the array \a aPointArray.
 * @param aPoints the points to draw.
 * @param doFill true if polygon is to be filled, else false and only the boundary is drawn.
 * @param aPenWidth is the width of the pen to use on the perimeter, can be zero.
 * @param aPenColor the color of the border.
 * @param aFillColor the fill color of the polygon's interior.
 */
void GRClosedPoly( EDA_RECT* ClipBox,
                   wxDC*     aDC,
                   int       aPointCount,
                   wxPoint   aPoints[],
                   bool      doFill,
                   int       aPenWidth,
                   COLOR4D   aPenColor,
                   COLOR4D   aFillColor );


/**
 * Function GRCircle
 * draws a circle onto the drawing context \a aDC centered at the user
 * coordinates (x,y)
 *
 * @param ClipBox defines a rectangular boundary outside of which no drawing will occur.
 * @param aDC the device context into which drawing should occur.
 * @param x The x coordinate in user space of the center of the circle.
 * @param y The y coordinate in user space of the center of the circle.
 * @param aRadius is the radius of the circle.
 * @param aColor is the color to draw
 * @see COLOR4D
 */
void GRCircle( EDA_RECT* ClipBox, wxDC* aDC, int x, int y, int aRadius, COLOR4D aColor );
void GRCircle( EDA_RECT* ClipBox, wxDC* DC, int x, int y, int r, int  width, COLOR4D Color );
void GRFilledCircle( EDA_RECT* ClipBox, wxDC* DC, int x, int y, int r, int width,
                     COLOR4D Color, COLOR4D BgColor );
void GRFilledCircle( EDA_RECT* aClipBox, wxDC* aDC, wxPoint aPos, int aRadius, COLOR4D aColor );
void GRCircle( EDA_RECT* aClipBox, wxDC* aDC, wxPoint aPos, int aRadius, int aWidth, COLOR4D aColor );

void GRArc( EDA_RECT* ClipBox, wxDC* DC, int x, int y, double StAngle,
            double EndAngle, int r, COLOR4D Color );
void GRArc( EDA_RECT* ClipBox, wxDC* DC, int x, int y, double StAngle,
            double EndAngle, int r, int width, COLOR4D Color );
void GRArc1( EDA_RECT* ClipBox, wxDC* DC, int x1, int y1, int x2, int y2,
             int xc, int yc, COLOR4D Color );
void GRArc1( EDA_RECT* ClipBox, wxDC* DC, int x1, int y1, int x2, int y2,
             int xc, int yc, int width, COLOR4D Color );
void GRArc1( EDA_RECT* aClipBox, wxDC* aDC, wxPoint aStart, wxPoint aEnd,
             wxPoint aCenter, int aWidth, COLOR4D aColor );
void GRFilledArc( EDA_RECT* ClipBox, wxDC* DC, int x, int y,
                  double StAngle, double EndAngle, int r, COLOR4D Color, COLOR4D BgColor );
void GRFilledArc( EDA_RECT* ClipBox, wxDC* DC, int x, int y, double StAngle,
                  double EndAngle, int r, int width, COLOR4D Color, COLOR4D BgColor );
void GRCSegm( EDA_RECT* ClipBox, wxDC* DC, int x1, int y1, int x2, int y2, int width, COLOR4D Color );

void GRFillCSegm( EDA_RECT* ClipBox, wxDC* DC, int x1, int y1, int x2, int y2,
                  int width, COLOR4D Color );
void GRFilledSegment( EDA_RECT* aClipBox, wxDC* aDC, wxPoint aStart, wxPoint aEnd,
                      int aWidth, COLOR4D aColor );

void GRCSegm( EDA_RECT* ClipBox, wxDC* DC, int x1, int y1, int x2, int y2,
              int width, int aPenSize, COLOR4D Color );
void GRCSegm( EDA_RECT* aClipBox, wxDC* aDC, wxPoint aStart, wxPoint aEnd,
              int aWidth, COLOR4D aColor );

void GRSetColor( COLOR4D Color );
void GRSetDefaultPalette();
COLOR4D  GRGetColor();
void GRPutPixel( EDA_RECT* ClipBox, wxDC* DC, int x, int y, COLOR4D color );
void GRFilledRect( EDA_RECT* ClipBox, wxDC* DC, int x1, int y1,
                   int x2, int y2, COLOR4D Color, COLOR4D BgColor );
void GRFilledRect( EDA_RECT* ClipBox, wxDC* DC, int x1, int y1,
                   int x2, int y2, int width, COLOR4D Color, COLOR4D BgColor );
void GRRect( EDA_RECT* ClipBox, wxDC* DC, int x1, int y1, int x2, int y2, COLOR4D Color );
void GRRect( EDA_RECT* ClipBox, wxDC* DC,const EDA_RECT& aRect, int aWidth, COLOR4D Color );
void GRRect( EDA_RECT* ClipBox, wxDC* DC, int x1, int y1,
             int x2, int y2, int width, COLOR4D Color );
void GRRectPs( EDA_RECT* aClipBox, wxDC* aDC,const EDA_RECT& aRect,
               int aWidth, COLOR4D aColor, wxPenStyle aStyle = wxPENSTYLE_SOLID );

void GRSFilledRect( EDA_RECT* ClipBox, wxDC* DC, int x1, int y1,
                    int x2, int y2, int width, COLOR4D Color, COLOR4D BgColor );

/**
 * Function GRLineArray
 * draws an array of lines (not a polygon).
 * @param aClipBox = the clip box
 * @param aDC = the device context into which drawing should occur.
 * @param aLines = a list of pair of coordinate in user space: a pair for each line.
 * @param aWidth = the width of each line.
 * @param aColor = the color of the lines
 * @see COLOR4D
 */
void GRLineArray(  EDA_RECT* aClipBox, wxDC* aDC,std::vector<wxPoint>& aLines,
                   int aWidth, COLOR4D aColor );

void GRDrawAnchor( EDA_RECT* aClipBox, wxDC *aDC, int x, int y, int aSize,
                   COLOR4D aColor );

#endif      /* define GR_BASIC */
