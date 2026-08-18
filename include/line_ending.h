/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#pragma once

#include <math/vector2d.h>
#include <geometry/eda_angle.h>
#include <stroke_params.h>
#include <wx/string.h>

class OUTPUTFORMATTER;
class PLOTTER;
struct EDA_IU_SCALE;

namespace KIGFX
{
class GAL;
}

/**
 * Line ending styles for graphic lines, arcs, and beziers.
 */
enum class LINE_ENDING_STYLE
{
    NONE = 0,
    ARROW,
    CIRCLE,
    SQUARE,
    ARROW_OPEN,
};


/**
 * Decorative shape (arrowhead, circle, square) at the start or end of a
 * graphic line, arc, or bezier.
 *
 * Closed arrows are tip-anchored at the line endpoint and extend back along
 * the line.  Open arrows are vertex-anchored at the endpoint and do not
 * shorten the line body.  Circles and squares are centered on the endpoint.
 */
class LINE_ENDING
{
public:
    LINE_ENDING() :
            m_style( LINE_ENDING_STYLE::NONE ),
            m_length( 0 ),
            m_width( 0 ),
            m_stroke( 0, LINE_STYLE::DEFAULT, KIGFX::COLOR4D::UNSPECIFIED )
    {
    }

    LINE_ENDING( LINE_ENDING_STYLE aStyle, int aLength = 0, int aWidth = 0,
                 int aStrokeWidth = 0 ) :
            m_style( aStyle ),
            m_length( aLength ),
            m_width( aWidth ),
            m_stroke( aStrokeWidth, LINE_STYLE::DEFAULT, KIGFX::COLOR4D::UNSPECIFIED )
    {
    }

    LINE_ENDING_STYLE GetStyle() const { return m_style; }
    void SetStyle( LINE_ENDING_STYLE aStyle ) { m_style = aStyle; }

    int GetLength() const { return m_length; }     ///< Along-line size. 0 = auto.
    void SetLength( int aLength ) { m_length = aLength; }

    int GetWidth() const { return m_width; }        ///< Perpendicular size. 0 = auto.
    void SetWidth( int aWidth ) { m_width = aWidth; }

    void SetUniformSize( int aSize )
    {
        m_length = aSize;
        m_width = aSize;
    }

    /**
     * Outline stroke width.  0 = sharp/filled (default), >0 = stroked outline
     * with line shortening reduced by strokeWidth/2.
     */
    int GetStrokeWidth() const { return m_stroke.GetWidth(); }
    void SetStrokeWidth( int aWidth ) { m_stroke.SetWidth( aWidth ); }

    const STROKE_PARAMS& GetStroke() const { return m_stroke; }
    void SetStroke( const STROKE_PARAMS& aStroke ) { m_stroke = aStroke; }

    /**
     * Return the rendered size per axis, applying auto-sizing when length or width is 0.
     * When one axis is 0 and the other is set, the 0 axis inherits the set value.
     */
    VECTOR2I GetEffectiveSize( int aLineWidth ) const;

    /**
     * Return how far the line should be shortened at this ending.
     * Arrow: full length.  Circle/Square: half length.  Arrow_Open/None: 0.
     * Reduced by strokeWidth/2 when stroked.
     */
    int GetShortenDepth( int aLineWidth ) const;

    /**
     * Return how far along a curved body to look when orienting this ending.
     *
     * This is separate from line-body shortening.  Open arrows do not shorten
     * the line body, but they still extend behind the endpoint and need that
     * visual length when aligning to curves.
     */
    int GetCurveOrientationDepth( int aLineWidth ) const;

    /**
     * Generate ending geometry as polygon vertices at the given point and direction.
     * ARROW_OPEN produces an open 3-point polyline; all others produce closed polygons.
     *
     * @param aTangent  Outward-facing tangent direction at the endpoint.
     */
    void GetShapes( const VECTOR2I& aPoint, const EDA_ANGLE& aTangent,
                    int aLineWidth, std::vector<VECTOR2I>& aPolygon ) const;

    bool operator==( const LINE_ENDING& aOther ) const;
    bool operator!=( const LINE_ENDING& aOther ) const { return !( *this == aOther ); }

    void Draw( KIGFX::GAL& aGal, const VECTOR2I& aPoint, const EDA_ANGLE& aTangent,
               double aLineWidth, const KIGFX::COLOR4D& aColor ) const;

    void Plot( PLOTTER* aPlotter, const VECTOR2I& aPoint, const EDA_ANGLE& aTangent,
               int aLineWidth, void* aData = nullptr ) const;

    void Format( OUTPUTFORMATTER* aOut, const EDA_IU_SCALE& aIuScale,
                 const char* aToken ) const;

    static wxString StyleToToken( LINE_ENDING_STYLE aStyle );
    static LINE_ENDING_STYLE TokenToStyle( const wxString& aToken );

    static constexpr double DEFAULT_RATIO_LENGTH = 5.0;
    static constexpr double DEFAULT_RATIO_WIDTH = 5.0;

    /**
     * Dropdown display order for line ending style choosers.
     * Order: None, Arrow, Open Arrow, Circle, Square.
     */
    static const LINE_ENDING_STYLE s_defaultChoiceOrder[];
    static const int               s_defaultChoiceCount;

    /**
     * Return the dropdown index for the given style, or 0 (NONE) if not found.
     */
    static int StyleToChoiceIndex( LINE_ENDING_STYLE aStyle );

private:
    LINE_ENDING_STYLE m_style;
    int               m_length;         ///< Along-line axis.  0 = auto (ratio * line width).
    int               m_width;          ///< Perpendicular axis.  0 = auto.
    STROKE_PARAMS     m_stroke;
};
