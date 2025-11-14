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
#include <limits>
#include <vector>

#include <preview_items/polygon_geom_manager.h>

#include <geometry/geometry_utils.h>
#include <geometry/shape_line_chain.h>


POLYGON_GEOM_MANAGER::POLYGON_GEOM_MANAGER( CLIENT& aClient ):
    m_client( aClient ),
    m_leaderMode( LEADER_MODE::DIRECT ),
    m_intersectionsAllowed( true )
{}


bool POLYGON_GEOM_MANAGER::AddPoint( const VECTOR2I& aPt )
{
    // if this is the first point, make sure the client is happy
    // for us to continue
    if( !IsPolygonInProgress() && !m_client.OnFirstPoint( *this ) )
        return false;

    if( m_leaderPts.PointCount() > 1 )
    {
        // there are enough leader points - the next
        // locked-in point is the end of the last leader
        // segment
        m_lockedPoints.Append( m_leaderPts.CPoints()[ m_leaderPts.PointCount() - 2 ] );
        m_lockedPoints.Append( m_leaderPts.CLastPoint() );
    }
    else
    {
        // no leader lines, directly add the cursor
        m_lockedPoints.Append( aPt );
    }

    // check for self-intersections
    if( !m_intersectionsAllowed && IsSelfIntersecting( false ) )
    {
        m_lockedPoints.Remove( m_lockedPoints.PointCount() - 1 );
        return false;
    }

    if( m_lockedPoints.PointCount() > 0 )
        updateTemporaryLines( aPt );

    m_client.OnGeometryChange( *this );
    return true;
}


void POLYGON_GEOM_MANAGER::SetFinished()
{

    m_client.OnComplete( *this );
}


void POLYGON_GEOM_MANAGER::SetLeaderMode( LEADER_MODE aMode )
{
    m_leaderMode = aMode;
}


bool POLYGON_GEOM_MANAGER::IsSelfIntersecting( bool aIncludeLeaderPts ) const
{
    SHAPE_LINE_CHAIN pts( m_lockedPoints );

    if( aIncludeLeaderPts )
    {
        for( int i = 0; i < m_leaderPts.PointCount(); ++i )
        {
            if( m_leaderPts.CPoint( i ) != pts.CPoint( 0 ) )
                pts.Append( m_leaderPts.CPoint( i ) );
        }
    }

    // line chain needs to be set as closed for proper checks
    pts.SetClosed( true );

    return !!pts.SelfIntersecting();
}


void POLYGON_GEOM_MANAGER::SetCursorPosition( const VECTOR2I& aPos )
{
    if( m_lockedPoints.PointCount() > 0 )
        updateTemporaryLines( aPos );
}


bool POLYGON_GEOM_MANAGER::IsPolygonInProgress() const
{
    return m_lockedPoints.PointCount() > 0;
}


int POLYGON_GEOM_MANAGER::PolygonPointCount() const
{
    return m_lockedPoints.PointCount();
}


bool POLYGON_GEOM_MANAGER::NewPointClosesOutline( const VECTOR2I& aPt ) const
{
    return m_lockedPoints.PointCount() > 0 && m_lockedPoints.CPoint( 0 ) == aPt;
}


std::optional<VECTOR2I> POLYGON_GEOM_MANAGER::DeleteLastCorner()
{
    std::optional<VECTOR2I> last;

    if( m_lockedPoints.PointCount() > 0 )
    {
        last = m_lockedPoints.GetPoint( m_lockedPoints.PointCount() - 1 );
        m_lockedPoints.Remove( m_lockedPoints.PointCount() - 1 );
    }

    // update the new last segment (was previously
    // locked in), reusing last constraints
    if( m_lockedPoints.PointCount() > 0 )
        updateTemporaryLines( m_leaderPts.CLastPoint() );

    m_client.OnGeometryChange( *this );
    return last;
}


void POLYGON_GEOM_MANAGER::Reset()
{
    m_lockedPoints.Clear();
    m_leaderPts.Clear();
    m_loopPts.Clear();

    m_client.OnGeometryChange( *this );
}


static SHAPE_LINE_CHAIN build45DegLeader( const VECTOR2I& aEndPoint, const SHAPE_LINE_CHAIN& aLastPoints )
{
    if( aLastPoints.PointCount() < 1 )
        return SHAPE_LINE_CHAIN();

    const VECTOR2I lastPt = aLastPoints.CLastPoint();
    const VECTOR2D endpointD = aEndPoint;
    const VECTOR2D lineVec = endpointD - lastPt;

    if( aLastPoints.SegmentCount() < 1 )
        return SHAPE_LINE_CHAIN(
                std::vector<VECTOR2I>{ lastPt, lastPt + GetVectorSnapped45( lineVec ) } );

    EDA_ANGLE lineA( lineVec );
    EDA_ANGLE prevA( GetVectorSnapped45( lastPt - aLastPoints.CPoints()[ aLastPoints.PointCount() - 2 ] ) );

    bool vertical = std::abs( lineVec.y ) > std::abs( lineVec.x );
    bool horizontal = std::abs( lineVec.y ) < std::abs( lineVec.x );

    double angDiff = std::abs( ( lineA - prevA ).Normalize180().AsDegrees() );

    bool bendEnd = ( angDiff < 45 ) || ( angDiff > 90 && angDiff < 135 );

    if( prevA.Normalize90() == ANGLE_45 || prevA.Normalize90() == -ANGLE_45 )
        bendEnd = !bendEnd;

    VECTOR2D mid = endpointD;

    if( bendEnd )
    {
        if( vertical )
        {
            if( lineVec.y > 0 )
                mid = VECTOR2D( lastPt.x, endpointD.y - std::abs( lineVec.x ) );
            else
                mid = VECTOR2D( lastPt.x, endpointD.y + std::abs( lineVec.x ) );
        }
        else if( horizontal )
        {
            if( lineVec.x > 0 )
                mid = VECTOR2D( endpointD.x - std::abs( lineVec.y ), lastPt.y );
            else
                mid = VECTOR2D( endpointD.x + std::abs( lineVec.y ), lastPt.y );
        }
    }
    else
    {
        if( vertical )
        {
            if( lineVec.y > 0 )
                mid = VECTOR2D( endpointD.x, lastPt.y + std::abs( lineVec.x ) );
            else
                mid = VECTOR2D( endpointD.x, lastPt.y - std::abs( lineVec.x ) );
        }
        else if( horizontal )
        {
            if( lineVec.x > 0 )
                mid = VECTOR2D( lastPt.x + std::abs( lineVec.y ), endpointD.y );
            else
                mid = VECTOR2D( lastPt.x - std::abs( lineVec.y ), endpointD.y );
        }
    }

    const VECTOR2I midInt = KiROUND( mid.x, mid.y );

    return SHAPE_LINE_CHAIN( std::vector<VECTOR2I>{ lastPt, midInt, aEndPoint } );
}

static SHAPE_LINE_CHAIN build90DegLeader( const VECTOR2I& aEndPoint, const SHAPE_LINE_CHAIN& aLastPoints )
{
    if( aLastPoints.PointCount() < 1 )
        return SHAPE_LINE_CHAIN();

    const VECTOR2I lastPt = aLastPoints.CLastPoint();

    if( lastPt.x == aEndPoint.x || lastPt.y == aEndPoint.y )
        return SHAPE_LINE_CHAIN( std::vector<VECTOR2I>{ lastPt, aEndPoint } );

    VECTOR2I mid( aEndPoint.x, lastPt.y );
    return SHAPE_LINE_CHAIN( std::vector<VECTOR2I>{ lastPt, mid, aEndPoint } );
}


void POLYGON_GEOM_MANAGER::updateTemporaryLines( const VECTOR2I& aEndPoint, LEADER_MODE aModifier )
{
    wxCHECK( m_lockedPoints.PointCount() > 0, /*void*/ );
    const VECTOR2I& last_pt = m_lockedPoints.CLastPoint();

    if( m_leaderMode == LEADER_MODE::DEG45 || aModifier == LEADER_MODE::DEG45 )
    {
        if( m_lockedPoints.PointCount() > 0 )
        {
            m_leaderPts = build45DegLeader( aEndPoint, m_lockedPoints );
            m_loopPts = build45DegLeader( aEndPoint, m_lockedPoints.Reverse() ).Reverse();
        }
    }
    else if( m_leaderMode == LEADER_MODE::DEG90 || aModifier == LEADER_MODE::DEG90 )
    {
        if( m_lockedPoints.PointCount() > 0 )
        {
            m_leaderPts = build90DegLeader( aEndPoint, m_lockedPoints );
            m_loopPts = build90DegLeader( aEndPoint, m_lockedPoints.Reverse() ).Reverse();
        }
    }
    else
    {
        // direct segment
        m_leaderPts = SHAPE_LINE_CHAIN( { last_pt, aEndPoint } );
        m_loopPts.Clear();
    }

    m_client.OnGeometryChange( *this );
}
