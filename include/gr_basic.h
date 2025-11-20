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

#ifndef GR_BASIC
#define GR_BASIC

#include <gal/color4d.h>
#include <math/box2.h>
#include <vector>
#include <wx/pen.h>
#include <wx/dc.h>

using KIGFX::COLOR4D;


/// Drawmode. Compositing mode plus a flag or two
enum GR_DRAWMODE : unsigned int
{
    GR_OR                 = 0x01000000,
    GR_XOR                = 0x02000000,
    GR_AND                = 0x04000000,
    GR_NXOR               = 0x08000000,
    GR_INVERT             = 0x10000000,
    GR_ALLOW_HIGHCONTRAST = 0x20000000,
    GR_COPY               = 0x40000000,
    GR_HIGHLIGHT          = 0x80000000,
};

inline GR_DRAWMODE operator~( const GR_DRAWMODE& a )
{
    return static_cast<GR_DRAWMODE>( ~int( a ) );
}

inline GR_DRAWMODE operator|( const GR_DRAWMODE& a, const GR_DRAWMODE& b )
{
    return static_cast<GR_DRAWMODE>( int( a ) | int( b ) );
}

inline GR_DRAWMODE operator&( const GR_DRAWMODE& a, const GR_DRAWMODE& b )
{
    return static_cast<GR_DRAWMODE>( int( a ) & int( b ) );
}

extern GR_DRAWMODE g_XorMode;

typedef enum {
    /* Line styles for Get/SetLineStyle. */
    GR_SOLID_LINE  = 0,
    GR_DOTTED_LINE = 1,
    GR_DASHED_LINE = 3
} GRLineStypeType;


void GRResetPenAndBrush( wxDC* DC );
void GRSetColorPen( wxDC* DC, const COLOR4D& Color, int width = 1,
                    wxPenStyle stype = wxPENSTYLE_SOLID );
void GRSetBrush( wxDC* DC, const COLOR4D& Color, bool fill = false );

/**
 * @param flagforce True to force a black pen whenever the asked color.
 */
void GRForceBlackPen( bool flagforce );

/**
 * @return True if a black pen was forced or false if not forced.
 */
bool GetGRForceBlackPenState( void );

void GRLine( wxDC* aDC, const VECTOR2I& aStart, const VECTOR2I& aEnd, int aWidth,
             const COLOR4D& aColor, wxPenStyle aStyle = wxPENSTYLE_SOLID );
void GRLine( wxDC* DC, int x1, int y1, int x2, int y2, int width, const COLOR4D& Color,
             wxPenStyle aStyle = wxPENSTYLE_SOLID );
void GRMoveTo( int x, int y );
void GRLineTo( wxDC* DC, int x, int y, int width, const COLOR4D& Color );

void GRPoly( wxDC* DC, int n, const VECTOR2I* Points, bool Fill, int width, const COLOR4D& Color,
             const COLOR4D& BgColor );

/**
 * Draw a closed polygon onto the drawing context \a aDC and optionally fills and/or draws
 * a border around it.
 *
 * @param aDC the device context into which drawing should occur.
 * @param aPointCount the number of points in the array \a aPoints.
 * @param aPoints The points to draw.
 * @param doFill true if polygon is to be filled, else false and only the boundary is drawn.
 * @param aColor the color of the border and the fill.
 */
void GRClosedPoly( wxDC* aDC, int aPointCount, const VECTOR2I* aPoints, bool doFill,
                   const COLOR4D& aColor );

/**
 * Draw a circle onto the drawing context \a aDC centered at the user coordinates (x,y).
 *
 * @param aDC the device context into which drawing should occur.
 * @param x The x coordinate in user space of the center of the circle.
 * @param y The y coordinate in user space of the center of the circle.
 * @param aRadius is the radius of the circle.
 * @param aColor is the color to draw.
 * @see COLOR4D
 */
void GRFilledCircle( wxDC* aDC, const VECTOR2I& aPos, int aRadius, int aWidth,
                     const COLOR4D& aStrokeColor, const COLOR4D& aFillColor );
void GRCircle( wxDC* aDC, const VECTOR2I& aPos, int aRadius, int aWidth, const COLOR4D& aColor );

void GRArc( wxDC* aDC, const VECTOR2I& aStart, const VECTOR2I& aEnd, const VECTOR2I& aCenter,
            int aWidth, const COLOR4D& aColor );
void GRFilledArc( wxDC* DC, const VECTOR2I& aStart, const VECTOR2I& aEnd, const VECTOR2I& aCenter,
                  int width, const COLOR4D& Color, const COLOR4D& BgColor );

void GRFilledSegment( wxDC* aDC, const VECTOR2I& aStart, const VECTOR2I& aEnd, int aWidth,
                      const COLOR4D& aColor );

void GRCSegm( wxDC* aDC, const VECTOR2I& aStart, const VECTOR2I& aEnd, int aWidth,
              const COLOR4D& aColor );

void GRFilledRect( wxDC* DC, const VECTOR2I& aStart, const VECTOR2I& aEnd, int aWidth,
                   const COLOR4D& aColor, const COLOR4D& aBgColor );
void GRRect( wxDC* DC, const VECTOR2I& aStart, const VECTOR2I& aEnd, int aWidth,
             const COLOR4D& aColor );

void GRSFilledRect( wxDC* DC, int x1, int y1, int x2, int y2, int width, const COLOR4D& Color,
                    const COLOR4D& BgColor );

#endif      /* define GR_BASIC */
