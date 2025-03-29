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

#include "pns_debug_decorator.h"
#include "pns_itemset.h"
#include "pns_meander_placer.h"
#include "pns_node.h"
#include "pns_router.h"
#include "pns_solid.h"
#include "pns_topology.h"

namespace PNS {

MEANDER_PLACER::MEANDER_PLACER( ROUTER* aRouter ) :
    MEANDER_PLACER_BASE( aRouter )
{
    m_currentNode = nullptr;

    // Init temporary variables (do not leave uninitialized members)
    m_initialSegment = nullptr;
    m_lastLength = 0;
    m_lastStatus = TOO_SHORT;
    m_padToDieLength = 0;
}


MEANDER_PLACER::~MEANDER_PLACER()
{
}


NODE* MEANDER_PLACER::CurrentNode( bool aLoopsRemoved ) const
{
    if( !m_currentNode )
        return m_world;

    return m_currentNode;
}


bool MEANDER_PLACER::Start( const VECTOR2I& aP, ITEM* aStartItem )
{
    if( !aStartItem || !aStartItem->OfKind( ITEM::SEGMENT_T | ITEM::ARC_T ) )
    {
        Router()->SetFailureReason( _( "Please select a track whose length you want to tune." ) );
        return false;
    }

    m_initialSegment = static_cast<LINKED_ITEM*>( aStartItem );
    m_currentNode    = nullptr;
    m_currentStart   = getSnappedStartPoint( m_initialSegment, aP );

    m_world = Router()->GetWorld()->Branch();
    m_originLine = m_world->AssembleLine( m_initialSegment );

    TOPOLOGY topo( m_world );
    m_tunedPath = topo.AssembleTuningPath( Router()->GetInterface(), m_initialSegment, &m_startPad_n, &m_endPad_n );

    m_padToDieLength = 0;

    if( m_startPad_n )
        m_padToDieLength += m_startPad_n->GetPadToDie();

    if( m_endPad_n )
        m_padToDieLength += m_endPad_n->GetPadToDie();

    m_world->Remove( m_originLine );

    m_currentWidth = m_originLine.Width();
    m_currentEnd = VECTOR2I( 0, 0 );

    return true;
}


long long int MEANDER_PLACER::origPathLength() const
{
    return m_padToDieLength + lineLength( m_tunedPath, m_startPad_n, m_endPad_n );
}


bool MEANDER_PLACER::Move( const VECTOR2I& aP, ITEM* aEndItem )
{
    return doMove( aP, aEndItem, m_settings.m_targetLength.Opt(), m_settings.m_targetLength.Min(),
                   m_settings.m_targetLength.Max() );
}


bool MEANDER_PLACER::doMove( const VECTOR2I& aP, ITEM* aEndItem, long long int aTargetLength,
                             long long int aTargetMin, long long int aTargetMax )
{
    if( m_currentStart == aP )
        return false;

    if( m_currentNode )
        delete m_currentNode;

    m_currentNode = m_world->Branch();

    SHAPE_LINE_CHAIN pre, tuned, post;

    m_originLine.CLine().Split( m_currentStart, aP, pre, tuned, post );

    m_result = MEANDERED_LINE( this, false );
    m_result.SetWidth( m_originLine.Width() );
    m_result.SetBaselineOffset( 0 );

    for( int i = 0; i < tuned.SegmentCount(); i++ )
    {
        if( tuned.IsArcSegment( i ) )
        {
            ssize_t arcIndex = tuned.ArcIndex( i );
            m_result.AddArc( tuned.Arc( arcIndex ) );
            i = tuned.NextShape( i );

            // NextShape will return -1 if last shape
            if( i < 0 )
                i = tuned.SegmentCount();

            continue;
        }

        bool      side = false;
        const SEG s = tuned.CSegment( i );

        if( m_settings.m_initialSide == 0 )
            side = s.Side( aP ) < 0;
        else
            side = m_settings.m_initialSide < 0;

        m_result.AddCorner( s.A );
        m_result.MeanderSegment( s, side );
        m_result.AddCorner( s.B );
    }

    long long int lineLen = origPathLength();

    m_lastLength = lineLen;
    m_lastStatus = TUNED;

    if( lineLen > m_settings.m_targetLength.Max() )
    {
        m_lastStatus = TOO_LONG;
    }
    else
    {
        m_lastLength = lineLen - tuned.Length();
        tuneLineLength( m_result, aTargetLength - lineLen );
    }

    for( const ITEM* item : m_tunedPath.CItems() )
    {
        if( const LINE* l = dyn_cast<const LINE*>( item ) )
        {
            PNS_DBG( Dbg(), AddItem, l, BLUE, 30000, wxT( "tuned-line" ) );

            m_router->GetInterface()->DisplayPathLine( l->CLine(), 1 );
        }
    }

    if( m_lastStatus != TOO_LONG )
    {
        tuned.Clear();

        for( MEANDER_SHAPE* m : m_result.Meanders() )
        {
            if( m->Type() != MT_EMPTY )
            {
                tuned.Append ( m->CLine( 0 ) );
            }
        }

        m_lastLength += tuned.Length();

        if( m_lastLength > aTargetMax )
            m_lastStatus = TOO_LONG;
        else if( m_lastLength < aTargetMin )
            m_lastStatus = TOO_SHORT;
        else
            m_lastStatus = TUNED;
    }

    m_finalShape.Clear();

    if( m_settings.m_keepEndpoints )
    {
        pre.Simplify();
        tuned.Simplify();
        post.Simplify();

        m_finalShape.Append( pre );
        m_finalShape.Append( tuned );
        m_finalShape.Append( post );
    }
    else
    {
        m_finalShape.Append( pre );
        m_finalShape.Append( tuned );
        m_finalShape.Append( post );
        m_finalShape.Simplify();
    }

    return true;
}


bool MEANDER_PLACER::FixRoute( const VECTOR2I& aP, ITEM* aEndItem, bool aForceFinish )
{
    if( !m_currentNode )
        return false;

    m_currentTrace = LINE( m_originLine, m_finalShape );
    m_currentNode->Add( m_currentTrace );
    CommitPlacement();

    return true;
}


bool MEANDER_PLACER::AbortPlacement()
{
    m_world->KillChildren();
    return true;
}


bool MEANDER_PLACER::HasPlacedAnything() const
{
     return m_currentTrace.SegmentCount() > 0;
}


bool MEANDER_PLACER::CommitPlacement()
{
    if( m_currentNode )
        Router()->CommitRouting( m_currentNode );

    m_currentNode = nullptr;
    return true;
}


bool MEANDER_PLACER::CheckFit( MEANDER_SHAPE* aShape )
{
    LINE l( m_originLine, aShape->CLine( 0 ) );

    if( m_currentNode->CheckColliding( &l ) )
        return false;

    int w = aShape->Width();
    int clearance = w + m_settings.m_spacing;

    return m_result.CheckSelfIntersections( aShape, clearance );
}


const ITEM_SET MEANDER_PLACER::Traces()
{
    m_currentTrace = LINE( m_originLine, m_finalShape );
    return ITEM_SET( &m_currentTrace );
}

const ITEM_SET MEANDER_PLACER::TunedPath()
{
    return m_tunedPath;
}

const VECTOR2I& MEANDER_PLACER::CurrentStart() const
{
    return m_currentStart;
}

const VECTOR2I& MEANDER_PLACER::CurrentEnd() const
{
    return m_currentEnd;
}

int MEANDER_PLACER::CurrentLayer() const
{
    return m_initialSegment->Layers().Start();
}


long long int MEANDER_PLACER::TuningResult() const
{
    if( m_lastLength )
        return m_lastLength;
    else
        return origPathLength();
}


MEANDER_PLACER::TUNING_STATUS MEANDER_PLACER::TuningStatus() const
{
    return m_lastStatus;
}

}
