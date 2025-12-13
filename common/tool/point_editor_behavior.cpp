/*
 * This program source code file is part of KICAD, a free EDA CAD application.
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

#include "tool/point_editor_behavior.h"

#include <advanced_config.h>
#include <commit.h>
#include <geometry/shape_poly_set.h>


void POLYGON_POINT_EDIT_BEHAVIOR::BuildForPolyOutline( EDIT_POINTS&          aPoints,
                                                       const SHAPE_POLY_SET& aOutline )
{
    const int cornersCount = aOutline.TotalVertices();

    if( cornersCount == 0 )
        return;

    for( auto iterator = aOutline.CIterateWithHoles(); iterator; iterator++ )
    {
        aPoints.AddPoint( *iterator );

        if( iterator.IsEndContour() )
            aPoints.AddBreak();
    }

    // Lines have to be added after creating edit points, as they use EDIT_POINT references
    for( int i = 0; i < cornersCount - 1; ++i )
    {
        if( aPoints.IsContourEnd( i ) )
            aPoints.AddLine( aPoints.Point( i ), aPoints.Point( aPoints.GetContourStartIdx( i ) ) );
        else
            aPoints.AddLine( aPoints.Point( i ), aPoints.Point( i + 1 ) );

        aPoints.Line( i ).SetConstraint( new EC_CONVERGING( aPoints.Line( i ), aPoints ) );
    }

    // The last missing line, connecting the last and the first polygon point
    aPoints.AddLine( aPoints.Point( cornersCount - 1 ),
                     aPoints.Point( aPoints.GetContourStartIdx( cornersCount - 1 ) ) );

    aPoints.Line( aPoints.LinesSize() - 1 )
            .SetConstraint( new EC_CONVERGING( aPoints.Line( aPoints.LinesSize() - 1 ), aPoints ) );
}


void POLYGON_POINT_EDIT_BEHAVIOR::UpdatePointsFromOutline( const SHAPE_POLY_SET& aOutline,
                                                           EDIT_POINTS&          aPoints )
{
    // No size check here, as we can and will rebuild if that fails
    if( aPoints.PointsSize() != (unsigned) aOutline.TotalVertices() )
    {
        // Rebuild the points list
        aPoints.Clear();
        BuildForPolyOutline( aPoints, aOutline );
    }
    else
    {
        for( int i = 0; i < aOutline.TotalVertices(); ++i )
            aPoints.Point( i ).SetPosition( aOutline.CVertex( i ) );
    }
}


void POLYGON_POINT_EDIT_BEHAVIOR::UpdateOutlineFromPoints( SHAPE_POLY_SET&   aOutline,
                                                           const EDIT_POINT& aEditedPoint,
                                                           EDIT_POINTS&      aPoints )
{
    CHECK_POINT_COUNT_GE( aPoints, (unsigned) aOutline.TotalVertices() );

    for( int i = 0; i < aOutline.TotalVertices(); ++i )
        aOutline.SetVertex( i, aPoints.Point( i ).GetPosition() );

    for( unsigned i = 0; i < aPoints.LinesSize(); ++i )
    {
        if( !isModified( aEditedPoint, aPoints.Line( i ) ) )
            aPoints.Line( i ).SetConstraint( new EC_CONVERGING( aPoints.Line( i ), aPoints ) );
    }
}


void POLYGON_POINT_EDIT_BEHAVIOR::FinalizeItem( EDIT_POINTS& aPoints, COMMIT& aCommit )
{
    m_polygon.RemoveNullSegments();
}


void EDA_SEGMENT_POINT_EDIT_BEHAVIOR::MakePoints( EDIT_POINTS& aPoints )
{
    aPoints.AddPoint( m_segment.GetStart() );
    aPoints.AddPoint( m_segment.GetEnd() );
}


bool EDA_SEGMENT_POINT_EDIT_BEHAVIOR::UpdatePoints( EDIT_POINTS& aPoints )
{
    wxCHECK( aPoints.PointsSize() == SEGMENT_MAX_POINTS, false );

    aPoints.Point( SEGMENT_START ) = m_segment.GetStart();
    aPoints.Point( SEGMENT_END ) = m_segment.GetEnd();
    return true;
}


void EDA_SEGMENT_POINT_EDIT_BEHAVIOR::UpdateItem( const EDIT_POINT& aEditedPoint,
                                                  EDIT_POINTS& aPoints, COMMIT& aCommit,
                                                  std::vector<EDA_ITEM*>& aUpdatedItems )
{
    CHECK_POINT_COUNT( aPoints, SEGMENT_MAX_POINTS );

    if( isModified( aEditedPoint, aPoints.Point( SEGMENT_START ) ) )
        m_segment.SetStart( aPoints.Point( SEGMENT_START ).GetPosition() );

    else if( isModified( aEditedPoint, aPoints.Point( SEGMENT_END ) ) )
        m_segment.SetEnd( aPoints.Point( SEGMENT_END ).GetPosition() );
}


OPT_VECTOR2I EDA_SEGMENT_POINT_EDIT_BEHAVIOR::Get45DegreeConstrainer( const EDIT_POINT& aEditedPoint,
                                                                      EDIT_POINTS&      aPoints ) const
{
    // Select the other end of line
    return aPoints.Next( aEditedPoint )->GetPosition();
}


void EDA_CIRCLE_POINT_EDIT_BEHAVIOR::MakePoints( EDIT_POINTS& aPoints )
{
    aPoints.AddPoint( m_circle.getCenter() );
    aPoints.AddPoint( m_circle.GetEnd() );
}


bool EDA_CIRCLE_POINT_EDIT_BEHAVIOR::UpdatePoints( EDIT_POINTS& aPoints )
{
    wxCHECK( aPoints.PointsSize() == CIRC_MAX_POINTS, false );

    aPoints.Point( CIRC_CENTER ).SetPosition( m_circle.getCenter() );
    aPoints.Point( CIRC_END ).SetPosition( m_circle.GetEnd() );
    return true;
}


void EDA_CIRCLE_POINT_EDIT_BEHAVIOR::UpdateItem( const EDIT_POINT& aEditedPoint,
                                                 EDIT_POINTS& aPoints, COMMIT& aCommit,
                                                 std::vector<EDA_ITEM*>& aUpdatedItems )
{
    CHECK_POINT_COUNT( aPoints, CIRC_MAX_POINTS );

    const VECTOR2I& center = aPoints.Point( CIRC_CENTER ).GetPosition();
    const VECTOR2I& end = aPoints.Point( CIRC_END ).GetPosition();

    if( isModified( aEditedPoint, aPoints.Point( CIRC_CENTER ) ) )
        m_circle.SetCenter( center );
    else
        m_circle.SetEnd( VECTOR2I( end.x, end.y ) );
}


OPT_VECTOR2I EDA_CIRCLE_POINT_EDIT_BEHAVIOR::Get45DegreeConstrainer( const EDIT_POINT& aEditedPoint,
                                                                     EDIT_POINTS&      aPoints ) const
{
    return aPoints.Point( CIRC_CENTER ).GetPosition();
}


void EDA_BEZIER_POINT_EDIT_BEHAVIOR::MakePoints( EDIT_POINTS& aPoints )
{
    aPoints.AddPoint( m_bezier.GetStart() );
    aPoints.AddPoint( m_bezier.GetBezierC1() );
    aPoints.AddPoint( m_bezier.GetBezierC2() );
    aPoints.AddPoint( m_bezier.GetEnd() );

    aPoints.AddIndicatorLine( aPoints.Point( BEZIER_START ), aPoints.Point( BEZIER_CTRL_PT1 ) );
    aPoints.AddIndicatorLine( aPoints.Point( BEZIER_CTRL_PT2 ), aPoints.Point( BEZIER_END ) );
}


bool EDA_BEZIER_POINT_EDIT_BEHAVIOR::UpdatePoints( EDIT_POINTS& aPoints )
{
    wxCHECK( aPoints.PointsSize() == BEZIER_MAX_POINTS, false );

    aPoints.Point( BEZIER_START ).SetPosition( m_bezier.GetStart() );
    aPoints.Point( BEZIER_CTRL_PT1 ).SetPosition( m_bezier.GetBezierC1() );
    aPoints.Point( BEZIER_CTRL_PT2 ).SetPosition( m_bezier.GetBezierC2() );
    aPoints.Point( BEZIER_END ).SetPosition( m_bezier.GetEnd() );
    return true;
}


void EDA_BEZIER_POINT_EDIT_BEHAVIOR::UpdateItem( const EDIT_POINT& aEditedPoint, EDIT_POINTS& aPoints,
                                                 COMMIT& aCommit, std::vector<EDA_ITEM*>& aUpdatedItems )
{
    CHECK_POINT_COUNT( aPoints, BEZIER_MAX_POINTS );

    if( isModified( aEditedPoint, aPoints.Point( BEZIER_START ) ) )
    {
        m_bezier.SetStart( aPoints.Point( BEZIER_START ).GetPosition() );
    }
    else if( isModified( aEditedPoint, aPoints.Point( BEZIER_CTRL_PT1 ) ) )
    {
        m_bezier.SetBezierC1( aPoints.Point( BEZIER_CTRL_PT1 ).GetPosition() );
    }
    else if( isModified( aEditedPoint, aPoints.Point( BEZIER_CTRL_PT2 ) ) )
    {
        m_bezier.SetBezierC2( aPoints.Point( BEZIER_CTRL_PT2 ).GetPosition() );
    }
    else if( isModified( aEditedPoint, aPoints.Point( BEZIER_END ) ) )
    {
        m_bezier.SetEnd( aPoints.Point( BEZIER_END ).GetPosition() );
    }

    m_bezier.RebuildBezierToSegmentsPointsList( m_maxError );
}


// Note: these static arc functions don't have to be in here - we could ship them out
// to a utils area for use by other code (e.g. polygon fillet editing).

/**
 * Move an end point of the arc, while keeping the tangent at the other endpoint.
 */
static void editArcEndpointKeepTangent( EDA_SHAPE& aArc, const VECTOR2I& aCenter, const VECTOR2I& aStart,
                                        const VECTOR2I& aMid, const VECTOR2I& aEnd, const VECTOR2I& aCursor )
{
    VECTOR2I center = aCenter;
    bool     movingStart;
    bool     arcValid = true;

    VECTOR2I p1, p2, p3;
    // p1 does not move, p2 does.

    if( aStart != aArc.GetStart() )
    {
        p1 = aEnd;
        p2 = aStart;
        p3 = aMid;
        movingStart = true;
    }
    else if( aEnd != aArc.GetEnd() )
    {
        p1 = aStart;
        p2 = aEnd;
        p3 = aMid;
        movingStart = false;
    }
    else
    {
        return;
    }

    VECTOR2D v1, v2, v3, v4;

    // Move the coordinate system
    v1 = p1 - aCenter;
    v2 = p2 - aCenter;
    v3 = p3 - aCenter;

    VECTOR2D u1, u2;

    // A point cannot be both the center and on the arc.
    if( ( v1.EuclideanNorm() == 0 ) || ( v2.EuclideanNorm() == 0 ) )
        return;

    u1 = v1 / v1.EuclideanNorm();
    u2 = v3 - ( u1.x * v3.x + u1.y * v3.y ) * u1;
    u2 = u2 / u2.EuclideanNorm();

    // [ u1, u3 ] is a base centered on the circle with:
    //  u1 : unit vector toward the point that does not move
    //  u2 : unit vector toward the mid point.

    // Get vectors v1, and v2 in that coordinate system.

    double det = u1.x * u2.y - u2.x * u1.y;

    // u1 and u2 are unit vectors, and perpendicular.
    // det should not be 0. In case it is, do not change the arc.
    if( det == 0 )
        return;

    double tmpx = v1.x * u2.y - v1.y * u2.x;
    double tmpy = -v1.x * u1.y + v1.y * u1.x;
    v1.x = tmpx;
    v1.y = tmpy;
    v1 = v1 / det;

    tmpx = v2.x * u2.y - v2.y * u2.x;
    tmpy = -v2.x * u1.y + v2.y * u1.x;
    v2.x = tmpx;
    v2.y = tmpy;
    v2 = v2 / det;

    double R = v1.EuclideanNorm();
    bool   transformCircle = false;

    /*                 p2
    *                     X***
    *                         **  <---- This is the arc
    *            y ^            **
    *              |      R       *
    *              | <-----------> *
    *       x------x------>--------x p1
    *     C' <----> C      x
    *         delta
    *
    * p1 does not move, and the tangent at p1 remains the same.
    *  => The new center, C', will be on the C-p1 axis.
    * p2 moves
    *
    * The radius of the new circle is delta + R
    *
    * || C' p2 || = || C' P1 ||
    * is the same as :
    * ( delta + p2.x ) ^ 2 + p2.y ^ 2 = ( R + delta ) ^ 2
    *
    * delta = ( R^2  - p2.x ^ 2 - p2.y ^2 ) / ( 2 * p2.x - 2 * R )
    *
    * We can use this equation for any point p2 with p2.x < R
    */

    if( v2.x == R )
    {
        // Straight line, do nothing
    }
    else
    {
        if( v2.x > R )
        {
            // If we need to invert the curvature.
            // We modify the input so we can use the same equation
            transformCircle = true;
            v2.x = 2 * R - v2.x;
        }

        // We can keep the tangent constraint.
        double delta = ( R * R - v2.x * v2.x - v2.y * v2.y ) / ( 2 * v2.x - 2 * R );

        // This is just to limit the radius, so nothing overflows later when drawing.
        if( abs( v2.y / ( R - v2.x ) ) > ADVANCED_CFG::GetCfg().m_DrawArcCenterMaxAngle )
            arcValid = false;

        // Never recorded a problem, but still checking.
        if( !std::isfinite( delta ) )
            arcValid = false;

        // v4 is the new center
        v4 = ( !transformCircle ) ? VECTOR2D( -delta, 0 ) : VECTOR2D( 2 * R + delta, 0 );

        tmpx = v4.x * u1.x + v4.y * u2.x;
        tmpy = v4.x * u1.y + v4.y * u2.y;
        v4.x = tmpx;
        v4.y = tmpy;

        center = v4 + aCenter;

        if( arcValid )
        {
            aArc.SetCenter( center );

            if( movingStart )
                aArc.SetStart( aStart );
            else
                aArc.SetEnd( aEnd );
        }
    }
}


/**
 * Move the arc center but keep endpoint locations.
 */
static void editArcCenterKeepEndpoints( EDA_SHAPE& aArc, const VECTOR2I& aCenter, const VECTOR2I& aStart,
                                        const VECTOR2I& aMid, const VECTOR2I& aEnd )
{
    const int c_snapEpsilon_sq = 4;

    VECTOR2I m = ( aStart / 2 + aEnd / 2 );
    VECTOR2I perp = ( aEnd - aStart ).Perpendicular().Resize( INT_MAX / 2 );

    SEG legal( m - perp, m + perp );

    const SEG testSegments[] = {
        SEG( aCenter, aCenter + VECTOR2( 1, 0 ) ),
        SEG( aCenter, aCenter + VECTOR2( 0, 1 ) ),
    };

    std::vector<VECTOR2I> points = { legal.A, legal.B };

    for( const SEG& seg : testSegments )
    {
        OPT_VECTOR2I vec = legal.IntersectLines( seg );

        if( vec && legal.SquaredDistance( *vec ) <= c_snapEpsilon_sq )
            points.push_back( *vec );
    }

    OPT_VECTOR2I nearest;
    SEG::ecoord  min_d_sq = VECTOR2I::ECOORD_MAX;

    // Snap by distance between cursor and intersections
    for( const VECTOR2I& pt : points )
    {
        SEG::ecoord d_sq = ( pt - aCenter ).SquaredEuclideanNorm();

        if( d_sq < min_d_sq - c_snapEpsilon_sq )
        {
            min_d_sq = d_sq;
            nearest = pt;
        }
    }

    if( nearest )
        aArc.SetCenter( *nearest );
}


/**
 * Move an end point of the arc around the circumference.
 */
static void editArcEndpointKeepCenter( EDA_SHAPE& aArc, const VECTOR2I& aCenter, const VECTOR2I& aStart,
                                       const VECTOR2I& aMid, const VECTOR2I& aEnd, const VECTOR2I& aCursor )
{
    int  minRadius = EDA_UNIT_UTILS::Mils2IU( pcbIUScale, 1 );
    bool movingStart;

    VECTOR2I p1, p2, prev_p1;

    // user is moving p1, we want to move p2 to the new radius.

    if( aStart != aArc.GetStart() )
    {
        prev_p1 = aArc.GetStart();
        p1 = aStart;
        p2 = aEnd;
        movingStart = true;
    }
    else
    {
        prev_p1 = aArc.GetEnd();
        p1 = aEnd;
        p2 = aStart;
        movingStart = false;
    }

    p1 = p1 - aCenter;
    p2 = p2 - aCenter;

    if( p1.x == 0 && p1.y == 0 )
        p1 = prev_p1 - aCenter;

    if( p2.x == 0 && p2.y == 0 )
        p2 = { 1, 0 };

    double radius = p1.EuclideanNorm();

    if( radius < minRadius )
        radius = minRadius;

    p1 = aCenter + p1.Resize( KiROUND( radius ) );
    p2 = aCenter + p2.Resize( KiROUND( radius ) );

    aArc.SetCenter( aCenter );

    if( movingStart )
    {
        aArc.SetStart( p1 );
        aArc.SetEnd( p2 );
    }
    else
    {
        aArc.SetStart( p2 );
        aArc.SetEnd( p1 );
    }
}


static void editArcEndpointKeepCenterAndRadius( EDA_SHAPE& aArc, const VECTOR2I& aCenter, const VECTOR2I& aStart,
                                                const VECTOR2I& aMid, const VECTOR2I& aEnd )
{
    VECTOR2I p1;
    bool     movingStart = false;

    // User is moving p1, we need to update whichever end that is
    // The other end won't move.

    if( aStart != aArc.GetStart() )
    {
        p1 = aStart;
        movingStart = true;
    }
    else
    {
        p1 = aEnd;
        movingStart = false;
    }

    // Do not change the radius
    p1 = p1 - aCenter;
    p1 = aCenter + p1.Resize( aArc.GetRadius() );

    if( movingStart )
    {
        aArc.SetStart( p1 );
    }
    else
    {
        aArc.SetEnd( p1 );
    }
}


/**
 * Move the mid point of the arc, while keeping the two endpoints.
 */
static void editArcMidKeepCenter( EDA_SHAPE& aArc, const VECTOR2I& aCenter, const VECTOR2I& aStart,
                                  const VECTOR2I& aMid, const VECTOR2I& aEnd, const VECTOR2I& aCursor )
{
    int minRadius = EDA_UNIT_UTILS::Mils2IU( pcbIUScale, 1 );

    // Now, update the edit point position
    // Express the point in a circle-centered coordinate system.
    VECTOR2I start = aStart - aCenter;
    VECTOR2I end = aEnd - aCenter;

    double radius = ( aCursor - aCenter ).EuclideanNorm();

    if( radius < minRadius )
        radius = minRadius;

    start = start.Resize( KiROUND( radius ) );
    end = end.Resize( KiROUND( radius ) );

    start = start + aCenter;
    end = end + aCenter;

    aArc.SetStart( start );
    aArc.SetEnd( end );
}


/**
 * Move the mid point of the arc, while keeping the angle.
 */
static void editArcMidKeepEndpoints( EDA_SHAPE& aArc, const VECTOR2I& aStart, const VECTOR2I& aEnd,
                                     const VECTOR2I& aCursor )
{
    // Let 'm' be the middle point of the chord between the start and end points
    VECTOR2I m = ( aStart + aEnd ) / 2;

    // Legal midpoints lie on a vector starting just off the chord midpoint and extending out
    // past the existing midpoint.  We do not allow arc inflection while point editing.
    const int JUST_OFF = ( aStart - aEnd ).EuclideanNorm() / 100;
    VECTOR2I  v = (VECTOR2I) aArc.GetArcMid() - m;
    SEG       legal( m + v.Resize( JUST_OFF ), m + v.Resize( INT_MAX / 2 ) );
    VECTOR2I  mid = legal.NearestPoint( aCursor );

    aArc.SetArcGeometry( aStart, mid, aEnd );
}


EDA_ARC_POINT_EDIT_BEHAVIOR::EDA_ARC_POINT_EDIT_BEHAVIOR( EDA_SHAPE& aArc, const ARC_EDIT_MODE& aArcEditMode,
                                                          KIGFX::VIEW_CONTROLS& aViewContols ) :
        m_arc( aArc ),
        m_arcEditMode( aArcEditMode ),
        m_viewControls( aViewContols )
{
    wxASSERT( m_arc.GetShape() == SHAPE_T::ARC );
}


void EDA_ARC_POINT_EDIT_BEHAVIOR::MakePoints( EDIT_POINTS& aPoints )
{
    aPoints.AddPoint( m_arc.GetStart() );
    aPoints.AddPoint( m_arc.GetArcMid() );
    aPoints.AddPoint( m_arc.GetEnd() );
    aPoints.AddPoint( m_arc.getCenter() );

    aPoints.AddIndicatorLine( aPoints.Point( ARC_CENTER ), aPoints.Point( ARC_START ) );
    aPoints.AddIndicatorLine( aPoints.Point( ARC_CENTER ), aPoints.Point( ARC_END ) );
}


bool EDA_ARC_POINT_EDIT_BEHAVIOR::UpdatePoints( EDIT_POINTS& aPoints )
{
    wxCHECK( aPoints.PointsSize() == 4, false );

    aPoints.Point( ARC_START ).SetPosition( m_arc.GetStart() );
    aPoints.Point( ARC_MID ).SetPosition( m_arc.GetArcMid() );
    aPoints.Point( ARC_END ).SetPosition( m_arc.GetEnd() );
    aPoints.Point( ARC_CENTER ).SetPosition( m_arc.getCenter() );
    return true;
}


void EDA_ARC_POINT_EDIT_BEHAVIOR::UpdateItem( const EDIT_POINT& aEditedPoint, EDIT_POINTS& aPoints, COMMIT& aCommit,
                                              std::vector<EDA_ITEM*>& aUpdatedItems )
{
    CHECK_POINT_COUNT( aPoints, 4 );

    VECTOR2I center = aPoints.Point( ARC_CENTER ).GetPosition();
    VECTOR2I mid = aPoints.Point( ARC_MID ).GetPosition();
    VECTOR2I start = aPoints.Point( ARC_START ).GetPosition();
    VECTOR2I end = aPoints.Point( ARC_END ).GetPosition();

    if( isModified( aEditedPoint, aPoints.Point( ARC_CENTER ) ) )
    {
        switch( m_arcEditMode )
        {
        case ARC_EDIT_MODE::KEEP_ENDPOINTS_OR_START_DIRECTION:
            editArcCenterKeepEndpoints( m_arc, center, start, mid, end );
            break;

        case ARC_EDIT_MODE::KEEP_CENTER_ADJUST_ANGLE_RADIUS:
        case ARC_EDIT_MODE::KEEP_CENTER_ENDS_ADJUST_ANGLE:
        {
            // Both these modes just move the arc
            VECTOR2I moveVector = center - m_arc.getCenter();

            m_arc.SetArcGeometry( m_arc.GetStart() + moveVector, m_arc.GetArcMid() + moveVector,
                                  m_arc.GetEnd() + moveVector );
            break;
        }
        }
    }
    else if( isModified( aEditedPoint, aPoints.Point( ARC_MID ) ) )
    {
        const VECTOR2I& cursorPos = m_viewControls.GetCursorPosition( false );

        switch( m_arcEditMode )
        {
        case ARC_EDIT_MODE::KEEP_ENDPOINTS_OR_START_DIRECTION:
            editArcMidKeepEndpoints( m_arc, start, end, cursorPos );
            break;
        case ARC_EDIT_MODE::KEEP_CENTER_ENDS_ADJUST_ANGLE:
        case ARC_EDIT_MODE::KEEP_CENTER_ADJUST_ANGLE_RADIUS:
            editArcMidKeepCenter( m_arc, center, start, mid, end, cursorPos );
            break;
        }
    }
    else if( isModified( aEditedPoint, aPoints.Point( ARC_START ) )
             || isModified( aEditedPoint, aPoints.Point( ARC_END ) ) )
    {
        const VECTOR2I& cursorPos = m_viewControls.GetCursorPosition();

        switch( m_arcEditMode )
        {
        case ARC_EDIT_MODE::KEEP_CENTER_ADJUST_ANGLE_RADIUS:
            editArcEndpointKeepCenter( m_arc, center, start, mid, end, cursorPos );
            break;
        case ARC_EDIT_MODE::KEEP_CENTER_ENDS_ADJUST_ANGLE:
            editArcEndpointKeepCenterAndRadius( m_arc, center, start, mid, end );
            break;
        case ARC_EDIT_MODE::KEEP_ENDPOINTS_OR_START_DIRECTION:
            editArcEndpointKeepTangent( m_arc, center, start, mid, end, cursorPos );
            break;
        }
    }
}


OPT_VECTOR2I EDA_ARC_POINT_EDIT_BEHAVIOR::Get45DegreeConstrainer( const EDIT_POINT& aEditedPoint,
                                                                  EDIT_POINTS&      aPoints ) const
{
    return aPoints.Point( ARC_CENTER ).GetPosition();
}


void EDA_TABLECELL_POINT_EDIT_BEHAVIOR::MakePoints( EDIT_POINTS& aPoints )
{
    aPoints.AddPoint( m_cell.GetEnd() - VECTOR2I( 0, m_cell.GetRectangleHeight() / 2 ) );
    aPoints.AddPoint( m_cell.GetEnd() - VECTOR2I( m_cell.GetRectangleWidth() / 2, 0 ) );
}


bool EDA_TABLECELL_POINT_EDIT_BEHAVIOR::UpdatePoints( EDIT_POINTS& aPoints )
{
    aPoints.Point( COL_WIDTH ).SetPosition( m_cell.GetEnd() - VECTOR2I( 0, m_cell.GetRectangleHeight() / 2 ) );
    aPoints.Point( ROW_HEIGHT ).SetPosition( m_cell.GetEnd() - VECTOR2I( m_cell.GetRectangleWidth() / 2, 0 ) );
    return true;
}


ARC_EDIT_MODE IncrementArcEditMode( ARC_EDIT_MODE aMode )
{
    switch( aMode )
    {
    case ARC_EDIT_MODE::KEEP_CENTER_ADJUST_ANGLE_RADIUS:
        return ARC_EDIT_MODE::KEEP_CENTER_ENDS_ADJUST_ANGLE;
    case ARC_EDIT_MODE::KEEP_CENTER_ENDS_ADJUST_ANGLE:
        return ARC_EDIT_MODE::KEEP_ENDPOINTS_OR_START_DIRECTION;
    case ARC_EDIT_MODE::KEEP_ENDPOINTS_OR_START_DIRECTION:
        return ARC_EDIT_MODE::KEEP_CENTER_ADJUST_ANGLE_RADIUS;
    default:
        wxFAIL_MSG( "Invalid arc edit mode" );
        return aMode;
    }
}
