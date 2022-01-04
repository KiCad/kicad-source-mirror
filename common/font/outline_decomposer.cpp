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
        m_outline( aOutline )
{
}


static VECTOR2D toVector2D( const FT_Vector* aFreeTypeVector )
{
    return VECTOR2D( aFreeTypeVector->x, aFreeTypeVector->y );
}


void OUTLINE_DECOMPOSER::newContour()
{
    CONTOUR contour;
    contour.orientation = FT_Outline_Get_Orientation( &m_outline );
    m_contours->push_back( contour );
}


void OUTLINE_DECOMPOSER::addContourPoint( const VECTOR2D& p )
{
    // don't add repeated points
    if( m_contours->back().points.empty() || m_contours->back().points.back() != p )
        m_contours->back().points.push_back( p );
}


int OUTLINE_DECOMPOSER::moveTo( const FT_Vector* aEndPoint, void* aCallbackData )
{
    OUTLINE_DECOMPOSER* decomposer = static_cast<OUTLINE_DECOMPOSER*>( aCallbackData );

    decomposer->m_lastEndPoint.x = aEndPoint->x;
    decomposer->m_lastEndPoint.y = aEndPoint->y;

    decomposer->newContour();
    decomposer->addContourPoint( decomposer->m_lastEndPoint );

    return 0;
}


int OUTLINE_DECOMPOSER::lineTo( const FT_Vector* aEndPoint, void* aCallbackData )
{
    OUTLINE_DECOMPOSER* decomposer = static_cast<OUTLINE_DECOMPOSER*>( aCallbackData );

    decomposer->m_lastEndPoint.x = aEndPoint->x;
    decomposer->m_lastEndPoint.y = aEndPoint->y;

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

    decomposer->m_lastEndPoint.x = aEndPoint->x;
    decomposer->m_lastEndPoint.y = aEndPoint->y;

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
        c.winding = winding( c.points );
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

    // TODO: find out what the minimum segment length should really be!
    static const int minimumSegmentLength = 50;
    GLYPH_POINTS     tmp;
    BEZIER_POLY      converter( aCubicBezier );
    converter.GetPoly( tmp, minimumSegmentLength );

    for( unsigned int i = 0; i < tmp.size(); i++ )
        aResult.push_back( tmp[i] );

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

    unsigned int i_lowest_vertex;
    double       lowest_y = std::numeric_limits<double>::max();

    for( unsigned int i = 0; i < aContour.size(); i++ )
    {
        VECTOR2D p = aContour[i];

        if( p.y < lowest_y )
        {
            i_lowest_vertex = i;
            lowest_y = p.y;

            // note: we should also check for p.y == lowest_y and then choose the point with
            // leftmost.x, but as p.x is a double, equality is a dubious concept; however
            // this should suffice in the general case
        }
    }

    unsigned int i_prev_vertex;
    unsigned int i_next_vertex;

    // TODO: this should be done with modulo arithmetic for clarity
    if( i_lowest_vertex == 0 )
        i_prev_vertex = aContour.size() - 1;
    else
        i_prev_vertex = i_lowest_vertex - 1;

    if( i_lowest_vertex == aContour.size() - 1 )
        i_next_vertex = 0;
    else
        i_next_vertex = i_lowest_vertex + 1;

    const VECTOR2D& lowest = aContour[i_lowest_vertex];
    VECTOR2D        prev( aContour[i_prev_vertex] );

    while( prev == lowest )
    {
        if( i_prev_vertex == 0 )
            i_prev_vertex = aContour.size() - 1;
        else
            i_prev_vertex--;

        if( i_prev_vertex == i_lowest_vertex )
        {
            // ERROR: degenerate contour (all points are equal)
            // TODO: signal error
            // for now let's just return something at random
            return cw;
        }

        prev = aContour[i_prev_vertex];
    }

    VECTOR2D next( aContour[i_next_vertex] );

    while( next == lowest )
    {
        if( i_next_vertex == aContour.size() - 1 )
            i_next_vertex = 0;
        else
            i_next_vertex++;

        if( i_next_vertex == i_lowest_vertex )
        {
            // ERROR: degenerate contour (all points are equal)
            // TODO: signal error
            // for now let's just return something at random
            return cw;
        }

        next = aContour[i_next_vertex];
    }

    // winding is figured out based on the angle between the lowest
    // vertex and its neighbours
    //
    // prev.x < lowest.x && next.x > lowest.x -> ccw
    //
    // prev.x > lowest.x && next.x < lowest.x -> cw
    //
    // prev.x < lowest.x && next.x < lowest.x:
    // ?
    //
    // prev.x > lowest.x && next.x > lowest.x:
    // ?
    //
    if( prev.x < lowest.x && next.x > lowest.x )
        return ccw;

    if( prev.x > lowest.x && next.x < lowest.x )
        return cw;

    double prev_deltaX = prev.x - lowest.x;
    double prev_deltaY = prev.y - lowest.y;
    double next_deltaX = next.x - lowest.x;
    double next_deltaY = next.y - lowest.y;

    double prev_atan = atan2( prev_deltaY, prev_deltaX );
    double next_atan = atan2( next_deltaY, next_deltaX );

    if( prev_atan > next_atan )
        return ccw;
    else
        return cw;
}
