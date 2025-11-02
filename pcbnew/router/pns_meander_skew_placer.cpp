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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "pns_node.h"
#include "pns_itemset.h"
#include "pns_topology.h"
#include "pns_meander_skew_placer.h"
#include "pns_solid.h"

#include "pns_router.h"
#include "pns_debug_decorator.h"

namespace PNS {

MEANDER_SKEW_PLACER::MEANDER_SKEW_PLACER ( ROUTER* aRouter ) :
    MEANDER_PLACER ( aRouter )
{
    // Init temporary variables (do not leave uninitialized members)
    m_coupledLength = 0;
    m_coupledDelay = 0;
    m_padToDieLengthN = 0;
    m_padToDieLengthP = 0;
    m_padToDieDelayN = 0;
    m_padToDieDelayP = 0;
}


MEANDER_SKEW_PLACER::~MEANDER_SKEW_PLACER( )
{
}


bool MEANDER_SKEW_PLACER::Start( const VECTOR2I& aP, ITEM* aStartItem )
{
    if( !aStartItem || !aStartItem->OfKind( ITEM::SEGMENT_T | ITEM::ARC_T) )
    {
        Router()->SetFailureReason( _( "Please select a differential pair track you want to tune." ) );
        return false;
    }

    m_initialSegment = static_cast<LINKED_ITEM*>( aStartItem );
    m_currentNode    = nullptr;
    m_currentStart   = getSnappedStartPoint( m_initialSegment, aP );

    m_world = Router()->GetWorld( )->Branch();
    m_originLine = m_world->AssembleLine( m_initialSegment );

    TOPOLOGY topo( m_world );
    m_tunedPath = topo.AssembleTrivialPath( m_initialSegment, nullptr, true );

    if( !topo.AssembleDiffPair ( m_initialSegment, m_originPair ) )
    {
        Router()->SetFailureReason( _( "Unable to find complementary differential pair "
                                       "net for skew tuning. Make sure the names of the nets belonging "
                                       "to a differential pair end with either _N/_P or +/-." ) );
        return false;
    }

    if( m_originPair.Gap() < 0 )
        m_originPair.SetGap( Router()->Sizes().DiffPairGap() );

    if( !m_originPair.PLine().SegmentCount() ||
        !m_originPair.NLine().SegmentCount() )
        return false;

    m_tunedPathP = topo.AssembleTuningPath( Router()->GetInterface(), m_originPair.PLine().GetLink( 0 ), &m_startPad_p,
                                            &m_endPad_p );

    m_padToDieLengthP = 0;
    m_padToDieDelayP = 0;

    if( m_startPad_p )
    {
        m_padToDieLengthP += m_startPad_p->GetPadToDie();
        m_padToDieDelayP += m_startPad_p->GetPadToDieDelay();
    }

    if( m_endPad_p )
    {
        m_padToDieLengthP += m_endPad_p->GetPadToDie();
        m_padToDieDelayP += m_endPad_p->GetPadToDieDelay();
    }

    m_tunedPathN = topo.AssembleTuningPath( Router()->GetInterface(), m_originPair.NLine().GetLink( 0 ), &m_startPad_n,
                                            &m_endPad_n );

    m_padToDieLengthN = 0;
    m_padToDieDelayN = 0;

    if( m_startPad_n )
    {
        m_padToDieLengthN += m_startPad_n->GetPadToDie();
        m_padToDieDelayN += m_startPad_n->GetPadToDieDelay();
    }

    if( m_endPad_n )
    {
        m_padToDieLengthN += m_endPad_n->GetPadToDie();
        m_padToDieDelayN += m_endPad_n->GetPadToDieDelay();
    }

    m_world->Remove( m_originLine );

    m_currentWidth = m_originLine.Width();
    m_currentEnd = VECTOR2I( 0, 0 );

    const BOARD_CONNECTED_ITEM* conItem = static_cast<BOARD_CONNECTED_ITEM*>( aStartItem->GetSourceItem() );
    m_netClass = conItem->GetEffectiveNetClass();
    m_settings.m_netClass = m_netClass;

    if ( m_originPair.NetP() == m_originLine.Net() )
    {
        m_coupledLength = m_padToDieLengthN + lineLength( m_tunedPathN, m_startPad_n, m_endPad_n );
        m_lastLength = m_padToDieLengthP + lineLength( m_tunedPathP, m_startPad_p, m_endPad_p );

        m_coupledDelay = m_padToDieDelayN + lineDelay( m_tunedPathN, m_startPad_n, m_endPad_n );
        m_lastDelay = m_padToDieDelayP + lineDelay( m_tunedPathP, m_startPad_p, m_endPad_p );

        m_tunedPath = m_tunedPathP;
    }
    else
    {
        m_coupledLength = m_padToDieLengthP + lineLength( m_tunedPathP, m_startPad_p, m_endPad_p );
        m_lastLength = m_padToDieLengthN + lineLength( m_tunedPathN, m_startPad_n, m_endPad_n );

        m_coupledDelay = m_padToDieDelayP + lineDelay( m_tunedPathP, m_startPad_p, m_endPad_p );
        m_lastDelay = m_padToDieDelayN + lineDelay( m_tunedPathN, m_startPad_n, m_endPad_n );

        m_tunedPath = m_tunedPathN;
    }

    calculateTimeDomainTargets();

    return true;
}


long long int MEANDER_SKEW_PLACER::origPathLength() const
{
    if ( m_originPair.NetP() == m_originLine.Net() )
        return m_padToDieLengthP + lineLength( m_tunedPath, m_startPad_p, m_endPad_p );

    return m_padToDieLengthN + lineLength( m_tunedPath, m_startPad_n, m_endPad_n );
}


int64_t MEANDER_SKEW_PLACER::origPathDelay() const
{
    if( m_originPair.NetP() == m_originLine.Net() )
        return m_padToDieDelayP + lineDelay( m_tunedPath, m_startPad_p, m_endPad_p );

    return m_padToDieDelayN + lineDelay( m_tunedPath, m_startPad_n, m_endPad_n );
}


long long int MEANDER_SKEW_PLACER::CurrentSkew() const
{
    return m_lastLength - m_coupledLength;
}


bool MEANDER_SKEW_PLACER::Move( const VECTOR2I& aP, ITEM* aEndItem )
{
    calculateTimeDomainTargets();

    bool isPositive = m_originPair.NetP() == m_originLine.Net();

    for( const ITEM* item : m_tunedPathP.CItems() )
    {
        if( const LINE* l = dyn_cast<const LINE*>( item ) )
        {
            PNS_DBG( Dbg(), AddItem, l, BLUE, 10000, wxT( "tuned-path-skew-p" ) );

            m_router->GetInterface()->DisplayPathLine( l->CLine(), isPositive ? 1 : 0 );
        }
    }

    for( const ITEM* item : m_tunedPathN.CItems() )
    {
        if( const LINE* l = dyn_cast<const LINE*>( item ) )
        {
            PNS_DBG( Dbg(), AddItem, l, YELLOW, 10000, wxT( "tuned-path-skew-n" ) );

            m_router->GetInterface()->DisplayPathLine( l->CLine(), isPositive ? 0 : 1 );
        }
    }

    return doMove( aP, aEndItem, m_coupledLength + m_settings.m_targetSkew.Opt(),
                   m_coupledLength + m_settings.m_targetSkew.Min(),
                   m_coupledLength + m_settings.m_targetSkew.Max() );
}


long long int MEANDER_SKEW_PLACER::TuningLengthResult() const
{
    return m_lastLength - m_coupledLength;
}


int64_t MEANDER_SKEW_PLACER::TuningDelayResult() const
{
    return m_lastDelay - m_coupledDelay;
}


void MEANDER_SKEW_PLACER::calculateTimeDomainTargets()
{
    auto calculateTargetSkew = [this]( const int64_t targetSkewDelay )
    {
        const int64_t curSkewDelay = m_lastDelay - m_coupledDelay;
        const int64_t skewDelayDifference = targetSkewDelay - curSkewDelay;

        int64_t skewLengthDiff = m_router->GetInterface()->CalculateLengthForDelay(
                std::abs( skewDelayDifference ), m_originPair.Width(), true, m_originPair.Gap(),
                m_router->GetCurrentLayer(), m_netClass );

        const int64_t curSkew = CurrentSkew();
        skewLengthDiff = skewDelayDifference > 0 ? skewLengthDiff : -skewLengthDiff;

        return static_cast<int>( curSkew + skewLengthDiff );
    };

    if( m_settings.m_isTimeDomain )
    {
        const int minSkew = calculateTargetSkew( m_settings.m_targetSkewDelay.Min() );
        m_settings.m_targetSkew.SetMin( static_cast<int>( minSkew ) );

        const int optSkew = calculateTargetSkew( m_settings.m_targetSkewDelay.Opt() );
        m_settings.m_targetSkew.SetOpt( static_cast<int>( optSkew ) );

        const int maxSkew = calculateTargetSkew( m_settings.m_targetSkewDelay.Max() );
        m_settings.m_targetSkew.SetMax( static_cast<int>( maxSkew ) );
    }
}
}
