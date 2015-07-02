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

#include "pns_line.h"
#include "pns_segment.h"
#include "pns_node.h"
#include "pns_joint.h"
#include "pns_solid.h"
#include "pns_router.h"
#include "pns_utils.h"

#include "pns_diff_pair.h"
#include "pns_topology.h"

#include <class_board.h>

bool PNS_TOPOLOGY::SimplifyLine( PNS_LINE* aLine )
{
    if( !aLine->LinkedSegments() || !aLine->SegmentCount() )
        return false;

    PNS_SEGMENT* root = ( *aLine->LinkedSegments() )[0];
    std::auto_ptr<PNS_LINE> l( m_world->AssembleLine( root ) );
    SHAPE_LINE_CHAIN simplified( l->CLine() );

    simplified.Simplify();

    if( simplified.PointCount() != l->PointCount() )
    {
        std::auto_ptr<PNS_LINE> lnew( l->Clone() );
        m_world->Remove( l.get() );
        lnew->SetShape( simplified );
        m_world->Add( lnew.get() );
        return true;
    }

    return false;
}


const PNS_TOPOLOGY::JOINT_SET PNS_TOPOLOGY::ConnectedJoints( PNS_JOINT* aStart )
{
    std::deque<PNS_JOINT*> searchQueue;
    JOINT_SET processed;

    searchQueue.push_back( aStart );
    processed.insert( aStart );

    while( !searchQueue.empty() )
    {
        PNS_JOINT* current = searchQueue.front();
        searchQueue.pop_front();

        BOOST_FOREACH( PNS_ITEM* item, current->LinkList() )
        {
            if( item->OfKind( PNS_ITEM::SEGMENT ) )
            {
                PNS_SEGMENT* seg = static_cast<PNS_SEGMENT*>( item );
                PNS_JOINT* a = m_world->FindJoint( seg->Seg().A, seg );
                PNS_JOINT* b = m_world->FindJoint( seg->Seg().B, seg );
                PNS_JOINT* next = ( *a == *current ) ? b : a;

                if( processed.find( next ) == processed.end() )
                {
                    processed.insert( next );
                    searchQueue.push_back( next );
                }
            }
        }
    }

    return processed;
}


bool PNS_TOPOLOGY::LeadingRatLine( const PNS_LINE* aTrack, SHAPE_LINE_CHAIN& aRatLine )
{
    PNS_LINE track( *aTrack );
    VECTOR2I end;

    if( !track.PointCount() )
        return false;

    std::auto_ptr<PNS_NODE> tmpNode( m_world->Branch() );
    tmpNode->Add( &track );

    PNS_JOINT* jt = tmpNode->FindJoint( track.CPoint( -1 ), &track );

    if( !jt )
       return false;

    if( ( !track.EndsWithVia() && jt->LinkCount() >= 2 ) || ( track.EndsWithVia() && jt->LinkCount() >= 3 ) ) // we got something connected
    {
        end = jt->Pos();
    }
    else
    {
        int anchor;

        PNS_TOPOLOGY topo( tmpNode.get() );
        PNS_ITEM* it = topo.NearestUnconnectedItem( jt, &anchor );

        if( !it )
            return false;

        end = it->Anchor( anchor );
    }

    aRatLine.Clear();
    aRatLine.Append( track.CPoint( -1 ) );
    aRatLine.Append( end );
    return true;
}


PNS_ITEM* PNS_TOPOLOGY::NearestUnconnectedItem( PNS_JOINT* aStart, int* aAnchor, int aKindMask )
{
    std::set<PNS_ITEM*> disconnected;

    m_world->AllItemsInNet( aStart->Net(), disconnected );

    BOOST_FOREACH( const PNS_JOINT* jt, ConnectedJoints( aStart ) )
    {
        BOOST_FOREACH( PNS_ITEM* link, jt->LinkList() )
        {
            if( disconnected.find( link ) != disconnected.end() )
                disconnected.erase( link );
        }
    }

    int best_dist = INT_MAX;
    PNS_ITEM* best = NULL;

    BOOST_FOREACH( PNS_ITEM* item, disconnected )
    {
        if( item->OfKind( aKindMask ) )
        {
            for(int i = 0; i < item->AnchorCount(); i++)
            {
                VECTOR2I p = item->Anchor( i );
                int d = ( p - aStart->Pos() ).EuclideanNorm();

                if( d < best_dist )
                {
                    best_dist = d;
                    best = item;

                    if( aAnchor )
                        *aAnchor = i;
                }
            }
        }
    }

    return best;
}


bool PNS_TOPOLOGY::followTrivialPath( PNS_LINE* aLine, bool aLeft, PNS_ITEMSET& aSet, std::set<PNS_ITEM*>& aVisited )
{
    VECTOR2I anchor = aLeft ? aLine->CPoint( 0 ) : aLine->CPoint( -1 );
    PNS_SEGMENT* last = aLeft ? aLine->LinkedSegments()->front() : aLine->LinkedSegments()->back();
    PNS_JOINT* jt = m_world->FindJoint( anchor, aLine );

    assert( jt != NULL );

    aVisited.insert( last );

    if( jt->IsNonFanoutVia() )
    {
        PNS_ITEM* via = NULL;
        PNS_SEGMENT* next_seg = NULL;

        BOOST_FOREACH( PNS_ITEM* link, jt->Links().Items() )
        {
            if( link->OfKind( PNS_ITEM::VIA ) )
                via = link;
            else if( aVisited.find( link ) == aVisited.end() )
                next_seg = static_cast<PNS_SEGMENT*>( link );
        }

        if( !next_seg )
            return false;

        PNS_LINE* l = m_world->AssembleLine( next_seg );

        VECTOR2I nextAnchor = ( aLeft ? l->CLine().CPoint( -1 ) : l->CLine().CPoint( 0 ) );

        if( nextAnchor != anchor )
        {
            l->Reverse();
        }

        if( aLeft )
        {
            aSet.Prepend( via );
            aSet.Prepend( l );
        }
        else
        {
            aSet.Add( via );
            aSet.Add( l );
        }

        return followTrivialPath( l, aLeft, aSet, aVisited );
    }

    return false;
}


const PNS_ITEMSET PNS_TOPOLOGY::AssembleTrivialPath( PNS_SEGMENT* aStart )
{
    PNS_ITEMSET path;
    std::set<PNS_ITEM*> visited;

    PNS_LINE* l = m_world->AssembleLine( aStart );

    path.Add( l );

    followTrivialPath( l, false, path, visited );
    followTrivialPath( l, true, path, visited );

    return path;
}


const PNS_ITEMSET PNS_TOPOLOGY::ConnectedItems( PNS_JOINT* aStart, int aKindMask )
{
    return PNS_ITEMSET();
}


const PNS_ITEMSET PNS_TOPOLOGY::ConnectedItems( PNS_ITEM* aStart, int aKindMask )
{
    return PNS_ITEMSET();
}


int PNS_TOPOLOGY::MatchDpSuffix( wxString aNetName, wxString& aComplementNet, wxString& aBaseDpName )
{
    int rv = 0;

    if( aNetName.EndsWith( "+" ) )
    {
        aComplementNet = "-";
        rv = 1;
    }
    else if( aNetName.EndsWith( "_P" ) )
    {
        aComplementNet = "_N";
        rv = 1;
    }
    else if( aNetName.EndsWith( "-" ) )
    {
        aComplementNet = "+";
        rv = -1;
    }
    else if( aNetName.EndsWith( "_N" ) )
    {
        aComplementNet = "_P";
        rv = -1;
    }

    if( rv != 0 )
    {
        aBaseDpName = aNetName.Left( aNetName.Length() - aComplementNet.Length() );
        aComplementNet = aBaseDpName + aComplementNet;
    }

    return rv;
}


int PNS_TOPOLOGY::DpCoupledNet( int aNet )
{
    BOARD* brd = PNS_ROUTER::GetInstance()->GetBoard();

    wxString refName = brd->FindNet( aNet )->GetNetname();
    wxString dummy, coupledNetName;

    if( MatchDpSuffix( refName, coupledNetName, dummy ) )
    {
        NETINFO_ITEM* net = brd->FindNet( coupledNetName );

        if( !net )
            return -1;

        return net->GetNet();

    }

    return -1;
}


int PNS_TOPOLOGY::DpNetPolarity( int aNet )
{
    BOARD* brd = PNS_ROUTER::GetInstance()->GetBoard();

    wxString refName = brd->FindNet( aNet )->GetNetname();
    wxString dummy1, dummy2;

    return MatchDpSuffix( refName, dummy1, dummy2 );
}


bool commonParallelProjection( SEG n, SEG p, SEG &pClip, SEG& nClip );


bool PNS_TOPOLOGY::AssembleDiffPair( PNS_ITEM* aStart, PNS_DIFF_PAIR& aPair )
{
    int refNet = aStart->Net();
    int coupledNet = DpCoupledNet( refNet );

    if( coupledNet < 0 )
        return false;

    std::set<PNS_ITEM*> coupledItems;

    m_world->AllItemsInNet( coupledNet, coupledItems );

    PNS_SEGMENT* coupledSeg = NULL, *refSeg;
    int minDist = std::numeric_limits<int>::max();

    if( ( refSeg = dyn_cast<PNS_SEGMENT*>( aStart ) ) != NULL )
    {
        BOOST_FOREACH( PNS_ITEM* item, coupledItems )
        {
            if( PNS_SEGMENT* s = dyn_cast<PNS_SEGMENT*>( item ) )
            {
                if( s->Layers().Start() == refSeg->Layers().Start() && s->Width() == refSeg->Width() )
                {
                    int dist = s->Seg().Distance( refSeg->Seg() );
		    		bool isParallel = refSeg->Seg().ApproxParallel( s->Seg() );
                    SEG p_clip, n_clip;

                    bool isCoupled = commonParallelProjection( refSeg->Seg(), s->Seg(), p_clip, n_clip );

                    if( isParallel && isCoupled && dist < minDist )
                    {
                        minDist = dist;
                        coupledSeg = s;
                    }
                }
            }
        }
    } else
        return false;

    if( !coupledSeg )
        return false;

    std::auto_ptr<PNS_LINE> lp( m_world->AssembleLine( refSeg ) );
    std::auto_ptr<PNS_LINE> ln( m_world->AssembleLine( coupledSeg ) );

    if( DpNetPolarity( refNet ) < 0 )
    {
        std::swap( lp, ln );
    }

    int gap = -1 ;
    if( refSeg->Seg().ApproxParallel( coupledSeg->Seg() ) )
    {
        // Segments are parallel -> compute pair gap
        const VECTOR2I refDir       = refSeg->Anchor( 1 ) - refSeg->Anchor( 0 );
        const VECTOR2I displacement = refSeg->Anchor( 1 ) - coupledSeg->Anchor( 1 );
        gap = (int) abs( refDir.Cross( displacement ) / refDir.EuclideanNorm() ) - lp->Width();
    }

    aPair = PNS_DIFF_PAIR( *lp, *ln );
    aPair.SetWidth( lp->Width() );
    aPair.SetLayers( lp->Layers() );
    aPair.SetGap( gap );

    return true;
}
