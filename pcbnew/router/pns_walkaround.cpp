/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013-2014 CERN
 * Copyright (C) 2016-2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <core/optional.h>

#include <geometry/shape_line_chain.h>

#include "pns_walkaround.h"
#include "pns_optimizer.h"
#include "pns_utils.h"
#include "pns_router.h"
#include "pns_debug_decorator.h"

namespace PNS {

void WALKAROUND::start( const LINE& aInitialPath )
{
    m_iteration = 0;
    m_iterationLimit = 50;
}


NODE::OPT_OBSTACLE WALKAROUND::nearestObstacle( const LINE& aPath )
{
    NODE::OPT_OBSTACLE obs = m_world->NearestObstacle( &aPath, m_itemMask, m_restrictedSet.empty() ? NULL : &m_restrictedSet );

    if( m_restrictedSet.empty() )
        return obs;

    else if( obs && m_restrictedSet.find ( obs->m_item ) != m_restrictedSet.end() )
        return obs;

    return NODE::OPT_OBSTACLE();
}


WALKAROUND::WALKAROUND_STATUS WALKAROUND::singleStep( LINE& aPath, bool aWindingDirection )
{
    OPT<OBSTACLE>& current_obs =
        aWindingDirection ? m_currentObstacle[0] : m_currentObstacle[1];

    if( !current_obs )
        return DONE;

    VECTOR2I initialLast = aPath.CPoint( -1 );

    SHAPE_LINE_CHAIN path_walk;

    bool s_cw = aPath.Walkaround( current_obs->m_hull, path_walk, aWindingDirection );

    PNS_DBG( Dbg(), BeginGroup, "hull/walk" );
    char name[128];
    snprintf(name, sizeof(name), "hull-%s-%d", aWindingDirection ? "cw" : "ccw", m_iteration );
    PNS_DBG( Dbg(), AddLine, current_obs->m_hull, RED, 1, name);
    snprintf(name, sizeof(name), "path-%s-%d", aWindingDirection ? "cw" : "ccw", m_iteration );
    PNS_DBG( Dbg(), AddLine, aPath.CLine(), GREEN, 1, name );
    snprintf(name, sizeof(name), "result-%s-%d", aWindingDirection ? "cw" : "ccw", m_iteration );
    PNS_DBG( Dbg(), AddLine, path_walk, BLUE, 10000, name );
    PNS_DBG( Dbg(), Message, wxString::Format( "Stat cw %d", !!s_cw ) );
    PNS_DBGN( Dbg(), EndGroup );

    path_walk.Simplify();
    aPath.SetShape( path_walk );

    // If the end of the line is inside an obstacle, additional walkaround iterations are not
    // going to help.  Also, if one walkaround step didn't produce a new last point, additional
    // steps won't either. Exit now to prevent pegging the iteration limiter and causing lag.
    if( initialLast == path_walk.CLastPoint() ||
        ( current_obs && current_obs->m_hull.PointInside( initialLast ) &&
         !current_obs->m_hull.PointOnEdge( initialLast ) ) )
    {
        return ALMOST_DONE;
    }

    current_obs = nearestObstacle( LINE( aPath, path_walk ) );

    return IN_PROGRESS;
}


const WALKAROUND::RESULT WALKAROUND::Route( const LINE& aInitialPath )
{
    LINE path_cw( aInitialPath ), path_ccw( aInitialPath );
    WALKAROUND_STATUS s_cw = IN_PROGRESS, s_ccw = IN_PROGRESS;
    SHAPE_LINE_CHAIN best_path;
    RESULT result;

    // special case for via-in-the-middle-of-track placement
    if( aInitialPath.PointCount() <= 1 )
    {
        if( aInitialPath.EndsWithVia() && m_world->CheckColliding( &aInitialPath.Via(), m_itemMask ) )
            return RESULT( STUCK, STUCK );

        return RESULT( DONE, DONE, aInitialPath, aInitialPath );
    }

    start( aInitialPath );

    m_currentObstacle[0] = m_currentObstacle[1] = nearestObstacle( aInitialPath );
    m_recursiveBlockageCount = 0;

    result.lineCw = aInitialPath;
    result.lineCcw = aInitialPath;

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
        if( s_cw != STUCK && s_cw != ALMOST_DONE )
            s_cw = singleStep( path_cw, true );

        if( s_ccw != STUCK && s_ccw != ALMOST_DONE )
            s_ccw = singleStep( path_ccw, false );

        if( s_cw != IN_PROGRESS )
        {
            result.lineCw = path_cw;
            result.statusCw = s_cw;
        }

        if( s_ccw != IN_PROGRESS )
        {
            result.lineCcw = path_ccw;
            result.statusCcw = s_ccw;
        }

        if( s_cw != IN_PROGRESS && s_ccw != IN_PROGRESS )
            break;

        m_iteration++;
    }

    if( s_cw == IN_PROGRESS )
    {
        result.lineCw = path_cw;
        result.statusCw = ALMOST_DONE;
    }

    if( s_ccw == IN_PROGRESS )
    {
        result.lineCcw = path_ccw;
        result.statusCcw = ALMOST_DONE;
    }

    if( result.lineCw.SegmentCount() < 1 || result.lineCw.CPoint( 0 ) != aInitialPath.CPoint( 0 ) )
    {
        result.statusCw = STUCK;
    }

    if( result.lineCw.PointCount() > 0 && result.lineCw.CPoint( -1 ) != aInitialPath.CPoint( -1 ) )
    {
        result.statusCw = ALMOST_DONE;
    }

    if( result.lineCcw.SegmentCount() < 1 || result.lineCcw.CPoint( 0 ) != aInitialPath.CPoint( 0 ) )
    {
        result.statusCcw = STUCK;
    }

    if( result.lineCcw.PointCount() > 0 && result.lineCcw.CPoint( -1 ) != aInitialPath.CPoint( -1 ) )
    {
        result.statusCcw = ALMOST_DONE;
    }

    return result;
}



WALKAROUND::WALKAROUND_STATUS WALKAROUND::Route( const LINE& aInitialPath,
        LINE& aWalkPath, bool aOptimize )
{
    LINE path_cw( aInitialPath ), path_ccw( aInitialPath );
    WALKAROUND_STATUS s_cw = IN_PROGRESS, s_ccw = IN_PROGRESS;
    SHAPE_LINE_CHAIN best_path;

    // special case for via-in-the-middle-of-track placement
    if( aInitialPath.PointCount() <= 1 )
    {
        if( aInitialPath.EndsWithVia() && m_world->CheckColliding( &aInitialPath.Via(), m_itemMask ) )
            return STUCK;

        aWalkPath = aInitialPath;
        return DONE;
    }

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

    aWalkPath.Line().Simplify();

    if( aWalkPath.SegmentCount() < 1 )
        return STUCK;
    if( aWalkPath.CPoint( -1 ) != aInitialPath.CPoint( -1 ) )
        return ALMOST_DONE;
    if( aWalkPath.CPoint( 0 ) != aInitialPath.CPoint( 0 ) )
        return STUCK;

    WALKAROUND_STATUS st = s_ccw == DONE || s_cw == DONE ? DONE : STUCK;

    if( st == DONE )
    {
        if( aOptimize )
            OPTIMIZER::Optimize( &aWalkPath, OPTIMIZER::MERGE_OBTUSE, m_world );
    }

    return st;
}

}
