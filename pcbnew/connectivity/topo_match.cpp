/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) Kicad Developers, see change_log.txt for contributors.
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

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <string>
#include <vector>
#include <algorithm>
#include <cassert>
#include <map>
#include <set>
#include <cctype>

#include <pad.h>
#include <footprint.h>
#include <refdes_utils.h>
#include <wx/string.h>
#include <wx/log.h>

#include "topo_match.h"


static const wxString traceTopoMatch = wxT( "TOPO_MATCH" );

namespace TMATCH
{

bool PIN::IsIsomorphic( const PIN& b ) const
{
    if( m_conns.size() != b.m_conns.size() )
    {
        wxLogTrace( traceTopoMatch, wxT("[conns mismatch n1 %d n2 %d c-ref %d c-other %d thispin %s-%s otherpin %s-%s"), 
            m_netcode, 
            b.m_netcode, 
            (int) m_conns.size(), 
            (int) b.m_conns.size(),
            m_parent->m_reference, 
            m_ref,
            b.m_parent->m_reference, 
            b.m_ref );

        for( auto c : m_conns )
        {
            wxLogTrace( traceTopoMatch, wxT("%s-%s "), c->m_parent->m_reference, c->m_ref );
        }

        wxLogTrace( traceTopoMatch, wxT("||") );

        for( auto c : b.m_conns )
        {
            wxLogTrace( traceTopoMatch, wxT("%s-%s "), c->m_parent->m_reference, c->m_ref );
        }


        wxLogTrace( traceTopoMatch, wxT("] ") );
        return false;
    }

    if( m_conns.empty() )
    {
        wxLogTrace( traceTopoMatch, wxT("[conns empty]") );
        return true;
    }

    std::vector<bool> matches( m_conns.size() );

    for( int i = 0; i < m_conns.size(); i++ )
        matches[i] = false;

    int nref = 0;

    for( auto& cref : m_conns )
    {
        //printf("[CREF: %s]", cref->Format().c_str().AsChar() );
        for( int i = 0; i < m_conns.size(); i++ )
        {
            if( b.m_conns[i]->IsTopologicallySimilar( *cref ) )
            {
                //printf("[CMATCH: %s]", b.m_conns[i]->Format().c_str().AsChar() );
                matches[nref] = true;
                break;
            }
        }

        nref++;
    }

    for( int i = 0; i < m_conns.size(); i++ )
    {
        if( !matches[i] )
        {
            return false;
        }
    }

    return true;
}

// fixme: terrible performance, but computers are fast these days, ain't they? :D
bool checkIfPadNetsMatch( BACKTRACK_STAGE& aMatches, CONNECTION_GRAPH* aRefGraph, COMPONENT* aRef, COMPONENT* aTgt )
{
    std::map<PIN*, PIN*> pairs;
    std::vector<PIN*>    pref, ptgt;

    // GetMatchingComponentPairs() returns target->reference map
    for( auto& m : aMatches.GetMatchingComponentPairs() ) 
    {
        for( PIN* p : m.second->Pins() )
        {
            pref.push_back( p );
        }

        for( PIN* p : m.first->Pins() )
        {
            ptgt.push_back( p );
        }
    }

    for( PIN* p : aRef->Pins() )
    {
        pref.push_back( p );
    }

    for( PIN* p : aTgt->Pins() )
    {
        ptgt.push_back( p );
    }

    if( pref.size() != ptgt.size() )
    {
        return false;
    }

    for( unsigned int i = 0; i < pref.size(); i++ )
    {
        pairs[pref[i]] = ptgt[i];
    }

    for( PIN* refPin : aRef->Pins() )
    {
        wxLogTrace( traceTopoMatch, wxT("pad %s-%s: ") , aRef->GetParent()->GetReferenceAsString() , refPin->GetReference() );

        std::optional<int> prevNet;

        for( COMPONENT* refCmp : aRefGraph->Components() )
        {
            for( PIN* ppin : refCmp->Pins() )
            {
                if ( ppin->GetNetCode() != refPin->GetNetCode() )
                    continue;

                wxLogTrace( traceTopoMatch, wxT("{ref %s-%s:%d} "), ppin->GetParent()->GetParent()->GetReferenceAsString(), ppin->GetReference(), ppin->GetNetCode() );

                auto tpin = pairs.find( ppin );

                if( tpin != pairs.end() )
                {
                    int nc = tpin->second->GetNetCode();

//                    printf("%s-%s:%d ", tpin->second->GetParent()->GetParent()->GetReferenceAsString(), tpin->second->GetReference().c_str().AsChar(), tpin->second->GetNetCode() );

                    if( prevNet && ( *prevNet != nc ) )
                    {
                        wxLogTrace( traceTopoMatch, wxT("nets inconsistent\n") );
                        return false;
                    }

                    prevNet = nc;
                }
            }
        }
    }

    return true;
}


std::vector<COMPONENT*>
CONNECTION_GRAPH::findMatchingComponents( CONNECTION_GRAPH* aRefGraph, COMPONENT* aRef, BACKTRACK_STAGE& partialMatches )
{
    std::vector<COMPONENT*> matches;
    for( auto cmpTarget : m_components )
    {
        // already matched to sth? move on.
        if( partialMatches.m_locked.find( cmpTarget ) != partialMatches.m_locked.end() )
        {
            continue;
        }

        wxLogTrace( traceTopoMatch, wxT("Check '%s'/'%s' "), aRef->m_reference, cmpTarget->m_reference );

        // first, a basic heuristic (reference prefix, pin count & footprint) followed by a pin connection topology check
        if( aRef->MatchesWith( cmpTarget ) )
        {
            // then a net integrity check (expensive because of poor optimization)
            if( checkIfPadNetsMatch( partialMatches, aRefGraph, aRef, cmpTarget ) )
            {
                wxLogTrace( traceTopoMatch, wxT("match!\n") );
                matches.push_back( cmpTarget );
            }
            else
            {
                wxLogTrace( traceTopoMatch, wxT("Reject [net topo mismatch]\n") );
            }
        }
        else
        {
            wxLogTrace( traceTopoMatch, wxT("reject\n") );
        }



    }

    return matches;
}

void COMPONENT::sortPinsByName()
{
std::sort( m_pins.begin(), m_pins.end(),
                   []( PIN* a, PIN* b )
                   {
                       return a->GetReference() < b->GetReference();
                   } );
}

void CONNECTION_GRAPH::BuildConnectivity()
{
    std::map<int, std::vector<PIN*>> nets;

    sortByPinCount();

    for( auto c : m_components )
    {
        c->sortPinsByName();

        for( auto p : c->Pins() )
        {
            if( p->GetNetCode() > 0 )
                nets[p->GetNetCode()].push_back( p );
        }
    }

    for( auto iter : nets )
    {
        wxLogTrace( traceTopoMatch, wxT("net %d: %d connections\n"), iter.first, (int) iter.second.size() );
        for( auto p : iter.second )
        {
            for( auto p2 : iter.second )
                if( p != p2 && !alg::contains( p->m_conns, p2 ) )
                {
                    p->m_conns.push_back( p2 );
                }
        }
    }

/*    for( auto c : m_components )
        for( auto p : c->Pins() )
        {
            printf("pin %s: \n", p->m_ref.c_str().AsChar() );

            for( auto c : p->m_conns )
                printf( "%s ", c->m_ref.c_str().AsChar() );
            printf("\n");
        }
         */
}


CONNECTION_GRAPH::STATUS CONNECTION_GRAPH::FindIsomorphism( CONNECTION_GRAPH* aTarget,
                                                            COMPONENT_MATCHES&  aResult )
{
    std::vector<BACKTRACK_STAGE> stack;
    BACKTRACK_STAGE              top;

    if( m_components.empty()|| aTarget->m_components.empty() )
        return ST_EMPTY;

    if( m_components.size() != aTarget->m_components.size() )
        return ST_COMPONENT_COUNT_MISMATCH;

    top.m_ref = m_components.front();
    top.m_refIndex = 0;

    stack.push_back( top );

    bool matchFound = false;
    int  nloops = 0;
    while( !stack.empty() )
    {
        nloops++;
        auto& current = stack.back();

        for( auto it = current.m_locked.begin(); it != current.m_locked.end(); it++ )
        {
            if (it->second == current.m_ref)
            {
                wxLogTrace( traceTopoMatch, wxT("stk: Remove %s from locked\n"), current.m_ref->m_reference );
                current.m_locked.erase( it );
                break;
            }
        }

        if( nloops >= c_ITER_LIMIT )
        {
            wxLogTrace( traceTopoMatch, wxT("stk: Iter cnt exceeded\n") );
            return ST_ITERATION_COUNT_EXCEEDED;
        }

        if( current.m_currentMatch < 0 )
        {
            current.m_matches = aTarget->findMatchingComponents( this, current.m_ref, current );
            current.m_currentMatch = 0;
        }

        wxLogTrace( traceTopoMatch, wxT("stk: Current '%s' stack %d cm %d/%d locked %d/%d\n" ),
                current.m_ref->m_reference, (int) stack.size(),
                current.m_currentMatch, (int) current.m_matches.size(),
                (int) current.m_locked.size(), (int) m_components.size() );

        if ( current.m_matches.empty() )
        {
            wxLogTrace( traceTopoMatch, wxT("stk: No matches at all, going up [level=%d]\n"), (int) stack.size() );
            stack.pop_back();
            continue;
        }

        if( current.m_currentMatch >= 0 && current.m_currentMatch >= current.m_matches.size() )
        {
            wxLogTrace( traceTopoMatch, wxT("stk: No more matches, going up [level=%d]\n"), (int) stack.size() );
            stack.pop_back();
            continue;
        }

        auto& match = current.m_matches[current.m_currentMatch];

        wxLogTrace( traceTopoMatch, wxT("stk: candidate '%s', match list : ( "),
                                current.m_matches[current.m_currentMatch]->m_reference,
                                current.m_refIndex );

        for( auto m : current.m_matches )
            wxLogTrace( traceTopoMatch, wxT("%s "), m->GetParent()->GetReferenceAsString() );

        wxLogTrace( traceTopoMatch, wxT("\n") );



        current.m_currentMatch++;
        current.m_locked[match] = current.m_ref;


        if( current.m_locked.size() == m_components.size() )
        {
            current.m_nloops = nloops;

            aResult.clear();

            for( auto iter : current.m_locked )
                aResult[ iter.second->GetParent() ] = iter.first->GetParent();

            return ST_OK;
        }


        BACKTRACK_STAGE next( current );
        next.m_currentMatch = -1;
        next.m_ref = m_components[current.m_refIndex + 1];
        next.m_refIndex = current.m_refIndex + 1;

        stack.push_back( next );
    };


    return ST_TOPOLOGY_MISMATCH;
}

#if 0
int main()
{
    FILE * f = fopen("connectivity.dump","rb" );
    auto cgRef = loadCGraph(f);
    auto cgTarget = loadCGraph(f);

    cgRef->buildConnectivity();
    cgTarget->buildConnectivity();

    int attempts = 0;
    int max_loops = 0;

    for( ;; )
    {
        cgRef->shuffle();
        cgTarget->shuffle();

        const BacktrackStage latest = cgRef->matchCGraphs( cgTarget );

        if( !latest.locked.size() )
        {
            printf("MATCH FAIL\n");
            break;
        }

        //printf("loops: %d\n", latest.nloops );
        //printf("Locked: %d\n", latest.locked.size() );

        //if (matchFound)
        //{
          //  for( auto& iter : latest.locked )
            //{
             //   printf("%-10s : %-10s\n", iter.first->reference.c_str(), iter.second->reference.c_str() );
            //}

        //}

        if( latest.nloops > max_loops )
        {
            max_loops = latest.nloops;
        }

        if (attempts % 10000 == 0)
        {
            printf("attempts: %d maxloops: %d\n", attempts, max_loops );
        }

        attempts++;

    }

    fclose(f);

    return 0;
}

#endif

COMPONENT::COMPONENT( const wxString& aRef, FOOTPRINT* aParentFp, std::optional<VECTOR2I> aRaOffset ) :
    m_reference( aRef ), m_parentFootprint( aParentFp ), m_raOffset( aRaOffset )
{
    m_prefix = UTIL::GetRefDesPrefix( aRef );
}

bool COMPONENT::IsSameKind( const COMPONENT& b ) const
{
    return m_prefix == b.m_prefix && m_parentFootprint->GetFPID() == b.m_parentFootprint->GetFPID();
}

void COMPONENT::AddPin( PIN* aPin )
{
    m_pins.push_back( aPin );
    aPin->SetParent( this );
}

bool COMPONENT::MatchesWith( COMPONENT* b )
{
    if( GetPinCount() != b->GetPinCount() )
    {
        return false;
    }

    if( m_parentFootprint->GetFPID() != b->m_parentFootprint->GetFPID() )
    {
        return false;
    }

    if( m_prefix != b->m_prefix )
    {
        return false;
    }

    for( int pin = 0; pin < b->GetPinCount(); pin++ )
    {
        if( !b->m_pins[pin]->IsIsomorphic( *m_pins[pin] ) )
        {
            return false;
        }

    }

    return true;
}

void CONNECTION_GRAPH::AddFootprint( FOOTPRINT* aFp, const VECTOR2I& aOffset )
{
    auto cmp = new COMPONENT( aFp->GetReference(), aFp );;

    for( auto pad : aFp->Pads() )
    {
        auto pin = new PIN( );
        pin->m_netcode = pad->GetNetCode();
        pin->m_ref = pad->GetNumber();
        cmp->AddPin( pin );
    }

    m_components.push_back( cmp );
}

std::unique_ptr<CONNECTION_GRAPH> CONNECTION_GRAPH::BuildFromFootprintSet( const std::set<FOOTPRINT*>& aFps )
{
    auto cgraph = std::make_unique<CONNECTION_GRAPH>();
    VECTOR2I ref(0, 0);

    if( aFps.size() > 0 )
        ref = (*aFps.begin())->GetPosition();

    for( auto fp : aFps )
    {
        cgraph->AddFootprint( fp, fp->GetPosition() - ref );
    }

    cgraph->BuildConnectivity();

    return std::move(cgraph);
}


CONNECTION_GRAPH::CONNECTION_GRAPH()
{

}


CONNECTION_GRAPH::~CONNECTION_GRAPH()
{
    for( COMPONENT* fp : m_components )
    {
        delete fp;
    }
}


COMPONENT::~COMPONENT()
{
    for( PIN* p : m_pins )
    {
        delete p;
    }
}


}; // namespace TMATCH
