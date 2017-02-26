/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2017 Kicad Developers, see AUTHORS.txt for contributors.
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

#include <preview_items/polygon_geom_manager.h>

#include <geometry/direction45.h>


POLYGON_GEOM_MANAGER::POLYGON_GEOM_MANAGER( CLIENT& aClient ):
    m_client( aClient ),
    m_leaderMode( LEADER_MODE::DIRECT )
{}


void POLYGON_GEOM_MANAGER::AddPoint( const VECTOR2I& aPt )
{
    // if this is the first point, make sure the client is happy
    // for us to continue
    if( !IsPolygonInProgress() && !m_client.OnFirstPoint() )
        return;

    if( m_leaderPts.size() > 1 )
    {
        // there are enough leader points - the next
        // locked-in point is the end of the first leader
        // segment
        m_lockedPoints.push_back( m_leaderPts[1] );
    }
    else
    {
        // no leader lines, directly add the cursor
        m_lockedPoints.push_back( aPt );
    }

    m_client.OnGeometryChange( *this );
}


void POLYGON_GEOM_MANAGER::SetFinished()
{
    m_client.OnComplete( *this );
}


void POLYGON_GEOM_MANAGER::SetLeaderMode( LEADER_MODE aMode )
{
    m_leaderMode = aMode;
}


void POLYGON_GEOM_MANAGER::SetCursorPosition( const VECTOR2I& aPos )
{
    updateLeaderPoints( aPos );
}


bool POLYGON_GEOM_MANAGER::IsPolygonInProgress() const
{
    return m_lockedPoints.size() > 0;
}


bool POLYGON_GEOM_MANAGER::NewPointClosesOutline( const VECTOR2I& aPt ) const
{
    return m_lockedPoints.size() && m_lockedPoints[0] == aPt;
}


void POLYGON_GEOM_MANAGER::DeleteLastCorner()
{
    if( m_lockedPoints.size() > 0 )
    {
        m_lockedPoints.pop_back();
    }

    // update the new last segment (was previously
    // locked in), reusing last constraints
    if( m_lockedPoints.size() > 0 )
    {
        updateLeaderPoints( m_leaderPts.back() );
    }

    m_client.OnGeometryChange( *this );
}


void POLYGON_GEOM_MANAGER::Reset()
{
    m_lockedPoints.clear();
    m_leaderPts.clear();

    m_client.OnGeometryChange( *this );
}


void POLYGON_GEOM_MANAGER::updateLeaderPoints( const VECTOR2I& aEndPoint )
{
    SHAPE_LINE_CHAIN newChain;

    if( m_leaderMode == LEADER_MODE::DEG45 )
    {
        // get a restricted 45/H/V line from the last fixed point to the cursor
        DIRECTION_45 direction( m_lockedPoints.back() - aEndPoint );
        newChain = direction.BuildInitialTrace( m_lockedPoints.back(), aEndPoint );

        // Can also add chain back to start, but this rearely produces
        // usable result
        //DIRECTION_45 directionToStart( aEndPoint - m_lockedPoints.front() );
        //newChain.Append( directionToStart.BuildInitialTrace( aEndPoint, m_lockedPoints.front() ) );
    }
    else
    {
        // direct segment
        newChain = SHAPE_LINE_CHAIN( m_lockedPoints.back(), aEndPoint );
    }

    // rebuild leader point list from the chain
    m_leaderPts.clear();

    for( int i = 0; i < newChain.PointCount(); ++i )
    {
        m_leaderPts.push_back( newChain.Point( i ) );
    }

    m_client.OnGeometryChange( *this );
}


const std::vector<VECTOR2I>& POLYGON_GEOM_MANAGER::GetLockedInPoints() const
{
    return m_lockedPoints;
}


const std::vector<VECTOR2I>& POLYGON_GEOM_MANAGER::GetLeaderLinePoints() const
{
    return m_leaderPts;
}
