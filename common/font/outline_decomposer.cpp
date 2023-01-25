/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2021 Ola Rinta-Koski <gitlab@rinta-koski.net>
 * Copyright (C) 2021 Kicad Developers, see AUTHORS.txt for contributors.
 *
 * Outline font class
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

#include <font/outline_decomposer.h>
#include <bezier_curves.h>

using namespace KIFONT;

OUTLINE_DECOMPOSER::OUTLINE_DECOMPOSER( FT_Outline& aOutline ) :
        m_outline( aOutline ),
        m_contours( nullptr )
{
}


static VECTOR2D toVector2D( const FT_Vector* aFreeTypeVector )
{
    return VECTOR2D( aFreeTypeVector->x * GLYPH_SIZE_SCALER,
                     aFreeTypeVector->y * GLYPH_SIZE_SCALER );
}


void OUTLINE_DECOMPOSER::newContour()
{
    CONTOUR contour;
    contour.m_Orientation = FT_Outline_Get_Orientation( &m_outline );
    m_contours->push_back( contour );
}


void OUTLINE_DECOMPOSER::addContourPoint( const VECTOR2D& p )
{
    // don't add repeated points
    if( m_contours->back().m_Points.empty() || m_contours->back().m_Points.back() != p )
        m_contours->back().m_Points.push_back( p );
}


int OUTLINE_DECOMPOSER::moveTo( const FT_Vector* aEndPoint, void* aCallbackData )
{
    OUTLINE_DECOMPOSER* decomposer = static_cast<OUTLINE_DECOMPOSER*>( aCallbackData );

    decomposer->m_lastEndPoint = toVector2D( aEndPoint );

    decomposer->newContour();
    decomposer->addContourPoint( decomposer->m_lastEndPoint );

    return 0;
}


int OUTLINE_DECOMPOSER::lineTo( const FT_Vector* aEndPoint, void* aCallbackData )
{
    OUTLINE_DECOMPOSER* decomposer = static_cast<OUTLINE_DECOMPOSER*>( aCallbackData );

    decomposer->m_lastEndPoint = toVector2D( aEndPoint );

    decomposer->addContourPoint( decomposer->m_lastEndPoint );

    return 0;
}


int OUTLINE_DECOMPOSER::quadraticTo( const FT_Vector* aControlPoint, const FT_Vector* aEndPoint,
                                     void* aCallbackData )
{
    return cubicTo( aControlPoint, nullptr, aEndPoint, aCallbackData );
}


int OUTLINE_DECOMPOSER::cubicTo( const FT_Vector* aFirstControlPoint,
                                 const FT_Vector* aSecondControlPoint, const FT_Vector* aEndPoint,
                                 void* aCallbackData )
{
    OUTLINE_DECOMPOSER* decomposer = static_cast<OUTLINE_DECOMPOSER*>( aCallbackData );

    GLYPH_POINTS bezier;
    bezier.push_back( decomposer->m_lastEndPoint );
    bezier.push_back( toVector2D( aFirstControlPoint ) );

    if( aSecondControlPoint )
    {
        // aSecondControlPoint == nullptr for quadratic Beziers
        bezier.push_back( toVector2D( aSecondControlPoint ) );
    }

    bezier.push_back( toVector2D( aEndPoint ) );

    GLYPH_POINTS result;
    decomposer->approximateBezierCurve( result, bezier );
    for( const VECTOR2D& p : result )
        decomposer->addContourPoint( p );

    decomposer->m_lastEndPoint = toVector2D( aEndPoint );

    return 0;
}


void OUTLINE_DECOMPOSER::OutlineToSegments( CONTOURS* aContours )
{
    m_contours = aContours;

    FT_Outline_Funcs callbacks;

    callbacks.move_to = moveTo;
    callbacks.line_to = lineTo;
    callbacks.conic_to = quadraticTo;
    callbacks.cubic_to = cubicTo;
    callbacks.shift = 0;
    callbacks.delta = 0;

    FT_Error e = FT_Outline_Decompose( &m_outline, &callbacks, this );

    if( e )
    {
        // TODO: handle error != 0
    }

    for( CONTOUR& c : *m_contours )
        c.m_Winding = winding( c.m_Points );
}


// use converter in kimath
bool OUTLINE_DECOMPOSER::approximateQuadraticBezierCurve( GLYPH_POINTS&       aResult,
                                                          const GLYPH_POINTS& aBezier ) const
{
    wxASSERT( aBezier.size() == 3 );

    // BEZIER_POLY only handles cubic Bezier curves, even though the
    // comments say otherwise...
    //
    // Quadratic to cubic Bezier conversion:
    // cpn = Cubic Bezier control points (n = 0..3, 4 in total)
    // qpn = Quadratic Bezier control points (n = 0..2, 3 in total)
    // cp0 = qp0, cp1 = qp0 + 2/3 * (qp1 - qp0), cp2 = qp2 + 2/3 * (qp1 - qp2), cp3 = qp2

    GLYPH_POINTS cubic;
    cubic.reserve( 4 );

    cubic.push_back( aBezier[0] );                                           // cp0
    cubic.push_back( aBezier[0] + ( ( aBezier[1] - aBezier[0] ) * 2 / 3 ) ); // cp1
    cubic.push_back( aBezier[2] + ( ( aBezier[1] - aBezier[2] ) * 2 / 3 ) ); // cp2
    cubic.push_back( aBezier[2] );                                           // cp3

    return approximateCubicBezierCurve( aResult, cubic );
}


bool OUTLINE_DECOMPOSER::approximateCubicBezierCurve( GLYPH_POINTS&       aResult,
                                                      const GLYPH_POINTS& aCubicBezier ) const
{
    wxASSERT( aCubicBezier.size() == 4 );

    // minimumSegmentLength defines the "smoothness" of the
    // curve-to-straight-segments conversion: the larger, the coarser
    // TODO: find out what the minimum segment length should really be!
    constexpr int minimumSegmentLength = 10;
    BEZIER_POLY   converter( aCubicBezier );
    converter.GetPoly( aResult, minimumSegmentLength );

    return true;
}


bool OUTLINE_DECOMPOSER::approximateBezierCurve( GLYPH_POINTS&       aResult,
                                                 const GLYPH_POINTS& aBezier ) const
{
    switch( aBezier.size() )
    {
    case 4: // cubic
        return approximateCubicBezierCurve( aResult, aBezier );
        break;
    case 3: // quadratic
        return approximateQuadraticBezierCurve( aResult, aBezier );
        break;
    default:
        // error, only 3 and 4 are acceptable values
        return false;
    }
}


int OUTLINE_DECOMPOSER::winding( const GLYPH_POINTS& aContour ) const
{
    // -1 == counterclockwise, 1 == clockwise

    const int cw = 1;
    const int ccw = -1;

    if( aContour.size() < 2 )
    {
        // zero or one points, so not a clockwise contour - in fact not a contour at all
        //
        // It could also be argued that a contour needs 3 extremum points at a minimum to be
        // considered a proper contour (ie. a glyph (subpart) outline, or a hole)
        return 0;
    }

    double sum = 0.0;
    size_t len = aContour.size();

    for( size_t i = 0; i < len - 1; i++ )
    {
        VECTOR2D p1 = aContour[ i ];
        VECTOR2D p2 = aContour[ i + 1 ];

        sum += ( ( p2.x - p1.x ) * ( p2.y + p1.y ) );
    }

    sum += ( ( aContour[0].x - aContour[len - 1].x ) * ( aContour[0].y + aContour[len - 1].y ) );

    if( sum > 0.0 )
        return cw;
    if( sum < 0.0 )
        return ccw;

    return 0;
}
