/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013-2015 CERN
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

#include <cstdio>
#include <cstdlib>
#include <limits>

#include <geometry/shape.h>
#include <geometry/shape_rect.h>
#include <geometry/shape_circle.h>
#include <geometry/shape_segment.h>

#include "direction.h"

#include "pns_diff_pair.h"
#include "pns_router.h"
#include "pns_solid.h"
#include "pns_utils.h"


class PNS_LINE;

PNS_DP_PRIMITIVE_PAIR::PNS_DP_PRIMITIVE_PAIR( PNS_ITEM* aPrimP, PNS_ITEM* aPrimN )
{
    m_primP = aPrimP->Clone();
    m_primN = aPrimN->Clone();

    m_anchorP = m_primP->Anchor( 0 );
    m_anchorN = m_primN->Anchor( 0 );
}


void PNS_DP_PRIMITIVE_PAIR::SetAnchors( const VECTOR2I& aAnchorP, const VECTOR2I& aAnchorN )
{
    m_anchorP = aAnchorP;
    m_anchorN = aAnchorN;
}


PNS_DP_PRIMITIVE_PAIR::PNS_DP_PRIMITIVE_PAIR( const VECTOR2I& aAnchorP, const VECTOR2I& aAnchorN )
{
    m_anchorP = aAnchorP;
    m_anchorN = aAnchorN;
    m_primP = m_primN = NULL;
}


PNS_DP_PRIMITIVE_PAIR::PNS_DP_PRIMITIVE_PAIR( const PNS_DP_PRIMITIVE_PAIR& aOther )
{
    if( aOther.m_primP )
        m_primP = aOther.m_primP->Clone();
    if( aOther.m_primN )
        m_primN = aOther.m_primN->Clone();

    m_anchorP = aOther.m_anchorP;
    m_anchorN = aOther.m_anchorN;
}


PNS_DP_PRIMITIVE_PAIR& PNS_DP_PRIMITIVE_PAIR::operator=( const PNS_DP_PRIMITIVE_PAIR& aOther )
{
    if( aOther.m_primP )
        m_primP = aOther.m_primP->Clone();
    if( aOther.m_primN )
        m_primN = aOther.m_primN->Clone();

    m_anchorP = aOther.m_anchorP;
    m_anchorN = aOther.m_anchorN;

    return *this;
}


PNS_DP_PRIMITIVE_PAIR::~PNS_DP_PRIMITIVE_PAIR()
{
    delete m_primP;
    delete m_primN;
}


bool PNS_DP_PRIMITIVE_PAIR::Directional() const
{
    if( !m_primP )
        return false;

    return m_primP->OfKind( PNS_ITEM::SEGMENT );
}


DIRECTION_45 PNS_DP_PRIMITIVE_PAIR::anchorDirection( PNS_ITEM* aItem, const VECTOR2I& aP ) const
{
    if( !aItem->OfKind ( PNS_ITEM::SEGMENT ) )
        return DIRECTION_45();

    PNS_SEGMENT* s = static_cast<PNS_SEGMENT*>( aItem );

    if( s->Seg().A == aP )
        return DIRECTION_45( s->Seg().A - s->Seg().B );
    else
        return DIRECTION_45( s->Seg().B - s->Seg().A );
}


DIRECTION_45 PNS_DP_PRIMITIVE_PAIR::DirP() const
{
    return anchorDirection( m_primP, m_anchorP );
}


DIRECTION_45 PNS_DP_PRIMITIVE_PAIR::DirN() const
{
    return anchorDirection( m_primN, m_anchorN );
}


static void drawGw( VECTOR2I p, int color )
{
    SHAPE_LINE_CHAIN l;

    l.Append( p - VECTOR2I( -50000, -50000 ) );
    l.Append( p + VECTOR2I( -50000, -50000 ) );

    //printf("router @ %p\n", PNS_ROUTER::GetInstance());
//    PNS_ROUTER::GetInstance()->DisplayDebugLine ( l, color, 10000 );

    l.Clear();
    l.Append( p - VECTOR2I( 50000, -50000 ) );
    l.Append( p + VECTOR2I( 50000, -50000 ) );

//    PNS_ROUTER::GetInstance()->DisplayDebugLine ( l, color, 10000 );
}


static DIRECTION_45::AngleType angle( const VECTOR2I &a, const VECTOR2I &b )
{
    DIRECTION_45 dir_a( a );
    DIRECTION_45 dir_b( b );

    return dir_a.Angle( dir_b );
}


static bool checkGap( const SHAPE_LINE_CHAIN &p, const SHAPE_LINE_CHAIN &n, int gap )
{
    int i, j;

    for( i = 0; i < p.SegmentCount(); i++ )
    {
        for( j = 0; j < n.SegmentCount() ; j++ )
        {
            int dist = p.CSegment( i ).Distance( n.CSegment( j ) );

            if( dist  < gap - 100 )
                return false;
        }
    }

    return true;
}


void PNS_DP_GATEWAY::Reverse()
{
    m_entryN = m_entryN.Reverse();
    m_entryP = m_entryP.Reverse();
}


bool PNS_DIFF_PAIR::BuildInitial( PNS_DP_GATEWAY& aEntry, PNS_DP_GATEWAY &aTarget, bool aPrefDiagonal )
{
    SHAPE_LINE_CHAIN p = DIRECTION_45().BuildInitialTrace ( aEntry.AnchorP(), aTarget.AnchorP(), aPrefDiagonal );
    SHAPE_LINE_CHAIN n = DIRECTION_45().BuildInitialTrace ( aEntry.AnchorN(), aTarget.AnchorN(), aPrefDiagonal );

    int mask = aEntry.AllowedAngles() | DIRECTION_45::ANG_STRAIGHT | DIRECTION_45::ANG_OBTUSE;

    SHAPE_LINE_CHAIN sum_n, sum_p;
    m_p = p;
    m_n = n;

    if( aEntry.HasEntryLines() )
    {
        if( !aEntry.Entry().CheckConnectionAngle( *this, mask ) )
            return false;

        sum_p = aEntry.Entry().CP();
        sum_n = aEntry.Entry().CN();
        sum_p.Append( p );
        sum_n.Append( n );
    }
    else
    {
        sum_p = p;
        sum_n = n;
    }

    mask = aTarget.AllowedAngles() | DIRECTION_45::ANG_STRAIGHT | DIRECTION_45::ANG_OBTUSE;

    m_p = sum_p;
    m_n = sum_n;

    if( aTarget.HasEntryLines() )
    {
        PNS_DP_GATEWAY t(aTarget) ;
        t.Reverse();

        if( !CheckConnectionAngle( t.Entry(), mask ) )
            return false;

        sum_p.Append( t.Entry().CP() );
        sum_n.Append( t.Entry().CN() );
    }

    m_p = sum_p;
    m_n = sum_n;

    if( !checkGap ( p, n, m_gapConstraint ) )
        return false;

    if( p.SelfIntersecting() || n.SelfIntersecting() )
        return false;

    if( p.Intersects( n ) )
        return false;
    
    return true;
}


bool PNS_DIFF_PAIR::CheckConnectionAngle( const PNS_DIFF_PAIR& aOther, int aAllowedAngles ) const
{
    bool checkP, checkN;

    if( m_p.SegmentCount() == 0 || aOther.m_p.SegmentCount() == 0 )
        checkP = true;
    else
    {
        DIRECTION_45 p0( m_p.CSegment( -1 ) );
        DIRECTION_45 p1( aOther.m_p.CSegment( 0 ) );

        checkP = ( p0.Angle( p1 ) & aAllowedAngles ) != 0;
    }

    if( m_n.SegmentCount() == 0 || aOther.m_n.SegmentCount() == 0 )
        checkN = true;
    else
    {
        DIRECTION_45 n0( m_n.CSegment( -1 ) );
        DIRECTION_45 n1( aOther.m_n.CSegment( 0 ) );

        checkN = ( n0.Angle( n1 ) & aAllowedAngles ) != 0;
    }

    return checkP && checkN;
}


const PNS_DIFF_PAIR PNS_DP_GATEWAY::Entry() const
{
    return PNS_DIFF_PAIR( m_entryP, m_entryN, 0 );
}


void PNS_DP_GATEWAYS::BuildOrthoProjections( PNS_DP_GATEWAYS& aEntries,
        const VECTOR2I& aCursorPos, int aOrthoScore )
{
    BOOST_FOREACH( PNS_DP_GATEWAY g, aEntries.Gateways() )
    {
        VECTOR2I dir = ( g.AnchorP() - g.AnchorN() ).Perpendicular();
        VECTOR2I midpoint( ( g.AnchorP() + g.AnchorN() ) / 2 );
        SEG guide( midpoint, midpoint + dir );
        VECTOR2I proj = guide.LineProject( aCursorPos );

        PNS_DP_GATEWAYS targets( m_gap );

        targets.m_viaGap = m_viaGap;
        targets.m_viaDiameter = m_viaDiameter;
        targets.m_fitVias = m_fitVias;

        targets.BuildForCursor( proj );

        BOOST_FOREACH( PNS_DP_GATEWAY t, targets.Gateways() )
        {
            t.SetPriority( aOrthoScore );
            m_gateways.push_back( t );
        }
    }
}


bool PNS_DP_GATEWAYS::FitGateways( PNS_DP_GATEWAYS& aEntry, PNS_DP_GATEWAYS& aTarget,
        bool aPrefDiagonal, PNS_DIFF_PAIR& aDp )
{
    std::vector<DP_CANDIDATE> candidates;

    BOOST_FOREACH( PNS_DP_GATEWAY g_entry, aEntry.Gateways() )
    {
        BOOST_FOREACH( PNS_DP_GATEWAY g_target, aTarget.Gateways() )
        {
            for( int attempt = 0; attempt < 2; attempt++ )
            {
                PNS_DIFF_PAIR l( m_gap );

                if( l.BuildInitial( g_entry, g_target, aPrefDiagonal ^ ( attempt ? true : false ) ) )
                {
                    int score = ( attempt == 1 ? -3 : 0 );
                    score +=g_entry.Priority();
                    score +=g_target.Priority();

                    DP_CANDIDATE c;
                    c.score = score;
                    c.p = l.CP();
                    c.n = l.CN();
                    candidates.push_back( c );
                }
            }
        }
    }

    int bestScore = -1000;
    DP_CANDIDATE best;
    bool found;

    BOOST_FOREACH( DP_CANDIDATE c, candidates )
    {
        if( c.score > bestScore )
        {
            bestScore = c.score;
            best = c;
            found = true;
        }
    }

    if( found )
    {
        aDp.SetGap( m_gap );
        aDp.SetShape( best.p, best.n );
        return true;
    }

    return false;
}


bool PNS_DP_GATEWAYS::checkDiagonalAlignment( const VECTOR2I& a, const VECTOR2I& b ) const
{
    VECTOR2I dir ( std::abs (a.x - b.x), std::abs ( a.y - b.y ));

    return (dir.x == 0 && dir.y != 0) || (dir.x == dir.y) || (dir.y == 0 && dir.x != 0);
}


void PNS_DP_GATEWAYS::BuildFromPrimitivePair( PNS_DP_PRIMITIVE_PAIR aPair, bool aPreferDiagonal )
{
    VECTOR2I majorDirection;
    VECTOR2I p0_p, p0_n;
    int orthoFanDistance;
    int diagFanDistance;
    const SHAPE* shP = NULL;

    if( aPair.PrimP() == NULL )
    {
        BuildGeneric( aPair.AnchorP(), aPair.AnchorN(), true );
        return;
    }

    const int pvMask = PNS_ITEM::SOLID | PNS_ITEM::VIA;

    if( aPair.PrimP()->OfKind( pvMask ) && aPair.PrimN()->OfKind(  pvMask ) )
    {
        p0_p = aPair.AnchorP();
        p0_n = aPair.AnchorN();

        shP = aPair.PrimP()->Shape();

    }
    else if( aPair.PrimP()->OfKind( PNS_ITEM::SEGMENT ) && aPair.PrimN()->OfKind( PNS_ITEM::SEGMENT ) )
    {
        buildDpContinuation( aPair, aPreferDiagonal );

        return;
    }

    majorDirection = ( p0_p - p0_n ).Perpendicular();

    switch( shP->Type() )
    {
        case SH_RECT:
        {
            int w = static_cast<const SHAPE_RECT*>( shP )->GetWidth();
            int h = static_cast<const SHAPE_RECT*>( shP )->GetHeight();

            if( w < h )
                std::swap( w, h );

            orthoFanDistance = w * 3/4;
            diagFanDistance = ( w - h ) / 2;
            break;
        }

        case SH_SEGMENT:
        {
            int w = static_cast<const SHAPE_SEGMENT*>( shP )->GetWidth();
            SEG s = static_cast<const SHAPE_SEGMENT*>( shP )->GetSeg();

            orthoFanDistance = w + ( s.B - s.A ).EuclideanNorm() / 2;
            diagFanDistance = ( s.B - s.A ).EuclideanNorm() / 2;
            break;
        }

        default:
            BuildGeneric ( p0_p, p0_n, true );
            return;
    }

    if( checkDiagonalAlignment( p0_p, p0_n ) )
    {
        int padDist = ( p0_p - p0_n ).EuclideanNorm();

        for( int k = 0; k < 2; k++ )
        {
            VECTOR2I dir, dp, dv;

            if( k == 0 )
            {
                dir = majorDirection.Resize( orthoFanDistance );
                int d = ( padDist - m_gap ) / 2;

                dp = dir.Resize( d );
                dv = ( p0_n - p0_p ).Resize( d );
            }
            else
            {
                dir = majorDirection.Resize( diagFanDistance );
                int d = ( padDist - m_gap ) / 2;
                dp = dir.Resize( d );
                dv = ( p0_n - p0_p ).Resize( d );
            }

            for( int i = 0; i < 2; i++ )
            {
                int sign = i ? -1 : 1;

                VECTOR2I gw_p( p0_p + sign * ( dir + dp ) + dv );
                VECTOR2I gw_n( p0_n + sign * ( dir + dp ) - dv );

                SHAPE_LINE_CHAIN entryP( p0_p, p0_p + sign * dir, gw_p );
                SHAPE_LINE_CHAIN entryN( p0_n, p0_n + sign * dir, gw_n );

                PNS_DP_GATEWAY gw( gw_p, gw_n, false );

                gw.SetEntryLines( entryP, entryN );
                gw.SetPriority( 100 - k );
                m_gateways.push_back( gw );
            }
        }
    }

    BuildGeneric( p0_p, p0_n, true );
}


void PNS_DP_GATEWAYS::BuildForCursor( const VECTOR2I& aCursorPos )
{
    int gap = m_fitVias ? m_viaGap + m_viaDiameter : m_gap;

    for( int attempt = 0; attempt < 2; attempt++ )
    {
        for( int i = 0; i < 4; i++ )
        {
            VECTOR2I dir;

            if( !attempt )
            {
                dir = VECTOR2I( gap, gap ).Resize( gap / 2 );

                if( i % 2 == 0 )
                    dir.x = -dir.x;

                if( i / 2 == 0 )
                    dir.y = -dir.y;
            }
            else
            {
                if( i /2 == 0 )
                    dir = VECTOR2I( gap / 2 * ( ( i % 2 ) ? -1 : 1 ), 0 );
                else
                    dir = VECTOR2I( 0, gap / 2 * ( ( i % 2 ) ? -1 : 1) );
            }

            if( m_fitVias )
                BuildGeneric( aCursorPos + dir, aCursorPos - dir, true, true );
            else
                m_gateways.push_back( PNS_DP_GATEWAY( aCursorPos + dir,
                                      aCursorPos - dir, attempt ? true : false ) );

            drawGw ( aCursorPos + dir, 2 );
            drawGw ( aCursorPos - dir, 3 );
        }
    }
}


void PNS_DP_GATEWAYS::buildEntries( const VECTOR2I& p0_p, const VECTOR2I& p0_n )
{
    BOOST_FOREACH( PNS_DP_GATEWAY &g, m_gateways )
    {
        if( !g.HasEntryLines() )
        {
            SHAPE_LINE_CHAIN lead_p = DIRECTION_45().BuildInitialTrace ( g.AnchorP(), p0_p, g.IsDiagonal() ).Reverse();
            SHAPE_LINE_CHAIN lead_n = DIRECTION_45().BuildInitialTrace ( g.AnchorN(), p0_n, g.IsDiagonal() ).Reverse();
            g.SetEntryLines( lead_p, lead_n );
        }
    }
}


void PNS_DP_GATEWAYS::buildDpContinuation( PNS_DP_PRIMITIVE_PAIR aPair, bool aIsDiagonal )
{
    PNS_DP_GATEWAY gw( aPair.AnchorP(), aPair.AnchorN(), aIsDiagonal );
    gw.SetPriority( 100 );
    m_gateways.push_back( gw );

    if( !aPair.Directional() )
        return;

    DIRECTION_45 dP = aPair.DirP();
    DIRECTION_45 dN = aPair.DirN();

    int gap = ( aPair.AnchorP() - aPair.AnchorN() ).EuclideanNorm();

    VECTOR2I vdP = aPair.AnchorP() + dP.Left().ToVector();
    VECTOR2I vdN = aPair.AnchorN() + dN.Left().ToVector();

    PNS_SEGMENT* sP = static_cast<PNS_SEGMENT*>( aPair.PrimP() );

    VECTOR2I t1, t2;

    if( sP->Seg().Side( vdP ) == sP->Seg().Side( vdN ) )
    {
        t1 = aPair.AnchorP() + dP.Left().ToVector().Resize( gap );
        t2 = aPair.AnchorN() + dP.Right().ToVector().Resize( gap );
    }
    else
    {
        t1 = aPair.AnchorP() + dP.Right().ToVector().Resize( gap );
        t2 = aPair.AnchorN() + dP.Left().ToVector().Resize( gap );
    }

    PNS_DP_GATEWAY gwL( t2, aPair.AnchorN(), !aIsDiagonal );
    SHAPE_LINE_CHAIN ep = dP.BuildInitialTrace( aPair.AnchorP(), t2, !aIsDiagonal );

    gwL.SetPriority( 10 );
    gwL.SetEntryLines( ep , SHAPE_LINE_CHAIN() );

    m_gateways.push_back( gwL );

    PNS_DP_GATEWAY gwR( aPair.AnchorP(), t1, !aIsDiagonal );
    SHAPE_LINE_CHAIN en = dP.BuildInitialTrace( aPair.AnchorN(), t1, !aIsDiagonal );
    gwR.SetPriority( 10) ;
    gwR.SetEntryLines( SHAPE_LINE_CHAIN(), en );

    m_gateways.push_back( gwR );
}


void PNS_DP_GATEWAYS::BuildGeneric( const VECTOR2I& p0_p, const VECTOR2I& p0_n, bool aBuildEntries, bool aViaMode )
{
    SEG st_p[2], st_n[2];
    SEG d_n[2], d_p[2];

    const int padToGapThreshold = 3;
    int padDist = ( p0_p - p0_p ).EuclideanNorm();

    st_p[0] = SEG(p0_p + VECTOR2I( -100, 0 ), p0_p + VECTOR2I( 100, 0 ) );
    st_n[0] = SEG(p0_n + VECTOR2I( -100, 0 ), p0_n + VECTOR2I( 100, 0 ) );
    st_p[1] = SEG(p0_p + VECTOR2I( 0, -100 ), p0_p + VECTOR2I( 0, 100 ) );
    st_n[1] = SEG(p0_n + VECTOR2I( 0, -100 ), p0_n + VECTOR2I( 0, 100 ) );
    d_p[0] = SEG( p0_p + VECTOR2I( -100, -100 ), p0_p + VECTOR2I( 100, 100 ) );
    d_p[1] = SEG( p0_p + VECTOR2I( 100, -100 ), p0_p + VECTOR2I( -100, 100 ) );
    d_n[0] = SEG( p0_n + VECTOR2I( -100, -100 ), p0_n + VECTOR2I( 100, 100 ) );
    d_n[1] = SEG( p0_n + VECTOR2I( 100, -100 ), p0_n + VECTOR2I( -100, 100 ) );

    // midpoint exit & side-by exits
    for( int i = 0; i < 2; i++ )
    {
        bool straightColl = st_p[i].Collinear( st_n[i] );
        bool diagColl = d_p[i].Collinear( d_n[i] );

        if( straightColl || diagColl )
        {
            VECTOR2I dir = ( p0_n - p0_p ).Resize( m_gap / 2 );
            VECTOR2I m = ( p0_p + p0_n ) / 2;
            int prio = ( padDist > padToGapThreshold * m_gap ? 2 : 1);

            if( !aViaMode )
            {
                m_gateways.push_back( PNS_DP_GATEWAY( m - dir, m + dir, diagColl, DIRECTION_45::ANG_RIGHT, prio ) );

                dir = ( p0_n - p0_p ).Resize( m_gap );
                m_gateways.push_back( PNS_DP_GATEWAY( p0_p - dir, p0_p - dir + dir.Perpendicular(), diagColl ) );
                m_gateways.push_back( PNS_DP_GATEWAY( p0_p - dir, p0_p - dir - dir.Perpendicular(), diagColl ) );
                m_gateways.push_back( PNS_DP_GATEWAY( p0_n + dir + dir.Perpendicular(), p0_n + dir, diagColl ) );
                m_gateways.push_back( PNS_DP_GATEWAY( p0_n + dir - dir.Perpendicular(), p0_n + dir, diagColl ) );
            }
        }
    }

    for( int i = 0; i < 2; i++ )
    {
        for( int j = 0; j < 2; j++ )
        {
            OPT_VECTOR2I ips[2], m;

            ips[0] = d_n[i].IntersectLines( d_p[j] );
            ips[1] = st_p[i].IntersectLines( st_n[j] );

            if( d_n[i].Collinear( d_p[j] ) )
                ips [0] = OPT_VECTOR2I();
            if( st_p[i].Collinear( st_p[j] ) )
                ips [1] = OPT_VECTOR2I();

            // diagonal-diagonal and straight-straight cases - the most typical case if the pads
            // are on the same straight/diagonal line
            for( int k = 0; k < 2; k++ )
            {
                m = ips[k];
                if( m && *m != p0_p && *m != p0_n )
                {
                    int prio = ( padDist > padToGapThreshold * m_gap ? 10 : 20 );
                    VECTOR2I g_p( ( p0_p - *m ).Resize( (double) m_gap * M_SQRT1_2 ) );
                    VECTOR2I g_n( ( p0_n - *m ).Resize( (double) m_gap * M_SQRT1_2 ) );

                    m_gateways.push_back( PNS_DP_GATEWAY( *m + g_p, *m + g_n, k == 0 ? true : false, DIRECTION_45::ANG_OBTUSE, prio ) );
                }
            }

            ips[0] = st_n[i].IntersectLines( d_p[j] );
            ips[1] = st_p[i].IntersectLines( d_n[j] );

  //          diagonal-straight cases: 8 possibilities of "weirder" exists
            for( int k = 0; k < 2; k++ )
            {
                m = ips[k];

                if( !aViaMode && m && *m != p0_p && *m != p0_n )
                {
                    VECTOR2I g_p, g_n;

                     g_p = ( p0_p - *m ).Resize( (double) m_gap * M_SQRT2 );
                     g_n = ( p0_n - *m ).Resize( (double) m_gap );

                    if( angle( g_p, g_n ) != DIRECTION_45::ANG_ACUTE )
                        m_gateways.push_back( PNS_DP_GATEWAY( *m + g_p, *m + g_n, true ) );

                     g_p = ( p0_p - *m ).Resize( m_gap );
                     g_n = ( p0_n - *m ).Resize( (double) m_gap * M_SQRT2 );

                    if( angle( g_p, g_n ) != DIRECTION_45::ANG_ACUTE )
                        m_gateways.push_back( PNS_DP_GATEWAY( *m + g_p, *m + g_n, true ) );
                }
            }
        }
    }

    if( aBuildEntries )
        buildEntries( p0_p, p0_n );
}


PNS_DP_PRIMITIVE_PAIR PNS_DIFF_PAIR::EndingPrimitives()
{
    if( m_hasVias )
        return PNS_DP_PRIMITIVE_PAIR( &m_via_p, &m_via_n );
    else
    {
        const PNS_LINE lP( PLine() );
        const PNS_LINE lN( NLine() );

        PNS_SEGMENT sP( lP, lP.CSegment( -1 ) );
        PNS_SEGMENT sN( lN, lN.CSegment( -1 ) );

        PNS_DP_PRIMITIVE_PAIR dpair( &sP, &sN );
        dpair.SetAnchors( sP.Seg().B, sN.Seg().B );

        return dpair;
    }
}


bool commonParallelProjection( SEG n, SEG p, SEG &pClip, SEG& nClip )
{
    SEG n_proj_p( p.LineProject( n.A ), p.LineProject( n.B ) );

    int64_t t_a = 0;
    int64_t t_b = p.TCoef( p.B );

    int64_t tproj_a = p.TCoef( n_proj_p.A );
    int64_t tproj_b = p.TCoef( n_proj_p.B );

    if( t_b < t_a )
        std::swap( t_b, t_a );

    if( tproj_b < tproj_a )
        std::swap( tproj_b, tproj_a );

    if( t_b <= tproj_a )
        return false;

    if( t_a >= tproj_b )
        return false;

    int64_t t[4] = { 0, p.TCoef( p.B ), p.TCoef( n_proj_p.A ), p.TCoef( n_proj_p.B ) };
    std::vector<int64_t> tv( t, t + 4 );
    std::sort( tv.begin(), tv.end() ); // fixme: awful and disgusting way of finding 2 midpoints

    int64_t pLenSq = p.SquaredLength();

    VECTOR2I dp = p.B - p.A;
    pClip.A.x = p.A.x + rescale( (int64_t)dp.x, tv[1], pLenSq );
    pClip.A.y = p.A.y + rescale( (int64_t)dp.y, tv[1], pLenSq );

    pClip.B.x = p.A.x + rescale( (int64_t)dp.x, tv[2], pLenSq );
    pClip.B.y = p.A.y + rescale( (int64_t)dp.y, tv[2], pLenSq );

    nClip.A = n.LineProject( pClip.A );
    nClip.B = n.LineProject( pClip.B );

    return true;
}


double PNS_DIFF_PAIR::Skew() const
{
    return m_p.Length() - m_n.Length();
}


void PNS_DIFF_PAIR::CoupledSegmentPairs( COUPLED_SEGMENTS_VEC& aPairs ) const
{
    SHAPE_LINE_CHAIN p( m_p );
    SHAPE_LINE_CHAIN n( m_n );

    p.Simplify();
    n.Simplify();

    for( int i = 0; i < p.SegmentCount(); i++ )
    {
        for( int j = 0; j < n.SegmentCount(); j++ )
        {
            SEG sp = p.CSegment( i );
            SEG sn = n.CSegment( j );

            SEG p_clip, n_clip;

            int64_t dist = std::abs( sp.Distance( sn ) - m_width );

            if( sp.ApproxParallel( sn ) && m_gapConstraint.Matches( dist ) && commonParallelProjection( sp, sn, p_clip, n_clip ) )
            {
                const COUPLED_SEGMENTS spair( p_clip, sp, i, n_clip, sn, j );
                aPairs.push_back( spair );
            }
        }
    }
}


int64_t PNS_DIFF_PAIR::CoupledLength( const SHAPE_LINE_CHAIN& aP, const SHAPE_LINE_CHAIN& aN ) const
{
    int64_t total = 0;

    for( int i = 0; i < aP.SegmentCount(); i++ )
    {
        for( int j = 0; j < aN.SegmentCount(); j++ )
        {
            SEG sp = aP.CSegment( i );
            SEG sn = aN.CSegment( j );

            SEG p_clip, n_clip;

            int64_t dist = std::abs( sp.Distance(sn) - m_width );

            if( sp.ApproxParallel( sn ) && m_gapConstraint.Matches( dist ) &&
                    commonParallelProjection( sp, sn, p_clip, n_clip ) )
                total += p_clip.Length();
        }
    }

    return total;
}


double PNS_DIFF_PAIR::CoupledLength() const
{
    COUPLED_SEGMENTS_VEC pairs;

    CoupledSegmentPairs( pairs );

    double l = 0.0;
    for( unsigned int i = 0; i < pairs.size(); i++ )
        l += pairs[i].coupledP.Length();

    return l;
}


double PNS_DIFF_PAIR::CoupledLengthFactor() const
{
    double t = TotalLength();

    if( t == 0.0 )
        return 0.0;

    return CoupledLength() / t;
}


double PNS_DIFF_PAIR::TotalLength() const
{
    double lenP = m_p.Length();
    double lenN = m_n.Length();

    return (lenN + lenP ) / 2.0;
}


int PNS_DIFF_PAIR::CoupledLength ( const SEG& aP, const SEG& aN ) const
{
    SEG p_clip, n_clip;
    int64_t dist = std::abs( aP.Distance( aN ) - m_width );

    if( aP.ApproxParallel( aN ) && m_gapConstraint.Matches( dist ) &&
            commonParallelProjection ( aP, aN, p_clip, n_clip ) )
        return p_clip.Length();

    return 0;
}
