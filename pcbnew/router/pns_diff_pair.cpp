/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013-2015 CERN
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <limits>

#include <algorithm>
#include <core/typeinfo.h>
#include <geometry/shape_rect.h>

#include "pns_diff_pair.h"
#include "pns_router.h"
#include "pns_debug_decorator.h"
#include "pns_utils.h"
#include "pns_arc.h"

namespace PNS {

class LINE;


DP_PRIMITIVE_PAIR::DP_PRIMITIVE_PAIR( ITEM* aPrimP, ITEM* aPrimN )
{
    m_primP = aPrimP;
    m_primN = aPrimN;

    m_anchorP = m_primP->Anchor( 0 );
    m_anchorN = m_primN->Anchor( 0 );
}


void DP_PRIMITIVE_PAIR::SetAnchors( const VECTOR2I& aAnchorP, const VECTOR2I& aAnchorN )
{
    m_anchorP = aAnchorP;
    m_anchorN = aAnchorN;
}

void  DP_PRIMITIVE_PAIR::SetPrimitives(ITEM* aPrimP, ITEM* aPrimN )
{
    m_primP = aPrimP;
    m_primN = aPrimN;
}


DP_PRIMITIVE_PAIR::DP_PRIMITIVE_PAIR( const VECTOR2I& aAnchorP, const VECTOR2I& aAnchorN )
{
    m_anchorP = aAnchorP;
    m_anchorN = aAnchorN;
    m_primP = m_primN = nullptr;
}


DP_PRIMITIVE_PAIR::DP_PRIMITIVE_PAIR( const DP_PRIMITIVE_PAIR& aOther )
{
    m_primP = m_primN = nullptr;
    m_primP = aOther.m_primP;
    m_primN = aOther.m_primN;

    m_anchorP = aOther.m_anchorP;
    m_anchorN = aOther.m_anchorN;
    m_isMidtrace = aOther.m_isMidtrace;
    m_name = aOther.m_name;
}


DP_PRIMITIVE_PAIR& DP_PRIMITIVE_PAIR::operator=( const DP_PRIMITIVE_PAIR& aOther )
{
    if( aOther.m_primP )
    {
        m_primP = aOther.m_primP;
    }

    if( aOther.m_primN )
    {
        m_primN = aOther.m_primN;
    }

    m_anchorP = aOther.m_anchorP;
    m_anchorN = aOther.m_anchorN;

    m_isMidtrace = aOther.m_isMidtrace;
    m_name = aOther.m_name;

    return *this;
}


DP_PRIMITIVE_PAIR::~DP_PRIMITIVE_PAIR()
{
}


bool DP_PRIMITIVE_PAIR::Directional() const
{
    if( !m_primP )
        return false;

    return m_primP->OfKind( ITEM::SEGMENT_T | ITEM::ARC_T );
}


DIRECTION_45 DP_PRIMITIVE_PAIR::anchorDirection( const ITEM* aItem, const VECTOR2I& aP ) const
{
    if( !aItem->OfKind( ITEM::SEGMENT_T | ITEM::ARC_T ) )
        return DIRECTION_45();

    if( aItem->Anchor( 0 ) == aP )
        return DIRECTION_45( aItem->Anchor( 0 ) - aItem->Anchor( 1 ) );
    else
        return DIRECTION_45( aItem->Anchor( 1 ) - aItem->Anchor( 0 ) );
}


void DP_PRIMITIVE_PAIR::CursorOrientation( const VECTOR2I& aCursorPos, VECTOR2I& aMidpoint, VECTOR2I& aDirection ) const
{
    if( !m_primN || !m_primP )
        return;

    VECTOR2I aP, aN;

    if( m_primP->OfKind( ITEM::SEGMENT_T ) && m_primN->OfKind( ITEM::SEGMENT_T ) )
    {
        aP = m_primP->Anchor( 1 );
        aN = m_primN->Anchor( 1 );

        // If both segments are parallel, use that as the direction.  Otherwise, fall back on the
        // direction perpendicular to the anchor points.
        const SEG& segP = static_cast<SEGMENT*>( m_primP )->Seg();
        const SEG& segN = static_cast<SEGMENT*>( m_primN )->Seg();

        if( ( segP.B != segP.A ) && ( segN.B != segN.A ) && segP.ApproxParallel( segN ) )
        {
            aMidpoint = ( aP + aN ) / 2;
            aDirection = segP.B - segP.A;
            aDirection = aDirection.Resize( ( aP - aN ).EuclideanNorm() );
            return;
        }
    }
    else
    {
        aP = m_primP->Anchor( 0 );
        aN = m_primN->Anchor( 0 );
    }

    aMidpoint = ( aP + aN ) / 2;
    aDirection = ( aP - aN ).Perpendicular();

    if( aDirection.Dot( aCursorPos - aMidpoint ) < 0 )
        aDirection = -aDirection;
}


DIRECTION_45 DP_PRIMITIVE_PAIR::DirP() const
{
    return anchorDirection( m_primP, m_anchorP );
}


DIRECTION_45 DP_PRIMITIVE_PAIR::DirN() const
{
    return anchorDirection( m_primN, m_anchorN );
}


static DIRECTION_45::AngleType angle( const VECTOR2I &a, const VECTOR2I &b )
{
    DIRECTION_45 dir_a( a );
    DIRECTION_45 dir_b( b );

    return dir_a.Angle( dir_b );
}


void DP_GATEWAY::Reverse()
{
    m_entryN = m_entryN.Reverse();
    m_entryP = m_entryP.Reverse();
}


DIRECTION_45 DIFF_PAIR::getDirection( bool aIsP, bool aEnd ) const
{
    const SHAPE_LINE_CHAIN& l = aIsP ? m_p : m_n;

    if( !l.SegmentCount() )
        return DIRECTION_45();

    const SEG s = aEnd ? l.CSegment( l.SegmentCount() - 1 ) : l.CSegment( 0 );

    if( aEnd )
        return DIRECTION_45( s ).Opposite();
    else
        return DIRECTION_45( s );
}


bool DIFF_PAIR::BuildInitial( const DP_GATEWAY& aEntry, const DP_GATEWAY& aTarget, bool aPrefDiagonal, bool aFitVias,
                              float& aBestCouplingRatio, float& aAspectRatio )
{
    SHAPE_LINE_CHAIN p = DIRECTION_45().BuildInitialTrace( aEntry.AnchorP(), aTarget.AnchorP(), aPrefDiagonal );
    SHAPE_LINE_CHAIN n = DIRECTION_45().BuildInitialTrace( aEntry.AnchorN(), aTarget.AnchorN(), aPrefDiagonal );

    int              mask = aEntry.AllowedAngles() | DIRECTION_45::ANG_STRAIGHT | DIRECTION_45::ANG_OBTUSE;
    auto             dbg = ROUTER::GetInstance()->GetInterface()->GetDebugDecorator();
    wxString         failReason;
    bool             fail = false;

    PNS_DBG( dbg, AddShape, &p, RED, 20000,
             wxString::Format( "init+ prefDiag %d (dims %s) %s/%s cl %d %d %d", aPrefDiagonal ? 1 : 0, m_dims.Format(),
                               aEntry.GetName(), aTarget.GetName(), m_dims.MinClearance(),
                               aEntry.Dimensions().MinClearance(), aTarget.Dimensions().MinClearance() ) );
    PNS_DBG( dbg, AddShape, &n, BLUE, 20000, wxT( "init-" ) );

    SHAPE_LINE_CHAIN sum_n, sum_p;
    m_p = p;
    m_n = n;

    bool entryIsStraight = false;
    bool targetIsStraight = false;

    if( aEntry.HasPrimaryDirection() && m_p.SegmentCount() >= 1 )
    {
        auto         dirMask = aEntry.PrimaryDirectionMask();
        DIRECTION_45 dir2( m_p.CSegment( 0 ) );
        if( !( dirMask & dir2.Mask() ) )
        {
            fail = true;
            failReason = wxT( "fail-primary-entry" );
        }
    }

    if( aTarget.HasPrimaryDirection() && m_p.SegmentCount() >= 1 )
    {
        auto         dirMask = aTarget.PrimaryDirectionMask();
        DIRECTION_45 dir2( m_p.CSegment( -1 ) );
        if( !( dirMask & ( dir2.Opposite().Mask() ) ) )
        {
            fail = true;
            failReason = wxT( "fail-primary-target" );
        }
    }


    if( aEntry.HasEntryLines() )
    {
        if( !aEntry.Entry().CheckConnectionAngle( *this, mask ) )
        {
            fail = true;
            failReason = wxT( "fail-entry-angle" );
        }


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

    if( !fail && aTarget.HasEntryLines() )
    {
        DP_GATEWAY t( aTarget );
        t.Reverse();

        if( !CheckConnectionAngle( t.Entry(), mask ) )
        {
            fail = true;
            failReason = wxT( "fail-exit-angle" );
        }

        sum_p.Append( t.Entry().CP() );
        sum_n.Append( t.Entry().CN() );
    }

    m_p = sum_p;
    m_n = sum_n;
    m_p.Simplify2();
    m_n.Simplify2();

    if( !fail )
    {
        float coupledLength;
        bool  gapOK;
        std::tie( coupledLength, gapOK ) = CoupledLength( m_p, m_n );

        if( !gapOK )
        {
            fail = true;
            failReason = wxT( "fail-gap" );
        }

        float minLength = std::min( m_p.Length(), m_n.Length() );
        if( minLength >= 1.0 )
            aBestCouplingRatio = coupledLength / minLength;
        else
            aBestCouplingRatio = 0;
    }


    auto ip_p = p.SelfIntersecting();
    auto ip_n = n.SelfIntersecting();

    if( !fail && ( ip_p || ip_n ) )
    {
        PNS_DBG( dbg, AddPoint, ip_p->p, RED, 20000, wxT( "ip+" ) );
        PNS_DBG( dbg, AddPoint, ip_n->p, BLUE, 20000, wxT( "ip-" ) );

        fail = true;
        failReason = wxT( "fail-self-intersect" );
    }


    if( !fail && m_p.Intersects( m_n ) )
    {
        fail = true;
        failReason = wxT( "fail-intersect" );
    }
    int distP = 0, distN = 0, threshold = 0;

    if( aFitVias )
    {
        distP = m_n.Distance( m_p.CLastPoint() );
        distN = m_p.Distance( m_n.CLastPoint() );

        threshold = ( m_dims.ViaDiameter() / 2 + 1 ) + m_dims.MinClearance() - m_dims.Width() / 2;

        if( distP < threshold || distN < threshold )
        {
            fail = true;
            failReason = wxString::Format( "fail-vias dp=%d dn=%d thr=%d cl=%d w=%d", distP, distN, threshold,
                                           m_dims.MinClearance(), m_dims.Width() );
        }
    }

    if( !fail )
        failReason = wxT( "OK" );

    if( entryIsStraight && targetIsStraight )
        aAspectRatio = 1.0;


    PNS_DBG( dbg, BeginGroup,
             wxString::Format( "fit-%s-%s-%s fail=%d gap=[%s] prioE=%d prioT=%d d=%d eis=%d tis=%d cpr=%.2f ar =%.2f v "
                               "%d %d %d fv %d",
                               failReason, aEntry.GetName(), aTarget.GetName(), fail ? 1 : 0,
                               ::PNS::Format( m_dims.GapConstraint() ), aEntry.Priority(), aTarget.Priority(),
                               aEntry.IsDiagonal() ? 1 : 0, entryIsStraight ? 1 : 0, targetIsStraight ? 1 : 0,
                               aBestCouplingRatio, aAspectRatio, distP, distN, threshold, aFitVias ? 1 : 0 ),
             0 );
    PNS_DBG( dbg, AddShape, &m_p, RED, 100000, wxT( "+" ) );
    PNS_DBG( dbg, AddShape, &m_n, BLUE, 100000, wxT( "-" ) );

    PNS_DBGN( dbg, EndGroup );


    return !fail;
}

const wxString DP_GATEWAY::GetName() const
{
    wxString s = wxString::Format("%s [p:%d d:%s/%s]", m_name, m_priority, m_dirP.Format(), m_dirN.Format() );
    return s;
}


bool DIFF_PAIR::CheckConnectionAngle( const DIFF_PAIR& aOther, int aAllowedAngles ) const
{
    bool checkP, checkN;

    if( m_p.SegmentCount() == 0 || aOther.m_p.SegmentCount() == 0 )
    {
        checkP = true;
    }
    else
    {
        DIRECTION_45 p0( m_p.CSegment( -1 ) );
        DIRECTION_45 p1( aOther.m_p.CSegment( 0 ) );

        checkP = ( p0.Angle( p1 ) & aAllowedAngles ) != 0;
    }

    if( m_n.SegmentCount() == 0 || aOther.m_n.SegmentCount() == 0 )
    {
        checkN = true;
    }
    else
    {
        DIRECTION_45 n0( m_n.CSegment( -1 ) );
        DIRECTION_45 n1( aOther.m_n.CSegment( 0 ) );

        checkN = ( n0.Angle( n1 ) & aAllowedAngles ) != 0;
    }

    return checkP && checkN;
}


const DIFF_PAIR DP_GATEWAY::Entry() const
{
    return DIFF_PAIR( m_entryP, m_entryN, 0 );
}


std::optional<DP_GATEWAY> DP_GATEWAY::Extend( int aLength )
{
    DIRECTION_45 dP = m_dirP;
    DIRECTION_45 dN = m_dirN;

    if( dP != dN )
        return std::nullopt;

    DP_GATEWAY extended( *this );

    VECTOR2I d = dP.ToVector();
    VECTOR2I l = d.Resize( aLength );
    VECTOR2I perp = dP.Right().Right().ToVector();

    SEG sN( AnchorN(), AnchorN() + l );
    SEG sP( AnchorP(), AnchorP() + l );

    SEG test( sN.B, sN.B + perp );
    int dist = test.LineDistance( sP.B, true );

    SEG test2( sP.B, sP.B + perp );
    int dist2 = test2.LineDistance( sN.B, true );

    // fixme: rework
    const int epsilon = 10;

    if( dist < -epsilon )
    {
        sP.B = test.LineProject( sP.B );
    }
    else if( dist2 < -epsilon )
    {
        sN.B = test2.LineProject( sN.B );
    }


    extended.m_entryP.Append( sP.B );
    extended.m_entryN.Append( sN.B );
    extended.m_anchorP = sP.B;
    extended.m_anchorN = sN.B;

    return extended;
}


std::optional<DP_GATEWAY> DP_GATEWAY::AddTurns( bool aSide, bool a90Deg, bool aLeft, bool aWiggle )
{
    DIRECTION_45 dP = m_dirP;
    DIRECTION_45 dN = m_dirN;
    VECTOR2I     perp = dP.Right().Right().ToVector();
    VECTOR2I     str = dP.ToVector();

    auto dbg = ROUTER::GetInstance()->GetInterface()->GetDebugDecorator();


    SEG sN( AnchorN(), AnchorN() + perp );
    SEG sP( AnchorP(), AnchorP() + perp );
    SEG stest( AnchorN(), AnchorN() + str );

    bool invert = stest.Side( AnchorP() ) > 0;
    int  side = 0;

    const double turnAngle = aWiggle ? 22.6 : 22.5;
    int          gap = m_dims.Gap() + m_dims.Width();
    int          leadLen = (int) ( (double) (gap) *tan( turnAngle * M_PI / 180.0 ) ) + 1;

    PNS_DBG( dbg, Message,
             wxString::Format( "addturn orig %s gap %d %s %s lead %d", m_name, gap, dP.Format(), dN.Format(),
                               leadLen ) );


    SHAPE_LINE_CHAIN leadP45( EntryP() );
    SHAPE_LINE_CHAIN leadN45( EntryN() );
    VECTOR2I         dpr = dP.ToVector().Resize( leadLen );
    VECTOR2I         dnr = dN.ToVector().Resize( leadLen );
    const VECTOR2I&  lastN = EntryN().CLastPoint();
    const VECTOR2I&  lastP = EntryP().CLastPoint();

    if( side )
    {
        dpr = -dpr;
        dnr = -dnr;
    }

    leadP45.Append( EntryP().CLastPoint() + dpr );
    leadN45.Append( EntryN().CLastPoint() + dnr );

    if( !aLeft )
    {
        DP_GATEWAY gw45_r( *this );
        gw45_r.SetAnchors( leadP45.CLastPoint(), lastN );
        gw45_r.m_isDiagonal = !IsDiagonal();
        //gw45_r.SetPriority( 10 );
        gw45_r.SetEntryLines( leadP45, EntryN() );
        DIRECTION_45 dPR = ( invert ? dP.Left() : dP.Right() );
        DIRECTION_45 dNR = ( invert ? dN.Left() : dN.Right() );
        gw45_r.SetDirections( dPR, dNR );
        gw45_r.SetPrimaryDirection( dPR );
        return gw45_r;
    }
    else
    {
        DP_GATEWAY gw45_l( *this );
        gw45_l.SetAnchors( lastP, leadN45.CLastPoint() );
        gw45_l.m_isDiagonal = !IsDiagonal();
        //gw45_l.SetPriority( 10 );
        gw45_l.SetEntryLines( EntryP(), leadN45 );
        DIRECTION_45 dPL = ( invert ? dP.Right() : dP.Left() );
        DIRECTION_45 dNL = ( invert ? dN.Right() : dN.Left() );
        gw45_l.SetDirections( dPL, dNL );
        gw45_l.SetPrimaryDirection( dPL );
        return gw45_l;
    }
}


void DP_GATEWAYS::addGateway( DP_GATEWAY& aGw, const wxString& name, bool aAddTurns )
{
    if( name != wxT( "" ) )
        aGw.SetName( name );

    aGw.SetDimensions( m_dims );
    m_gateways.push_back( aGw );

    if( aAddTurns )
    {
        auto router = ROUTER::GetInstance();
        const double widthToMiterRatio = router->Settings().DiffPairWidthToMiterRatio();
        const int    extensionDist = (int) ( widthToMiterRatio * (double) aGw.Dimensions().Width() );
        std::optional<DP_GATEWAY> extend, turn45_l, turn45_r, turn45_lw, turn45_rw;
        std::optional<DP_GATEWAY> turn45_le, turn45_re;

        turn45_l = aGw.AddTurns( false, false, true, false );
        if( turn45_l.has_value() )
        {
            addGateway( *turn45_l, "45-l" );
            turn45_le = turn45_l->Extend( extensionDist );
            if( turn45_le.has_value() )
            {
                addGateway( *turn45_le, "45-lext" );

                std::optional<DP_GATEWAY> turn90_lel = turn45_le->AddTurns( false, false, true, false );
                if( turn90_lel.has_value() )
                    addGateway( *turn90_lel, "90-lext-l" );
            }
        }
        //if( turn45_lw = aGw.AddTurns( false, false, true, true ) )
        //  addGateway( *turn45_lw, "45-lw" );
        turn45_r = aGw.AddTurns( false, false, false, false );
        if( turn45_r.has_value() )
        {
            addGateway( *turn45_r, "45-r" );
            turn45_re = turn45_r->Extend( extensionDist );
            if( turn45_re.has_value() )
            {
                std::optional<DP_GATEWAY> turn90_rer = turn45_re->AddTurns( false, false, false, false );
                if( turn90_rer.has_value() )
                    addGateway( *turn90_rer, "90-rext-r" );
            }
        }

        //if( turn45_rw = aGw.AddTurns( false, false, false, true ) )
        //  addGateway( *turn45_rw, "45-rw" );
    }
}


void DP_GATEWAYS::BuildOrthoProjections( DP_GATEWAYS& aEntries, const VECTOR2I& aCursorPos,
                                         int aOrthoScore )
{
    int cnt = 0;
    for( const DP_GATEWAY& g : aEntries.Gateways() )
    {
        VECTOR2I midpoint( ( g.AnchorP() + g.AnchorN() ) / 2 );
        SEG guide_s( midpoint, midpoint + VECTOR2I( 1, 0 ) );
        SEG guide_d( midpoint, midpoint + VECTOR2I( 1, 1 ) );

        VECTOR2I proj_s = guide_s.LineProject( aCursorPos );
        VECTOR2I proj_d = guide_d.LineProject( aCursorPos );

        int dist_s = ( proj_s - aCursorPos ).EuclideanNorm();
        int dist_d = ( proj_d - aCursorPos ).EuclideanNorm();

        VECTOR2I proj = ( dist_s < dist_d ? proj_s : proj_d );

        DP_GATEWAYS targets( m_dims );
        targets.m_fitVias = m_fitVias;

        targets.BuildForCursor( proj );

        for( DP_GATEWAY t : targets.Gateways() )
        {
            t.SetPriority( aOrthoScore );
            t.SetName( wxString::Format("ortho-%d", cnt).ToStdString() );
            m_gateways.push_back( t );
            cnt++;
        }
    }
}


std::vector<DP_GATEWAYS::FIT_RESULT> DP_GATEWAYS::FitGateways( DP_GATEWAYS& aEntry, DP_GATEWAYS& aTarget,
                                                               bool aFitVias )
{
    std::vector<DP_GATEWAYS::FIT_RESULT> results;

    auto dbg = ROUTER::GetInstance()->GetInterface()->GetDebugDecorator();

    PNS_DBG( dbg, BeginGroup, wxT( "fit-gateways" ), 0 );
    
    for( bool diagonal : { true, false } )
    {
        for( const DP_GATEWAY& g_entry : aEntry.Gateways() )
        {
            for( const DP_GATEWAY& g_target : aTarget.Gateways() )
            {
                FIT_RESULT result;
                result.score = g_entry.Priority();
                result.score += g_target.Priority();

                DIFF_PAIR l( m_dims );
                if( l.BuildInitial( g_entry, g_target, diagonal, aFitVias, result.coupledRatio, result.aspectRatio ) )
                {
                    result.p = l.CP();
                    result.n = l.CN();
                    result.diagonal = diagonal;
                    result.entry = g_entry;
                    result.target = g_target;
                    results.push_back( result );
                }
            }
        }
    }
    PNS_DBGN( dbg, EndGroup );

    return results;
}


bool DP_GATEWAYS::checkDiagonalAlignment( const VECTOR2I& a, const VECTOR2I& b ) const
{
    VECTOR2I dir( std::abs( a.x - b.x ), std::abs( a.y - b.y ) );

    return ( dir.x == 0 && dir.y != 0 ) || ( dir.x == dir.y ) || ( dir.y == 0 && dir.x != 0 );
}


void DP_GATEWAYS::FilterByOrientation( int aDirectionMask )
{
    std::erase_if( m_gateways,
                   [aDirectionMask]( const DP_GATEWAY& dp )
                   {
                       return ( !( !dp.HasPrimaryDirection() || ( dp.PrimaryDirectionMask() & aDirectionMask ) ) );
                   } );
}


static VECTOR2I makeGapVector( VECTOR2I dir, int length )
{
    int l = length / 2;
    VECTOR2I rv;

    if( dir.EuclideanNorm() == 0 )
        return dir;

    do
	{
        rv = dir.Resize( l );
        l++;
    } while( ( rv * 2 ).EuclideanNorm() < length );

    return rv;
}


void DP_GATEWAYS::BuildFromPrimitivePair( const DP_PRIMITIVE_PAIR& aPair, bool aPreferDiagonal )
{
    VECTOR2I majorDirection;
    VECTOR2I p0_p, p0_n;
    int orthoFanDistance = 0;
    int diagFanDistance = 0;
    const int gap = m_dims.Gap() + m_dims.Width();
    const SHAPE* shP = nullptr;

    if( aPair.PrimP() == nullptr )
    {
        BuildGeneric( aPair.AnchorP(), aPair.AnchorN(), 0, true );
        return;
    }

    const int pvMask = ITEM::SOLID_T | ITEM::VIA_T;

    if( aPair.PrimP()->OfKind( pvMask ) && aPair.PrimN()->OfKind( pvMask ) )
    {
        p0_p = aPair.AnchorP();
        p0_n = aPair.AnchorN();

        // TODO(JE) padstacks
        shP = aPair.PrimP()->Shape( -1 );
    }
    else if( aPair.PrimP()->OfKind( ITEM::SEGMENT_T | ITEM::ARC_T )
             && aPair.PrimN()->OfKind( ITEM::SEGMENT_T | ITEM::ARC_T ) )
    {
        buildDpContinuation( aPair, aPreferDiagonal );

        return;
    }

    majorDirection = ( p0_p - p0_n ).Perpendicular();

    int colinearityThreshold = DP_PRIMITIVE_PAIR::DP_ASSUME_PRIMS_COLINEAR_FACTOR * aPair.GetMinDimension();

    if( shP == nullptr )
        return;

    switch( shP->Type() )
    {
    case SH_CIRCLE:
        BuildGeneric ( p0_p, p0_n, colinearityThreshold, true );
        return;

    case SH_RECT:
    {
        int w = static_cast<const SHAPE_RECT*>( shP )->GetWidth();
        int h = static_cast<const SHAPE_RECT*>( shP )->GetHeight();

        if( w < h )
            std::swap( w, h );

        orthoFanDistance = ( w + 1 )* 3 / 2;
        diagFanDistance = ( w - h );
        break;
    }

    case SH_SEGMENT:
    {
        int w = static_cast<const SHAPE_SEGMENT*>( shP )->GetWidth();
        SEG s = static_cast<const SHAPE_SEGMENT*>( shP )->GetSeg();

        orthoFanDistance = w + ( s.B - s.A ).EuclideanNorm();
        diagFanDistance = ( s.B - s.A ).EuclideanNorm();
        break;
    }

    case SH_SIMPLE:
    case SH_COMPOUND:
    {
        BOX2I bbox = shP->BBox();
        int   w = bbox.GetWidth();
        int   h = bbox.GetHeight();

        if( w < h )
            std::swap( w, h );

        orthoFanDistance = ( w + 1 )* 3 / 2;
        diagFanDistance = ( w - h );
        break;
    }

    default:
        wxFAIL_MSG( wxString::Format( wxT( "Unsupported starting primitive: %d (%s)." ),
                                      shP->Type(),
                                      SHAPE_TYPE_asString( shP->Type() ) ) );
        break;
    }

    if( checkDiagonalAlignment( p0_p, p0_n ) )
    {
        int padDist = ( p0_p - p0_n ).EuclideanNorm();

        for( int k = 0; k < 2; k++ )
        {
            VECTOR2I dir, dp, dv;

            if( k == 0 )
                dir = makeGapVector( majorDirection, orthoFanDistance );
            else
                dir = makeGapVector( majorDirection, diagFanDistance );

            int d = std::max( 0, padDist - gap );
            dp = makeGapVector( dir, d );
            dv = makeGapVector( p0_n - p0_p, d );

            for( int i = 0; i < 2; i++ )
            {
                int sign = i ? -1 : 1;

                VECTOR2I gw_p( p0_p + sign * ( dir + dp ) + dv );
                VECTOR2I gw_n( p0_n + sign * ( dir + dp ) - dv );

                SHAPE_LINE_CHAIN entryP( { p0_p, p0_p + sign * dir, gw_p } );
                SHAPE_LINE_CHAIN entryN( { p0_n, p0_n + sign * dir, gw_n } );

                DP_GATEWAY gw( gw_p, gw_n, false );

                gw.SetName( wxString::Format( "pp-%d-%d", k, i ).ToStdString() );
                gw.SetEntryLines( entryP, entryN );

                DIRECTION_45 dir1 = DIRECTION_45( sign * dir );

                gw.SetDimensions( m_dims );
                gw.SetPriority( 101 - k );
                gw.SetDirections( dir1, dir1 );
                gw.SetPrimaryDirection( dir1 );
                m_gateways.push_back( gw );

                auto gw_ext = gw.Extend( 400000 );
                if( gw_ext )
                    addGateway( *gw_ext, "pp-ext", true );
            }
        }
    }

    BuildGeneric( p0_p, p0_n, colinearityThreshold, true );
}


void DP_GATEWAYS::BuildForCursor( const VECTOR2I& aCursorPos, int aDirectionMask  )
{
    int gap = m_fitVias ? m_dims.ViaGap() + m_dims.ViaDiameter() : m_dims.Gap() + m_dims.Width();

    for( bool diagonal : { false, true } )
    {
        for( int i = 0; i < 4; i++ )
        {
            VECTOR2I dir;

            if( !diagonal )
            {
                dir = makeGapVector( VECTOR2I( gap, gap ), gap );

                if( i % 2 == 0 )
                    dir.x = -dir.x;

                if( i / 2 == 0 )
                    dir.y = -dir.y;
            }
            else
            {
                if( i /2 == 0 )
                    dir = VECTOR2I( (gap + 1) / 2 * ( ( i % 2 ) ? -1 : 1 ), 0 );
                else
                    dir = VECTOR2I( 0, (gap + 1) / 2 * ( ( i % 2 ) ? -1 : 1 ) );
            }

            if( m_fitVias )
            {
                DIRECTION_45 dirV( dir );
                BuildGeneric( aCursorPos + dir, aCursorPos - dir, 0, true, true );
            }
            else
            {
                DP_GATEWAY gw( aCursorPos + dir, aCursorPos - dir, diagonal );
                gw.SetName( wxString::Format( "cursor-%d-%d", diagonal ? 1 : 0, i ).ToStdString() );
                gw.SetPrimaryDirection( DIRECTION_45( dir ).Right().Right() );
                gw.AddPrimaryDirection( DIRECTION_45( dir ).Right().Right().Opposite() );
                m_gateways.emplace_back( gw );
            }
        }
    }
}


void DP_GATEWAYS::buildEntries( DP_GATEWAY& aGw, const VECTOR2I& p0_p, const VECTOR2I& p0_n )
{
    if( !aGw.HasEntryLines() )
    {
        SHAPE_LINE_CHAIN lead_p = DIRECTION_45().BuildInitialTrace( aGw.AnchorP(), p0_p, aGw.IsDiagonal() ).Reverse();
        SHAPE_LINE_CHAIN lead_n = DIRECTION_45().BuildInitialTrace( aGw.AnchorN(), p0_n, aGw.IsDiagonal() ).Reverse();
        aGw.SetEntryLines( lead_p, lead_n );
    }
}


void DP_GATEWAYS::buildDpContinuation( const DP_PRIMITIVE_PAIR& aPair, bool aIsDiagonal )
{
    auto dbg = ROUTER::GetInstance()->GetInterface()->GetDebugDecorator();

    DP_GATEWAY gw( aPair.AnchorP(), aPair.AnchorN(), aIsDiagonal );
    gw.SetPriority( 100 );
    m_gateways.push_back( gw );

    if( !aPair.Directional() )
        return;

    DIRECTION_45 dP = aPair.DirP();
    DIRECTION_45 dN = aPair.DirN();

    if( dN != dP )
        return;

    VECTOR2I perp = dP.Right().Right().ToVector();

    SEG sN( aPair.AnchorN(), aPair.AnchorN() + perp );
    SEG sP( aPair.AnchorP(), aPair.AnchorP() + perp );

    SEGMENT* primN = static_cast<SEGMENT*>( aPair.PrimN() );
    SEGMENT* primP = static_cast<SEGMENT*>( aPair.PrimP() );

    int gap = primP->Seg().LineDistance( aPair.AnchorN() );

    OPT_VECTOR2I ipN = sN.IntersectLines( primP->Seg() );
    OPT_VECTOR2I ipP = sP.IntersectLines( primN->Seg() );

    PNS_DBG( dbg, Message, wxString::Format( "buildDpCont: gap=%d dn=%s dp=%s", gap, dN.Format(), dP.Format() ) );
    PNS_DBG( dbg, AddItem, aPair.PrimP(), RED, 100000, "+" );
    PNS_DBG( dbg, AddItem, aPair.PrimN(), BLUE, 100000, "-" );

    SHAPE_LINE_CHAIN leadP, leadN;

    leadP.Append( aPair.AnchorP() );
    leadN.Append( aPair.AnchorN() );

    if( ipN && !primP->Seg().Contains( *ipN ) )
    {
        leadP.Append( *ipN );
    }

    if( ipP && !primN->Seg().Contains( *ipP ) )
    {
        leadN.Append( *ipP );
    }

    // now leadP/leadN are aligned for a 0/180-degree turn

    DP_GATEWAY gw0( leadP.CPoint( -1 ), leadN.CPoint( -1 ), !aIsDiagonal );
    gw0.SetPriority( 100 );
    gw0.SetEntryLines( leadP, leadN );
    gw0.SetName( "0" );
    gw0.SetDirections( dP, dN );
    gw0.SetPrimaryDirection( dP );
    gw0.SetDimensions( m_dims );

    addGateway( gw0, "gw0", true );

    DP_GATEWAY gw180( gw0 );
    gw180.SetDirections( dP.Opposite(), dN.Opposite() );
    gw0.SetPrimaryDirection( dP.Opposite() );
    addGateway( gw180, "gw180", true );
}


void DP_GATEWAYS::BuildGeneric( const VECTOR2I& p0_p, const VECTOR2I& p0_n, int aColinearityThreshold, bool aBuildEntries,
                                bool aViaMode )
{
    SEG st_p[2], st_n[2];
    SEG d_n[2], d_p[2];
    const int gap = m_dims.Gap() + m_dims.Width();

    const int padToGapThreshold = 3;
    int padDist = ( p0_n - p0_p ).EuclideanNorm();

    st_p[0] = SEG(p0_p + VECTOR2I( -100, 0 ), p0_p + VECTOR2I( 100, 0 ) );
    st_n[0] = SEG(p0_n + VECTOR2I( -100, 0 ), p0_n + VECTOR2I( 100, 0 ) );
    st_p[1] = SEG(p0_p + VECTOR2I( 0, -100 ), p0_p + VECTOR2I( 0, 100 ) );
    st_n[1] = SEG(p0_n + VECTOR2I( 0, -100 ), p0_n + VECTOR2I( 0, 100 ) );
    d_p[0] = SEG( p0_p + VECTOR2I( -100, -100 ), p0_p + VECTOR2I( 100, 100 ) );
    d_p[1] = SEG( p0_p + VECTOR2I( 100, -100 ), p0_p + VECTOR2I( -100, 100 ) );
    d_n[0] = SEG( p0_n + VECTOR2I( -100, -100 ), p0_n + VECTOR2I( 100, 100 ) );
    d_n[1] = SEG( p0_n + VECTOR2I( 100, -100 ), p0_n + VECTOR2I( -100, 100 ) );

    DIRECTION_45 fallbackDir( p0_p - p0_n );

    int mask = fallbackDir.Right().Right().Mask() | fallbackDir.Right().Right().Opposite().Mask();

    m_gateways.emplace_back( p0_p, p0_n, false, DIRECTION_45::ANG_UNDEFINED, -1, mask, "gen-fallback" );

    // midpoint exit & side-by exits
    for( int i = 0; i < 2; i++ )
    {
        int threshold = aColinearityThreshold ? aColinearityThreshold : DIFF_PAIR::DP_PARALLELITY_THRESHOLD;
        bool straightColl = st_p[i].ApproxCollinear( st_n[i], threshold );
        bool diagColl = d_p[i].ApproxCollinear( d_n[i], threshold );

        if( straightColl || diagColl )
        {
            VECTOR2I dir = makeGapVector( p0_n - p0_p, gap + gap / 2 );
            VECTOR2I m = ( p0_p + p0_n ) / 2;
            int prio = ( padDist > padToGapThreshold * gap ) ? 2 : 1;

            if( !aViaMode )
            {
                m_gateways.emplace_back( m - dir, m + dir, diagColl, DIRECTION_45::ANG_OBTUSE, prio, DIRECTION_45::AllDirectionsMask(), wxString::Format( "gen-mp-gap %d", gap ).ToStdString() );

                dir = makeGapVector( p0_n - p0_p, 2 * gap );
                m_gateways.emplace_back( p0_p - dir, p0_p - dir + dir.Perpendicular(), diagColl, DIRECTION_45::ANG_OBTUSE, 0, DIRECTION_45::AllDirectionsMask(), "gen-d1" );
                m_gateways.emplace_back( p0_p - dir, p0_p - dir - dir.Perpendicular(), diagColl, DIRECTION_45::ANG_OBTUSE, 0, DIRECTION_45::AllDirectionsMask(), "gen-d2" );
                m_gateways.emplace_back( p0_n + dir + dir.Perpendicular(), p0_n + dir, diagColl, DIRECTION_45::ANG_OBTUSE, 0, DIRECTION_45::AllDirectionsMask(), "gen-d3" );
                m_gateways.emplace_back( p0_n + dir - dir.Perpendicular(), p0_n + dir, diagColl, DIRECTION_45::ANG_OBTUSE, 0, DIRECTION_45::AllDirectionsMask(), "gen-d4" );
            }
        }
    }

    for( int i = 0; i < 2; i++ )
    {
        for( int j = 0; j < 2; j++ )
        {
            OPT_VECTOR2I ips[2];

            ips[0] = d_n[i].IntersectLines( d_p[j] );
            ips[1] = st_p[i].IntersectLines( st_n[j] );

            if( d_n[i].Collinear( d_p[j] ) )
                ips[0] = OPT_VECTOR2I();

            if( st_p[i].Collinear( st_p[j] ) )
                ips[1] = OPT_VECTOR2I();

            DIRECTION_45 dir1 = DIRECTION_45( p0_p - p0_n ).Left().Left();

            // diagonal-diagonal and straight-straight cases - the most typical case if the pads
            // are on the same straight/diagonal line
            for( int k = 0; k < 2; k++ )
            {
                if( ips[k] )
                {
                    const VECTOR2I m( *ips[k] );

                    if( m != p0_p && m != p0_n )
                    {
                        int      prio = ( padDist > padToGapThreshold * gap ? 10 : 20 );
                        VECTOR2I g_p( ( p0_p - m ).Resize( ceil( (double) gap * M_SQRT1_2 ) ) );
                        VECTOR2I g_n( ( p0_n - m ).Resize( ceil( (double) gap * M_SQRT1_2 ) ) );

                        DP_GATEWAY gw( m + g_p, m + g_n, k == 0 ? true : false, DIRECTION_45::ANG_OBTUSE, prio, 0,
                                       "gen-s" );

                        DIRECTION_45 dir2( g_p );
                        DIRECTION_45 dir_next = dir1.IsObtuse( dir2 ) ? dir1.Opposite() : dir1;

                        gw.SetDimensions( m_dims );
                        gw.SetDirections( dir_next, dir_next );
                        gw.SetPrimaryDirection( dir_next );
                        buildEntries( gw, p0_p, p0_n );

                        addGateway( gw, "gw-gen-s", false );

                        auto gw_ext = gw.Extend( 400000 );
                        if( gw_ext && !aViaMode )
                            addGateway( *gw_ext, "gw-gen-s-ext", true );
                    }
                }
            }

            ips[0] = st_n[i].IntersectLines( d_p[j] );
            ips[1] = st_p[i].IntersectLines( d_n[j] );

            // diagonal-straight cases: 8 possibilities of "weirder" exists
            for( int k = 0; k < 2; k++ )
            {
                if( ips[k] )
                {
                    const VECTOR2I m( *ips[k] );

                    if( !aViaMode && m != p0_p && m != p0_n )
                    {
                        VECTOR2I g_p, g_n;

                        g_p = ( p0_p - m ).Resize( ceil( (double) gap * M_SQRT2 ) );
                        g_n = ( p0_n - m ).Resize( ceil( (double) gap ) );

                        if( angle( g_p, g_n ) != DIRECTION_45::ANG_ACUTE )
                            m_gateways.emplace_back( m + g_p, m + g_n, true );

                        g_p = ( p0_p - m ).Resize( gap );
                        g_n = ( p0_n - m ).Resize( ceil( (double) gap * M_SQRT2 ) );

                        if( angle( g_p, g_n ) != DIRECTION_45::ANG_ACUTE )
                            m_gateways.emplace_back( m + g_p, m + g_n, true );
                    }
                }
            }
        }
    }


    if( aBuildEntries )
    {
        for( auto&gw : m_gateways )
          buildEntries( gw, p0_p, p0_n );
    }

}


static int minDimensionForPrimitive( const ITEM* aPrim )
{
    if( const SEGMENT* seg = dyn_cast<const SEGMENT*>( aPrim ) )
        return seg->Width();
    else if( const ARC* arc = dyn_cast<const ARC*>( aPrim ) )
        return arc->Width();
    else
    {
        const SHAPE* shape = aPrim->Shape( -1 );
        if( !shape )
            return 0;

        const BOX2I& bbox = shape->BBox();
        return std::min( bbox.GetWidth(), bbox.GetHeight() );
    }

    return 0;
}


int DP_PRIMITIVE_PAIR::GetMinDimension() const
{
    if( !m_primN || !m_primN )
        return 0;

    return std::min( minDimensionForPrimitive( m_primN ), minDimensionForPrimitive( m_primP ) );
}


DP_PRIMITIVE_PAIR DIFF_PAIR::EndingPrimitives()
{
    if( m_hasVias )
    {
        return DP_PRIMITIVE_PAIR( &m_via_p, &m_via_n );
    }
    else
    {
        const LINE lP( PLine() );
        const LINE lN( NLine() );

        SEGMENT sP( lP, lP.CSegment( -1 ) );
        SEGMENT sN( lN, lN.CSegment( -1 ) );

        DP_PRIMITIVE_PAIR dpair( &sP, &sN );

        if( PLine().IsLinked() )
        {
            auto lp = PLine().GetLink( PLine().LinkCount() - 1 );
            auto ln = NLine().GetLink( NLine().LinkCount() - 1 );
            dpair.SetPrimitives( lp, ln );
        }
        dpair.SetAnchors( sP.Seg().B, sN.Seg().B );

        return dpair;
    }
}


bool commonParallelProjection( SEG p, SEG n, SEG &pClip, SEG& nClip )
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


double DIFF_PAIR::Skew() const
{
    return m_p.Length() - m_n.Length();
}


void DIFF_PAIR::CoupledSegmentPairs( COUPLED_SEGMENTS_VEC& aPairs, 
        bool aUseGapConstraint,
        const std::optional<DP_GAP_CONSTRAINT>& aOverrideGapConstraint ) const
{
    SHAPE_LINE_CHAIN p (m_p);
    SHAPE_LINE_CHAIN n (m_n);

    // Do not simplify the line chains here, otherwise the indices will be invalid
    double threshold = ROUTER::GetInstance()->Settings().DiffPairGapCouplingRecognitionThreshold();

    MINOPTMAX<int> gapConstraint;
    gapConstraint.SetMin( 0 );
    gapConstraint.SetMax( (int) ( (double)m_dims.Width() * threshold ) );
 
    if ( aOverrideGapConstraint )
        gapConstraint = aOverrideGapConstraint.value();
    else if( aUseGapConstraint )
        gapConstraint = m_dims.GapConstraint();
    
    double opt = gapConstraint.Opt();

    if( !gapConstraint.HasMax() )
    {
        gapConstraint.SetMax( opt + 10000 );
    }

    if( !gapConstraint.HasMin() )
    {
        gapConstraint.SetMin( opt - 10000 );
    }

    for( int i = 0; i < p.SegmentCount(); i++ )
    {
        if( p.IsArcSegment( i ) )
            continue;

        for( int j = 0; j < n.SegmentCount(); j++ )
        {
            if( n.IsArcSegment( j ) )
                continue;
            
            SEG sp = p.Segment( i );
            SEG sn = n.Segment( j );

            SEG p_clip, n_clip;

            int64_t dist = std::abs( sp.Distance( sn ) ) - m_dims.Width();

            if( sp.ApproxParallel( sn, DIFF_PAIR::DP_PARALLELITY_THRESHOLD ) && gapConstraint.Matches( dist ) &&
                commonParallelProjection( sp, sn, p_clip, n_clip ) )
            {
                SEG test0 ( p_clip.A, n_clip.A );
                SEG test1 ( p_clip.B, n_clip.B );

		// fixme: gives false negatives
                /*if( m_p.Intersects( test0 ) )
                    continue;
                if( m_n.Intersects( test0 ) )
                    continue;
                if( m_p.Intersects( test1 ) )
                    continue;
                if( m_n.Intersects( test1 ) )
                    continue;*/

                COUPLED_SEGMENTS spair( p_clip, sp, i, n_clip, sn, j );

		        spair.linkP = m_line_p.FindLinkedSegment( sp );
                spair.linkN = m_line_n.FindLinkedSegment( sn );

                aPairs.push_back( spair );
            }
        }
    }
}


std::pair<int64_t, bool> DIFF_PAIR::CoupledLength( const SHAPE_LINE_CHAIN& aP, const SHAPE_LINE_CHAIN& aN ) const
{
    int64_t total = 0;
    int     clearance = m_dims.MinClearance();


    if( m_dims.GapConstraint().HasMin() && m_dims.GapConstraint().Min() < clearance )
        clearance = std::min( clearance, m_dims.GapConstraint().Min() );

    for( int i = 0; i < aP.SegmentCount(); i++ )
    {
        for( int j = 0; j < aN.SegmentCount(); j++ )
        {
            SEG sp = aP.CSegment( i );
            SEG sn = aN.CSegment( j );

            SEG p_clip, n_clip;

            int64_t dist = std::abs( sp.Distance( sn ) ) - m_dims.Width();

            if( dist < clearance )
                return { 0, false };

            if( !( sp.ApproxParallel( sn, DP_PARALLELITY_THRESHOLD ) ) )
                continue;

            if( !commonParallelProjection( sp, sn, p_clip, n_clip ) )
                continue;

            if( m_dims.GapConstraint().Matches( dist ) )
            {
                total += p_clip.Length();
            }
        }
    }

    return { total, true };
}


double DIFF_PAIR::CoupledLength() const
{
    COUPLED_SEGMENTS_VEC pairs;

    CoupledSegmentPairs( pairs );

    double l = 0.0;

    for( const COUPLED_SEGMENTS& pair : pairs )
        l += pair.coupledP.Length();

    return l;
}

double DIFF_PAIR::TotalLength() const
{
    double lenP = m_p.Length();
    double lenN = m_n.Length();

    return (lenN + lenP ) / 2.0;
}


int DIFF_PAIR::CoupledLength( const SEG& aP, const SEG& aN ) const
{
    SEG     p_clip, n_clip;
    int64_t dist = std::abs( aP.Distance( aN ) - m_dims.Width() );

    if( aP.ApproxParallel( aN ) && m_dims.GapConstraint().Matches( dist )
        && commonParallelProjection( aP, aN, p_clip, n_clip ) )
    {
        return p_clip.Length();
    }

    return 0;
}


std::optional<DP_PRIMITIVE_PAIR> DIFF_PAIR::BuildMidpairIntersection( PNS::SEGMENT* aStartSeg, const VECTOR2I& aP )
{
    bool             nHasStart = NLine().ContainsLink( aStartSeg );
    const PNS::LINE& refLine = nHasStart ? NLine() : PLine();
    const PNS::LINE& coupledLine = nHasStart ? PLine() : NLine();

    PNS::DIFF_PAIR::COUPLED_SEGMENTS_VEC csVec;
    CoupledSegmentPairs( csVec );
    std::optional<PNS::DP_PRIMITIVE_PAIR> prims;

    VECTOR2I pproj = refLine.CLine().NearestPoint( aP );

    // coupled segments take priority
    for( auto& cpair : csVec )
    {
        if( cpair.coupledN.Contains( pproj ) )
        {
            auto cproj = cpair.coupledP.LineProject( pproj );
            prims = PNS::DP_PRIMITIVE_PAIR( cproj, pproj );
            prims->SetPrimitives( cpair.linkP, cpair.linkN );
            prims->SetName( wxT( "prim-coupled-p" ) );
            break;
        }
        else if( cpair.coupledP.Contains( pproj ) )
        {
            auto cproj = cpair.coupledN.LineProject( pproj );
            prims = PNS::DP_PRIMITIVE_PAIR( pproj, cproj );
            prims->SetPrimitives( cpair.linkP, cpair.linkN );
            prims->SetName( wxT( "prim-coupled-n" ) );
            break;
        }
    }

    // parallel segments, but noncoupled parts (bends, corner, etc) go second
    if( !prims )
    {
        for( auto& cpair : csVec )
        {
            auto origP = PLine().CSegment( cpair.indexP );
            auto origN = NLine().CSegment( cpair.indexN );

            auto dirP = DIRECTION_45( origP );
            auto dirN = DIRECTION_45( origN );

            if( dirP != dirN )
                continue;


            if( origN.Contains( pproj ) )
            {
                auto cproj = origP.LineProject( pproj );
                cproj = origP.NearestPoint( cproj );
                prims = PNS::DP_PRIMITIVE_PAIR( cproj, pproj );
                prims->SetPrimitives( cpair.linkP, cpair.linkN );
                prims->SetName( wxT( "prim-extend-n" ) );
                break;
            }
            else if( origP.Contains( pproj ) )
            {
                auto cproj = origN.LineProject( pproj );
                cproj = origN.NearestPoint( cproj );
                prims = PNS::DP_PRIMITIVE_PAIR( pproj, cproj );
                prims->SetPrimitives( cpair.linkP, cpair.linkN );
                prims->SetName( wxT( "prim-extend-p" ) );
                break;
            }
        }
    }

    // still nothing? take the nearest vertex of the complement track
    if( !prims )
    {
        auto nearest = coupledLine.CLine().NearestPoint( pproj );

        if( nHasStart )
        {
            prims = PNS::DP_PRIMITIVE_PAIR( nearest, pproj );
            prims->SetPrimitives( coupledLine.FindLinkContainingVertex( nearest ),
                                  refLine.FindLinkContainingVertex( pproj ) );
        }
        else
        {
            prims = PNS::DP_PRIMITIVE_PAIR( pproj, nearest );
            prims->SetPrimitives( refLine.FindLinkContainingVertex( pproj ),
                                  coupledLine.FindLinkContainingVertex( nearest ) );
        }

        prims->SetName( wxT( "nearest-fallback" ) );
    }

    return prims;
}


int DIFF_PAIR::GuessMostLikelyGap() const
{
    const int                            gapTollerance = 100;
    PNS::DIFF_PAIR::COUPLED_SEGMENTS_VEC csVec;

    CoupledSegmentPairs( csVec );

    std::map<int, int> gapMap;

    for( auto& cs : csVec )
    {
        auto segP = dyn_cast<SEGMENT*>( cs.linkP );
        auto segN = dyn_cast<SEGMENT*>( cs.linkN );

        if( !segN || !segP )
            continue;

        int gap = cs.coupledN.LineDistance( cs.coupledP.A ) - ( segP->Width() + segN->Width() ) / 2;

        auto iter = gapMap.lower_bound( gap - gapTollerance );
        for( ; iter != gapMap.end(); ++iter )
        {
            if( iter->first < gap + gapTollerance )
            {
                iter->second += cs.coupledN.Length();
                break;
            }
        }

        if( iter == gapMap.end() )
            gapMap[gap] = cs.coupledN.Length();
    }

    int bestGapLen = 0;
    int bestGap = 0;

    for( auto iter : gapMap )
    {
        if( bestGapLen < iter.second )
        {
            bestGapLen = iter.second;
            bestGap = iter.first;
        }
    }

    return bestGap;
}


const wxString DP_DIMENSIONS::Format() const
{
    wxString ret = wxString::Format( "w:%d gap:%d vgap:%d vdiam:%d mincl:%d gap:[%s]", m_width, m_gap, m_viaGap,
                                     m_viaDiameter, m_minClearance, ::PNS::Format( m_gapConstraint ) );

    return ret;
}
}
