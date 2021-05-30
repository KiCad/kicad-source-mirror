/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2017 CERN
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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

#ifndef __VIEW_OVERLAY_H
#define __VIEW_OVERLAY_H

#include <view/view_item.h>
#include <gal/graphics_abstraction_layer.h>

#include <vector>
#include <deque>

#include <geometry/shape_poly_set.h>

class SEG;

namespace KIGFX
{
class VIEW;

class VIEW_OVERLAY : public VIEW_ITEM
{
public:

    VIEW_OVERLAY();
    virtual ~VIEW_OVERLAY();

    struct COMMAND;
    struct COMMAND_ARC;
    struct COMMAND_LINE;
    struct COMMAND_CIRCLE;
    struct COMMAND_RECTANGLE;

    struct COMMAND_SET_STROKE;
    struct COMMAND_SET_FILL;
    struct COMMAND_SET_COLOR;
    struct COMMAND_SET_WIDTH;

    struct COMMAND_POLYGON;
    struct COMMAND_POINT_POLYGON;
    struct COMMAND_POLY_POLYGON;

    struct COMMAND_POLYLINE;
    struct COMMAND_POINT_POLYLINE;

    struct COMMAND_GLYPH_SIZE;
    struct COMMAND_BITMAP_TEXT;

    void Clear();

    virtual const BOX2I ViewBBox() const override;
    virtual void ViewDraw( int aLayer, VIEW *aView ) const override;
    virtual void ViewGetLayers( int aLayers[], int& aCount ) const override;

    // Basic shape primitives
    void Line( const VECTOR2D& aStartPoint, const VECTOR2D& aEndPoint );
    void Line( const SEG& aSeg );
    void Segment( const VECTOR2D& aStartPoint, const VECTOR2D& aEndPoint, double aWidth );
    void Circle( const VECTOR2D& aCenterPoint, double aRadius );
    void Arc( const VECTOR2D& aCenterPoint, double aRadius, double aStartAngle, double aEndAngle );
    void Rectangle( const VECTOR2D& aStartPoint, const VECTOR2D& aEndPoint );
    void Cross( const VECTOR2D& aP, int aSize );

    // polygon primitives
    void Polygon( const std::deque<VECTOR2D>& aPointList );
    void Polygon( const SHAPE_POLY_SET& aPolySet );
    void Polyline( const SHAPE_LINE_CHAIN& aPolyLine );
    void Polygon( const VECTOR2D aPointList[], int aListSize );

    void BitmapText( const wxString& aText, const VECTOR2D& aPosition,
                             double aRotationAngle );
    
    // Draw settings
    void SetIsFill( bool aIsFillEnabled );
    void SetIsStroke( bool aIsStrokeEnabled );
    void SetFillColor( const COLOR4D& aColor );
    void SetStrokeColor( const COLOR4D& aColor );
    void SetGlyphSize( const VECTOR2D aSize );
    void SetLineWidth( double aLineWidth );

    const COLOR4D& GetStrokeColor() const { return m_strokeColor; }
    const COLOR4D& GetFillColor() const { return m_fillColor; }

private:
    void releaseCommands();

    COLOR4D m_strokeColor;
    COLOR4D m_fillColor;

    std::vector<COMMAND*> m_commands;
};

} // namespace KIGFX


#endif
