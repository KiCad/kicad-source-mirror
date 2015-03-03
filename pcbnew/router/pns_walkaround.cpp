/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013-2014 CERN
 * Author: Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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

#include <boost/foreach.hpp>
#include <boost/optional.hpp>

#include <geometry/shape_line_chain.h>

#include "pns_walkaround.h"
#include "pns_optimizer.h"
#include "pns_utils.h"
#include "pns_router.h"
using boost::optional;

void PNS_WALKAROUND::start( const PNS_LINE& aInitialPath )
{
    m_iteration = 0;
    m_iterationLimit = 50;
}


PNS_NODE::OPT_OBSTACLE PNS_WALKAROUND::nearestObstacle( const PNS_LINE& aPath )
{
    return m_world->NearestObstacle( &aPath, m_itemMask );
}


PNS_WALKAROUND::WALKAROUND_STATUS PNS_WALKAROUND::singleStep( PNS_LINE& aPath,
                                                              bool aWindingDirection )
{
    optional<PNS_OBSTACLE>& current_obs =
        aWindingDirection ? m_currentObstacle[0] : m_currentObstacle[1];

    bool& prev_recursive = aWindingDirection ? m_recursiveCollision[0] : m_recursiveCollision[1];

    if( !current_obs )
        return DONE;

    SHAPE_LINE_CHAIN path_pre[2], path_walk[2], path_post[2];

    VECTOR2I last = aPath.CPoint( -1 );

    if( ( current_obs->m_hull ).PointInside( last ) || ( current_obs->m_hull ).PointOnEdge( last ) )
    {
        m_recursiveBlockageCount++;

        if( m_recursiveBlockageCount < 3 )
            aPath.Line().Append( current_obs->m_hull.NearestPoint( last ) );
        else
        {
            aPath = aPath.ClipToNearestObstacle( m_world );
            return DONE;
        }
    }

    aPath.Walkaround( current_obs->m_hull, path_pre[0], path_walk[0],
                      path_post[0], aWindingDirection );
    aPath.Walkaround( current_obs->m_hull, path_pre[1], path_walk[1],
                      path_post[1], !aWindingDirection );

#ifdef DEBUG
    m_logger.NewGroup( aWindingDirection ? "walk-cw" : "walk-ccw", m_iteration );
    m_logger.Log( &path_walk[0], 0, "path-walk" );
    m_logger.Log( &path_pre[0], 1, "path-pre" );
    m_logger.Log( &path_post[0], 4, "path-post" );
    m_logger.Log( &current_obs->m_hull, 2, "hull" );
    m_logger.Log( current_obs->m_item, 3, "item" );
#endif

    int len_pre = path_walk[0].Length();
    int len_alt = path_walk[1].Length();

    PNS_LINE walk_path( aPath, path_walk[1] );

    bool alt_collides = m_world->CheckColliding( &walk_path, m_itemMask );

    SHAPE_LINE_CHAIN pnew;

    if( !m_forceSingleDirection && len_alt < len_pre && !alt_collides && !prev_recursive )
    {
        pnew = path_pre[1];
        pnew.Append( path_walk[1] );
        pnew.Append( path_post[1] );

        if( !path_post[1].PointCount() || !path_walk[1].PointCount() )
            current_obs = nearestObstacle( PNS_LINE( aPath, path_pre[1] ) );
        else
            current_obs = nearestObstacle( PNS_LINE( aPath, path_post[1] ) );
        prev_recursive = false;
    }
    else
    {
        pnew = path_pre[0];
        pnew.Append( path_walk[0] );
        pnew.Append( path_post[0] );

        if( !path_post[0].PointCount() || !path_walk[0].PointCount() )
            current_obs = nearestObstacle( PNS_LINE( aPath, path_pre[0] ) );
        else
            current_obs = nearestObstacle( PNS_LINE( aPath, path_walk[0] ) );

        if( !current_obs )
        {
            prev_recursive = false;
            current_obs = nearestObstacle( PNS_LINE( aPath, path_post[0] ) );
        }
        else
            prev_recursive = true;
    }

    pnew.Simplify();
    aPath.SetShape( pnew );

    return IN_PROGRESS;
}


PNS_WALKAROUND::WALKAROUND_STATUS PNS_WALKAROUND::Route( const PNS_LINE& aInitialPath,
        PNS_LINE& aWalkPath, bool aOptimize )
{
    PNS_LINE path_cw( aInitialPath ), path_ccw( aInitialPath );
    WALKAROUND_STATUS s_cw = IN_PROGRESS, s_ccw = IN_PROGRESS;
    SHAPE_LINE_CHAIN best_path;

    start( aInitialPath );

    m_currentObstacle[0] = m_currentObstacle[1] = nearestObstacle( aInitialPath );
    m_recursiveBlockageCount = 0;

    aWalkPath = aInitialPath;

    if( m_forceWinding )
    {
        s_cw = m_forceCw ? IN_PROGRESS : STUCK;
        s_ccw = m_forceCw ? STUCK : IN_PROGRESS;
        m_forceSingleDirection = true;
    } else {
        m_forceSingleDirection = false;
    }

    while( m_iteration < m_iterationLimit )
    {
        if( s_cw != STUCK )
            s_cw = singleStep( path_cw, true );

        if( s_ccw != STUCK )
            s_ccw = singleStep( path_ccw, false );

        if( ( s_cw == DONE && s_ccw == DONE ) || ( s_cw == STUCK && s_ccw == STUCK ) )
        {
            int len_cw  = path_cw.CLine().Length();
            int len_ccw = path_ccw.CLine().Length();

            if( m_forceLongerPath )
                aWalkPath = ( len_cw > len_ccw ? path_cw : path_ccw );
            else
                aWalkPath = ( len_cw < len_ccw ? path_cw : path_ccw );

            break;
        }
        else if( s_cw == DONE && !m_forceLongerPath )
        {
            aWalkPath = path_cw;
            break;
        }
        else if( s_ccw == DONE && !m_forceLongerPath )
        {
            aWalkPath = path_ccw;
            break;
        }

        m_iteration++;
    }

    if( m_iteration == m_iterationLimit )
    {
        int len_cw  = path_cw.CLine().Length();
        int len_ccw = path_ccw.CLine().Length();

        if( m_forceLongerPath )
            aWalkPath = ( len_cw > len_ccw ? path_cw : path_ccw );
        else
            aWalkPath = ( len_cw < len_ccw ? path_cw : path_ccw );
    }

    if( m_cursorApproachMode )
    {
        // int len_cw = path_cw.GetCLine().Length();
        // int len_ccw = path_ccw.GetCLine().Length();
        bool found = false;

        SHAPE_LINE_CHAIN l = aWalkPath.CLine();

        for( int i = 0; i < l.SegmentCount(); i++ )
        {
            const SEG s = l.Segment( i );

            VECTOR2I nearest = s.NearestPoint( m_cursorPos );
            VECTOR2I::extended_type dist_a = ( s.A - m_cursorPos ).SquaredEuclideanNorm();
            VECTOR2I::extended_type dist_b = ( s.B - m_cursorPos ).SquaredEuclideanNorm();
            VECTOR2I::extended_type dist_n = ( nearest - m_cursorPos ).SquaredEuclideanNorm();

            if( dist_n <= dist_a && dist_n < dist_b )
            {
                l.Remove( i + 1, -1 );
                l.Append( nearest );
                l.Simplify();
                found = true;
                break;
            }
        }

        if( found )
        {
            aWalkPath = aInitialPath;
            aWalkPath.SetShape( l );
        }
    }

    aWalkPath.Line().Simplify();

    if( aWalkPath.SegmentCount() < 1 )
        return STUCK;
    if( aWalkPath.CPoint( -1 ) != aInitialPath.CPoint( -1 ) )
        return STUCK;
    if( aWalkPath.CPoint( 0 ) != aInitialPath.CPoint( 0 ) )
        return STUCK;

    WALKAROUND_STATUS st = s_ccw == DONE || s_cw == DONE ? DONE : STUCK;

    if( st == DONE )
    {
        if( aOptimize )
            PNS_OPTIMIZER::Optimize( &aWalkPath, PNS_OPTIMIZER::MERGE_OBTUSE, m_world );
    }

    return st;
}
