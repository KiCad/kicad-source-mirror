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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <preview_items/arc_geom_manager.h>

#include <math/util.h>      // for KiROUND
#include <geometry/eda_angle.h>
#include <trigo.h>

using namespace KIGFX::PREVIEW;


/// Snap an angle to the nearest 45 degrees
static EDA_ANGLE snapAngle( const EDA_ANGLE& aAngle )
{
    return ANGLE_45 * KiROUND( aAngle / ANGLE_45 );
}


bool ARC_GEOM_MANAGER::acceptPoint( const VECTOR2I& aPt )
{
    if( m_mode != ARC_DRAW_MODE::CENTER_START_END )
        return acceptExactPoint( aPt );

    switch( getStep() )
    {
    case FIRST_POINT:  return setOrigin( aPt );
    case SECOND_POINT: return setStart( aPt );
    case THIRD_POINT:  return setEnd( aPt );
    case COMPLETE:     return false;
    }

    return false;
}


bool ARC_GEOM_MANAGER::acceptExactPoint( const VECTOR2I& aPt )
{
    if( IsComplete() )
        return false;

    const int index = GetPointIndex();

    m_pts[index] = aPt;

    // The points before the last only draw guides
    if( index < 2 && !HasTangentSeed() )
    {
        clearSolution();

        if( index == 0 )
            return true;

        return UsesDirectionClick() ? setDirection( aPt ) : aPt != m_pts[0];
    }

    if( HasTangentSeed() && m_seedIsAxis )
    {
        // The nearer of the two directions along the axis gives the sweep up to a half turn
        const VECTOR2D chord = VECTOR2D( aPt ) - VECTOR2D( m_pts[0] );
        const double   sign = ( chord.Dot( m_axis ) >= 0.0 ) != m_axisFlipped ? 1.0 : -1.0;

        m_dir = m_axis * sign;
    }

    KIGEOM::ARC_SOLUTION solution;

    switch( m_mode )
    {
    case ARC_DRAW_MODE::START_END_MID:
        solution = KIGEOM::ArcFromStartEndMidDrag( m_pts[0], m_pts[1], aPt );
        break;

    case ARC_DRAW_MODE::START_END_CENTER:
        solution = KIGEOM::ArcFromStartEndCenterDrag( m_pts[0], m_pts[1], aPt, m_major );
        break;

    case ARC_DRAW_MODE::TANGENT:
    case ARC_DRAW_MODE::START_DIR_END:
        solution = KIGEOM::ArcFromStartTangentEnd( m_pts[0], m_dir, aPt );
        break;

    case ARC_DRAW_MODE::CENTER_START_END:
        break;
    }

    if( solution.valid )
        applySolution( solution );

    return solution.valid;
}


void ARC_GEOM_MANAGER::applySolution( const KIGEOM::ARC_SOLUTION& aSolution )
{
    m_solution = aSolution;

    const VECTOR2D start = VECTOR2D( aSolution.start ) - aSolution.center;
    const VECTOR2D end = VECTOR2D( aSolution.end ) - aSolution.center;
    const VECTOR2D mid = VECTOR2D( aSolution.mid ) - aSolution.center;

    m_origin = VECTOR2I( KiROUND( aSolution.center.x, true ), KiROUND( aSolution.center.y, true ) );
    m_radius = aSolution.radius;
    m_startAngle = EDA_ANGLE( start ).Normalize();
    m_endAngle = EDA_ANGLE( end ).Normalize();

    // Raw angles grow toward the start to mid side unless the cross product is negative
    m_clockwise = start.Cross( mid ) < 0.0;
}


void ARC_GEOM_MANAGER::clearSolution()
{
    m_solution = KIGEOM::ARC_SOLUTION();
    m_origin = VECTOR2I();
    m_radius = 0.0;
    m_startAngle = ANGLE_0;
    m_endAngle = ANGLE_0;
    m_clockwise = true;
}


bool ARC_GEOM_MANAGER::setDirection( const VECTOR2I& aPt )
{
    const VECTOR2D vec = VECTOR2D( aPt ) - VECTOR2D( m_pts[0] );
    const double   length = vec.EuclideanNorm();

    if( length == 0.0 )
        return false;

    m_dir = vec;

    if( m_angleSnap )
    {
        // Table lookup keeps snapped axis directions free of trigonometric rounding
        static const double   d = M_SQRT1_2;
        static const VECTOR2D units[8] = { { 1, 0 },  { d, d },   { 0, 1 },  { -d, d },
                                           { -1, 0 }, { -d, -d }, { 0, -1 }, { d, -d } };

        const int octant = ( KiROUND( EDA_ANGLE( vec ) / ANGLE_45 ) % 8 + 8 ) % 8;

        m_dir = units[octant] * length;
    }

    return true;
}


void ARC_GEOM_MANAGER::Reset()
{
    MULTISTEP_GEOM_MANAGER::Reset();

    // The manager is reused for every arc in a chain, so the next arc must not inherit the
    // previous arc's direction, its locked-in direction choice, or its geometry
    m_clockwise = true;
    m_directionLocked = false;
    m_angleSnap = false;
    m_origin = VECTOR2I();
    m_radius = 0.0;
    m_startAngle = ANGLE_0;
    m_endAngle = ANGLE_0;
    m_pts = {};
    m_dir = VECTOR2D();
    m_hasSeed = false;
    m_seedIsAxis = false;
    m_axisFlipped = false;
    m_axis = VECTOR2D();
    m_major = false;
    m_solution = KIGEOM::ARC_SOLUTION();
}


void ARC_GEOM_MANAGER::SetMode( ARC_DRAW_MODE aMode )
{
    if( aMode == m_mode )
        return;

    if( IsReset() )
    {
        // Nothing is locked, so a seed set beforehand stays available
        m_mode = aMode;
        clearSolution();
        setGeometryChanged();
        return;
    }

    // Only the first point of a start-first mode carries over, and only into another start-first mode
    const bool keepStart = m_mode != ARC_DRAW_MODE::CENTER_START_END && aMode != ARC_DRAW_MODE::CENTER_START_END;

    const VECTOR2I start = m_pts[0];
    const VECTOR2I cursor = GetLastPoint();
    const bool     snap = m_angleSnap;

    m_mode = aMode;
    Reset();

    if( !keepStart )
        return;

    m_angleSnap = snap;
    AddPoint( start, true );
    AddPoint( cursor, false );
}


void ARC_GEOM_MANAGER::SetTangentSeed( const VECTOR2I& aStart, const VECTOR2D& aDirection, bool aDirectionIsAxis )
{
    if( !IsReset() || ( aDirection.x == 0.0 && aDirection.y == 0.0 ) )
        return;

    m_pts[0] = aStart;
    m_dir = aDirection;
    m_axis = aDirection;
    m_seedIsAxis = aDirectionIsAxis;
    m_axisFlipped = false;
    m_hasSeed = true;
    clearSolution();
    setGeometryChanged();
}


void ARC_GEOM_MANAGER::ToggleClockwise()
{
    switch( m_mode )
    {
    case ARC_DRAW_MODE::CENTER_START_END:
        m_clockwise = !m_clockwise;
        m_directionLocked = true;
        setGeometryChanged();
        break;

    case ARC_DRAW_MODE::START_END_CENTER:
        m_major = !m_major;

        // Rebuild the arc under the cursor with the other posture
        if( getStep() == THIRD_POINT )
            acceptPoint( GetLastPoint() );

        setGeometryChanged();
        break;

    case ARC_DRAW_MODE::TANGENT:
        if( HasTangentSeed() && m_seedIsAxis )
        {
            m_axisFlipped = !m_axisFlipped;
            acceptPoint( GetLastPoint() );
            setGeometryChanged();
        }

        break;

    default:
        break;
    }
}


VECTOR2I ARC_GEOM_MANAGER::GetOrigin() const
{
    if( UsesExactEndpoints() && !m_solution.valid )
        return m_pts[0];

    return m_origin;
}


VECTOR2I ARC_GEOM_MANAGER::GetStartRadiusEnd() const
{
    if( UsesExactEndpoints() )
        return m_pts[0];

    VECTOR2I vec( static_cast<int>( m_radius ), 0 );
    RotatePoint( vec, -m_startAngle );
    return m_origin +vec;
}


VECTOR2I ARC_GEOM_MANAGER::GetEndRadiusEnd() const
{
    if( UsesExactEndpoints() )
        return m_solution.valid ? m_solution.end : m_pts[0];

    VECTOR2I vec( static_cast<int>( m_radius ), 0 );
    RotatePoint( vec, -m_endAngle );
    return m_origin + vec;
}


double ARC_GEOM_MANAGER::GetRadius() const
{
    return m_radius;
}


EDA_ANGLE ARC_GEOM_MANAGER::GetStartAngle() const
{
    EDA_ANGLE angle = m_startAngle;

    if( m_clockwise )
        angle -= ANGLE_360;

    return -angle;
}


EDA_ANGLE ARC_GEOM_MANAGER::GetSubtended() const
{
    EDA_ANGLE angle = m_endAngle - m_startAngle;

    if( m_endAngle <= m_startAngle )
        angle += ANGLE_360;

    if( m_clockwise )
        angle -= ANGLE_360;

    return -angle;
}


bool ARC_GEOM_MANAGER::setOrigin( const VECTOR2I& aOrigin )
{
    m_origin     = aOrigin;
    m_startAngle = ANGLE_0;
    m_endAngle   = ANGLE_0;

    return true;
}


bool ARC_GEOM_MANAGER::setStart( const VECTOR2I& aEnd )
{
    const VECTOR2I radVec = aEnd - m_origin;

    m_radius = radVec.EuclideanNorm();
    m_startAngle = EDA_ANGLE( radVec );

    if( m_angleSnap )
        m_startAngle = snapAngle( m_startAngle );

    // normalise to 0..360
    while( m_startAngle < ANGLE_0 )
        m_startAngle += ANGLE_360;

    m_endAngle = m_startAngle;

    return m_radius != 0.0;
}


bool ARC_GEOM_MANAGER::setEnd( const VECTOR2I& aCursor )
{
    const VECTOR2I radVec = aCursor - m_origin;

    m_endAngle = EDA_ANGLE( radVec );

    if( m_angleSnap )
        m_endAngle = snapAngle( m_endAngle );

    // normalise to 0..360
    while( m_endAngle < ANGLE_0 )
        m_endAngle += ANGLE_360;

    if( !m_directionLocked )
    {
        EDA_ANGLE ccwAngle = m_endAngle - m_startAngle;

        if( m_endAngle <= m_startAngle )
            ccwAngle += ANGLE_360;

        EDA_ANGLE cwAngle = std::abs( ccwAngle - ANGLE_360 );

        if( std::min( ccwAngle, cwAngle ) >= ANGLE_90 )
            m_directionLocked = true;
        else
            m_clockwise = cwAngle < ccwAngle;
    }
    else if( std::abs( GetSubtended() ) < ANGLE_90 )
    {
        m_directionLocked = false;
    }

    // if the end is the same as the start, this is a bad point
    return m_endAngle != m_startAngle;
}
