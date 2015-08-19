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

#include <base_units.h> // God forgive me doing this...
#include <colors.h>

#include "trace.h"

#include "pns_node.h"
#include "pns_itemset.h"
#include "pns_topology.h"
#include "pns_meander_placer.h"
#include "pns_router.h"


PNS_MEANDER_PLACER::PNS_MEANDER_PLACER( PNS_ROUTER* aRouter ) :
    PNS_MEANDER_PLACER_BASE( aRouter )
{
    m_world = NULL;
    m_currentNode = NULL;

    // Init temporary variables (do not leave uninitialized members)
    m_initialSegment = NULL;
    m_lastLength = 0;
    m_lastStatus = TOO_SHORT;
}


PNS_MEANDER_PLACER::~PNS_MEANDER_PLACER()
{
}


PNS_NODE* PNS_MEANDER_PLACER::CurrentNode( bool aLoopsRemoved ) const
{
    if( !m_currentNode )
        return m_world;

    return m_currentNode;
}


bool PNS_MEANDER_PLACER::Start( const VECTOR2I& aP, PNS_ITEM* aStartItem )
{
    VECTOR2I p;

    if( !aStartItem || !aStartItem->OfKind( PNS_ITEM::SEGMENT ) )
    {
        Router()->SetFailureReason( _( "Please select a track whose length you want to tune." ) );
        return false;
    }

    m_initialSegment = static_cast<PNS_SEGMENT*>( aStartItem );

    p = m_initialSegment->Seg().NearestPoint( aP );

    m_currentNode = NULL;
    m_currentStart = p;

    m_world = Router()->GetWorld()->Branch();
    m_originLine = m_world->AssembleLine( m_initialSegment );

    PNS_TOPOLOGY topo( m_world );
    m_tunedPath = topo.AssembleTrivialPath( m_initialSegment );

    m_world->Remove( &m_originLine );

    m_currentWidth = m_originLine.Width();
    m_currentEnd = VECTOR2I( 0, 0 );

    return true;
}


int PNS_MEANDER_PLACER::origPathLength() const
{
    int total = 0;
    BOOST_FOREACH( const PNS_ITEM* item, m_tunedPath.CItems() )
    {
        if( const PNS_LINE* l = dyn_cast<const PNS_LINE*>( item ) )
        {
            total += l->CLine().Length();
        }
    }

    return total;
}


bool PNS_MEANDER_PLACER::Move( const VECTOR2I& aP, PNS_ITEM* aEndItem )
{
    return doMove( aP, aEndItem, m_settings.m_targetLength );
}


bool PNS_MEANDER_PLACER::doMove( const VECTOR2I& aP, PNS_ITEM* aEndItem, int aTargetLength )
{
    SHAPE_LINE_CHAIN pre, tuned, post;

    if( m_currentNode )
        delete m_currentNode;

    m_currentNode = m_world->Branch();

    cutTunedLine( m_originLine.CLine(), m_currentStart, aP, pre, tuned, post );

    m_result = PNS_MEANDERED_LINE( this, false );
    m_result.SetWidth( m_originLine.Width() );
    m_result.SetBaselineOffset( 0 );

    for( int i = 0; i < tuned.SegmentCount(); i++ )
    {
        const SEG s = tuned.CSegment( i );
        m_result.AddCorner( s.A );
        m_result.MeanderSegment( s );
        m_result.AddCorner( s.B );
    }

    int lineLen = origPathLength();

    m_lastLength = lineLen;
    m_lastStatus = TUNED;

    if( compareWithTolerance( lineLen, aTargetLength, m_settings.m_lengthTolerance ) > 0 )
    {
        m_lastStatus = TOO_LONG;
    } else {
        m_lastLength = lineLen - tuned.Length();
        tuneLineLength( m_result, aTargetLength - lineLen );
    }

    BOOST_FOREACH ( const PNS_ITEM* item, m_tunedPath.CItems() )
    {
        if( const PNS_LINE* l = dyn_cast<const PNS_LINE*>( item ) )
        {
            Router()->DisplayDebugLine( l->CLine(), 5, 30000 );
        }
    }

    if( m_lastStatus != TOO_LONG )
    {
        tuned.Clear();

        BOOST_FOREACH( PNS_MEANDER_SHAPE* m, m_result.Meanders() )
        {
            if( m->Type() != MT_EMPTY )
            {
                tuned.Append ( m->CLine( 0 ) );
            }
        }

        m_lastLength += tuned.Length();

        int comp = compareWithTolerance( m_lastLength - aTargetLength, 0, m_settings.m_lengthTolerance );

        if( comp > 0 )
            m_lastStatus = TOO_LONG;
        else if( comp < 0 )
            m_lastStatus = TOO_SHORT;
        else
            m_lastStatus = TUNED;
    }

    m_finalShape.Clear();
    m_finalShape.Append( pre );
    m_finalShape.Append( tuned );
    m_finalShape.Append( post );
    m_finalShape.Simplify();

    return true;
}


bool PNS_MEANDER_PLACER::FixRoute( const VECTOR2I& aP, PNS_ITEM* aEndItem )
{
    if( !m_currentNode )
        return false;

    m_currentTrace = PNS_LINE( m_originLine, m_finalShape );
    m_currentNode->Add( &m_currentTrace );

    Router()->CommitRouting( m_currentNode );
    return true;
}


bool PNS_MEANDER_PLACER::CheckFit( PNS_MEANDER_SHAPE* aShape )
{
    PNS_LINE l( m_originLine, aShape->CLine( 0 ) );

    if( m_currentNode->CheckColliding( &l ) )
        return false;

    int w = aShape->Width();
    int clearance = w + m_settings.m_spacing;

    return m_result.CheckSelfIntersections( aShape, clearance );
}


const PNS_ITEMSET PNS_MEANDER_PLACER::Traces()
{
    m_currentTrace = PNS_LINE( m_originLine, m_finalShape );
    return PNS_ITEMSET( &m_currentTrace );
}


const VECTOR2I& PNS_MEANDER_PLACER::CurrentEnd() const
{
    return m_currentEnd;
}


int PNS_MEANDER_PLACER::CurrentNet() const
{
    return m_initialSegment->Net();
}


int PNS_MEANDER_PLACER::CurrentLayer() const
{
    return m_initialSegment->Layers().Start();
}


const wxString PNS_MEANDER_PLACER::TuningInfo() const
{
    wxString status;

    switch ( m_lastStatus )
    {
        case TOO_LONG:
            status = _( "Too long: " );
            break;
        case TOO_SHORT:
            status = _( "Too short: " );
            break;
        case TUNED:
            status = _( "Tuned: " );
            break;
        default:
            return _( "?" );
    }

    status += LengthDoubleToString( (double) m_lastLength, false );
    status += "/";
    status += LengthDoubleToString( (double) m_settings.m_targetLength, false );

    return status;
}


PNS_MEANDER_PLACER::TUNING_STATUS PNS_MEANDER_PLACER::TuningStatus() const
{
    return m_lastStatus;
}
