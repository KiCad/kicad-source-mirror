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

#include <line_ending.h>
#include <stroke_params.h>
#include <eda_units.h>
#include <macros.h>
#include <string_utils.h>
#include <trigo.h>
#include <richio.h>
#include <gal/graphics_abstraction_layer.h>
#include <plotters/plotter.h>

#include <cmath>
#include <iterator>


const LINE_ENDING_STYLE LINE_ENDING::s_defaultChoiceOrder[] =
{
    LINE_ENDING_STYLE::NONE,
    LINE_ENDING_STYLE::ARROW,
    LINE_ENDING_STYLE::ARROW_OPEN,
    LINE_ENDING_STYLE::CIRCLE,
    LINE_ENDING_STYLE::SQUARE,
};

const int LINE_ENDING::s_defaultChoiceCount =
        static_cast<int>( std::size( LINE_ENDING::s_defaultChoiceOrder ) );


int LINE_ENDING::StyleToChoiceIndex( LINE_ENDING_STYLE aStyle )
{
    for( int i = 0; i < s_defaultChoiceCount; ++i )
    {
        if( s_defaultChoiceOrder[i] == aStyle )
            return i;
    }

    return 0;
}


VECTOR2I LINE_ENDING::GetEffectiveSize( int aLineWidth ) const
{
    int autoLen = KiROUND( aLineWidth * DEFAULT_RATIO_LENGTH );
    int autoWid = KiROUND( aLineWidth * DEFAULT_RATIO_WIDTH );

    int len, wid;

    if( m_length > 0 )
        len = m_length;
    else if( m_width > 0 )
        len = m_width;
    else
        len = autoLen;

    if( m_width > 0 )
        wid = m_width;
    else if( m_length > 0 )
        wid = m_length;
    else
        wid = autoWid;

    return VECTOR2I( len, wid );
}


int LINE_ENDING::GetShortenDepth( int aLineWidth ) const
{
    if( m_style == LINE_ENDING_STYLE::NONE )
        return 0;

    VECTOR2I sz = GetEffectiveSize( aLineWidth );

    int raw;

    if( m_style == LINE_ENDING_STYLE::ARROW )
        raw = sz.x;
    else if( m_style == LINE_ENDING_STYLE::ARROW_OPEN )
        raw = 0;
    else
        raw = sz.x / 2;

    if( m_stroke.GetWidth() > 0 )
        raw = std::max( 0, raw - m_stroke.GetWidth() / 2 );

    return raw;
}


int LINE_ENDING::GetCurveOrientationDepth( int aLineWidth ) const
{
    if( m_style == LINE_ENDING_STYLE::ARROW_OPEN )
        return GetEffectiveSize( aLineWidth ).x;

    return GetShortenDepth( aLineWidth );
}


void LINE_ENDING::GetShapes( const VECTOR2I& aPoint, const EDA_ANGLE& aTangent,
                             int aLineWidth, std::vector<VECTOR2I>& aPolygon ) const
{
    if( m_style == LINE_ENDING_STYLE::NONE )
        return;

    VECTOR2I size = GetEffectiveSize( aLineWidth );
    int      sizeX = size.x;
    int      sizeY = size.y;

    aPolygon.clear();

    switch( m_style )
    {
    case LINE_ENDING_STYLE::ARROW:
    {
        int halfWidth = sizeY / 2;

        aPolygon.emplace_back( 0, 0 );
        aPolygon.emplace_back( -sizeX,  halfWidth );
        aPolygon.emplace_back( -sizeX, -halfWidth );
        aPolygon.emplace_back( 0, 0 );
        break;
    }

    case LINE_ENDING_STYLE::ARROW_OPEN:
    {
        int halfWidth = sizeY / 2;

        aPolygon.emplace_back( -sizeX, -halfWidth );
        aPolygon.emplace_back( 0, 0 );
        aPolygon.emplace_back( -sizeX,  halfWidth );
        break;
    }

    case LINE_ENDING_STYLE::CIRCLE:
    {
        int halfX = sizeX / 2;
        int halfY = sizeY / 2;
        int nSeg = 32;

        for( int i = 0; i <= nSeg; i++ )
        {
            double angle = 2.0 * M_PI * i / nSeg;
            int    px = KiROUND( halfX * std::cos( angle ) );
            int    py = KiROUND( halfY * std::sin( angle ) );
            aPolygon.emplace_back( px, py );
        }

        break;
    }

    case LINE_ENDING_STYLE::SQUARE:
    {
        int halfX = sizeX / 2;
        int halfY = sizeY / 2;

        aPolygon.emplace_back(  halfX,  halfY );
        aPolygon.emplace_back(  halfX, -halfY );
        aPolygon.emplace_back( -halfX, -halfY );
        aPolygon.emplace_back( -halfX,  halfY );
        aPolygon.emplace_back(  halfX,  halfY );
        break;
    }

    case LINE_ENDING_STYLE::NONE:
        return;
    }

    for( VECTOR2I& pt : aPolygon )
    {
        RotatePoint( pt, -aTangent );
        pt += aPoint;
    }
}


void LINE_ENDING::Draw( KIGFX::GAL& aGal, const VECTOR2I& aPoint, const EDA_ANGLE& aTangent,
                        double aLineWidth, const KIGFX::COLOR4D& aColor ) const
{
    if( m_style == LINE_ENDING_STYLE::NONE )
        return;

    std::vector<VECTOR2I> polygon;
    GetShapes( aPoint, aTangent, KiROUND( aLineWidth ), polygon );

    if( polygon.empty() )
        return;

    KIGFX::GAL_SCOPED_ATTRS scopedAttrs( aGal, KIGFX::GAL_SCOPED_ATTRS::STROKE_FILL );

    if( m_style == LINE_ENDING_STYLE::ARROW_OPEN )
    {
        aGal.SetIsFill( false );
        aGal.SetIsStroke( true );
        aGal.SetStrokeColor( aColor );
        aGal.SetLineWidth( m_stroke.GetWidth() > 0 ? m_stroke.GetWidth() : aLineWidth );
        aGal.DrawPolyline( polygon );
    }
    else if( m_stroke.GetWidth() > 0 )
    {
        aGal.SetIsFill( true );
        aGal.SetFillColor( aColor );
        aGal.SetIsStroke( true );
        aGal.SetStrokeColor( aColor );
        aGal.SetLineWidth( m_stroke.GetWidth() );
        aGal.DrawPolygon( polygon );
    }
    else
    {
        aGal.SetIsFill( true );
        aGal.SetFillColor( aColor );
        aGal.SetIsStroke( false );
        aGal.DrawPolygon( polygon );
    }
}


void LINE_ENDING::Plot( PLOTTER* aPlotter, const VECTOR2I& aPoint, const EDA_ANGLE& aTangent,
                        int aLineWidth, void* aData ) const
{
    if( m_style == LINE_ENDING_STYLE::NONE )
        return;

    std::vector<VECTOR2I> polygon;
    GetShapes( aPoint, aTangent, aLineWidth, polygon );

    if( polygon.empty() )
        return;

    if( m_style == LINE_ENDING_STYLE::ARROW_OPEN )
    {
        int plotWidth = m_stroke.GetWidth() > 0 ? m_stroke.GetWidth() : aLineWidth;
        aPlotter->PlotPoly( polygon, FILL_T::NO_FILL, plotWidth, aData );
    }
    else if( m_stroke.GetWidth() > 0 )
    {
        aPlotter->PlotPoly( polygon, FILL_T::FILLED_SHAPE, m_stroke.GetWidth(), aData );
    }
    else
    {
        aPlotter->PlotPoly( polygon, FILL_T::FILLED_SHAPE, 0, aData );
    }
}


bool LINE_ENDING::operator==( const LINE_ENDING& aOther ) const
{
    return m_style == aOther.m_style
            && m_length == aOther.m_length
            && m_width == aOther.m_width
            && m_stroke == aOther.m_stroke;
}


wxString LINE_ENDING::StyleToToken( LINE_ENDING_STYLE aStyle )
{
    switch( aStyle )
    {
    case LINE_ENDING_STYLE::NONE:       return wxT( "none" );
    case LINE_ENDING_STYLE::ARROW:      return wxT( "arrow" );
    case LINE_ENDING_STYLE::CIRCLE:     return wxT( "circle" );
    case LINE_ENDING_STYLE::SQUARE:     return wxT( "square" );
    case LINE_ENDING_STYLE::ARROW_OPEN: return wxT( "arrow_open" );
    default:                            return wxT( "none" );
    }
}


LINE_ENDING_STYLE LINE_ENDING::TokenToStyle( const wxString& aToken )
{
    if( aToken == wxT( "arrow" ) )       return LINE_ENDING_STYLE::ARROW;
    if( aToken == wxT( "circle" ) )      return LINE_ENDING_STYLE::CIRCLE;
    if( aToken == wxT( "square" ) )      return LINE_ENDING_STYLE::SQUARE;
    if( aToken == wxT( "arrow_open" ) )  return LINE_ENDING_STYLE::ARROW_OPEN;
    return LINE_ENDING_STYLE::NONE;
}


void LINE_ENDING::Format( OUTPUTFORMATTER* aOut, const EDA_IU_SCALE& aIuScale,
                           const char* aToken ) const
{
    if( m_style == LINE_ENDING_STYLE::NONE )
        return;

    std::string result = "(" + std::string( aToken ) + " "
                         + TO_UTF8( StyleToToken( m_style ) );

    if( m_length > 0 )
    {
        result += " (length "
                  + EDA_UNIT_UTILS::FormatInternalUnits( aIuScale, m_length )
                  + ")";
    }

    if( m_width > 0 )
    {
        result += " (width "
                  + EDA_UNIT_UTILS::FormatInternalUnits( aIuScale, m_width )
                  + ")";
    }

    if( m_stroke.GetWidth() > 0 )
    {
        result += " (stroke (width "
                  + EDA_UNIT_UTILS::FormatInternalUnits( aIuScale, m_stroke.GetWidth() )
                  + ") (type "
                  + TO_UTF8( STROKE_PARAMS::GetLineStyleToken( m_stroke.GetLineStyle() ) )
                  + "))";
    }

    result += ")";

    aOut->Print( "%s", result.c_str() );
}
