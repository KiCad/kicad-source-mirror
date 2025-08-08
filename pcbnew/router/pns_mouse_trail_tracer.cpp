/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013-2020 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include "pns_mouse_trail_tracer.h"
#include "pns_router.h"
#include "pns_debug_decorator.h"

namespace PNS {

MOUSE_TRAIL_TRACER::MOUSE_TRAIL_TRACER()
{
    m_tolerance = 0;
    m_disableMouse = false;
    Clear();
}


MOUSE_TRAIL_TRACER::~MOUSE_TRAIL_TRACER() {}


void MOUSE_TRAIL_TRACER::Clear()
{
    m_forced         = false;
    m_manuallyForced = false;
    m_trail.Clear();
}


void MOUSE_TRAIL_TRACER::AddTrailPoint( const VECTOR2I& aP )
{
    if( m_trail.SegmentCount() == 0 )
    {
        m_trail.Append( aP );
    }
    else
    {
        SEG s_new( m_trail.CLastPoint(), aP );

        if( m_trail.SegmentCount() > 2 )
        {
            SEG::ecoord limit = ( static_cast<SEG::ecoord>( m_tolerance ) * m_tolerance );

            for( int i = 0; i < m_trail.SegmentCount() - 2; i++ )
            {
                const SEG& s_trail = m_trail.CSegment( i );

                if( s_trail.SquaredDistance( s_new ) <= limit )
                {
                    m_trail = m_trail.Slice( 0, i );
                    break;
                }
            }
        }

        m_trail.Append( aP );
    }

    m_trail.Simplify();

    DEBUG_DECORATOR *dbg = ROUTER::GetInstance()->GetInterface()->GetDebugDecorator();

    PNS_DBG( dbg, AddShape, &m_trail, CYAN, 50000, wxT( "mt-trail" ) );
}


DIRECTION_45 MOUSE_TRAIL_TRACER::GetPosture( const VECTOR2I& aP )
{
    // Tuning factor for how good the "fit" of the trail must be to the posture
    const double areaRatioThreshold = 1.3;

    // Tuning factor to minimize flutter
    const double areaRatioEpsilon = 0.25;

    // Minimum distance factor of the trail before the min area test is used to lock the solver
    const double minAreaCutoffDistanceFactor = 6;

    // Adjusts how far away from p0 we get before whatever posture we solved is locked in
    const int lockDistanceFactor = 30;

    // Adjusts how close to p0 we unlock the posture again if one was locked already
    const int unlockDistanceFactor = 10;

    if( m_trail.PointCount() < 2 || m_manuallyForced )
    {
        // If mouse trail detection is enabled; using the last seg direction as a starting point
        // will give the best results.  Otherwise, just assume that we switch postures every
        // segment.
        if( !m_manuallyForced && m_lastSegDirection != DIRECTION_45::UNDEFINED )
            m_direction = m_disableMouse ? m_lastSegDirection.Right() : m_lastSegDirection;

        return m_direction;
    }

    DEBUG_DECORATOR* dbg = ROUTER::GetInstance()->GetInterface()->GetDebugDecorator();
    VECTOR2I         p0 = m_trail.CPoint( 0 );
    double refLength = SEG( p0, aP ).Length();
    SHAPE_LINE_CHAIN straight( DIRECTION_45().BuildInitialTrace( p0, aP, false ) );

    straight.SetClosed( true );
    straight.Append( m_trail.Reverse() );
    straight.Simplify();

    PNS_DBG( dbg, AddShape, &straight, m_forced ? BLUE : GREEN, 100000, wxT( "mt-straight" ) );

    double areaS = straight.Area();

    SHAPE_LINE_CHAIN diag( DIRECTION_45().BuildInitialTrace( p0, aP, true ) );
    diag.Append( m_trail.Reverse() );
    diag.SetClosed( true );
    diag.Simplify();

    PNS_DBG( dbg, AddShape, &diag, YELLOW, 100000, wxT( "mt-diag" ) );

    double areaDiag = diag.Area();
    double ratio    = areaS / ( areaDiag + 1.0 );

    // heuristic to detect that the user dragged back the cursor to the beginning of the trace
    // in this case, we cancel any forced posture and restart the trail
    if( m_forced && refLength < unlockDistanceFactor * m_tolerance )
    {
        PNS_DBG( dbg, Message, "Posture: Unlocked and reset" );
        m_forced = false;
        VECTOR2I start = p0;
        m_trail.Clear();
        m_trail.Append( start );
    }

    bool areaOk = false;

    // Check the actual trail area against the cutoff.  This prevents flutter when the trail is
    // very close to a straight line.
    if( !m_forced && refLength > minAreaCutoffDistanceFactor * m_tolerance )
    {
        double areaCutoff = m_tolerance * refLength;
        SHAPE_LINE_CHAIN trail( m_trail );
        trail.SetClosed( true );

        if( trail.Area() > areaCutoff )
            areaOk = true;
    }

    PNS_DBG( dbg, Message, wxString::Format( "Posture: rl %.0f thr %d tol %d as %.3f area OK %d forced %d\n", refLength, (int)(unlockDistanceFactor * m_tolerance), m_tolerance, ratio, areaOk?1:0, m_forced?1:0 ) );

    DIRECTION_45 straightDirection;
    DIRECTION_45 diagDirection;
    DIRECTION_45 newDirection = m_direction;

    straightDirection = DIRECTION_45( straight.CSegment( 0 ) );
    diagDirection     = DIRECTION_45( diag.CSegment( 0 ) );

    if( !m_forced && areaOk && ratio > areaRatioThreshold + areaRatioEpsilon )
        newDirection = diagDirection;
    else if( !m_forced && areaOk && ratio < ( 1.0 / areaRatioThreshold ) - areaRatioEpsilon )
        newDirection = straightDirection;
    else
        newDirection = m_direction.IsDiagonal() ? diagDirection : straightDirection;

    if( !m_disableMouse && newDirection != m_direction )
    {
        PNS_DBG( dbg, Message, wxString::Format( "Posture: direction update %s => %s",
                                                 m_direction.Format(), newDirection.Format() ) );
        m_direction = newDirection;
    }

    // If we have a last segment, correct the direction relative to it.  For segment exit, we want
    // to correct to the least obtuse
    if( !m_manuallyForced && !m_disableMouse && m_lastSegDirection != DIRECTION_45::UNDEFINED )
    {
        PNS_DBG( dbg, Message,
                 wxString::Format( wxT( "Posture: checking direction %s against last seg %s" ),
                                   m_direction.Format(), m_lastSegDirection.Format() ) );

        if( straightDirection == m_lastSegDirection )
        {
            if( m_direction != straightDirection )
            {
                PNS_DBG( dbg, Message, wxString::Format( wxT( "Posture: forcing straight => %s" ),
                                                         straightDirection.Format() ) );
            }

            m_direction = straightDirection;
        }
        else if( diagDirection == m_lastSegDirection )
        {
            if( m_direction != diagDirection )
            {
                PNS_DBG( dbg, Message, wxString::Format( wxT( "Posture: forcing diagonal => %s" ),
                                                         diagDirection.Format() ) );
            }

            m_direction = diagDirection;
        }
        else
        {
            switch( m_direction.Angle( m_lastSegDirection ) )
            {
            case DIRECTION_45::ANG_HALF_FULL:
                // Force a better (acute) connection
                m_direction = m_direction.IsDiagonal() ? straightDirection : diagDirection;
                PNS_DBG( dbg, Message, wxString::Format( wxT( "Posture: correcting half full => %s" ),
                                                         m_direction.Format() ) );
                break;

            case DIRECTION_45::ANG_ACUTE:
            {
                // Force a better connection by flipping if possible
                DIRECTION_45 candidate = m_direction.IsDiagonal() ? straightDirection
                                                                  : diagDirection;

                if( candidate.Angle( m_lastSegDirection ) == DIRECTION_45::ANG_RIGHT )
                {
                    PNS_DBG( dbg, Message, wxString::Format( wxT( "Posture: correcting right => %s" ),
                                                             candidate.Format() ) );
                    m_direction = candidate;
                }

                break;
            }

            case DIRECTION_45::ANG_RIGHT:
            {
                // Force a better connection by flipping if possible
                DIRECTION_45 candidate = m_direction.IsDiagonal() ? straightDirection
                                                                  : diagDirection;

                if( candidate.Angle( m_lastSegDirection ) == DIRECTION_45::ANG_OBTUSE )
                {
                    PNS_DBG( dbg, Message, wxString::Format( wxT( "Posture: correcting obtuse => %s" ),
                                                             candidate.Format() ) );
                    m_direction = candidate;
                }

                break;
            }

            default:
                break;
            }
        }
    }

    // If we get far away from the initial point, lock in the current solution to prevent flutter
    if( !m_forced && refLength > lockDistanceFactor * m_tolerance )
    {
        PNS_DBG( dbg, Message, "Posture: solution locked" );
        m_forced = true;
    }

    return m_direction;
}


void MOUSE_TRAIL_TRACER::FlipPosture()
{
    m_direction = m_direction.Right();
    m_forced = true;
    m_manuallyForced = true;
}


VECTOR2I MOUSE_TRAIL_TRACER::GetTrailLeadVector() const
{
    if( m_trail.PointCount() < 2 )
    {
        return VECTOR2I(0, 0);
    }
    else
    {
        return m_trail.CLastPoint() - m_trail.CPoint( 0 );
    }
}

}

