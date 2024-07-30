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

#include <footprint.h>
#include <wx/string.h>

#include "topo_match.h"

namespace TMATCH
{

bool PIN::IsIsomorphic( const PIN& b ) const
{
    if( m_conns.size() != b.m_conns.size() )
    {
        printf("[conns mismatch n1 %d n2 %d c-ref %d c-other %d thispin %s-%s otherpin %s-%s", m_netcode, b.m_netcode, (int) m_conns.size(), (int) b.m_conns.size(),
        m_parent->m_reference.c_str().AsChar(), m_ref.c_str().AsChar(),
        b.m_parent->m_reference.c_str().AsChar(), b.m_ref.c_str().AsChar() );

        for( auto c : m_conns )
            printf("%s-%s ", c->m_parent->m_reference.c_str().AsChar(), c->m_ref.c_str().AsChar() );

        printf("||");

        for( auto c : b.m_conns )
            printf("%s-%s ", c->m_parent->m_reference.c_str().AsChar(), c->m_ref.c_str().AsChar() );


        printf("] ");
        return false;
    }

    if( m_conns.empty() )
    {
        printf("[conns empty]");
        return true;
    }

    bool matches[m_conns.size()];

    for( int i = 0; i < m_conns.size(); i++ )
        matches[i] = false;

    //printf("REF: %s\n", format().c_str() );
    //printf("B  : %s\n", b.format().c_str() );

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

    bool r = true;

    for( int i = 0; i < m_conns.size(); i++ )
        if( !matches[i] )
            r = false;

    //printf("pin %s vs %s iso=%d\n", format().c_str(), b.format().c_str(), r ? 1: 0   );

    return r;
}


std::vector<COMPONENT*>
CONNECTION_GRAPH::findMatchingComponents( COMPONENT* aRef, const BACKTRACK_STAGE& partialMatches )
{
    std::vector<COMPONENT*> matches;
    for( auto cmpTarget : m_components )
    {
        printf("Check '%s'/'%s' ", aRef->m_reference.c_str().AsChar(), cmpTarget->m_reference.c_str().AsChar() );
        if( partialMatches.m_locked.find( cmpTarget ) != partialMatches.m_locked.end() )
        {
            printf("discard\n");
            continue;
        }


        if( aRef->MatchesWith( cmpTarget ) )
        {
            printf("match!");
            //printf("possible match: %s/%s [fp %s/%s]\n", cmpTarget->reference.c_str(), ref->reference.c_str(), cmpTarget->footprintName.c_str(), ref->footprintName.c_str() );

            matches.push_back( cmpTarget );
        }
        printf("\n");
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
            printf("NC %d pin %s\n", p->GetNetCode(), p->m_ref.c_str().AsChar() );
            if( p->GetNetCode() > 0 )
                nets[p->GetNetCode()].push_back( p );
        }
    }

    for( auto iter : nets )
    {
        printf("net %d: %d connections\n", iter.first, iter.second.size() );
        for( auto p : iter.second )
        {
            for( auto p2 : iter.second )
                if( p != p2 && !alg::contains( p->m_conns, p2 ) )
                {
                    p->m_conns.push_back( p2 );
                }
        }
    }

    for( auto c : m_components )
        for( auto p : c->Pins() )
        {
            printf("pin %s: \n", p->m_ref.c_str().AsChar() );

            for( auto c : p->m_conns )
                printf( "%s ", c->m_ref.c_str().AsChar() );
            printf("\n");
        }
}

CONNECTION_GRAPH::STATUS CONNECTION_GRAPH::FindIsomorphism( CONNECTION_GRAPH* aTarget,
                                                            COMPONENT_MATCHES&  aResult )
{
    std::vector<BACKTRACK_STAGE> stack;
    BACKTRACK_STAGE              top;

    //printf("Ref: %d, tgt: %d\n", m_components.size(), aTarget->m_components.size() );

    if( m_components.empty()|| aTarget->m_components.empty() )
        return ST_EMPTY;

    if( m_components.size() != aTarget->m_components.size() )
        return ST_COMPONENT_COUNT_MISMATCH;

    top.m_ref = m_components.front();
    top.m_matches = aTarget->findMatchingComponents( top.m_ref, top );

    stack.push_back( top );

    int  refIndex = 1;
    bool matchFound = false;
    int  nloops = 0;
    while( !stack.empty() )
    {
        nloops++;
        auto& current = stack.back();

        if( nloops >= c_ITER_LIMIT )
        {
            return ST_ITERATION_COUNT_EXCEEDED;
        }


        if( current.m_currentMatch >= current.m_matches.size() )
        {
            stack.pop_back();
            continue;
        }

        printf("Current '%s' stack %d cm %d/%d locked %d/%d candidate '%s'\n", current.m_ref->m_reference.c_str().AsChar(), (int) stack.size(), current.m_currentMatch, (int) current.m_matches.size(),(int) current.m_locked.size(), (int)m_components.size(),
        current.m_matches[current.m_currentMatch]->m_reference.c_str().AsChar() );

        auto& match = current.m_matches[current.m_currentMatch];

        current.m_currentMatch++;
        current.m_locked[match] = current.m_ref;


        if( current.m_locked.size() == m_components.size() )
        {
            //printf("NLoops: %d\n", nloops);
            current.m_nloops = nloops;

            aResult.clear();

            for( auto iter : current.m_locked )
                aResult[ iter.second->GetParent() ] = iter.first->GetParent();

            return ST_OK;
        }


        printf("RI %d cs %d\n", refIndex, (int) m_components.size() );

        if( refIndex >= m_components.size() )
            break;

        //printf("ref '%s', locked %d, stack %d\n",  current.locked.size(), stack.size() );

        BACKTRACK_STAGE next( current );
        next.m_currentMatch = 0;
        next.m_ref = m_components[refIndex++];
        next.m_matches = aTarget->findMatchingComponents( next.m_ref, next );

        printf("Nxt '%s' matches %d\n", next.m_ref->m_reference.c_str().AsChar(), next.m_matches.size() );
        printf("m: ");
        for( auto l : next.m_matches )
        {
            printf("%s ", l->m_reference.c_str().AsChar() );
        }
        printf("\n");


        //    printf("  - matches: %d\n", (int) next.matches.size() );

        if( next.m_matches.empty() )
            continue;


        /*  printf("LOCKED: ");
        for( auto l : next.locked )
        {
            printf("%s ", l.first->reference.c_str() );
        }
        printf("\n");

        printf("PUSH L %d\n", (int) next.locked.size() );*/
        stack.push_back( next );

        //printf("NL %d/%d\n", (int) next.locked.size(), (int) cgRef->components.size() );
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

COMPONENT::COMPONENT( const wxString& aRef, FOOTPRINT* aParentFp ) :
        m_reference( aRef ), m_parentFootprint( aParentFp )
{
    int i;
    for( i = 0; i < aRef.length(); i++ )
    {
        if( std::iswalpha( aRef[i].GetValue() ) )
            break;
    }

    m_prefix = aRef.substr( 0, i );
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
        printf("[cp mismatch]");
        return false;
    }
    if( m_parentFootprint->GetFPID() != b->m_parentFootprint->GetFPID() )
    {
        printf("[fpid mismatch]");
        return false;
    }
    if( m_prefix != b->m_prefix )
    {
        printf("[pre mismatch]");
        return false;
    }

    bool fail = false;
    for( int pin = 0; pin < b->GetPinCount(); pin++ )
    {
        if( !b->m_pins[pin]->IsIsomorphic( *m_pins[pin] ) )
        {
            printf("[iso fail p%d]", pin );
            fail = true;
            break;
        }
    }

    return !fail;
}

void CONNECTION_GRAPH::AddFootprint( FOOTPRINT* aFp )
{
    auto cmp = new COMPONENT( aFp->GetReference(), aFp );;

    for( auto pad : aFp->Pads() )
    {
        //printf("pad %p\n", pad );
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

    for( auto fp : aFps )
    {
        cgraph->AddFootprint( fp );
    }

    cgraph->BuildConnectivity();

    return std::move(cgraph);
}

CONNECTION_GRAPH::CONNECTION_GRAPH() {}
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
